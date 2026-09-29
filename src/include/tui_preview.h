// Copyright (C) 2025 Hitmux
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <functional>
#include "include/file_type.h"
#include "include/tui_archive_ops.h"
#include "include/tui_image.h"
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/box.hpp>

namespace tui {
    class PreviewPanel {
    public:
        void load(const std::string& archive_path, const std::string& entry_path, file_type::FileType type, const std::string& password = "");
        void load_directory(const std::string& dir_path, const std::vector<archive_ops::ArchiveEntry>& entries);
        void clear();
        ftxui::Element render() const;
        bool has_content() const { return !lines_.empty() || !status_message_.empty() || is_image_view_; }
        const std::string& loaded_entry_path() const { return loaded_entry_path_; }

        void scroll_up();
        void scroll_down();
        void page_up();
        void page_down();
        void scroll_to_top();
        void scroll_to_bottom();
        bool handle_mouse(ftxui::Event event);
        bool contains_point(int x, int y) const { return box_.Contain(x, y); }

        std::function<void(const std::string& entry_path, bool is_directory)> on_entry_activated;
        bool is_directory_view() const { return is_directory_view_; }
        bool is_image_view() const { return is_image_view_; }
        int selected_dir_entry() const { return selected_dir_entry_; }
        size_t dir_entry_count() const { return dir_entries_.size(); }
        void dir_entry_up();
        void dir_entry_down();
        void dir_entry_activate();
        void dir_entry_page_up();
        void dir_entry_page_down();
        void dir_entry_home();
        void dir_entry_end();

    private:
        int content_width() const;
        int visible_line_capacity() const;
        int max_scroll_offset() const;
        void clamp_scroll();
        void refresh_wrapped_lines() const;
        void append_wrapped_line(const std::string& line, int width) const;
        std::string scroll_progress_text(int start, int last) const;
        std::string format_size(uint64_t size) const;

        void reset_image_state();
        bool load_image(const std::string& archive_path, const std::string& entry_path, file_type::FileType type, const std::string& password);
        int box_height() const;
        int image_area_width() const;
        int image_area_rows() const;
        ftxui::Element build_image_element() const;

        std::vector<std::string> lines_;
        mutable std::vector<std::string> wrapped_lines_;
        std::string status_message_;
        std::string loaded_entry_path_;
        int scroll_offset_ = 0;
        mutable int wrapped_width_ = 0;
        mutable ftxui::Box box_;
        bool is_directory_view_ = false;
        int selected_dir_entry_ = -1;

        bool is_image_view_ = false;
        image::Image image_;
        std::string image_caption_;
        mutable ftxui::Element image_element_;
        mutable int image_element_columns_ = 0;
        mutable int image_element_rows_ = 0;

        struct DirEntryInfo {
            std::string full_path;
            bool is_directory;
        };
        std::vector<DirEntryInfo> dir_entries_;
        std::string dir_base_path_;
    };
}
