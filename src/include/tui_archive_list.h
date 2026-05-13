// Copyright (C) 2025 Hitmux
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include "include/tui_archive_ops.h"
#include "include/file_type.h"

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/box.hpp>

namespace tui {
    class ArchiveList {
    public:
        void load(const std::string& archive_path, file_type::FileType type, const std::string& password = "");
        bool loaded() const { return !archive_path_.empty(); }

        ftxui::Component component();

        const std::vector<archive_ops::ArchiveEntry>& entries() const { return entries_; }
        const archive_ops::ArchiveEntry* selected_entry() const;
        int selected_index() const { return selected_idx_; }
        void set_selected_index(int idx) { selected_idx_ = idx; }
        const std::string& current_directory() const { return current_directory_; }
        bool contains_point(int x, int y) const { return box_.Contain(x, y); }

        void set_search_query(const std::string& query);
        const std::string& search_query() const { return search_query_; }

        void scroll_up();
        void scroll_down();
        void page_up();
        void page_down();
        bool enter_selected();
        bool go_parent();
        void select_path(const std::string& path);

    private:
        struct VisibleEntry {
            std::string display_name;
            std::string full_path;
            bool is_directory = false;
            bool is_parent = false;
            std::optional<size_t> entry_index;
        };

        std::string archive_path_;
        file_type::FileType type_ = file_type::FileType::UNKNOWN;
        std::vector<archive_ops::ArchiveEntry> entries_;
        std::vector<VisibleEntry> visible_entries_;
        int selected_idx_ = 0;
        int scroll_offset_ = 0;
        std::string current_directory_;
        std::string search_query_;
        ftxui::Component component_;
        mutable archive_ops::ArchiveEntry synthetic_entry_;

        void apply_filter();
        void clamp_selection();
        std::string build_full_path(const std::string& name) const;
        int visible_capacity() const;
        void ensure_selected_visible();
        ftxui::Element render();
        uint64_t calculate_directory_size(const std::string& dir_path) const;

        ftxui::Box box_;
    };
}
