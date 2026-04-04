// Copyright (C) 2025 Hitmux
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#include "include/args.h"
#include "include/error.h"
#include "include/i18n.h"

#include <iostream>
#include <string_view>
#include <thread>

namespace args {
    constexpr std::string_view APP_VERSION = "2.0.4";
    constexpr std::string_view APP_WEBSITE = "https://hitmux.top";
    constexpr std::string_view APP_GITHUB = "https://github.com/Hitmux/hitpag";

    Options parse(int argc, char* argv[]) {
        Options options;
        if (argc < 2) {
            options.show_help = true;
            return options;
        }

        std::vector<std::string> args_vec(argv + 1, argv + argc);

        size_t i = 0;
        while (i < args_vec.size() && args_vec[i][0] == '-') {
            const std::string& opt = args_vec[i];

            if (opt == "--") {
                i++;
                break;
            }

            if (opt == "-i") {
                options.interactive_mode = true;
                i++;
            } else if (opt == "-h" || opt == "--help") {
                options.show_help = true;
                return options;
            } else if (opt == "-v" || opt == "--version") {
                options.show_version = true;
                return options;
            } else if (opt.rfind("-p", 0) == 0) {
                if (opt.length() > 2) {
                    options.password = opt.substr(2);
                } else {
                    options.password_prompt = true;
                }
                i++;
            } else if (opt.rfind("-l", 0) == 0) {
                if (opt.length() > 2) {
                    try {
                        size_t pos;
                        long level = std::stol(opt.substr(2), &pos);
                        if (pos != opt.length() - 2 || level < 1 || level > 9) {
                            error::throw_error(error::ErrorCode::MISSING_ARGS, {{"ADDITIONAL_INFO", "Compression level must be between 1-9"}});
                        }
                        options.compression_level = static_cast<int>(level);
                    } catch (const std::out_of_range&) {
                        error::throw_error(error::ErrorCode::MISSING_ARGS, {{"ADDITIONAL_INFO", "Compression level value out of range"}});
                    } catch (const std::exception&) {
                        error::throw_error(error::ErrorCode::MISSING_ARGS, {{"ADDITIONAL_INFO", "Invalid compression level"}});
                    }
                } else {
                    options.compression_level = 0; // Default depends on format
                }
                i++;
            } else if (opt.rfind("-t", 0) == 0) {
                if (opt.length() > 2) {
                    try {
                        size_t pos;
                        long thread_val = std::stol(opt.substr(2), &pos);
                        if (pos != opt.length() - 2 || thread_val < 1 || thread_val > 1024) {
                            error::throw_error(error::ErrorCode::MISSING_ARGS, {{"ADDITIONAL_INFO", "Thread count must be between 1-1024"}});
                        }
                        options.thread_count = static_cast<int>(thread_val);
                    } catch (const std::out_of_range&) {
                        error::throw_error(error::ErrorCode::MISSING_ARGS, {{"ADDITIONAL_INFO", "Thread count value out of range"}});
                    } catch (const std::exception&) {
                        error::throw_error(error::ErrorCode::MISSING_ARGS, {{"ADDITIONAL_INFO", "Invalid thread count"}});
                    }
                } else {
                    unsigned int hw_threads = std::thread::hardware_concurrency();
                    options.thread_count = (hw_threads > 0) ? hw_threads : 1;
                }
                i++;
            } else if (opt == "--verbose") {
                options.verbose = true;
                i++;
            } else if (opt == "--benchmark") {
                options.benchmark = true;
                i++;
            } else if (opt == "--verify") {
                options.verify = true;
                i++;
            } else if (opt.rfind("--exclude=", 0) == 0) {
                options.exclude_patterns.push_back(opt.substr(10));
                i++;
            } else if (opt.rfind("--include=", 0) == 0) {
                options.include_patterns.push_back(opt.substr(10));
                i++;
            } else if (opt.rfind("--format=", 0) == 0) {
                std::string format_value = opt.substr(9);
                if (format_value.empty()) {
                    error::throw_error(error::ErrorCode::MISSING_ARGS, {{"ADDITIONAL_INFO", "--format requires a value"}});
                }
                options.force_format = format_value;
                i++;
            } else {
                error::throw_error(error::ErrorCode::MISSING_ARGS, {{"ADDITIONAL_INFO", "Unknown option: " + opt}});
            }
        }

        std::vector<std::string> positional_args;
        while (i < args_vec.size()) {
            positional_args.push_back(args_vec[i++]);
        }

        if (!positional_args.empty()) {
            options.target_path = positional_args.back();
            positional_args.pop_back();
            options.source_paths = positional_args;
            if (!options.source_paths.empty()) {
                options.source_path = options.source_paths.front();
            }
        }

        if (!options.source_paths.empty() && options.target_path.empty()) {
            error::throw_error(error::ErrorCode::MISSING_ARGS, {{"ADDITIONAL_INFO", "Target path missing"}});
        }
        if (options.source_paths.empty() && !options.target_path.empty()) {
            error::throw_error(error::ErrorCode::MISSING_ARGS, {{"ADDITIONAL_INFO", "Source path missing"}});
        }

        return options;
    }

    struct HelpOption { std::string flag; std::string key; };

    void show_help() {
        std::cout << "hitpag - Smart Compression/Decompression Tool (Version " << APP_VERSION << ")" << std::endl;
        std::cout << "Website: " << APP_WEBSITE << std::endl;
        std::cout << "GitHub: " << APP_GITHUB << std::endl;
        std::cout << "================================================================================" << std::endl << std::endl;

        std::cout << i18n::get("usage") << std::endl << std::endl;
        std::cout << i18n::get("help_options") << std::endl;

        const std::vector<HelpOption> help_options = {
            {"-i", "help_i"}, {"-p", "help_p"}, {"-l", "help_l"}, {"-t", "help_t"},
            {"--verbose", "help_verbose"}, {"--exclude", "help_exclude"},
            {"--include", "help_include"}, {"--benchmark", "help_benchmark"},
            {"--verify", "help_verify"}, {"--format", "help_format"}, {"-h", "help_h"}, {"-v", "help_v"}
        };
        for (const auto& opt : help_options) std::cout << i18n::get(opt.key) << std::endl;

        std::cout << std::endl << i18n::get("help_examples") << std::endl;
        const std::vector<std::string> example_keys = {
            "help_example1", "help_example2", "help_example_new_path", "help_example3",
            "help_example4", "help_example5", "help_example6", "help_example7", "help_example8", "help_example9"
        };
        for (const auto& key : example_keys) std::cout << i18n::get(key) << std::endl;
    }

    void show_version() {
        std::cout << "hitpag Version " << APP_VERSION << std::endl;
    }
}
