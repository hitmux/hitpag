// Copyright (C) 2025 Hitmux
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#pragma once

#include <chrono>
#include <string>

namespace progress {
    class ProgressTracker {
    public:
        struct Stats {
            size_t original_size = 0;
            size_t compressed_size = 0;
            double compression_time = 0.0;
            int thread_count = 1;

            double get_compression_ratio() const {
                return original_size > 0 ? (1.0 - static_cast<double>(compressed_size) / original_size) * 100.0 : 0.0;
            }

            size_t get_saved_bytes() const {
                return original_size > compressed_size ? original_size - compressed_size : 0;
            }
        };

        void start_operation();
        void end_operation();
        void set_thread_count(int threads);
        void set_original_size(size_t size);
        void set_compressed_size(size_t size);
        void print_stats(bool verbose, bool benchmark) const;
        size_t calculate_directory_size(const std::string& path) const;
        const Stats& stats() const { return stats_; }

    private:
        Stats stats_;
        std::chrono::high_resolution_clock::time_point start_time_;
    };
}
