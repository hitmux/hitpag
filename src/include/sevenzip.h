// Copyright (C) 2025 Hitmux
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#pragma once
#include <string>
#include <vector>

namespace sevenzip {
    struct CreationFormat {
        std::string label;
        std::string format;
        bool supports_password = false;
    };

    // Existing formats keep their original dispatch; this catalog is for additions.
    std::string format_for_extension(const std::string& extension);
    std::string normalize_format(const std::string& format);
    std::string creation_format(const std::string& extension_or_format);
    std::vector<CreationFormat> creation_formats();
    bool can_create(const std::string& format);
    bool single_file_format(const std::string& format);
    bool supports_password(const std::string& format);

    // Backends are ordered by preference. A successful operation is remembered
    // per archive/password so later probes and extractions reuse that backend.
    std::vector<std::string> executables();
    std::vector<std::string> backend_candidates(const std::string& path = "", const std::string& password = "");
    void remember_backend(const std::string& path, const std::string& password, const std::string& tool);
    void remember_probe(const std::string& path, const std::string& password, const std::string& tool, const std::string& format);
    std::string cached_format(const std::string& path, const std::string& password = "");
    std::string executable();
    std::string require_executable();
    std::string probe_format(const std::string& path, const std::string& password = "");
    bool is_stream_format(const std::string& format);
    bool recognizes(const std::string& path);
}
