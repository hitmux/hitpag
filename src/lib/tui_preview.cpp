// Copyright (C) 2025 Hitmux
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#include "include/tui_preview.h"
#include "include/tui_archive_ops.h"
#include "include/i18n.h"

#include <ftxui/dom/elements.hpp>

#include <algorithm>
#include <sstream>
#include <iomanip>
#include <map>

namespace tui {
    using namespace ftxui;

    std::string PreviewPanel::format_size(uint64_t size) const {
        const char* units[] = {"B", "KB", "MB", "GB", "TB"};
        int unit_idx = 0;
        double dsize = static_cast<double>(size);

        while (dsize >= 1024.0 && unit_idx < 4) {
            dsize /= 1024.0;
            unit_idx++;
        }

        std::ostringstream oss;
        oss << std::fixed << std::setprecision(unit_idx > 0 ? 1 : 0) << dsize << " " << units[unit_idx];
        return oss.str();
    }

    void PreviewPanel::load(const std::string& archive_path, const std::string& entry_path, file_type::FileType type, const std::string& password) {
        lines_.clear();
        wrapped_lines_.clear();
        status_message_.clear();
        loaded_entry_path_ = entry_path;
        wrapped_width_ = 0;
        scroll_offset_ = 0;
        is_directory_view_ = false;

        archive_ops::TextExtractionResult extraction = archive_ops::extract_text(archive_path, entry_path, type, password);

        if (!extraction.success) {
            status_message_ = i18n::get("tui_preview_extract_failed");
            return;
        }

        if (extraction.empty_file) {
            status_message_ = i18n::get("tui_preview_empty_file");
            return;
        }

        std::string content = std::move(extraction.content);

        if (!archive_ops::is_text_content(content)) {
            status_message_ = i18n::get("tui_binary_file");
            return;
        }

        constexpr size_t MAX_PREVIEW_LINES = 500;
        constexpr size_t MAX_PREVIEW_SIZE = 64 * 1024;

        if (content.size() > MAX_PREVIEW_SIZE) {
            content = content.substr(0, MAX_PREVIEW_SIZE);
            status_message_ = i18n::get("tui_file_too_large");
        }

        std::istringstream stream(content);
        std::string line;
        size_t line_count = 0;

        while (std::getline(stream, line) && line_count < MAX_PREVIEW_LINES) {
            if (line.size() > 200) {
                line = line.substr(0, 197) + "...";
            }
            lines_.push_back(line);
            line_count++;
        }

        if (line_count >= MAX_PREVIEW_LINES) {
            lines_.push_back("... (truncated)");
        }
    }

    void PreviewPanel::load_directory(const std::string& dir_path, const std::vector<archive_ops::ArchiveEntry>& entries) {
        lines_.clear();
        wrapped_lines_.clear();
        status_message_.clear();
        loaded_entry_path_ = dir_path;
        wrapped_width_ = 0;
        scroll_offset_ = 0;
        is_directory_view_ = true;
        dir_entries_.clear();
        dir_base_path_ = dir_path;
        selected_dir_entry_ = -1;

        std::map<std::string, std::pair<bool, uint64_t>> children;
        std::string prefix = dir_path.empty() ? "" : dir_path + "/";

        for (const auto& entry : entries) {
            if (prefix.empty() || entry.path.rfind(prefix, 0) == 0) {
                std::string relative = prefix.empty() ? entry.path : entry.path.substr(prefix.size());
                if (relative.empty()) continue;

                size_t slash_pos = relative.find('/');
                if (slash_pos == std::string::npos) {
                    std::string name = relative;
                    if (entry.is_directory) {
                        auto it = children.find(name);
                        if (it == children.end()) {
                            children[name] = {true, 0};
                        }
                    } else {
                        children[name] = {false, entry.size};
                    }
                } else {
                    std::string dir_name = relative.substr(0, slash_pos);
                    auto it = children.find(dir_name);
                    if (it == children.end()) {
                        children[dir_name] = {true, 0};
                    }
                }
            }
        }

        size_t directory_count = 0;
        size_t file_count = 0;
        uint64_t total_size = 0;
        for (const auto& [_, info] : children) {
            if (info.first) {
                ++directory_count;
            } else {
                ++file_count;
                total_size += info.second;
            }
        }

        if (children.empty()) {
            status_message_ = i18n::get("tui_preview_empty_directory");
            return;
        }

        std::ostringstream summary;
        summary << i18n::get("tui_preview_directory_prefix") << ": "
                << directory_count << " " << i18n::get("tui_preview_directories_suffix") << ", "
                << file_count << " " << i18n::get("tui_preview_files_suffix") << ", "
                << format_size(total_size);
        lines_.push_back(summary.str());
        lines_.push_back("");
        lines_.push_back(i18n::get("tui_preview_first_entries"));

        size_t shown = 0;
        for (const auto& [name, info] : children) {
            if (shown >= 12) {
                lines_.push_back("...");
                break;
            }
            bool is_dir = info.first;
            uint64_t size = info.second;
            std::string icon = is_dir ? "[D] " : "    ";
            std::string size_str = is_dir ? "" : format_size(size);
            lines_.push_back(icon + name + (is_dir ? "" : " (" + size_str + ")"));

            std::string full_path = prefix.empty() ? name : prefix + name;
            dir_entries_.push_back({full_path, is_dir});
            ++shown;
        }
    }

    void PreviewPanel::clear() {
        lines_.clear();
        wrapped_lines_.clear();
        status_message_.clear();
        loaded_entry_path_.clear();
        wrapped_width_ = 0;
        scroll_offset_ = 0;
        is_directory_view_ = false;
        dir_entries_.clear();
        dir_base_path_.clear();
    }

    int PreviewPanel::content_width() const {
        int width = box_.x_max - box_.x_min + 1;
        if (width <= 2) {
            return 80;
        }
        return std::max(1, width - 2);
    }

    int PreviewPanel::visible_line_capacity() const {
        int height = box_.y_max - box_.y_min + 1;
        if (height <= 1) {
            return 15;
        }

        int reserved_lines = 2;
        if (!lines_.empty()) {
            reserved_lines += 2;
        }
        if (!status_message_.empty()) {
            reserved_lines += 2;
        }

        return std::max(1, height - reserved_lines);
    }

    int PreviewPanel::max_scroll_offset() const {
        if (is_directory_view_) {
            return std::max(0, static_cast<int>(lines_.size()) - visible_line_capacity());
        }
        refresh_wrapped_lines();
        return std::max(0, static_cast<int>(wrapped_lines_.size()) - visible_line_capacity());
    }

    void PreviewPanel::clamp_scroll() {
        scroll_offset_ = std::clamp(scroll_offset_, 0, max_scroll_offset());
    }

    void PreviewPanel::refresh_wrapped_lines() const {
        int width = content_width();
        if (wrapped_width_ == width && !wrapped_lines_.empty()) {
            return;
        }

        wrapped_lines_.clear();
        wrapped_width_ = width;

        if (lines_.empty()) {
            return;
        }

        for (const auto& line : lines_) {
            append_wrapped_line(line, width);
        }
    }

    void PreviewPanel::append_wrapped_line(const std::string& line, int width) const {
        if (width <= 0) {
            wrapped_lines_.push_back("");
            return;
        }

        if (line.empty()) {
            wrapped_lines_.push_back("");
            return;
        }

        size_t start = 0;
        while (start < line.size()) {
            size_t remaining = line.size() - start;
            if (remaining <= static_cast<size_t>(width)) {
                wrapped_lines_.push_back(line.substr(start));
                return;
            }

            size_t candidate = start + static_cast<size_t>(width);
            size_t break_pos = line.find_last_of(" \t", candidate - 1);
            if (break_pos != std::string::npos && break_pos >= start) {
                if (break_pos == start) {
                    wrapped_lines_.push_back(line.substr(start, static_cast<size_t>(width)));
                    start += static_cast<size_t>(width);
                    continue;
                }

                wrapped_lines_.push_back(line.substr(start, break_pos - start));
                start = break_pos + 1;
                while (start < line.size() && (line[start] == ' ' || line[start] == '\t')) {
                    ++start;
                }
                continue;
            }

            wrapped_lines_.push_back(line.substr(start, static_cast<size_t>(width)));
            start += static_cast<size_t>(width);
        }
    }

    std::string PreviewPanel::scroll_progress_text(int start, int last) const {
        if (wrapped_lines_.empty()) {
            return i18n::get("tui_preview_progress_empty");
        }

        std::ostringstream oss;
        oss << i18n::get("tui_preview") << ": " << (start + 1) << "-" << last << " / " << wrapped_lines_.size();
        return oss.str();
    }

    void PreviewPanel::scroll_up() {
        if (scroll_offset_ > 0) {
            --scroll_offset_;
        }
    }

    void PreviewPanel::scroll_down() {
        if (scroll_offset_ < max_scroll_offset()) {
            ++scroll_offset_;
        }
    }

    void PreviewPanel::page_up() {
        scroll_offset_ -= visible_line_capacity();
        clamp_scroll();
    }

    void PreviewPanel::page_down() {
        scroll_offset_ += visible_line_capacity();
        clamp_scroll();
    }

    void PreviewPanel::scroll_to_top() {
        scroll_offset_ = 0;
    }

    void PreviewPanel::scroll_to_bottom() {
        scroll_offset_ = max_scroll_offset();
    }

    bool PreviewPanel::handle_mouse(Event event) {
        if (!event.is_mouse() || !box_.Contain(event.mouse().x, event.mouse().y)) {
            return false;
        }

        const Mouse& mouse = event.mouse();
        if (mouse.button == Mouse::WheelUp) {
            scroll_up();
            return true;
        }
        if (mouse.button == Mouse::WheelDown) {
            scroll_down();
            return true;
        }

        return false;
    }

    void PreviewPanel::dir_entry_up() {
        if (!is_directory_view_ || dir_entries_.empty()) return;
        if (selected_dir_entry_ > 0) {
            --selected_dir_entry_;
        }
        if (selected_dir_entry_ < scroll_offset_) {
            scroll_offset_ = selected_dir_entry_;
        }
    }

    void PreviewPanel::dir_entry_down() {
        if (!is_directory_view_ || dir_entries_.empty()) return;
        if (selected_dir_entry_ < static_cast<int>(dir_entries_.size()) - 1) {
            ++selected_dir_entry_;
        }
        int capacity = visible_line_capacity();
        if (selected_dir_entry_ >= scroll_offset_ + capacity) {
            scroll_offset_ = selected_dir_entry_ - capacity + 1;
        }
    }

    void PreviewPanel::dir_entry_activate() {
        if (!is_directory_view_ || dir_entries_.empty()) return;
        if (selected_dir_entry_ >= 0 && selected_dir_entry_ < static_cast<int>(dir_entries_.size())) {
            if (on_entry_activated) {
                on_entry_activated(dir_entries_[selected_dir_entry_].full_path, dir_entries_[selected_dir_entry_].is_directory);
            }
        }
    }

    void PreviewPanel::dir_entry_page_up() {
        if (!is_directory_view_ || dir_entries_.empty()) return;
        int capacity = visible_line_capacity();
        selected_dir_entry_ = std::max(0, selected_dir_entry_ - capacity);
        scroll_offset_ = std::max(0, scroll_offset_ - capacity);
    }

    void PreviewPanel::dir_entry_page_down() {
        if (!is_directory_view_ || dir_entries_.empty()) return;
        int capacity = visible_line_capacity();
        selected_dir_entry_ = std::min(static_cast<int>(dir_entries_.size()) - 1, selected_dir_entry_ + capacity);
        scroll_offset_ = std::min(static_cast<int>(dir_entries_.size()) - 1, scroll_offset_ + capacity);
        int max_scroll = std::max(0, static_cast<int>(dir_entries_.size()) - capacity);
        scroll_offset_ = std::clamp(scroll_offset_, 0, max_scroll);
    }

    void PreviewPanel::dir_entry_home() {
        if (!is_directory_view_ || dir_entries_.empty()) return;
        selected_dir_entry_ = 0;
        scroll_offset_ = 0;
    }

    void PreviewPanel::dir_entry_end() {
        if (!is_directory_view_ || dir_entries_.empty()) return;
        selected_dir_entry_ = static_cast<int>(dir_entries_.size()) - 1;
        int capacity = visible_line_capacity();
        scroll_offset_ = std::max(0, static_cast<int>(dir_entries_.size()) - capacity);
    }

    Element PreviewPanel::render() const {
        Elements items;

        if (!status_message_.empty() && lines_.empty()) {
            items.push_back(paragraph(status_message_) | dim);
        } else if (is_directory_view_) {
            int capacity = visible_line_capacity();
            int start = std::clamp(scroll_offset_, 0, max_scroll_offset());
            int last = std::min(static_cast<int>(lines_.size()), start + capacity);
            for (int i = start; i < last; ++i) {
                bool is_dir = lines_[i].rfind("[D] ", 0) == 0;
                if (is_dir) {
                    items.push_back(text("  " + lines_[i]) | color(Color::White));
                } else {
                    items.push_back(text("  " + lines_[i]) | color(Color::GrayLight));
                }
            }
            if (!lines_.empty()) {
                items.push_back(separator() | color(Color::Blue));
                std::ostringstream oss;
                oss << i18n::get("tui_preview_directory_prefix") << ": " << (start + 1) << "-" << last << " / " << lines_.size() << " " << i18n::get("tui_preview_entries_suffix");
                items.push_back(text(oss.str()) | color(Color::Blue) | dim);
            }
        } else {
            refresh_wrapped_lines();
            int start = std::clamp(scroll_offset_, 0, max_scroll_offset());
            int visible_count = visible_line_capacity();
            int last = std::min(static_cast<int>(wrapped_lines_.size()), start + visible_count);
            for (int i = start; i < last; ++i) {
                items.push_back(text(wrapped_lines_[i]));
            }

            if (!wrapped_lines_.empty()) {
                items.push_back(separator());
                items.push_back(text(scroll_progress_text(start, last)) | dim);
            }

            if (!status_message_.empty()) {
                items.push_back(separator());
                items.push_back(paragraph(status_message_) | dim);
            }
        }

        return vbox(items) | border | size(HEIGHT, GREATER_THAN, 5) | reflect(box_);
    }
}
