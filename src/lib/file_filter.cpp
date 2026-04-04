// Copyright (C) 2025 Hitmux
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#include "include/file_filter.h"
#include "include/i18n.h"

#include <filesystem>
#include <regex>
#include <iostream>

namespace fs = std::filesystem;

namespace file_filter {
    bool matches_pattern(const std::string& filename, const std::string& pattern) {
        try {
            std::regex regex_pattern(pattern);
            return std::regex_match(filename, regex_pattern);
        } catch (const std::regex_error&) {
            return filename.find(pattern) != std::string::npos;
        }
    }

    bool should_include_file(const std::string& filepath,
                            const std::vector<std::string>& include_patterns,
                            const std::vector<std::string>& exclude_patterns) {
        fs::path p(filepath);
        std::string filename = p.filename().string();

        for (const auto& pattern : exclude_patterns) {
            if (matches_pattern(filename, pattern) || matches_pattern(filepath, pattern)) {
                return false;
            }
        }

        if (!include_patterns.empty()) {
            for (const auto& pattern : include_patterns) {
                if (matches_pattern(filename, pattern) || matches_pattern(filepath, pattern)) {
                    return true;
                }
            }
            return false;
        }

        return true;
    }

    std::vector<std::string> filter_files(const std::vector<std::string>& files,
                                         const std::vector<std::string>& include_patterns,
                                         const std::vector<std::string>& exclude_patterns,
                                         bool verbose) {
        std::vector<std::string> filtered;
        size_t excluded_count = 0;

        for (const auto& file : files) {
            if (should_include_file(file, include_patterns, exclude_patterns)) {
                filtered.push_back(file);
            } else {
                excluded_count++;
                if (verbose) {
                    std::cout << "Excluded: " << file << std::endl;
                }
            }
        }

        if (verbose) {
            std::cout << i18n::get("filtering_files", {
                {"INCLUDED", std::to_string(filtered.size())},
                {"EXCLUDED", std::to_string(excluded_count)}
            }) << std::endl;
        }

        return filtered;
    }
}
