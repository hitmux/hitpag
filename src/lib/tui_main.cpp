// Copyright (C) 2025 Hitmux
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#include "include/tui.h"
#include "include/tui_archive_list.h"
#include "include/tui_details.h"
#include "include/tui_preview.h"
#include "include/tui_preview_model.h"
#include "include/tui_editor.h"
#include "include/tui_settings.h"
#include "include/tui_state.h"
#include "include/tui_archive_ops.h"
#include "include/i18n.h"
#include "include/file_type.h"
#include "include/interactive.h"

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/node.hpp>
#include <ftxui/screen/screen.hpp>

#include <algorithm>
#include <cctype>
#include <functional>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace tui {
    using namespace ftxui;
    namespace fs = std::filesystem;

    namespace {
        constexpr int kFileListWidth = 44;
        constexpr int kSettingsModalWidth = 72;

        class HideCursorNode : public Node {
        public:
            explicit HideCursorNode(Elements children) : Node(std::move(children)) {}

            void ComputeRequirement() override {
                Node::ComputeRequirement();
                requirement_ = children_[0]->requirement();
            }

            void SetBox(Box box) override {
                Node::SetBox(box);
                children_[0]->SetBox(box);
            }

            void Render(Screen& screen) override {
                children_[0]->Render(screen);
                screen.SetCursor(Screen::Cursor{
                    box_.x_min,
                    box_.y_min,
                    Screen::Cursor::Shape::Hidden,
                });
            }
        };

        Element hide_terminal_cursor(Element child) {
            return std::make_shared<HideCursorNode>(Elements{std::move(child)});
        }

        enum class PanelFocus {
            List,
            Preview,
        };

        enum class SettingsModalFocus {
            Choices,
            CustomInput,
        };
    }

    static bool is_archive_type(file_type::FileType type) {
        return type != file_type::FileType::UNKNOWN &&
               type != file_type::FileType::REGULAR_FILE &&
               type != file_type::FileType::DIRECTORY;
    }

    void run(args::Options& options, progress::ProgressTracker& tracker) {
        std::string archive_path = options.source_path;

        if (archive_path.empty()) {
            std::cout << i18n::get("tui_select_archive");
            archive_path = interactive::get_input();
            if (archive_path.empty()) {
                std::cout << i18n::get("operation_canceled") << std::endl;
                return;
            }
        }

        file_type::FileType type = file_type::recognize_source_type(archive_path);

        if (!is_archive_type(type)) {
            std::cout << i18n::get("tui_not_archive") << std::endl;
            return;
        }

        ArchiveList list;
        DetailsPanel details;
        PreviewModel preview_model;
        auto tui_settings = settings::load();

        list.load(archive_path, type, options.password);

        if (!list.loaded() || list.entries().empty()) {
            std::cout << i18n::get("tui_no_entries") << std::endl;
            return;
        }

        PreviewPanel& preview = preview_model.panel();
        preview_model.sync_selection(list.selected_entry(), list.entries(), archive_path, type, options.password);

        TuiState state;
        std::string search_input;
        PanelFocus focus = PanelFocus::List;
        bool first_time_editor_setup = false;
        int editor_choice = 0;
        std::vector<std::string> editor_choices = {
            "nano $file",
            "vim $file",
            i18n::get("tui_settings_custom_command"),
        };
        std::string custom_editor_command;
        SettingsModalFocus settings_modal_focus = SettingsModalFocus::Choices;
        int write_back_action = 0;
        bool resume_edit_after_settings = false;
        std::string resume_edit_path;
        std::function<void(const std::string&)> launch_edit_for_path;

        auto show_alert = [&](std::string title, std::string message, bool is_error) {
            state.alert.title = std::move(title);
            state.alert.message = std::move(message);
            state.alert.is_error = is_error;
            state.mode = UiMode::Alert;
        };

        auto sync_preview = [&]() {
            preview_model.sync_selection(list.selected_entry(), list.entries(), archive_path, type, options.password);
        };

        auto looks_like_error = [](const std::string& message) {
            std::string lower = message;
            std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char ch) {
                return static_cast<char>(std::tolower(ch));
            });
            return lower.find("failed") != std::string::npos ||
                   lower.find("error") != std::string::npos ||
                   lower.find("invalid") != std::string::npos ||
                   lower.find("not support") != std::string::npos ||
                   lower.find("not configured") != std::string::npos ||
                   lower.find("non-zero") != std::string::npos;
        };

        auto selected_editor_preview = [&]() -> std::string {
            if (editor_choice == 0) {
                return "nano $file";
            }
            if (editor_choice == 1) {
                return "vim $file";
            }
            return custom_editor_command.empty() ? i18n::get("tui_settings_custom_command_not_set") : custom_editor_command;
        };

        auto selected_editor_validation = [&]() -> std::string {
            if (editor_choice != 2) {
                return i18n::get("tui_settings_builtin_template_ready");
            }
            if (custom_editor_command.empty()) {
                return i18n::get("tui_settings_custom_command_empty");
            }
            if (custom_editor_command.find("$file") == std::string::npos) {
                return i18n::get("tui_settings_custom_command_missing_file");
            }
            return i18n::get("tui_settings_custom_command_ready");
        };

        auto close_settings_modal = [&]() {
            state.mode = UiMode::Browse;
            resume_edit_after_settings = false;
            resume_edit_path.clear();
        };

        auto save_settings = [&]() -> bool {
            std::string editor_command;
            if (editor_choice == 0) {
                editor_command = "nano $file";
            } else if (editor_choice == 1) {
                editor_command = "vim $file";
            } else {
                editor_command = custom_editor_command;
            }

            if (editor_command.empty()) {
                show_alert(i18n::get("tui_settings_title"), i18n::get("tui_settings_save_empty"), true);
                return false;
            }

            tui_settings.editor_command = editor_command;
            std::string error_message;
            if (!settings::save(tui_settings, &error_message)) {
                show_alert(i18n::get("tui_settings_title"), error_message, true);
                return false;
            }

            bool continue_edit = resume_edit_after_settings && !resume_edit_path.empty();
            std::string target_path = resume_edit_path;
            close_settings_modal();

            if (continue_edit) {
                launch_edit_for_path(target_path);
            } else {
                show_alert(
                    i18n::get("tui_settings_saved_title"),
                    i18n::get("tui_settings_saved_message", {{"COMMAND", tui_settings.editor_command}}),
                    false);
            }
            return true;
        };

        auto prepare_settings_modal = [&](bool for_editing, const std::string& target_path = std::string()) {
            first_time_editor_setup = !tui_settings.has_editor_command();
            if (tui_settings.editor_command == "vim $file") {
                editor_choice = 1;
                custom_editor_command.clear();
            } else if (tui_settings.editor_command.empty() || tui_settings.editor_command == "nano $file") {
                editor_choice = 0;
                custom_editor_command.clear();
            } else {
                editor_choice = 2;
                custom_editor_command = tui_settings.editor_command;
            }
            settings_modal_focus = (editor_choice == 2) ? SettingsModalFocus::CustomInput : SettingsModalFocus::Choices;
            resume_edit_after_settings = for_editing;
            resume_edit_path = target_path;
            state.mode = UiMode::EditorSettings;
        };

        auto search_input_comp = Input(&search_input, "");
        auto extract_input = Input(&state.extract.output_directory, "");
        auto custom_editor_input = Input(&custom_editor_command, i18n::get("tui_settings_custom_command_placeholder"));
        auto editor_choice_menu = Menu(&editor_choices, &editor_choice);
        auto screen = ScreenInteractive::Fullscreen();

        auto is_error_message = [&](const std::string& message) {
            if (message.empty()) {
                return false;
            }
            return looks_like_error(message);
        };

        launch_edit_for_path = [&](const std::string& selected_path) {
            editor::EditSession edit_session;
            auto run_editor = screen.WithRestoredIO([&] {
                edit_session = editor::start_edit_session(archive_path, selected_path, type, options.password, options, tui_settings);
            });
            run_editor();

            if (edit_session.pending_write_back()) {
                state.pending_edit_session = std::move(edit_session);
                write_back_action = 0;
                state.mode = UiMode::WriteBackConfirm;
                return;
            }

            if (edit_session.temporary_only_kept()) {
                show_alert(i18n::get("tui_edit_alert_title"), edit_session.message, false);
                return;
            }

            if (!edit_session.message.empty()) {
                show_alert(i18n::get("tui_edit_alert_title"), edit_session.message, is_error_message(edit_session.message));
            }
        };

        auto perform_extract = [&]() {
            std::string out_dir = state.extract.output_directory.empty() ? "." : state.extract.output_directory;
            std::error_code create_ec;
            fs::create_directories(fs::path(out_dir), create_ec);
            if (create_ec) {
                show_alert(
                    i18n::get("tui_extract_alert_title"),
                    i18n::get("tui_extract_create_dir_failed", {{"PATH", out_dir}, {"REASON", create_ec.message()}}),
                    true);
                return;
            }

            bool success = false;
            std::string message;
            try {
                auto run_extract = screen.WithRestoredIO([&] {
                    success = archive_ops::extract_single(archive_path, state.extract.entry_path, out_dir, type, options.password);
                });
                run_extract();
                message = success ? i18n::get("tui_extract_success") : i18n::get("tui_extract_failed");
            } catch (const std::exception& ex) {
                success = false;
                message = i18n::get("tui_extract_exception_failed", {{"REASON", ex.what()}});
            }

            show_alert(i18n::get("tui_extract_alert_title"), message, !success);
        };

        auto main_renderer = Renderer([&]() -> Element {
            Elements left_panel;
            std::string left_title = i18n::get("tui_file_list");
            if (!list.current_directory().empty()) {
                left_title += ": /" + list.current_directory();
            }
            if (focus == PanelFocus::List) {
                left_title += " " + i18n::get("tui_active_suffix");
            }
            left_panel.push_back(text(left_title) | bold | color(Color::Blue));
            left_panel.push_back(separator() | color(Color::Blue));
            left_panel.push_back(list.component()->Render() | flex);

            Elements right_panel;

            const auto* entry = list.selected_entry();
            if (entry) {
                details.set_entry(entry);
                right_panel.push_back(details.render());
                right_panel.push_back(separator() | color(Color::White));
                std::string preview_title = i18n::get("tui_preview");
                if (focus == PanelFocus::Preview) {
                    preview_title += " " + i18n::get("tui_active_suffix");
                }
                right_panel.push_back(text(preview_title) | bold | color(Color::White));

                right_panel.push_back(preview.render() | flex);
            } else {
                right_panel.push_back((text(i18n::get("tui_no_file_selected")) | dim | border) | flex);
            }

            Elements status_items;

            if (state.mode == UiMode::Search) {
                status_items.push_back(hbox({text(i18n::get("tui_search_prompt") + " ") | bold | color(Color::Blue), search_input_comp->Render()}));
            } else {
                std::string shortcut_hint;
                if (focus == PanelFocus::Preview) {
                    shortcut_hint = i18n::get("tui_preview_shortcut_hint");
                } else {
                    shortcut_hint = i18n::get("tui_list_shortcut_hint");
                }
                status_items.push_back(text(shortcut_hint) | dim);
            }

            if (state.mode == UiMode::Help) {
                status_items.push_back(text(i18n::get("tui_help_content")) | color(Color::White));
            }

            status_items.push_back(separator());
            status_items.push_back(hbox({
                text(i18n::get("tui_title")) | bold | color(Color::White) | bgcolor(Color::Blue),
                filler() | bgcolor(Color::Blue),
                text(archive_path) | dim | color(Color::White) | bgcolor(Color::Blue),
            }));

            auto left_border = focus == PanelFocus::List ? border : borderLight;
            auto right_border = focus == PanelFocus::Preview ? border : borderLight;

            Color focused_bg = Color::RGB(14, 28, 50);
            Color unfocused_bg = Color::RGB(12, 24, 42);

            auto main_content = hbox({
                vbox(left_panel) | size(WIDTH, EQUAL, kFileListWidth) | left_border |
                    bgcolor(focus == PanelFocus::List ? focused_bg : unfocused_bg) |
                    color(focus == PanelFocus::List ? Color::Blue : Color::GrayDark),
                vbox(right_panel) | flex | right_border |
                    bgcolor(focus == PanelFocus::Preview ? focused_bg : unfocused_bg) |
                    color(focus == PanelFocus::Preview ? Color::White : Color::GrayDark),
            });

            Element document = vbox({
                main_content | flex,
                vbox(status_items) | bgcolor(Color::Black),
            });

            if (state.mode == UiMode::EditorSettings) {
                Elements modal_items;
                modal_items.push_back(text(first_time_editor_setup ? i18n::get("tui_settings_configure_editor") : i18n::get("tui_settings_editor_settings")) | bold);
                modal_items.push_back(separator());
                modal_items.push_back(text(first_time_editor_setup
                    ? i18n::get("tui_settings_first_time_hint")
                    : i18n::get("tui_settings_update_hint")) | dim);
                modal_items.push_back(separator());
                modal_items.push_back(hbox({
                    text(i18n::get("tui_settings_current_label")) | dim | size(WIDTH, EQUAL, 10),
                    paragraph(tui_settings.has_editor_command() ? tui_settings.editor_command : i18n::get("tui_settings_not_configured")) | flex,
                }));
                modal_items.push_back(hbox({
                    text(i18n::get("tui_settings_selected_label")) | dim | size(WIDTH, EQUAL, 10),
                    paragraph(selected_editor_preview()) | flex,
                }));
                modal_items.push_back(hbox({
                    text(i18n::get("tui_settings_rule_label")) | dim | size(WIDTH, EQUAL, 10),
                    paragraph(i18n::get("tui_settings_rule_text")) | flex,
                }));
                modal_items.push_back(hbox({
                    text(i18n::get("tui_settings_check_label")) | dim | size(WIDTH, EQUAL, 10),
                    paragraph(selected_editor_validation()) |
                        color(editor_choice == 2 && custom_editor_command.empty() ? Color::White : Color::GrayLight) | flex,
                }));
                modal_items.push_back(separator());
                modal_items.push_back(text(i18n::get("tui_settings_command_templates")) | dim);
                modal_items.push_back(editor_choice_menu->Render() | size(HEIGHT, LESS_THAN, 5));
                if (editor_choice == 2) {
                    modal_items.push_back(separator());
                    modal_items.push_back(text(i18n::get("tui_settings_custom_command_help")) | dim);
                    modal_items.push_back(custom_editor_input->Render() |
                        size(WIDTH, EQUAL, kSettingsModalWidth - 4) |
                        bgcolor(settings_modal_focus == SettingsModalFocus::CustomInput ? Color::Blue : Color::Black));
                }
                modal_items.push_back(separator());
                modal_items.push_back(text(i18n::get("tui_settings_footer_hint")) | dim);

                document = dbox({
                    document,
                    vbox(std::move(modal_items)) | border | size(WIDTH, EQUAL, kSettingsModalWidth) | clear_under | center,
                });
            }

            if (state.mode == UiMode::WriteBackConfirm) {
                document = dbox({
                    document,
                    vbox({
                        text(i18n::get("tui_write_back_prompt")) | bold,
                        separator(),
                        hbox({
                            text(i18n::get("tui_archive_label")) | dim | size(WIDTH, EQUAL, 10),
                            paragraph(archive_path) | flex,
                        }),
                        hbox({
                            text(i18n::get("tui_entry_label")) | dim | size(WIDTH, EQUAL, 10),
                            paragraph(state.pending_edit_session.entry_path) | flex,
                        }),
                        hbox({
                            text(i18n::get("tui_format_label")) | dim | size(WIDTH, EQUAL, 10),
                            text(file_type::get_file_type_string(type)) | flex,
                        }),
                        separator(),
                        hbox({
                            text(write_back_action == 0 ? "[ " + i18n::get("tui_write_back_action") + " ]" : "  " + i18n::get("tui_write_back_action") + "  ") | bold |
                                bgcolor(write_back_action == 0 ? Color::Blue : Color::RGB(10, 25, 50)) |
                                color(write_back_action == 0 ? Color::White : Color::GrayLight),
                            text("  "),
                            text(write_back_action == 1 ? "[ " + i18n::get("tui_keep_temporary") + " ]" : "  " + i18n::get("tui_keep_temporary") + "  ") | bold |
                                bgcolor(write_back_action == 1 ? Color::White : Color::RGB(10, 25, 50)) |
                                color(write_back_action == 1 ? Color::Black : Color::GrayLight),
                        }),
                        text(i18n::get("tui_write_back_choose_action")) | dim,
                    }) | border | size(WIDTH, GREATER_THAN, 44) | clear_under | center,
                });
            }

            if (state.mode == UiMode::ExtractDialog) {
                document = dbox({
                    document,
                    vbox({
                        text(i18n::get("tui_extract")) | bold,
                        separator(),
                        text(i18n::get("tui_extracting_entry", {{"PATH", state.extract.entry_path}})),
                        text(i18n::get("tui_extract_prompt")),
                        extract_input->Render(),
                        separator(),
                        hbox({
                            text(i18n::get("tui_confirm_hint")) | dim,
                            text("  "),
                            text(i18n::get("tui_cancel_hint")) | dim,
                        }),
                    }) | border | size(WIDTH, GREATER_THAN, 56) | clear_under | center,
                });
            }

            if (state.mode == UiMode::Alert) {
                document = dbox({
                    document,
                    vbox({
                        text(state.alert.title) | bold | color(state.alert.is_error ? Color::White : Color::Blue),
                        separator(),
                        paragraph(state.alert.message),
                        separator(),
                        text(i18n::get("tui_modal_close_hint")) | dim,
                    }) | border | size(WIDTH, GREATER_THAN, 56) | clear_under | center,
                });
            }

            return hide_terminal_cursor(document);
        }) | CatchEvent([&](Event event) {
            if (state.mode == UiMode::Alert) {
                if (event == Event::Escape || event == Event::Return) {
                    state.mode = UiMode::Browse;
                    return true;
                }
                return true;
            }

            if (state.mode == UiMode::Help) {
                if (event == Event::Escape || event == Event::Return || event == Event::Character('?')) {
                    state.mode = UiMode::Browse;
                    return true;
                }
                return true;
            }

            if (state.mode == UiMode::Search) {
                if (event == Event::Escape) {
                    state.mode = UiMode::Browse;
                    list.set_search_query("");
                    sync_preview();
                    return true;
                }
                if (event == Event::Return) {
                    list.set_search_query(search_input);
                    state.mode = UiMode::Browse;
                    sync_preview();
                    return true;
                }
                return search_input_comp->OnEvent(event);
            }

            if (state.mode == UiMode::ExtractDialog) {
                if (event == Event::Escape) {
                    state.mode = UiMode::Browse;
                    return true;
                }
                if (event == Event::Return) {
                    perform_extract();
                    return true;
                }
                return extract_input->OnEvent(event);
            }

            if (state.mode == UiMode::EditorSettings) {
                if (event == Event::Escape || event == Event::Character('q')) {
                    close_settings_modal();
                    return true;
                }

                if (event == Event::Tab) {
                    settings_modal_focus = (settings_modal_focus == SettingsModalFocus::Choices && editor_choice == 2)
                        ? SettingsModalFocus::CustomInput
                        : SettingsModalFocus::Choices;
                    return true;
                }

                if (event.is_mouse()) {
                    const bool handled_by_menu = editor_choice_menu->OnEvent(event);
                    if (handled_by_menu) {
                        settings_modal_focus = (editor_choice == 2) ? SettingsModalFocus::CustomInput : SettingsModalFocus::Choices;
                        const Mouse& mouse = event.mouse();
                        const bool is_left_click = mouse.button == Mouse::Left && mouse.motion == Mouse::Pressed;
                        if (is_left_click && editor_choice != 2) {
                            save_settings();
                        }
                        return true;
                    }

                    if (editor_choice == 2 && custom_editor_input->OnEvent(event)) {
                        settings_modal_focus = SettingsModalFocus::CustomInput;
                        return true;
                    }

                    return true;
                }

                if (settings_modal_focus == SettingsModalFocus::CustomInput) {
                    if (event == Event::Return) {
                        save_settings();
                        return true;
                    }
                    return custom_editor_input->OnEvent(event);
                }

                if (event == Event::Return) {
                    if (editor_choice == 2) {
                        settings_modal_focus = SettingsModalFocus::CustomInput;
                    } else {
                        save_settings();
                    }
                    return true;
                }
                return editor_choice_menu->OnEvent(event);
            }

            if (state.mode == UiMode::WriteBackConfirm) {
                if (event == Event::Escape || event == Event::Character('q')) {
                    editor::discard_edit_session(state.pending_edit_session);
                    state.pending_edit_session = editor::EditSession{};
                    show_alert(i18n::get("tui_write_back_skipped"), i18n::get("tui_write_back_skipped_message"), false);
                    return true;
                }
                if (event == Event::Tab || event == Event::ArrowLeft || event == Event::ArrowRight) {
                    write_back_action = 1 - write_back_action;
                    return true;
                }
                if (event == Event::Return) {
                    if (write_back_action == 1) {
                        std::string kept_path = state.pending_edit_session.temp_root.string();
                        editor::keep_edit_session(state.pending_edit_session);
                        state.pending_edit_session = editor::EditSession{};
                        show_alert(i18n::get("tui_temp_files_kept_title"), i18n::get("tui_temp_files_kept_message", {{"PATH", kept_path}}), false);
                        return true;
                    }

                    editor::EditResult write_back_result;
                    auto finish_write_back = screen.WithRestoredIO([&] {
                        write_back_result = editor::write_back_session(state.pending_edit_session);
                    });
                    finish_write_back();
                    bool archive_updated = write_back_result.archive_updated;
                    std::string selected_path = state.pending_edit_session.entry_path;
                    state.pending_edit_session = editor::EditSession{};
                    if (archive_updated) {
                        list.load(archive_path, type, options.password);
                        list.select_path(selected_path);
                        preview_model.clear();
                        sync_preview();
                        show_alert(i18n::get("tui_write_back_complete"), write_back_result.message, false);
                    } else {
                        show_alert(i18n::get("tui_write_back_failed"), write_back_result.message, true);
                    }
                    return true;
                }
                return true;
            }

            if (event == Event::Escape) {
                state.quit = true;
                screen.Exit();
                return true;
            }

            if (focus == PanelFocus::Preview) {
                if (event == Event::Character('q')) {
                    state.quit = true;
                    screen.Exit();
                    return true;
                }
                if (preview.is_directory_view()) {
                    if (event == Event::Return) {
                        focus = PanelFocus::List;
                        state.mode = UiMode::Browse;
                        return true;
                    }
                    if (event == Event::ArrowLeft) {
                        focus = PanelFocus::List;
                        state.mode = UiMode::Browse;
                        return true;
                    }
                    if (event == Event::ArrowUp || event == Event::PageUp) {
                        list.component()->OnEvent(event);
                        sync_preview();
                        return true;
                    }
                    if (event == Event::ArrowDown || event == Event::PageDown) {
                        list.component()->OnEvent(event);
                        sync_preview();
                        return true;
                    }
                    if (event == Event::Home || event == Event::End) {
                        list.component()->OnEvent(event);
                        sync_preview();
                        return true;
                    }
                } else {
                    if (event == Event::Return) {
                        focus = PanelFocus::List;
                        state.mode = UiMode::Browse;
                        return true;
                    }
                    if (event == Event::ArrowUp) {
                        preview.scroll_up();
                        return true;
                    }
                    if (event == Event::ArrowDown) {
                        preview.scroll_down();
                        return true;
                    }
                    if (event == Event::ArrowLeft) {
                        focus = PanelFocus::List;
                        state.mode = UiMode::Browse;
                        return true;
                    }
                    if (event == Event::PageUp) {
                        preview.page_up();
                        return true;
                    }
                    if (event == Event::ArrowRight || event == Event::PageDown) {
                        preview.page_down();
                        return true;
                    }
                    if (event == Event::Home) {
                        preview.scroll_to_top();
                        return true;
                    }
                    if (event == Event::End) {
                        preview.scroll_to_bottom();
                        return true;
                    }
                }
            }

            if (event == Event::Character('q')) {
                state.quit = true;
                screen.Exit();
                return true;
            }

            if (event == Event::Character('/')) {
                state.mode = UiMode::Search;
                search_input = list.search_query();
                return true;
            }

            if (event == Event::Character('s')) {
                prepare_settings_modal(false);
                return true;
            }

            if (event == Event::Character('?')) {
                state.mode = UiMode::Help;
                return true;
            }

            if (event == Event::Tab || event == Event::TabReverse) {
                focus = (focus == PanelFocus::List) ? PanelFocus::Preview : PanelFocus::List;
                state.mode = (focus == PanelFocus::Preview) ? UiMode::PreviewScroll : UiMode::Browse;
                return true;
            }

            if (event == Event::Character('x')) {
                const auto* entry = list.selected_entry();
                if (entry && !entry->is_directory) {
                    state.extract.entry_path = entry->path;
                    state.extract.output_directory = ".";
                    state.mode = UiMode::ExtractDialog;
                }
                return true;
            }

            if (event == Event::Character('e')) {
                const auto* entry = list.selected_entry();
                if (!entry || entry->is_directory) {
                    show_alert(i18n::get("tui_edit_alert_title"), i18n::get("tui_edit_select_file_first"), true);
                    return true;
                }

                if (!tui_settings.has_editor_command()) {
                    prepare_settings_modal(true, entry->path);
                    return true;
                }

                launch_edit_for_path(entry->path);
                return true;
            }

            if (event.is_mouse()) {
                const Mouse& mouse = event.mouse();
                bool is_left_click = mouse.button == Mouse::Left && mouse.motion == Mouse::Pressed;
                bool is_wheel = mouse.button == Mouse::WheelUp || mouse.button == Mouse::WheelDown;

                if (preview.contains_point(mouse.x, mouse.y)) {
                    if (is_left_click || is_wheel) {
                        focus = PanelFocus::Preview;
                        state.mode = UiMode::PreviewScroll;
                    }
                    if (preview.handle_mouse(event)) {
                        return true;
                    }
                } else if (list.contains_point(mouse.x, mouse.y)) {
                    if (is_left_click || is_wheel) {
                        focus = PanelFocus::List;
                        state.mode = UiMode::Browse;
                    }
                }
            }

            if (event == Event::ArrowRight || event == Event::Return) {
                const auto* entry = list.selected_entry();
                if (entry && entry->is_directory && list.enter_selected()) {
                    sync_preview();
                    return true;
                }
                if (entry && !entry->is_directory) {
                    focus = PanelFocus::Preview;
                    state.mode = UiMode::PreviewScroll;
                    return true;
                }
            }

            if (event == Event::ArrowLeft || event == Event::Backspace) {
                if (list.go_parent()) {
                    sync_preview();
                    return true;
                }
            }

            bool handled = list.component()->OnEvent(event);
            if (handled) {
                sync_preview();
            }
            return handled;
        });

        screen.Loop(main_renderer);

        if (state.quit) {
            std::cout << i18n::get("goodbye") << std::endl;
        }
    }
}
