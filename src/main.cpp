// Copyright (C) 2025 Hitmux
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// hitpag - Smart Compression/Decompression Tool
// Version: 2.0.4
// website: https://hitmux.top
// github: https://github.com/Hitmux/hitpag

#include <iostream>
#include <filesystem>

#include "include/args.h"
#include "include/error.h"
#include "include/i18n.h"
#include "include/file_type.h"
#include "include/operation.h"
#include "include/interactive.h"
#include "include/progress.h"
#include "include/target_path.h"

namespace fs = std::filesystem;

int main(int argc, char* argv[]) {
    try {
        args::Options options = args::parse(argc, argv);

        if (options.show_help) {
            args::show_help();
            return 0;
        }
        if (options.show_version) {
            args::show_version();
            return 0;
        }

        progress::ProgressTracker tracker;

        if (options.password_prompt) {
            options.password = interactive::get_password_interactively(i18n::get("enter_password"));
        }

        if (options.interactive_mode) {
            interactive::run(options, tracker);
        } else {
            const auto cli_input_adapter = []() -> std::string {
                std::string input;
                if (!std::getline(std::cin, input)) {
                    throw std::runtime_error(i18n::get("error_input_stream_closed"));
                }
                return input;
            };
            const auto cli_output_adapter = [](const std::string& message) { std::cout << message << std::flush; };
            const auto cli_error_adapter = [](const std::string& message) { std::cerr << message << std::flush; };

            if (options.source_paths.size() > 1) {
                if (fs::exists(options.target_path)) {
                    for (const auto& src : options.source_paths) {
                        if (fs::exists(src)) {
                            std::error_code ec;
                            if (fs::equivalent(src, options.target_path, ec) && !ec) {
                                error::throw_error(error::ErrorCode::SAME_PATH);
                            }
                        }
                    }
                }

                file_type::FileType target_type = file_type::recognize_by_extension(options.target_path);
                if (!options.force_format.empty()) {
                    file_type::FileType forced_type = file_type::parse_format_string(options.force_format);
                    if (forced_type == file_type::FileType::UNKNOWN) {
                        error::throw_error(error::ErrorCode::UNKNOWN_FORMAT, {{"INFO", "Invalid format specified: " + options.force_format}});
                    }
                    target_type = forced_type;
                }

                if (target_type == file_type::FileType::UNKNOWN) {
                    error::throw_error(error::ErrorCode::UNKNOWN_FORMAT, {{"INFO", "Target format could not be determined. Please specify --format or use archive extension in target path."}});
                }

                if (!target_path::resolve_existing_target(options.target_path, cli_input_adapter, cli_output_adapter, cli_error_adapter)) {
                    std::cout << i18n::get("operation_canceled") << std::endl;
                    std::cout << i18n::get("goodbye") << std::endl;
                    return 0;
                }

                std::vector<operation::CompressionSource> compression_sources;
                compression_sources.reserve(options.source_paths.size());
                for (const auto& src : options.source_paths) {
                    compression_sources.push_back({src, false});
                }

                operation::compress(compression_sources, options.target_path, target_type, options.password, options, tracker);
            } else {
                if (fs::exists(options.source_path) && fs::exists(options.target_path)) {
                    std::error_code ec;
                    if (fs::equivalent(options.source_path, options.target_path, ec) && !ec) {
                        error::throw_error(error::ErrorCode::SAME_PATH);
                    }
                }

                file_type::RecognitionResult result = file_type::recognize(options.source_path, options.target_path);

                if (!options.force_format.empty()) {
                    file_type::FileType forced_type = file_type::parse_format_string(options.force_format);
                    if (forced_type == file_type::FileType::UNKNOWN) {
                        error::throw_error(error::ErrorCode::UNKNOWN_FORMAT, {{"INFO", "Invalid format specified: " + options.force_format}});
                    }

                    if (result.operation == file_type::OperationType::COMPRESS) {
                        result.target_type_hint = forced_type;
                    } else {
                        result.source_type = forced_type;
                    }
                }

                if (result.operation == file_type::OperationType::COMPRESS &&
                    result.target_type_hint == file_type::FileType::UNKNOWN) {
                    error::throw_error(error::ErrorCode::UNKNOWN_FORMAT, {{"INFO", "Target format could not be determined. Please specify --format or use archive extension in target path."}});
                }

                if (!target_path::resolve_existing_target(options.target_path, cli_input_adapter, cli_output_adapter, cli_error_adapter)) {
                    std::cout << i18n::get("operation_canceled") << std::endl;
                    std::cout << i18n::get("goodbye") << std::endl;
                    return 0;
                }

                if (result.operation == file_type::OperationType::COMPRESS) {
                    operation::compress(options.source_path, options.target_path, result.target_type_hint, options.password, options, tracker);
                } else if (result.operation == file_type::OperationType::DECOMPRESS) {
                    operation::decompress(options.source_path, options.target_path, result.source_type, options.password, options, tracker);
                }
            }
        }

        std::cout << i18n::get("goodbye") << std::endl;

    } catch (const error::HitpagException& e) {
        std::cerr << e.what() << std::endl;
        return static_cast<int>(e.code());
    } catch (const std::exception& e) {
        std::cerr << "An unexpected error occurred: " << e.what() << std::endl;
        return static_cast<int>(error::ErrorCode::UNKNOWN_ERROR);
    } catch (...) {
        std::cerr << "An unknown, non-standard error occurred." << std::endl;
        return static_cast<int>(error::ErrorCode::UNKNOWN_ERROR);
    }

    return 0;
}
