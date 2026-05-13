// Copyright (C) 2025 Hitmux
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include "include/file_type.h"

namespace tui::archive_ops {
    struct ArchiveEntry {
        std::string path;
        bool is_directory = false;
        uint64_t size = 0;
        uint64_t compressed_size = 0;
        std::string modified;
        std::string method;
        uint32_t crc = 0;
    };

    struct TextExtractionResult {
        bool success = false;
        bool empty_file = false;
        std::string content;
    };

    struct CommandResult {
        int exit_code = -1;
        std::string stdout_output;
    };

    CommandResult run_command_capture(const std::vector<std::string>& cmd);
    int run_command_status(const std::vector<std::string>& cmd);

    std::vector<ArchiveEntry> list_archive(const std::string& archive_path, file_type::FileType type, const std::string& password = "");
    TextExtractionResult extract_text(const std::string& archive_path, const std::string& entry_path, file_type::FileType type, const std::string& password = "");
    std::string extract_to_string(const std::string& archive_path, const std::string& entry_path, file_type::FileType type, const std::string& password = "");
    bool extract_single(const std::string& archive_path, const std::string& entry_path, const std::string& output_dir, file_type::FileType type, const std::string& password = "");
    bool is_text_content(const std::string& content);
}
