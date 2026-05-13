// Copyright (C) 2025 Hitmux
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#include "include/tui_archive_list.h"
#include "include/i18n.h"

#include <ftxui/dom/elements.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>

#include <algorithm>
#include <cctype>
#include <map>
#include <utility>

namespace tui {
    using namespace ftxui;

    namespace {
        std::string to_lower(std::string value) {
            std::transform(value.begin(), value.end(), value.begin(),
                [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            return value;
        }

        std::string parent_directory(std::string path) {
            if (path.empty()) return "";

            size_t pos = path.rfind('/');
            if (pos == std::string::npos) return "";
            return path.substr(0, pos);
        }
    }

    void ArchiveList::load(const std::string& archive_path, file_type::FileType type, const std::string& password) {
        archive_path_ = archive_path;
        type_ = type;
        entries_ = archive_ops::list_archive(archive_path, type, password);
        selected_idx_ = 0;
        scroll_offset_ = 0;
        current_directory_.clear();
        search_query_.clear();
        apply_filter();
        component_.reset();
    }

    void ArchiveList::set_search_query(const std::string& query) {
        search_query_ = query;
        selected_idx_ = 0;
        scroll_offset_ = 0;
        apply_filter();
    }

    void ArchiveList::apply_filter() {
        visible_entries_.clear();

        std::string lower_query = to_lower(search_query_);
        std::map<std::string, VisibleEntry> child_directories;

        if (search_query_.empty() && !current_directory_.empty()) {
            VisibleEntry parent;
            parent.display_name = "..";
            parent.full_path = parent_directory(current_directory_);
            parent.is_directory = true;
            parent.is_parent = true;
            visible_entries_.push_back(parent);
        }

        for (size_t i = 0; i < entries_.size(); ++i) {
            const auto& entry = entries_[i];
            std::string lower_path = to_lower(entry.path);

            if (!lower_query.empty() && lower_path.find(lower_query) == std::string::npos) {
                continue;
            }

            if (!search_query_.empty()) {
                VisibleEntry visible;
                size_t pos = entry.path.find_last_of('/');
                visible.display_name = (pos == std::string::npos) ? entry.path : entry.path.substr(pos + 1);
                visible.full_path = entry.path;
                visible.is_directory = entry.is_directory;
                visible.entry_index = i;
                visible_entries_.push_back(visible);
                continue;
            }

            std::string prefix = current_directory_.empty() ? "" : current_directory_ + "/";
            if (!prefix.empty() && entry.path.rfind(prefix, 0) != 0) {
                continue;
            }

            std::string relative = prefix.empty() ? entry.path : entry.path.substr(prefix.size());
            if (relative.empty()) {
                continue;
            }

            size_t slash_pos = relative.find('/');
            if (slash_pos == std::string::npos) {
                if (entry.is_directory) {
                    auto [it, inserted] = child_directories.emplace(relative, VisibleEntry{});
                    if (inserted) {
                        it->second.display_name = relative;
                        it->second.full_path = entry.path;
                        it->second.is_directory = true;
                        it->second.entry_index = i;
                    } else if (!it->second.entry_index.has_value()) {
                        it->second.entry_index = i;
                    }
                } else {
                    VisibleEntry visible;
                    visible.display_name = relative;
                    visible.full_path = entry.path;
                    visible.is_directory = false;
                    visible.entry_index = i;
                    visible_entries_.push_back(visible);
                }
                continue;
            }

            std::string child_name = relative.substr(0, slash_pos);
            if (child_name.empty()) {
                continue;
            }

            auto [it, inserted] = child_directories.emplace(child_name, VisibleEntry{});
            if (inserted) {
                it->second.display_name = child_name;
                it->second.full_path = build_full_path(child_name);
                it->second.is_directory = true;
            }
        }

        for (auto& [_, child] : child_directories) {
            visible_entries_.push_back(std::move(child));
        }

        std::sort(visible_entries_.begin(), visible_entries_.end(), [](const ArchiveList::VisibleEntry& a, const ArchiveList::VisibleEntry& b) {
            if (a.is_parent != b.is_parent) return a.is_parent > b.is_parent;
            if (a.is_directory != b.is_directory) return a.is_directory > b.is_directory;
            if (a.display_name != b.display_name) return a.display_name < b.display_name;
            if (a.full_path != b.full_path) return a.full_path < b.full_path;
            if (a.entry_index.has_value() != b.entry_index.has_value()) return a.entry_index.has_value() > b.entry_index.has_value();
            return false;
        });

        visible_entries_.erase(
            std::unique(visible_entries_.begin(), visible_entries_.end(), [](const ArchiveList::VisibleEntry& a, const ArchiveList::VisibleEntry& b) {
                return a.display_name == b.display_name &&
                       a.full_path == b.full_path &&
                       a.is_directory == b.is_directory &&
                       a.is_parent == b.is_parent;
            }),
            visible_entries_.end());

        clamp_selection();
    }

    void ArchiveList::clamp_selection() {
        if (visible_entries_.empty()) {
            selected_idx_ = 0;
            scroll_offset_ = 0;
            return;
        }

        if (selected_idx_ >= static_cast<int>(visible_entries_.size())) {
            selected_idx_ = static_cast<int>(visible_entries_.size()) - 1;
        }
        if (selected_idx_ < 0) {
            selected_idx_ = 0;
        }
        if (scroll_offset_ > selected_idx_) {
            scroll_offset_ = selected_idx_;
        }
        if (scroll_offset_ < 0) {
            scroll_offset_ = 0;
        }

        ensure_selected_visible();
    }

    int ArchiveList::visible_capacity() const {
        int height = box_.y_max - box_.y_min + 1;
        if (height <= 1) {
            return 12;
        }
        return std::max(1, height);
    }

    void ArchiveList::ensure_selected_visible() {
        int capacity = visible_capacity();
        if (selected_idx_ < scroll_offset_) {
            scroll_offset_ = selected_idx_;
        }
        if (selected_idx_ >= scroll_offset_ + capacity) {
            scroll_offset_ = selected_idx_ - capacity + 1;
        }
        if (scroll_offset_ < 0) {
            scroll_offset_ = 0;
        }
    }

    std::string ArchiveList::build_full_path(const std::string& name) const {
        if (current_directory_.empty()) {
            return name;
        }
        if (name.empty()) {
            return current_directory_;
        }
        return current_directory_ + "/" + name;
    }

    const archive_ops::ArchiveEntry* ArchiveList::selected_entry() const {
        if (selected_idx_ < 0 || selected_idx_ >= static_cast<int>(visible_entries_.size())) {
            return nullptr;
        }

        const auto& visible = visible_entries_[selected_idx_];
        if (visible.entry_index.has_value()) {
            const auto* entry = &entries_[*visible.entry_index];
            if (entry->is_directory && entry->size == 0) {
                synthetic_entry_ = *entry;
                synthetic_entry_.size = calculate_directory_size(visible.full_path);
                return &synthetic_entry_;
            }
            return entry;
        }

        synthetic_entry_ = archive_ops::ArchiveEntry{};
        synthetic_entry_.path = visible.full_path;
        synthetic_entry_.is_directory = visible.is_directory;
        if (visible.is_directory) {
            synthetic_entry_.size = calculate_directory_size(visible.full_path);
        }
        return &synthetic_entry_;
    }

    uint64_t ArchiveList::calculate_directory_size(const std::string& dir_path) const {
        uint64_t total_size = 0;
        std::string prefix = dir_path.empty() ? "" : dir_path + "/";

        for (const auto& entry : entries_) {
            if (!entry.is_directory) {
                if (prefix.empty() || entry.path.rfind(prefix, 0) == 0) {
                    total_size += entry.size;
                }
            }
        }

        return total_size;
    }

    void ArchiveList::scroll_up() {
        if (selected_idx_ > 0) {
            selected_idx_--;
            ensure_selected_visible();
        }
    }

    void ArchiveList::scroll_down() {
        if (selected_idx_ < static_cast<int>(visible_entries_.size()) - 1) {
            selected_idx_++;
            ensure_selected_visible();
        }
    }

    void ArchiveList::page_up() {
        selected_idx_ = std::max(0, selected_idx_ - visible_capacity());
        ensure_selected_visible();
    }

    void ArchiveList::page_down() {
        selected_idx_ = std::min(static_cast<int>(visible_entries_.size()) - 1, selected_idx_ + visible_capacity());
        ensure_selected_visible();
    }

    bool ArchiveList::enter_selected() {
        if (selected_idx_ < 0 || selected_idx_ >= static_cast<int>(visible_entries_.size())) {
            return false;
        }

        const auto& visible = visible_entries_[selected_idx_];
        if (!visible.is_directory) {
            return false;
        }

        current_directory_ = visible.full_path;
        selected_idx_ = 0;
        scroll_offset_ = 0;
        apply_filter();
        return true;
    }

    bool ArchiveList::go_parent() {
        if (!search_query_.empty()) {
            return false;
        }
        if (current_directory_.empty()) {
            return false;
        }

        std::string previous_directory = current_directory_;
        current_directory_ = parent_directory(current_directory_);
        selected_idx_ = 0;
        scroll_offset_ = 0;
        apply_filter();

        std::string target_name = previous_directory.substr(current_directory_.empty() ? 0 : current_directory_.size() + 1);
        for (size_t i = 0; i < visible_entries_.size(); ++i) {
            if (visible_entries_[i].display_name == target_name) {
                selected_idx_ = static_cast<int>(i);
                break;
            }
        }
        clamp_selection();
        return true;
    }

    void ArchiveList::select_path(const std::string& path) {
        search_query_.clear();
        current_directory_ = parent_directory(path);
        selected_idx_ = 0;
        scroll_offset_ = 0;
        apply_filter();

        for (size_t i = 0; i < visible_entries_.size(); ++i) {
            if (visible_entries_[i].full_path == path) {
                selected_idx_ = static_cast<int>(i);
                break;
            }
        }

        clamp_selection();
    }

    Element ArchiveList::render() {
        Elements items;

        if (entries_.empty()) {
            items.push_back(text(i18n::get("tui_no_entries")));
        } else if (visible_entries_.empty()) {
            items.push_back(text(i18n::get("tui_no_entries")));
        } else {
            int capacity = visible_capacity();
            int last = std::min(static_cast<int>(visible_entries_.size()), scroll_offset_ + capacity);

            for (int i = scroll_offset_; i < last; ++i) {
                const auto& entry = visible_entries_[i];
                std::string icon = entry.is_parent ? "[U] " : (entry.is_directory ? "[D] " : "    ");
                std::string display = icon + entry.display_name;

                if (i == selected_idx_) {
                    items.push_back(text("> " + display) | bold | color(Color::White) | bgcolor(Color::Blue));
                } else if (entry.is_parent) {
                    items.push_back(text("  " + display) | color(Color::Blue));
                } else if (entry.is_directory) {
                    items.push_back(text("  " + display) | color(Color::White));
                } else {
                    items.push_back(text("  " + display) | color(Color::GrayLight));
                }
            }
        }

        return vbox(items) | size(HEIGHT, GREATER_THAN, 3) | reflect(box_);
    }

    Component ArchiveList::component() {
        if (component_) return component_;

        component_ = Renderer([&]() -> Element {
            return render();
        }) | CatchEvent([&](Event event) {
            if (visible_entries_.empty()) return false;

            if (event == Event::ArrowUp || event == Event::Character('k')) {
                scroll_up();
                return true;
            }
            if (event == Event::ArrowDown || event == Event::Character('j')) {
                scroll_down();
                return true;
            }
            if (event == Event::PageUp) {
                page_up();
                return true;
            }
            if (event == Event::PageDown) {
                page_down();
                return true;
            }
            if (event == Event::Home) {
                selected_idx_ = 0;
                scroll_offset_ = 0;
                return true;
            }
            if (event == Event::End) {
                selected_idx_ = static_cast<int>(visible_entries_.size()) - 1;
                ensure_selected_visible();
                return true;
            }
            if (event.is_mouse() && box_.Contain(event.mouse().x, event.mouse().y)) {
                const Mouse& mouse = event.mouse();

                if (mouse.button == Mouse::WheelUp) {
                    scroll_up();
                    return true;
                }
                if (mouse.button == Mouse::WheelDown) {
                    scroll_down();
                    return true;
                }

                if (mouse.button == Mouse::Left && mouse.motion == Mouse::Pressed) {
                    int clicked_index = scroll_offset_ + (mouse.y - box_.y_min);
                    if (clicked_index >= 0 && clicked_index < static_cast<int>(visible_entries_.size())) {
                        bool activate = clicked_index == selected_idx_;
                        selected_idx_ = clicked_index;
                        ensure_selected_visible();
                        if (activate) {
                            enter_selected();
                        }
                        return true;
                    }
                }

                if (mouse.button == Mouse::Right && mouse.motion == Mouse::Pressed) {
                    return go_parent();
                }
            }
            if ((event == Event::ArrowRight || event == Event::Return) && enter_selected()) {
                return true;
            }
            if (event == Event::ArrowLeft && go_parent()) {
                return true;
            }
            return false;
        });

        return component_;
    }
}
