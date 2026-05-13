// Copyright (C) 2025 Hitmux
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#pragma once

#include "include/file_type.h"
#include "include/tui_archive_ops.h"
#include "include/tui_preview.h"

#include <string>
#include <vector>

namespace tui {
    class PreviewModel {
    public:
        void sync_selection(const archive_ops::ArchiveEntry* entry,
                            const std::vector<archive_ops::ArchiveEntry>& entries,
                            const std::string& archive_path,
                            file_type::FileType type,
                            const std::string& password);
        void clear();
        PreviewPanel& panel() { return panel_; }
        const PreviewPanel& panel() const { return panel_; }
        const std::string& loaded_entry_path() const { return loaded_entry_path_; }

    private:
        PreviewPanel panel_;
        std::string loaded_entry_path_;
        bool loaded_directory_ = false;
    };
}
