// Copyright (C) 2025 Hitmux
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#include "include/util.h"

namespace util {
    std::string trim_copy(const std::string& value) {
        const auto first = value.find_first_not_of(" \t\n\r");
        if (first == std::string::npos) {
            return "";
        }
        const auto last = value.find_last_not_of(" \t\n\r");
        return value.substr(first, last - first + 1);
    }
}
