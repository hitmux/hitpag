// Copyright (C) 2025 Hitmux
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#include "include/i18n.h"

namespace i18n {
    const std::map<std::string, std::string> messages = {
        {"welcome", "Welcome to hitpag smart compression/decompression tool"},
        {"goodbye", "Thank you for using hitpag, goodbye!"},
        {"processing", "Processing {COUNT} items..."},
        {"compression_ratio", "Compression ratio: {RATIO}% (saved {SAVED} bytes)"},
        {"operation_time", "Operation completed in {TIME} seconds"},
        {"threads_info", "Using {COUNT} threads for parallel processing"},
        {"usage", "Usage: hitpag [options] [--] SOURCE_PATH TARGET_PATH"},
        {"help_options", "Options:"},
        {"help_i", "  -i              Interactive mode"},
        {"help_p", "  -p[password]    Encrypt/Decrypt with a password. If password is not attached, prompts for it."},
        {"help_l", "  -l[level]       Compression level (1-9, default depends on format)"},
        {"help_t", "  -t[threads]     Number of threads to use (default: auto-detect)"},
        {"help_verbose", "  --verbose       Show detailed progress information"},
        {"help_exclude", "  --exclude=PATTERN  Exclude files/directories matching pattern"},
        {"help_include", "  --include=PATTERN  Include only files/directories matching pattern"},
        {"help_benchmark", "  --benchmark     Show compression performance statistics"},
        {"help_verify", "  --verify        Verify archive integrity after compression"},
        {"help_format", "  --format=TYPE   Force archive type (zip, 7z, tar.gz, tar.bz2, tar.xz, rar, lz4, zstd, xar)"},
        {"help_h", "  -h, --help      Display help information"},
        {"help_v", "  -v, --version   Display version information"},
        {"help_examples", "Examples:"},
        {"help_example1", "  hitpag arch.tar.gz ./extracted_dir    # Decompress arch.tar.gz to extracted_dir"},
        {"help_example2", "  hitpag ./my_folder my_archive.zip     # Compress my_folder to my_archive.zip (creates my_folder inside zip)"},
        {"help_example_new_path", "  hitpag ./my_folder/ my_archive.zip    # Compress contents of my_folder (no root folder in zip)"},
        {"help_example3", "  hitpag -i big_file.rar .              # Interactive decompression of big_file.rar to current directory"},
        {"help_example4", "  hitpag -pmysecret my_docs.7z ./docs  # Encrypt ./docs into my_docs.7z with password 'mysecret'"},
        {"help_example5", "  hitpag -p secret.zip .                # Decompress secret.zip, will prompt for password"},
        {"help_example6", "  hitpag -l9 -t4 big_data.tar.gz ./data # Compress with max level using 4 threads"},
        {"help_example7", "  hitpag --verbose --benchmark ./files archive.7z # Verbose compression with benchmarking"},
        {"help_example8", "  hitpag --exclude='*.tmp' --include='*.cpp' src/ code.tar.gz # Filter files during compression"},
        {"help_example9", "  hitpag --format=zip data.7z ./extracted # Force treat data.7z as ZIP and decompress"},
        {"error_missing_args", "Error: Missing arguments. {ADDITIONAL_INFO}"},
        {"error_invalid_source", "Error: Source path '{PATH}' does not exist or is invalid. {REASON}"},
        {"error_invalid_target", "Error: Invalid target path '{PATH}'. {REASON}"},
        {"error_same_path", "Error: Source and target paths cannot be the same"},
        {"error_unknown_format", "Error: Unrecognized file format or ambiguous operation. {INFO}"},
        {"error_tool_not_found", "Error: Required tool not found: {TOOL_NAME}. Please ensure it is installed and in your system's PATH."},
        {"error_operation_failed", "Error: Operation failed (command: {COMMAND}, exit code: {EXIT_CODE}). Might be due to a wrong password."},
        {"error_permission_denied", "Error: Permission denied. {PATH}"},
        {"error_not_enough_space", "Error: Not enough disk space"},
        {"error_input_stream_closed", "Input stream closed. Operation canceled."},
        {"interactive_mode", "Interactive mode started"},
        {"ask_operation", "Please select operation type:"},
        {"operation_compress", "1. Compress"},
        {"operation_decompress", "2. Decompress"},
        {"ask_format", "Please select compression format:"},
        {"format_tar_gz", "tar.gz (gzip compression)"},
        {"format_zip", "zip (supports password)"},
        {"format_7z", "7z (supports password)"},
        {"format_tar", "tar (no compression)"},
        {"format_tar_bz2", "tar.bz2 (bzip2 compression)"},
        {"format_tar_xz", "tar.xz (xz compression)"},
        {"format_rar", "rar (decompression only recommended)"},
        {"format_lz4", "lz4 (fast compression)"},
        {"format_zstd", "zstd (modern compression)"},
        {"format_xar", "xar (macOS archive format)"},
        {"ask_overwrite", "Target '{TARGET_PATH}' already exists, overwrite? (y/n): "},
        {"ask_delete_source", "Delete source '{SOURCE_PATH}' after operation? (y/n): "},
        {"ask_set_password", "Set a password for the archive? (y/n): "},
        {"ask_has_password", "Does the archive require a password? (y/n): "},
        {"enter_password", "Enter password: "},
        {"confirm_password", "Confirm password: "},
        {"password_mismatch", "Passwords do not match. Please try again."},
        {"invalid_choice", "Invalid choice, please try again"},
        {"compressing", "Compressing..."},
        {"decompressing", "Decompressing..."},
        {"verifying", "Verifying archive integrity..."},
        {"verification_success", "Archive verification successful"},
        {"verification_failed", "Archive verification failed"},
        {"operation_complete", "Operation complete"},
        {"operation_canceled", "Operation canceled"},
        {"warning_tar_password", "Warning: Password protection is not supported for tar formats. The password will be ignored."},
        {"info_split_zip_detected", "Split ZIP archive detected, using 7z for extraction."},
        {"error_split_zip_requires_7z", "Error: Split ZIP archives require '7z' (p7zip) for extraction. Please install p7zip-full."},
        {"error_split_zip_main_not_found", "Main ZIP file not found for split archive. Expected: {PATH}"},
        {"filtering_files", "Filtering files: included {INCLUDED}, excluded {EXCLUDED}"},
        {"target_exists_header", "Target {OBJECT_TYPE} '{TARGET_PATH}' already exists."},
        {"target_exists_options", "Choose action: [O]verwrite / [C]ancel / [R]ename"},
        {"target_exists_choice_prompt", "Choice (o/c/r): "},
        {"target_exists_invalid", "Invalid choice, please enter o, c, or r."},
        {"target_exists_rename_prompt", "Enter a new target path (default: {DEFAULT}): "},
        {"target_exists_empty", "Target path cannot be empty."},
        {"target_exists_same", "New target path matches the current path. Please choose a different value."},
        {"target_exists_remove_failed", "Failed to remove existing target '{TARGET_PATH}': {REASON}"},
        {"target_exists_keep_directory", "Proceeding without deleting the existing directory. Existing files may be overwritten."},
        {"target_exists_rename_conflict", "Path '{TARGET_PATH}' already exists. You may overwrite it or choose a different name."},
        {"target_exists_object_file", "file"},
        {"target_exists_object_directory", "directory"},
    };

    std::string get(std::string_view key, const std::map<std::string, std::string>& placeholders) {
        auto it = messages.find(std::string(key));
        std::string message_template;
        if (it != messages.end()) {
            message_template = it->second;
        } else {
            return "[" + std::string(key) + "]";
        }

        for (const auto& p : placeholders) {
            std::string placeholder_key = "{" + p.first + "}";
            size_t pos = 0;
            while ((pos = message_template.find(placeholder_key, pos)) != std::string::npos) {
                message_template.replace(pos, placeholder_key.length(), p.second);
                pos += p.second.length();
            }
        }

        size_t start_ph = 0;
        while ((start_ph = message_template.find("{", start_ph)) != std::string::npos) {
            size_t end_ph = message_template.find("}", start_ph);
            if (end_ph != std::string::npos) {
                message_template.erase(start_ph, end_ph - start_ph + 1);
            } else {
                break;
            }
        }
        return message_template;
    }
}
