// Copyright (C) 2025 Hitmux
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#pragma once

#include <string>
#include <map>

namespace file_type {
    enum class FileType {
        REGULAR_FILE, DIRECTORY, ARCHIVE_TAR, ARCHIVE_TAR_GZ, ARCHIVE_TAR_BZ2,
        ARCHIVE_TAR_XZ, ARCHIVE_TAR_ZSTD, ARCHIVE_ZIP, ARCHIVE_RAR, ARCHIVE_7Z,
        ARCHIVE_LZ4, ARCHIVE_ZSTD, ARCHIVE_XAR, UNKNOWN
    };

    enum class OperationType { COMPRESS, DECOMPRESS, UNKNOWN };

    struct RecognitionResult {
        FileType source_type = FileType::UNKNOWN;
        FileType target_type_hint = FileType::UNKNOWN;
        OperationType operation = OperationType::UNKNOWN;
    };

    bool is_split_zip_extension(const std::string& ext_lower);
    FileType recognize_by_extension(const std::string& path_str);
    FileType recognize_by_header(const std::string& path);
    FileType recognize_source_type(const std::string& source_path_str);
    RecognitionResult recognize(const std::string& source_path_str, const std::string& target_path_str);
    std::string get_file_type_string(FileType type);
    FileType parse_format_string(const std::string& format_str);
}
