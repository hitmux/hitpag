// Copyright (C) 2025 Hitmux
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#pragma once

#include <string>
#include <vector>

namespace file_filter {
    bool matches_pattern(const std::string& filename, const std::string& pattern);

    bool should_include_file(const std::string& filepath,
                            const std::vector<std::string>& include_patterns,
                            const std::vector<std::string>& exclude_patterns);

    std::vector<std::string> filter_files(const std::vector<std::string>& files,
                                         const std::vector<std::string>& include_patterns,
                                         const std::vector<std::string>& exclude_patterns,
                                         bool verbose = false);
}
