// Copyright (C) 2025 Hitmux
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#include "include/file_type.h"
#include "include/error.h"

#include <filesystem>
#include <fstream>
#include <array>
#include <algorithm>
#include <cctype>

namespace fs = std::filesystem;

namespace file_type {
    bool is_split_zip_extension(const std::string& ext_lower) {
        return ext_lower.size() == 4 && ext_lower[0] == '.' && ext_lower[1] == 'z' &&
               std::isdigit(static_cast<unsigned char>(ext_lower[2])) &&
               std::isdigit(static_cast<unsigned char>(ext_lower[3]));
    }

    FileType recognize_by_extension(const std::string& path_str) {
        fs::path p(path_str);
        if (!p.has_extension()) return FileType::UNKNOWN;
        std::string ext = p.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c){ return std::tolower(c); });

        if (ext == ".tar") return FileType::ARCHIVE_TAR;
        if (ext == ".zip") return FileType::ARCHIVE_ZIP;
        if (is_split_zip_extension(ext)) return FileType::ARCHIVE_ZIP;
        if (ext == ".rar") return FileType::ARCHIVE_RAR;
        if (ext == ".7z") return FileType::ARCHIVE_7Z;
        if (ext == ".lz4") return FileType::ARCHIVE_LZ4;
        if (ext == ".xar") return FileType::ARCHIVE_XAR;
        if (ext == ".tgz") return FileType::ARCHIVE_TAR_GZ;
        if (ext == ".tbz2" || ext == ".tbz") return FileType::ARCHIVE_TAR_BZ2;
        if (ext == ".txz") return FileType::ARCHIVE_TAR_XZ;

        if (p.has_stem() && fs::path(p.stem()).has_extension()) {
            std::string stem_ext = fs::path(p.stem()).extension().string();
            std::transform(stem_ext.begin(), stem_ext.end(), stem_ext.begin(), [](unsigned char c){ return std::tolower(c); });
            if (stem_ext == ".tar") {
                if (ext == ".gz") return FileType::ARCHIVE_TAR_GZ;
                if (ext == ".bz2") return FileType::ARCHIVE_TAR_BZ2;
                if (ext == ".xz") return FileType::ARCHIVE_TAR_XZ;
                if (ext == ".zst" || ext == ".zstd") return FileType::ARCHIVE_TAR_ZSTD;
            }
        }

        if (ext == ".zst" || ext == ".zstd") return FileType::ARCHIVE_ZSTD;
        return FileType::UNKNOWN;
    }

    FileType recognize_by_header(const std::string& path) {
        std::ifstream file(path, std::ios::binary);
        if (!file) return FileType::UNKNOWN;
        std::array<char, 16> header{};
        file.read(header.data(), header.size());
        if(file.gcount() < 4 || file.fail()) return FileType::UNKNOWN;

        if (header[0] == 0x50 && header[1] == 0x4B) {
            if ((header[2] == 0x03 && header[3] == 0x04) ||
                (header[2] == 0x05 && header[3] == 0x06) ||
                (header[2] == 0x01 && header[3] == 0x02)) {
                return FileType::ARCHIVE_ZIP;
            }
        }

        if (header[0] == 0x52 && header[1] == 0x61 && header[2] == 0x72 && header[3] == 0x21) {
            return FileType::ARCHIVE_RAR;
        }

        if (file.gcount() >= 6 && header[0] == 0x37 && header[1] == 0x7A && header[2] == (char)0xBC && header[3] == (char)0xAF &&
            header[4] == 0x27 && header[5] == 0x1C) {
            return FileType::ARCHIVE_7Z;
        }


        if (file.gcount() >= 6 && header[0] == (char)0xFD && header[1] == 0x37 && header[2] == 0x7A &&
            header[3] == 0x58 && header[4] == 0x5A && header[5] == 0x00) {
            return FileType::ARCHIVE_TAR_XZ;
        }

        if (header[0] == 0x04 && header[1] == 0x22 && header[2] == 0x4D && header[3] == 0x18) {
            return FileType::ARCHIVE_LZ4;
        }

        if ((header[0] == 0x28 && header[1] == (char)0xB5 && header[2] == 0x2F && header[3] == (char)0xFD) ||
            (header[0] == 0x22 && header[1] == (char)0xB5 && header[2] == 0x2F && header[3] == (char)0xFD)) {
            return FileType::ARCHIVE_ZSTD;
        }

        file.clear();
        file.seekg(257);
        std::array<char, 6> tar_header{};
        file.read(tar_header.data(), tar_header.size());
        if (file.gcount() >= 5 && std::string(tar_header.data(), 5) == "ustar") {
            return FileType::ARCHIVE_TAR;
        }

        file.clear();
        file.seekg(0);
        std::array<char, 512> tar_block{};
        file.read(tar_block.data(), tar_block.size());
        if (file.gcount() >= 512) {
            bool looks_like_tar = true;
            bool has_filename = false;

            for (int i = 0; i < 100; ++i) {
                if (tar_block[i] != 0) {
                    has_filename = true;
                    if (tar_block[i] < 32 || tar_block[i] > 126) {
                        looks_like_tar = false;
                        break;
                    }
                }
            }

            if (looks_like_tar && has_filename) {
                return FileType::ARCHIVE_TAR;
            }
        }

        return FileType::UNKNOWN;
    }

    FileType recognize_source_type(const std::string& source_path_str) {
        if (!fs::exists(source_path_str)) {
            error::throw_error(error::ErrorCode::INVALID_SOURCE, {{"PATH", source_path_str}});
        }

        if (fs::is_directory(source_path_str)) return FileType::DIRECTORY;

        if (fs::is_regular_file(source_path_str)) {
            FileType type = recognize_by_extension(source_path_str);
            if (type == FileType::UNKNOWN || type == FileType::REGULAR_FILE) {
                type = recognize_by_header(source_path_str);
            }
            return (type == FileType::UNKNOWN) ? FileType::REGULAR_FILE : type;
        }

        error::throw_error(error::ErrorCode::INVALID_SOURCE, {{"PATH", source_path_str}, {"REASON", "not a regular file or directory"}});
        return FileType::UNKNOWN;
    }

    RecognitionResult recognize(const std::string& source_path_str, const std::string& target_path_str) {
        RecognitionResult result;
        result.source_type = recognize_source_type(source_path_str);

        if (!target_path_str.empty()) {
            result.target_type_hint = recognize_by_extension(target_path_str);
        }

        bool target_is_archive = (result.target_type_hint != FileType::UNKNOWN && result.target_type_hint != FileType::REGULAR_FILE && result.target_type_hint != FileType::DIRECTORY);

        if (result.source_type == FileType::DIRECTORY || result.source_type == FileType::REGULAR_FILE) {
            result.operation = OperationType::COMPRESS;
        } else {
            result.operation = OperationType::DECOMPRESS;
            if (fs::exists(target_path_str) && !fs::is_directory(target_path_str)) {
                error::throw_error(error::ErrorCode::INVALID_TARGET, {{"PATH", target_path_str}, {"REASON", "Target for decompression must be a directory."}});
            }
        }

        return result;
    }

    std::string get_file_type_string(FileType type) {
        static const std::map<FileType, std::string> type_map = {
            {FileType::REGULAR_FILE, "Regular File"}, {FileType::DIRECTORY, "Directory"},
            {FileType::ARCHIVE_TAR, "TAR Archive"}, {FileType::ARCHIVE_TAR_GZ, "TAR.GZ Archive"},
            {FileType::ARCHIVE_TAR_BZ2, "TAR.BZ2 Archive"}, {FileType::ARCHIVE_TAR_XZ, "TAR.XZ Archive"},
            {FileType::ARCHIVE_TAR_ZSTD, "TAR.ZST Archive"},
            {FileType::ARCHIVE_ZIP, "ZIP Archive"}, {FileType::ARCHIVE_RAR, "RAR Archive"},
            {FileType::ARCHIVE_7Z, "7Z Archive"}, {FileType::ARCHIVE_LZ4, "LZ4 Archive"},
            {FileType::ARCHIVE_ZSTD, "ZSTD Archive"}, {FileType::ARCHIVE_XAR, "XAR Archive"},
            {FileType::UNKNOWN, "Unknown Type"}
        };
        auto it = type_map.find(type);
        return it != type_map.end() ? it->second : "Unknown";
    }

    FileType parse_format_string(const std::string& format_str) {
        std::string fmt = format_str;
        std::transform(fmt.begin(), fmt.end(), fmt.begin(), [](unsigned char c){ return std::tolower(c); });

        if (fmt == "zip") return FileType::ARCHIVE_ZIP;
        if (fmt == "7z") return FileType::ARCHIVE_7Z;
        if (fmt == "tar") return FileType::ARCHIVE_TAR;
        if (fmt == "tar.gz" || fmt == "tgz") return FileType::ARCHIVE_TAR_GZ;
        if (fmt == "tar.bz2" || fmt == "tbz2") return FileType::ARCHIVE_TAR_BZ2;
        if (fmt == "tar.xz" || fmt == "txz") return FileType::ARCHIVE_TAR_XZ;
        if (fmt == "tar.zst" || fmt == "tar.zstd") return FileType::ARCHIVE_TAR_ZSTD;
        if (fmt == "rar") return FileType::ARCHIVE_RAR;
        if (fmt == "lz4") return FileType::ARCHIVE_LZ4;
        if (fmt == "zstd" || fmt == "zst") return FileType::ARCHIVE_ZSTD;
        if (fmt == "xar") return FileType::ARCHIVE_XAR;

        return FileType::UNKNOWN;
    }
}
