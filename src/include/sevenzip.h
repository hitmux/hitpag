// Copyright (C) 2025 Hitmux
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#pragma once
#include <string>

namespace sevenzip {
    // Existing formats keep their original dispatch; this catalog is for additions.
    std::string format_for_extension(const std::string& extension);
    std::string normalize_format(const std::string& format);
    std::string creation_format(const std::string& extension_or_format);
    bool can_create(const std::string& format);
    bool single_file_format(const std::string& format);
    bool supports_password(const std::string& format);
    std::string executable();
    std::string require_executable();
    std::string probe_format(const std::string& path, const std::string& password = "");
    bool is_stream_format(const std::string& format);
    bool recognizes(const std::string& path);
}
