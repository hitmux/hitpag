// Copyright (C) 2025 Hitmux
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#include "include/tui_preview_model.h"

namespace tui {
    void PreviewModel::sync_selection(const archive_ops::ArchiveEntry* entry,
                                      const std::vector<archive_ops::ArchiveEntry>& entries,
                                      const std::string& archive_path,
                                      file_type::FileType type,
                                      const std::string& password) {
        if (!entry) {
            clear();
            return;
        }

        if (loaded_entry_path_ == entry->path && loaded_directory_ == entry->is_directory && panel_.has_content()) {
            return;
        }

        loaded_entry_path_ = entry->path;
        loaded_directory_ = entry->is_directory;
        if (entry->is_directory) {
            panel_.load_directory(entry->path, entries);
        } else {
            panel_.load(archive_path, entry->path, type, password);
        }
    }

    void PreviewModel::clear() {
        panel_.clear();
        loaded_entry_path_.clear();
        loaded_directory_ = false;
    }
}
