// Copyright (C) 2025 Hitmux
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#include "include/tui_details.h"
#include "include/i18n.h"

#include <ftxui/dom/elements.hpp>
#include <sstream>
#include <iomanip>

namespace tui {
    using namespace ftxui;

    std::string DetailsPanel::format_size(uint64_t size) const {
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

    void DetailsPanel::set_entry(const archive_ops::ArchiveEntry* entry) {
        current_entry_ = entry;
    }

    Element DetailsPanel::render() const {
        if (!current_entry_) {
            return text(i18n::get("tui_no_file_selected")) | dim;
        }

        auto detail_row = [](const std::string& label, const std::string& value) {
            return hbox({
                text(label) | color(Color::Blue) | size(WIDTH, EQUAL, 12),
                paragraph(value) | flex,
            });
        };

        Elements items;
        items.push_back(paragraph(current_entry_->path) | bold | color(Color::White));
        items.push_back(separator() | color(Color::Blue));

        items.push_back(detail_row(i18n::get("tui_detail_type"), current_entry_->is_directory ? i18n::get("tui_detail_directory") : i18n::get("tui_detail_file")));
        items.push_back(detail_row(i18n::get("tui_detail_size"), format_size(current_entry_->size)));

        if (current_entry_->compressed_size > 0) {
            items.push_back(detail_row(i18n::get("tui_detail_compressed"), format_size(current_entry_->compressed_size)));

            if (current_entry_->size > 0) {
                double ratio = (1.0 - static_cast<double>(current_entry_->compressed_size) / current_entry_->size) * 100.0;
                std::string ratio_str;
                {
                    std::ostringstream oss;
                    oss << std::fixed << std::setprecision(1) << ratio << "%";
                    ratio_str = oss.str();
                }
                items.push_back(detail_row(i18n::get("tui_detail_ratio"), ratio_str));
            }
        }

        if (!current_entry_->modified.empty()) {
            items.push_back(detail_row(i18n::get("tui_detail_modified"), current_entry_->modified));
        }

        if (!current_entry_->method.empty()) {
            items.push_back(detail_row(i18n::get("tui_detail_method"), current_entry_->method));
        }

        if (current_entry_->crc > 0) {
            std::string crc_str;
            {
                std::ostringstream oss;
                oss << std::hex << std::setw(8) << std::setfill('0') << current_entry_->crc;
                crc_str = oss.str();
            }
            items.push_back(detail_row(i18n::get("tui_detail_crc"), crc_str));
        }

        return vbox(items) | border;
    }
}
