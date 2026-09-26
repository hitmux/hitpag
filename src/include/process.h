// Copyright (C) 2025 Hitmux
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#pragma once
#include <string>
#include <vector>
#include <limits>
#include <cstddef>

namespace process {
    struct CommandResult {
        int exit_code = -1;
        std::string stdout_output;
    };
    CommandResult run_command_capture(const std::vector<std::string>& cmd,
                                     size_t max_output = std::numeric_limits<size_t>::max());
    bool run_command_to_file(const std::vector<std::string>& cmd, const std::string& path);
    int run_command_status(const std::vector<std::string>& cmd);
}
