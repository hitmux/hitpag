// Copyright (C) 2025 Hitmux
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#include "include/progress.h"
#include "include/i18n.h"

#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

namespace progress {
    void ProgressTracker::start_operation() {
        start_time_ = std::chrono::high_resolution_clock::now();
    }

    void ProgressTracker::end_operation() {
        auto end_time = std::chrono::high_resolution_clock::now();
        stats_.compression_time = std::chrono::duration<double>(end_time - start_time_).count();
    }

    void ProgressTracker::set_thread_count(int threads) {
        stats_.thread_count = threads;
    }

    void ProgressTracker::set_original_size(size_t size) {
        stats_.original_size = size;
    }

    void ProgressTracker::set_compressed_size(size_t size) {
        stats_.compressed_size = size;
    }

    size_t ProgressTracker::calculate_directory_size(const std::string& path) const {
        size_t total_size = 0;

        for (const auto& entry : fs::recursive_directory_iterator(path)) {
            std::error_code ec;
            if (entry.is_regular_file(ec)) {
                total_size += entry.file_size(ec);
            }
        }

        return total_size;
    }

    void ProgressTracker::print_stats(bool verbose, bool benchmark) const {
        if (benchmark) {
            std::cout << i18n::get("operation_time", {
                {"TIME", std::to_string(stats_.compression_time)}
            }) << std::endl;

            if (stats_.original_size > 0 && stats_.compressed_size > 0) {
                std::cout << i18n::get("compression_ratio", {
                    {"RATIO", std::to_string(stats_.get_compression_ratio())},
                    {"SAVED", std::to_string(stats_.get_saved_bytes())}
                }) << std::endl;
            }

            if (stats_.thread_count > 1) {
                std::cout << i18n::get("threads_info", {
                    {"COUNT", std::to_string(stats_.thread_count)}
                }) << std::endl;
            }
        }
    }
}
