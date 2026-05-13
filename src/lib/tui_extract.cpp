// Copyright (C) 2025 Hitmux
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#include "include/tui_extract.h"
#include "include/tui_archive_ops.h"
#include "include/i18n.h"

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include <filesystem>

namespace fs = std::filesystem;

namespace tui {
    using namespace ftxui;

    bool ExtractDialog::show(const std::string& archive_path, const std::string& entry_path, file_type::FileType type, const std::string& password) {
        std::string input_value = ".";
        bool confirmed = false;
        bool canceled = false;
        message_.clear();
        success_ = false;
        result_ = Result::Canceled;

        auto screen = ScreenInteractive::Fullscreen();

        auto input = Input(&input_value, "");

        auto container = Container::Vertical({input});

        auto renderer = Renderer(container, [&]() -> Element {
            return vbox({
                text(i18n::get("tui_extract")) | bold,
                separator(),
                text(i18n::get("tui_extracting_entry", {{"PATH", entry_path}})),
                text(i18n::get("tui_extract_prompt")),
                input->Render(),
                separator(),
                hbox({
                    text(i18n::get("tui_confirm_hint")) | dim,
                    text("  "),
                    text(i18n::get("tui_cancel_hint")) | dim,
                }),
            }) | border | center;
        }) | CatchEvent([&](Event event) {
            if (event == Event::Return) {
                confirmed = true;
                screen.Exit();
                return true;
            }
            if (event == Event::Escape) {
                canceled = true;
                screen.Exit();
                return true;
            }
            return false;
        });

        screen.Loop(renderer);

        if (canceled) {
            result_ = Result::Canceled;
            return false;
        }

        if (input_value.empty()) {
            input_value = ".";
        }

        std::string out_dir = input_value;
        if (out_dir.empty()) out_dir = ".";
        fs::path out_path(out_dir);
        std::error_code create_ec;
        fs::create_directories(out_path, create_ec);
        if (create_ec) {
            success_ = false;
            result_ = Result::Failure;
            message_ = i18n::get("tui_extract_create_dir_failed", {{"PATH", out_dir}, {"REASON", create_ec.message()}});
            return false;
        }

        try {
            success_ = archive_ops::extract_single(archive_path, entry_path, out_dir, type, password);
        } catch (const std::exception& ex) {
            success_ = false;
            result_ = Result::Failure;
            message_ = i18n::get("tui_extract_exception_failed", {{"REASON", ex.what()}});
            return false;
        }
        result_ = success_ ? Result::Success : Result::Failure;
        message_ = success_ ? i18n::get("tui_extract_success") : i18n::get("tui_extract_failed");
        return success_;
    }
}
