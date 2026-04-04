// Copyright (C) 2025 Hitmux
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#pragma once

#include <string>
#include <string_view>
#include <map>

namespace i18n {
    std::string get(std::string_view key, const std::map<std::string, std::string>& placeholders = {});
}
