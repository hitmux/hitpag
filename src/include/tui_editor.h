// Copyright (C) 2025 Hitmux
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#pragma once

#include <filesystem>
#include <string>

#include "include/args.h"
#include "include/file_type.h"
#include "include/tui_settings.h"

namespace tui::editor {
    struct EditSession {
        enum class Outcome {
            None,
            NoChanges,
            WriteBackPending,
            TemporaryOnlyKept,
            Error,
        };

        bool launched = false;
        bool file_changed = false;
        bool write_back_supported = false;
        bool keep_temp_files = false;
        std::string archive_path;
        std::string entry_path;
        std::string password;
        file_type::FileType type = file_type::FileType::UNKNOWN;
        args::Options options;
        std::filesystem::path temp_root;
        std::filesystem::path temp_file;
        std::string message;
        Outcome outcome = Outcome::None;

        bool pending_write_back() const {
            return launched && file_changed && write_back_supported && !temp_root.empty();
        }

        bool temporary_only_kept() const {
            return launched && file_changed && keep_temp_files && !temp_root.empty();
        }
    };

    struct EditResult {
        bool launched = false;
        bool file_changed = false;
        bool archive_updated = false;
        std::string message;
    };

    EditSession start_edit_session(const std::string& archive_path,
                                   const std::string& entry_path,
                                   file_type::FileType type,
                                   const std::string& password,
                                   const args::Options& options,
                                   const settings::TuiSettings& settings);

    EditResult write_back_session(EditSession& session);
    void discard_edit_session(EditSession& session);
    void keep_edit_session(EditSession& session);
}
