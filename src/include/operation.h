// Copyright (C) 2025 Hitmux
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#pragma once

#include <string>
#include <vector>
#include "include/file_type.h"
#include "include/args.h"
#include "include/progress.h"

namespace operation {
    struct CompressionSource {
        std::string path;
        bool include_contents = false;
    };

    bool is_tool_available(std::string_view tool);
    bool is_split_zip_part(const std::string& path);
    std::string find_split_zip_main(const std::string& any_part_path);
    bool is_split_zip(const std::string& zip_path);

    void compress(const std::vector<CompressionSource>& sources,
                  const std::string& target_path_str,
                  file_type::FileType target_format,
                  const std::string& password,
                  const args::Options& options,
                  progress::ProgressTracker& tracker);

    void compress(const std::string& source_path_str,
                  const std::string& target_path_str,
                  file_type::FileType target_format,
                  const std::string& password,
                  const args::Options& options,
                  progress::ProgressTracker& tracker);

    void decompress(const std::string& source_path,
                    const std::string& target_dir_path,
                    file_type::FileType source_type,
                    const std::string& password,
                    const args::Options& options,
                    progress::ProgressTracker& tracker);
}
