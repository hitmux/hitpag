// Copyright (C) 2025 Hitmux
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#pragma once

#include <string>
#include <vector>

namespace args {
    struct Options {
        bool interactive_mode = false;
        bool show_help = false;
        bool show_version = false;
        std::string source_path;
        std::vector<std::string> source_paths;
        std::string target_path;
        std::string password;
        bool password_prompt = false;
        int compression_level = 0;
        int thread_count = 0;
        bool verbose = false;
        bool benchmark = false;
        bool verify = false;
        std::vector<std::string> exclude_patterns;
        std::vector<std::string> include_patterns;
        std::string force_format;
    };

    Options parse(int argc, char* argv[]);
    void show_help();
    void show_version();
}
