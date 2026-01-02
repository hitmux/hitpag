// Copyright (C) 2025 Hitmux
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU Affero General Public License for more details.

// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.


// hitpag - Smart Compression/Decompression Tool
// Version: 2.0.0
// website: https://hitmux.top
// github: https://github.com/Hitmux/hitpag
// A versatile command-line utility for compressing and decompressing files and directories.
// It intelligently determines the operation type based on file extensions and provides
// a user-friendly interactive mode.
//
// ====================================================================================
// IMPORTANT: External Dependencies
// This program acts as a wrapper around standard command-line compression tools.
// For it to function correctly, the following tools must be installed and accessible
// in your system's PATH:
//
// - For .tar, .tar.gz, .tar.bz2, .tar.xz: `tar`
// - For .zip: `zip` (for compression) and `unzip` (for decompression)
// - For .7z: `7z`
// - For .rar: `unrar` (or `rar` for decompression)
// - For .lz4: `lz4`
// - For .zst: `zstd`
// - For .xar: `xar`
// ====================================================================================

#include <iostream>
#include <string>
#include <string_view> // For std::string_view
#include <vector>
#include <map>
#include <filesystem>
#include <stdexcept>
#include <array>
#include <fstream>
#include <algorithm> // For std::transform
#include <cctype>    // For std::tolower
#include <chrono>    // For timing operations
#include <thread>    // For multi-threading support
#include <regex>     // For file pattern matching

#include "target_conflict.h"

// Platform-specific includes for process management and terminal control
#ifdef _WIN32
#include <windows.h>
#include <process.h> // For _pclose, _popen on Windows
#include <conio.h>   // For _getch
#else
#include <cstdio>    // For popen, pclose
#include <termios.h> // For tcgetattr, tcsetattr
#include <unistd.h>  // For STDIN_FILENO, read, fork, execvp, chdir
#include <sys/wait.h> // For waitpid
#endif

namespace fs = std::filesystem;

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

// [NEW] Application constants for easy maintenance and display.
constexpr std::string_view APP_VERSION = "2.0.4";
constexpr std::string_view APP_WEBSITE = "https://hitmux.top";
constexpr std::string_view APP_GITHUB = "https://github.com/Hitmux/hitpag";

/**
 * @brief Internationalization (i18n) module.
 * 
 * Manages all user-facing strings to facilitate easy translation and text changes
 * without altering the core application logic.
 */
namespace i18n {
    // A map holding all user-visible text messages, keyed by a unique identifier.
    const std::map<std::string, std::string> messages = {
        // General messages
        {"welcome", "Welcome to hitpag smart compression/decompression tool"},
        {"goodbye", "Thank you for using hitpag, goodbye!"},
        {"processing", "Processing {COUNT} items..."},
        {"compression_ratio", "Compression ratio: {RATIO}% (saved {SAVED} bytes)"},
        {"operation_time", "Operation completed in {TIME} seconds"},
        {"threads_info", "Using {COUNT} threads for parallel processing"},
        
        // Help messages
        {"usage", "Usage: hitpag [options] [--] SOURCE_PATH TARGET_PATH"},
        {"help_options", "Options:"},
        {"help_i", "  -i              Interactive mode"},
        {"help_p", "  -p[password]    Encrypt/Decrypt with a password. If password is not attached, prompts for it."},
        {"help_l", "  -l[level]       Compression level (1-9, default depends on format)"},
        {"help_t", "  -t[threads]     Number of threads to use (default: auto-detect)"},
        {"help_verbose", "  --verbose       Show detailed progress information"},
        {"help_exclude", "  --exclude=PATTERN  Exclude files/directories matching pattern"},
        {"help_include", "  --include=PATTERN  Include only files/directories matching pattern"},
        {"help_benchmark", "  --benchmark     Show compression performance statistics"},
        {"help_verify", "  --verify        Verify archive integrity after compression"},
        {"help_format", "  --format=TYPE   Force archive type (zip, 7z, tar.gz, tar.bz2, tar.xz, rar, lz4, zstd, xar)"},
        {"help_h", "  -h, --help      Display help information"},
        {"help_v", "  -v, --version   Display version information"},
        {"help_examples", "Examples:"},
        {"help_example1", "  hitpag arch.tar.gz ./extracted_dir    # Decompress arch.tar.gz to extracted_dir"},
        {"help_example2", "  hitpag ./my_folder my_archive.zip     # Compress my_folder to my_archive.zip (creates my_folder inside zip)"},
        {"help_example_new_path", "  hitpag ./my_folder/ my_archive.zip    # Compress contents of my_folder (no root folder in zip)"},
        {"help_example3", "  hitpag -i big_file.rar .              # Interactive decompression of big_file.rar to current directory"},
        {"help_example4", "  hitpag -pmysecret my_docs.7z ./docs  # Encrypt ./docs into my_docs.7z with password 'mysecret'"},
        {"help_example5", "  hitpag -p secret.zip .                # Decompress secret.zip, will prompt for password"},
        {"help_example6", "  hitpag -l9 -t4 big_data.tar.gz ./data # Compress with max level using 4 threads"},
        {"help_example7", "  hitpag --verbose --benchmark ./files archive.7z # Verbose compression with benchmarking"},
        {"help_example8", "  hitpag --exclude='*.tmp' --include='*.cpp' src/ code.tar.gz # Filter files during compression"},
        {"help_example9", "  hitpag --format=zip data.7z ./extracted # Force treat data.7z as ZIP and decompress"},
        
        // Error messages
        {"error_missing_args", "Error: Missing arguments. {ADDITIONAL_INFO}"},
        {"error_invalid_source", "Error: Source path '{PATH}' does not exist or is invalid. {REASON}"},
        {"error_invalid_target", "Error: Invalid target path '{PATH}'. {REASON}"},
        {"error_same_path", "Error: Source and target paths cannot be the same"},
        {"error_unknown_format", "Error: Unrecognized file format or ambiguous operation. {INFO}"},
        {"error_tool_not_found", "Error: Required tool not found: {TOOL_NAME}. Please ensure it is installed and in your system's PATH."},
        {"error_operation_failed", "Error: Operation failed (command: {COMMAND}, exit code: {EXIT_CODE}). Might be due to a wrong password."},
        {"error_permission_denied", "Error: Permission denied. {PATH}"},
        {"error_not_enough_space", "Error: Not enough disk space"},
        // [NEW] Added message for closed input stream (EOF).
        {"error_input_stream_closed", "Input stream closed. Operation canceled."},
        
        // Interactive mode messages
        {"interactive_mode", "Interactive mode started"},
        {"ask_operation", "Please select operation type:"},
        {"operation_compress", "1. Compress"},
        {"operation_decompress", "2. Decompress"},
        {"ask_format", "Please select compression format:"},
        {"format_tar_gz", "tar.gz (gzip compression)"},
        {"format_zip", "zip (supports password)"},
        {"format_7z", "7z (supports password)"},
        {"format_tar", "tar (no compression)"},
        {"format_tar_bz2", "tar.bz2 (bzip2 compression)"},
        {"format_tar_xz", "tar.xz (xz compression)"},
        {"format_rar", "rar (decompression only recommended)"},
        {"format_lz4", "lz4 (fast compression)"},
        {"format_zstd", "zstd (modern compression)"},
        {"format_xar", "xar (macOS archive format)"},
        {"ask_overwrite", "Target '{TARGET_PATH}' already exists, overwrite? (y/n): "},
        {"ask_delete_source", "Delete source '{SOURCE_PATH}' after operation? (y/n): "},
        {"ask_set_password", "Set a password for the archive? (y/n): "},
        {"ask_has_password", "Does the archive require a password? (y/n): "},
        {"enter_password", "Enter password: "},
        {"confirm_password", "Confirm password: "},
        {"password_mismatch", "Passwords do not match. Please try again."},
        {"invalid_choice", "Invalid choice, please try again"},
        
        // Operation messages
        {"compressing", "Compressing..."},
        {"decompressing", "Decompressing..."},
        {"verifying", "Verifying archive integrity..."},
        {"verification_success", "Archive verification successful"},
        {"verification_failed", "Archive verification failed"},
        {"operation_complete", "Operation complete"},
        {"operation_canceled", "Operation canceled"},
        {"warning_tar_password", "Warning: Password protection is not supported for tar formats. The password will be ignored."},
        {"info_split_zip_detected", "Split ZIP archive detected, using 7z for extraction."},
        {"error_split_zip_requires_7z", "Error: Split ZIP archives require '7z' (p7zip) for extraction. Please install p7zip-full."},
        {"error_split_zip_main_not_found", "Main ZIP file not found for split archive. Expected: {PATH}"},
        {"filtering_files", "Filtering files: included {INCLUDED}, excluded {EXCLUDED}"},
        {"target_exists_header", "Target {OBJECT_TYPE} '{TARGET_PATH}' already exists."},
        {"target_exists_options", "Choose action: [O]verwrite / [C]ancel / [R]ename"},
        {"target_exists_choice_prompt", "Choice (o/c/r): "},
        {"target_exists_invalid", "Invalid choice, please enter o, c, or r."},
        {"target_exists_rename_prompt", "Enter a new target path (default: {DEFAULT}): "},
        {"target_exists_empty", "Target path cannot be empty."},
        {"target_exists_same", "New target path matches the current path. Please choose a different value."},
        {"target_exists_remove_failed", "Failed to remove existing target '{TARGET_PATH}': {REASON}"},
        {"target_exists_keep_directory", "Proceeding without deleting the existing directory. Existing files may be overwritten."},
        {"target_exists_rename_conflict", "Path '{TARGET_PATH}' already exists. You may overwrite it or choose a different name."},
        {"target_exists_object_file", "file"},
        {"target_exists_object_directory", "directory"},
    };
    
    /**
     * @brief Retrieves and formats a message string.
     * @param key The unique identifier for the message template. Using string_view for performance.
     * @param placeholders A map of placeholder keys to their replacement values.
     * @return The formatted message string.
     */
    std::string get(std::string_view key, const std::map<std::string, std::string>& placeholders = {}) {
        auto it = messages.find(std::string(key));
        std::string message_template;
        if (it != messages.end()) {
            message_template = it->second;
        } else {
            return "[" + std::string(key) + "]"; // Return key itself if not found
        }

        // Replace placeholders with provided values
        for(const auto& p : placeholders) {
            std::string placeholder_key = "{" + p.first + "}";
            size_t pos = 0;
            while((pos = message_template.find(placeholder_key, pos)) != std::string::npos) {
                message_template.replace(pos, placeholder_key.length(), p.second);
                pos += p.second.length();
            }
        }
        // Remove any unused placeholders to keep the output clean
        size_t start_ph = 0;
        while((start_ph = message_template.find("{", start_ph)) != std::string::npos) {
            size_t end_ph = message_template.find("}", start_ph);
            if (end_ph != std::string::npos) {
                message_template.erase(start_ph, end_ph - start_ph + 1);
            } else {
                break; // No closing brace
            }
        }
        return message_template;
    }
}

namespace cli_io {
    std::string get_input() {
        std::string input;
        if (!std::getline(std::cin, input)) {
            throw std::runtime_error(i18n::get("error_input_stream_closed"));
        }
        return util::trim_copy(input);
    }
}

namespace target_path {
    using InputFn = std::function<std::string()>;
    using OutputFn = std::function<void(const std::string&)>;

    namespace {
        std::string generate_sequential_candidate(const std::string& base_path, int suffix_index) {
            fs::path original(base_path);
            fs::path parent = original.parent_path();
            std::string filename = original.filename().string();

            static const std::vector<std::string> multi_extensions = {
                ".tar.gz", ".tar.bz2", ".tar.xz", ".tar.zst", ".tar.lz4"
            };

            std::string stem;
            std::string extension;

            for (const auto& ext : multi_extensions) {
                if (filename.size() > ext.size() &&
                    filename.compare(filename.size() - ext.size(), ext.size(), ext) == 0) {
                    stem = filename.substr(0, filename.size() - ext.size());
                    extension = ext;
                    break;
                }
            }

            if (stem.empty()) {
                auto pos = filename.find_last_of('.');
                if (pos == std::string::npos || pos == 0) {
                    stem = filename;
                } else {
                    stem = filename.substr(0, pos);
                    extension = filename.substr(pos);
                }
            }

            if (stem.empty() || stem == "." || stem == "..") {
                stem = "target";
            }

            std::string suffixed_name = stem + "_" + std::to_string(suffix_index) + extension;

            fs::path combined = parent / suffixed_name;
            return combined.string();
        }
    } // namespace

    bool resolve_existing_target(std::string& target_path,
                                 const InputFn& input_fn,
                                 const OutputFn& output_fn,
                                 const OutputFn& error_fn) {
        const std::string original_target = target_path;
        std::string rename_base = original_target;
        int suffix_counter = 1;

        while (fs::exists(target_path)) {
            const bool is_dir = fs::is_directory(target_path);
            const std::string object_label = i18n::get(
                is_dir ? "target_exists_object_directory" : "target_exists_object_file");

            const std::string header = i18n::get("target_exists_header", {
                {"TARGET_PATH", target_path},
                {"OBJECT_TYPE", object_label}
            });

            const std::string options_line = i18n::get("target_exists_options");
            const std::string choice_prompt = i18n::get("target_exists_choice_prompt");
            const std::string invalid_choice_line = i18n::get("target_exists_invalid");

            const auto action = target_conflict::prompt_action(
                output_fn,
                input_fn,
                header,
                options_line,
                choice_prompt,
                invalid_choice_line
            );

            if (action == target_conflict::Action::Overwrite) {
                if (is_dir) {
                    output_fn(i18n::get("target_exists_keep_directory") + "\n");
                    break;
                }

                std::error_code ec;
                fs::remove(target_path, ec);
                if (ec) {
                    error_fn(i18n::get("target_exists_remove_failed", {
                        {"TARGET_PATH", target_path},
                        {"REASON", ec.message()}
                    }) + "\n");
                    continue;
                }
                break;
            }

            if (action == target_conflict::Action::Cancel) {
                return false;
            }

            // Rename flow
            while (true) {
                std::string default_candidate = generate_sequential_candidate(rename_base, suffix_counter);
                std::string candidate = target_conflict::prompt_new_path(
                    output_fn,
                    input_fn,
                    i18n::get("target_exists_rename_prompt", {{"DEFAULT", default_candidate}}),
                    default_candidate
                );
                candidate = util::trim_copy(candidate);

                if (candidate.empty()) {
                    candidate = default_candidate;
                }

                if (candidate == target_path) {
                    error_fn(i18n::get("target_exists_same") + "\n");
                    if (candidate == default_candidate) {
                        ++suffix_counter;
                    }
                    continue;
                }

                if (fs::exists(candidate)) {
                    error_fn(i18n::get("target_exists_rename_conflict", {
                        {"TARGET_PATH", candidate}
                    }) + "\n");
                    if (candidate == default_candidate) {
                        ++suffix_counter;
                    } else {
                        rename_base = candidate;
                        suffix_counter = 1;
                    }
                    continue;
                }

                target_path = candidate;
                if (candidate == default_candidate) {
                    ++suffix_counter;
                } else {
                    rename_base = candidate;
                    suffix_counter = 1;
                }
                break;
            }
        }

        return true;
    }
}

/**
 * @brief Centralized error handling module.
 * 
 * Defines custom exception types and a unified way to report errors with
 * appropriate error codes and internationalized messages.
 */
namespace error {
    enum class ErrorCode {
        SUCCESS = 0,
        MISSING_ARGS = 1,
        INVALID_SOURCE = 2,
        INVALID_TARGET = 3,
        SAME_PATH = 4,
        UNKNOWN_FORMAT = 5,
        TOOL_NOT_FOUND = 6,
        OPERATION_FAILED = 7,
        PERMISSION_DENIED = 8,
        NOT_ENOUGH_SPACE = 9,
        UNKNOWN_ERROR = 99
    };
    
    // Custom exception class for application-specific errors.
    class HitpagException : public std::runtime_error {
    private:
        ErrorCode code_;
    public:
        HitpagException(ErrorCode code, const std::string& message)
            : std::runtime_error(message), code_(code) {}
        ErrorCode code() const { return code_; }
    };
    
    /**
     * @brief Throws a HitpagException with a formatted message.
     * @param code The error code corresponding to the error.
     * @param placeholders Values for placeholders in the error message.
     */
    void throw_error(ErrorCode code, const std::map<std::string, std::string>& placeholders = {}) {
        std::string message_key;
        switch (code) {
            case ErrorCode::MISSING_ARGS: message_key = "error_missing_args"; break;
            case ErrorCode::INVALID_SOURCE: message_key = "error_invalid_source"; break;
            case ErrorCode::INVALID_TARGET: message_key = "error_invalid_target"; break;
            case ErrorCode::SAME_PATH: message_key = "error_same_path"; break;
            case ErrorCode::UNKNOWN_FORMAT: message_key = "error_unknown_format"; break;
            case ErrorCode::TOOL_NOT_FOUND: message_key = "error_tool_not_found"; break;
            case ErrorCode::OPERATION_FAILED: message_key = "error_operation_failed"; break;
            case ErrorCode::PERMISSION_DENIED: message_key = "error_permission_denied"; break;
            case ErrorCode::NOT_ENOUGH_SPACE: message_key = "error_not_enough_space"; break;
            default: message_key = "Unknown error"; code = ErrorCode::UNKNOWN_ERROR;
        }
        throw HitpagException(code, i18n::get(message_key, placeholders));
    }
}

/**
 * @brief Command-line argument parsing module.
 * 
 * Responsible for parsing argc/argv, populating an options struct,
 * and handling requests for help or version information.
 */
namespace args {
    struct Options {
        bool interactive_mode = false;
        bool show_help = false;
        bool show_version = false;
        std::string source_path;
        std::vector<std::string> source_paths;
        std::string target_path;
        std::string password;
        bool password_prompt = false;
        int compression_level = 0; // 0 means use default for format
        int thread_count = 0; // 0 means auto-detect
        bool verbose = false;
        bool benchmark = false;
        bool verify = false;
        std::vector<std::string> exclude_patterns;
        std::vector<std::string> include_patterns;
        std::string force_format; // Manual format specification
    };
    
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
                i++; // Consume the "--"
                break; // Stop processing options
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
            } else if (opt.rfind("-p", 0) == 0) { // Handles -p and -p<password>
                if (opt.length() > 2) {
                    // Password is attached, e.g., -psecret
                    options.password = opt.substr(2);
                } else {
                    // Just -p, prompt for password later
                    options.password_prompt = true;
                }
                i++;
            } else if (opt.rfind("-l", 0) == 0) { // Compression level
                if (opt.length() > 2) {
                    try {
                        size_t pos;
                        long level = std::stol(opt.substr(2), &pos);
                        // Check if entire string was consumed and value is in valid range
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
                    options.compression_level = 6; // Default compression level
                }
                i++;
            } else if (opt.rfind("-t", 0) == 0) { // Thread count
                if (opt.length() > 2) {
                    try {
                        size_t pos;
                        long thread_val = std::stol(opt.substr(2), &pos);
                        // Check if entire string was consumed and value is positive and reasonable
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
                    options.thread_count = (hw_threads > 0) ? hw_threads : 1; // Fallback to 1 if detection fails
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
        
        if (!options.interactive_mode && !options.show_help && !options.show_version) {
            if (options.source_paths.empty()) error::throw_error(error::ErrorCode::MISSING_ARGS, {{"ADDITIONAL_INFO", "Source path missing"}});
            if (options.target_path.empty()) error::throw_error(error::ErrorCode::MISSING_ARGS, {{"ADDITIONAL_INFO", "Target path missing"}});
        }
        
        return options;
    }
    
    // Data-driven help display for easy maintenance.
    struct HelpOption { std::string flag; std::string key; };
    void show_help() {
        // [MODIFIED] Display hardcoded app info for a richer help screen.
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
        for(const auto& opt : help_options) std::cout << i18n::get(opt.key) << std::endl;

        std::cout << std::endl << i18n::get("help_examples") << std::endl;
        const std::vector<std::string> example_keys = {
            "help_example1", "help_example2", "help_example_new_path", "help_example3", 
            "help_example4", "help_example5", "help_example6", "help_example7", "help_example8", "help_example9"
        };
        for(const auto& key : example_keys) std::cout << i18n::get(key) << std::endl;
    }
    
    void show_version() {
        // [MODIFIED] Use the hardcoded constant for version display.
        std::cout << "hitpag Version " << APP_VERSION << std::endl;
    }
}

/**
 * @brief File type recognition and operation determination module.
 * 
 * Uses a combination of file extension and magic number (file header) analysis
 * to identify file types and infer the user's intended operation (compress/decompress).
 */
namespace file_type {
    enum class FileType {
        REGULAR_FILE, DIRECTORY, ARCHIVE_TAR, ARCHIVE_TAR_GZ, ARCHIVE_TAR_BZ2,
        ARCHIVE_TAR_XZ, ARCHIVE_ZIP, ARCHIVE_RAR, ARCHIVE_7Z, 
        ARCHIVE_LZ4, ARCHIVE_ZSTD, ARCHIVE_XAR, UNKNOWN
    };
    enum class OperationType { COMPRESS, DECOMPRESS, UNKNOWN };
    
    struct RecognitionResult {
        FileType source_type = FileType::UNKNOWN;
        FileType target_type_hint = FileType::UNKNOWN; 
        OperationType operation = OperationType::UNKNOWN;
    };
    

    // Check if extension matches split ZIP part pattern (.z01, .z02, ... .z99)
    // This is a helper function used by both file_type and operation namespaces
    bool is_split_zip_extension(const std::string& ext_lower) {
        return ext_lower.size() == 4 && ext_lower[0] == '.' && ext_lower[1] == 'z' &&
               std::isdigit(static_cast<unsigned char>(ext_lower[2])) &&
               std::isdigit(static_cast<unsigned char>(ext_lower[3]));
    }

    FileType recognize_by_extension(const std::string& path_str) {
        fs::path p(path_str);
        if (!p.has_extension()) return FileType::UNKNOWN;
        std::string ext = p.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c){ return std::tolower(c); });

        if (ext == ".tar") return FileType::ARCHIVE_TAR;
        if (ext == ".zip") return FileType::ARCHIVE_ZIP;
        if (is_split_zip_extension(ext)) return FileType::ARCHIVE_ZIP;
        if (ext == ".rar") return FileType::ARCHIVE_RAR;
        if (ext == ".7z") return FileType::ARCHIVE_7Z;
        if (ext == ".lz4") return FileType::ARCHIVE_LZ4;
        if (ext == ".zst" || ext == ".zstd") return FileType::ARCHIVE_ZSTD;
        if (ext == ".xar") return FileType::ARCHIVE_XAR;
        if (ext == ".tgz") return FileType::ARCHIVE_TAR_GZ;
        if (ext == ".tbz2" || ext == ".tbz") return FileType::ARCHIVE_TAR_BZ2;
        if (ext == ".txz") return FileType::ARCHIVE_TAR_XZ;

        // Handle double extensions like ".tar.gz"
        if (p.has_stem() && fs::path(p.stem()).has_extension()) {
            std::string stem_ext = fs::path(p.stem()).extension().string();
            std::transform(stem_ext.begin(), stem_ext.end(), stem_ext.begin(), [](unsigned char c){ return std::tolower(c); });
            if (stem_ext == ".tar") {
                if (ext == ".gz") return FileType::ARCHIVE_TAR_GZ;
                if (ext == ".bz2") return FileType::ARCHIVE_TAR_BZ2;
                if (ext == ".xz") return FileType::ARCHIVE_TAR_XZ;
            }
        }
        return FileType::UNKNOWN;
    }
        
    FileType recognize_by_header(const std::string& path) {
        std::ifstream file(path, std::ios::binary);
        if (!file) return FileType::UNKNOWN;
        std::array<char, 16> header{}; 
        file.read(header.data(), header.size());
        // Check both that we read enough bytes and that the read operation succeeded
        if(file.gcount() < 4 || file.fail()) return FileType::UNKNOWN; 

        // ZIP archives (PK signature, but check if it's really ZIP vs other PK-based formats)
        if (header[0] == 0x50 && header[1] == 0x4B) {
            // Check specific ZIP signatures
            if ((header[2] == 0x03 && header[3] == 0x04) ||  // Local file header
                (header[2] == 0x05 && header[3] == 0x06) ||  // End central directory
                (header[2] == 0x01 && header[3] == 0x02)) {  // Central directory file header
                return FileType::ARCHIVE_ZIP;
            }
        }
        
        // RAR archives
        if (header[0] == 0x52 && header[1] == 0x61 && header[2] == 0x72 && header[3] == 0x21) {
            return FileType::ARCHIVE_RAR; // Rar!
        }
        
        // 7Z archives
        if (file.gcount() >= 6 && header[0] == 0x37 && header[1] == 0x7A && header[2] == (char)0xBC && header[3] == (char)0xAF && 
            header[4] == 0x27 && header[5] == 0x1C) {
            return FileType::ARCHIVE_7Z;
        }
        
        // GZIP files (could be .tar.gz or standalone .gz)
        if (header[0] == (char)0x1F && header[1] == (char)0x8B) {
            return FileType::ARCHIVE_TAR_GZ; // Default to tar.gz, could be refined further
        }
        
        // BZIP2 files
        if (header[0] == 0x42 && header[1] == 0x5A && header[2] == 0x68) {
            return FileType::ARCHIVE_TAR_BZ2; // BZh
        }
        
        // XZ files
        if (file.gcount() >= 6 && header[0] == (char)0xFD && header[1] == 0x37 && header[2] == 0x7A && 
            header[3] == 0x58 && header[4] == 0x5A && header[5] == 0x00) {
            return FileType::ARCHIVE_TAR_XZ; // .7zXZ..
        }
        
        // LZ4 files
        if (header[0] == 0x04 && header[1] == 0x22 && header[2] == 0x4D && header[3] == 0x18) {
            return FileType::ARCHIVE_LZ4;
        }
        
        // ZSTD files
        if ((header[0] == 0x28 && header[1] == (char)0xB5 && header[2] == 0x2F && header[3] == (char)0xFD) ||
            (header[0] == 0x22 && header[1] == (char)0xB5 && header[2] == 0x2F && header[3] == (char)0xFD)) {
            return FileType::ARCHIVE_ZSTD;
        }

        // TAR archives have "ustar" at byte offset 257
        file.clear(); 
        file.seekg(257);
        std::array<char, 6> tar_header{}; 
        file.read(tar_header.data(), tar_header.size());
        if (file.gcount() >= 5 && std::string(tar_header.data(), 5) == "ustar") {
            return FileType::ARCHIVE_TAR;
        }
        
        // Check for old TAR format (may not have ustar signature)
        file.clear();
        file.seekg(0);
        std::array<char, 512> tar_block{};
        file.read(tar_block.data(), tar_block.size());
        if (file.gcount() >= 512) {
            // Check if looks like TAR header (filename in first 100 bytes, checksum calculation)
            bool looks_like_tar = true;
            bool has_filename = false;
            
            // Check if there's at least one non-null character in filename field
            for (int i = 0; i < 100; ++i) {
                if (tar_block[i] != 0) {
                    has_filename = true;
                    // Check if character is printable
                    if (tar_block[i] < 32 || tar_block[i] > 126) {
                        looks_like_tar = false;
                        break;
                    }
                }
            }
            
            if (looks_like_tar && has_filename) {
                return FileType::ARCHIVE_TAR;
            }
        }
        
        return FileType::UNKNOWN;
    }

    // Helper function to robustly determine the type of a source path.
    FileType recognize_source_type(const std::string& source_path_str) {
        if (!fs::exists(source_path_str)) {
            error::throw_error(error::ErrorCode::INVALID_SOURCE, {{"PATH", source_path_str}});
        }
        
        if (fs::is_directory(source_path_str)) return FileType::DIRECTORY;
        
        if (fs::is_regular_file(source_path_str)) {
            // Try header detection first (more reliable)
            FileType type = recognize_by_header(source_path_str);
            if (type == FileType::UNKNOWN) {
                // Fall back to extension if header detection fails
                type = recognize_by_extension(source_path_str);
            }
            // If still unknown, it's just a regular file, not a known archive type.
            return (type == FileType::UNKNOWN) ? FileType::REGULAR_FILE : type;
        }
        
        error::throw_error(error::ErrorCode::INVALID_SOURCE, {{"PATH", source_path_str}, {"REASON", "not a regular file or directory"}});
        return FileType::UNKNOWN; // Should not be reached
    }
    
    RecognitionResult recognize(const std::string& source_path_str, const std::string& target_path_str) {
        RecognitionResult result;
        result.source_type = recognize_source_type(source_path_str);

        if (!target_path_str.empty()) {
            result.target_type_hint = recognize_by_extension(target_path_str);
        }

        bool target_is_archive = (result.target_type_hint != FileType::UNKNOWN && result.target_type_hint != FileType::REGULAR_FILE && result.target_type_hint != FileType::DIRECTORY);

        if (result.source_type == FileType::DIRECTORY || result.source_type == FileType::REGULAR_FILE) {
            if (target_is_archive) {
                result.operation = OperationType::COMPRESS;
            } else {
                // If no archive extension detected, we'll default to compression operation
                // The format will be determined by --format option if provided
                result.operation = OperationType::COMPRESS;
                result.target_type_hint = FileType::UNKNOWN;
            }
        } else { // Source is an archive
            result.operation = OperationType::DECOMPRESS;
            if (fs::exists(target_path_str) && !fs::is_directory(target_path_str)) {
                error::throw_error(error::ErrorCode::INVALID_TARGET, {{"PATH", target_path_str}, {"REASON", "Target for decompression must be a directory."}});
            }
        }
        
        return result;
    }

    std::string get_file_type_string(FileType type) {
        static const std::map<FileType, std::string> type_map = {
            {FileType::REGULAR_FILE, "Regular File"}, {FileType::DIRECTORY, "Directory"},
            {FileType::ARCHIVE_TAR, "TAR Archive"}, {FileType::ARCHIVE_TAR_GZ, "TAR.GZ Archive"},
            {FileType::ARCHIVE_TAR_BZ2, "TAR.BZ2 Archive"}, {FileType::ARCHIVE_TAR_XZ, "TAR.XZ Archive"},
            {FileType::ARCHIVE_ZIP, "ZIP Archive"}, {FileType::ARCHIVE_RAR, "RAR Archive"},
            {FileType::ARCHIVE_7Z, "7Z Archive"}, {FileType::ARCHIVE_LZ4, "LZ4 Archive"},
            {FileType::ARCHIVE_ZSTD, "ZSTD Archive"}, {FileType::ARCHIVE_XAR, "XAR Archive"},
            {FileType::UNKNOWN, "Unknown Type"}
        };
        auto it = type_map.find(type);
        return it != type_map.end() ? it->second : "Unknown";
    }

    FileType parse_format_string(const std::string& format_str) {
        std::string fmt = format_str;
        std::transform(fmt.begin(), fmt.end(), fmt.begin(), [](unsigned char c){ return std::tolower(c); });
        
        if (fmt == "zip") return FileType::ARCHIVE_ZIP;
        if (fmt == "7z") return FileType::ARCHIVE_7Z;
        if (fmt == "tar") return FileType::ARCHIVE_TAR;
        if (fmt == "tar.gz" || fmt == "tgz") return FileType::ARCHIVE_TAR_GZ;
        if (fmt == "tar.bz2" || fmt == "tbz2") return FileType::ARCHIVE_TAR_BZ2;
        if (fmt == "tar.xz" || fmt == "txz") return FileType::ARCHIVE_TAR_XZ;
        if (fmt == "rar") return FileType::ARCHIVE_RAR;
        if (fmt == "lz4") return FileType::ARCHIVE_LZ4;
        if (fmt == "zstd" || fmt == "zst") return FileType::ARCHIVE_ZSTD;
        if (fmt == "xar") return FileType::ARCHIVE_XAR;
        
        return FileType::UNKNOWN;
    }
}

/**
 * @brief File filtering module for include/exclude pattern matching.
 */
namespace file_filter {
    bool matches_pattern(const std::string& filename, const std::string& pattern) {
        try {
            std::regex regex_pattern(pattern);
            return std::regex_match(filename, regex_pattern);
        } catch (const std::regex_error&) {
            // Fallback to simple wildcard matching
            return filename.find(pattern) != std::string::npos;
        }
    }
    
    bool should_include_file(const std::string& filepath, 
                           const std::vector<std::string>& include_patterns,
                           const std::vector<std::string>& exclude_patterns) {
        fs::path p(filepath);
        std::string filename = p.filename().string();
        
        // Check exclude patterns first
        for (const auto& pattern : exclude_patterns) {
            if (matches_pattern(filename, pattern) || matches_pattern(filepath, pattern)) {
                return false;
            }
        }
        
        // If include patterns are specified, file must match at least one
        if (!include_patterns.empty()) {
            for (const auto& pattern : include_patterns) {
                if (matches_pattern(filename, pattern) || matches_pattern(filepath, pattern)) {
                    return true;
                }
            }
            return false; // No include pattern matched
        }
        
        return true; // No include patterns, and not excluded
    }
    
    std::vector<std::string> filter_files(const std::vector<std::string>& files,
                                         const std::vector<std::string>& include_patterns,
                                         const std::vector<std::string>& exclude_patterns,
                                         bool verbose = false) {
        std::vector<std::string> filtered;
        size_t excluded_count = 0;
        
        for (const auto& file : files) {
            if (should_include_file(file, include_patterns, exclude_patterns)) {
                filtered.push_back(file);
            } else {
                excluded_count++;
                if (verbose) {
                    std::cout << "Excluded: " << file << std::endl;
                }
            }
        }
        
        if (verbose) {
            std::cout << i18n::get("filtering_files", {
                {"INCLUDED", std::to_string(filtered.size())},
                {"EXCLUDED", std::to_string(excluded_count)}
            }) << std::endl;
        }
        
        return filtered;
    }
}

/**
 * @brief Progress tracking and performance measurement module.
 */
namespace progress {
    struct CompressionStats {
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
    
    CompressionStats current_stats;
    std::chrono::high_resolution_clock::time_point start_time;
    
    void start_operation() {
        start_time = std::chrono::high_resolution_clock::now();
    }
    
    void end_operation() {
        auto end_time = std::chrono::high_resolution_clock::now();
        current_stats.compression_time = std::chrono::duration<double>(end_time - start_time).count();
    }
    
    void set_thread_count(int threads) {
        current_stats.thread_count = threads;
    }
    
    void set_original_size(size_t size) {
        current_stats.original_size = size;
    }
    
    void set_compressed_size(size_t size) {
        current_stats.compressed_size = size;
    }
    
    size_t calculate_directory_size(const std::string& path) {
        size_t total_size = 0;
        std::error_code ec;
        
        for (const auto& entry : fs::recursive_directory_iterator(path, ec)) {
            if (!ec && entry.is_regular_file()) {
                total_size += entry.file_size(ec);
            }
        }
        
        return total_size;
    }
    
    void print_stats(bool verbose, bool benchmark) {
        if (benchmark) {
            std::cout << i18n::get("operation_time", {
                {"TIME", std::to_string(current_stats.compression_time)}
            }) << std::endl;
            
            if (current_stats.original_size > 0 && current_stats.compressed_size > 0) {
                std::cout << i18n::get("compression_ratio", {
                    {"RATIO", std::to_string(current_stats.get_compression_ratio())},
                    {"SAVED", std::to_string(current_stats.get_saved_bytes())}
                }) << std::endl;
            }
            
            if (current_stats.thread_count > 1) {
                std::cout << i18n::get("threads_info", {
                    {"COUNT", std::to_string(current_stats.thread_count)}
                }) << std::endl;
            }
        }
    }
}
namespace operation {
    struct CompressionSource {
        std::string path;
        bool include_contents = false;
    };

    bool is_tool_available(std::string_view tool) {
        #ifdef _WIN32
            std::string command = "where " + std::string(tool) + " > nul 2>&1";
        #else
            std::string command = "command -v " + std::string(tool) + " > /dev/null 2>&1";
        #endif
        return system(command.c_str()) == 0;
    }

    // Check if file extension is a split ZIP part (.z01, .z02, ... .z99)
    bool is_split_zip_part(const std::string& path) {
        fs::path p(path);
        if (!p.has_extension()) return false;

        std::string ext = p.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c){ return std::tolower(c); });

        return file_type::is_split_zip_extension(ext);
    }

    // Find the main .zip file for a split archive
    // Input can be .zip file or any split part (.z01, .z02, etc.)
    // Returns empty string if main file not found
    std::string find_split_zip_main(const std::string& any_part_path) {
        fs::path p(any_part_path);
        fs::path main_zip = p;
        main_zip.replace_extension(".zip");

        if (fs::exists(main_zip)) {
            return main_zip.string();
        }
        return "";
    }

    // Check if a ZIP file is part of a split archive (has .z01, .z02, etc.)
    // Works with both .zip main file and .z01/.z02/etc. split parts
    bool is_split_zip(const std::string& zip_path) {
        fs::path p(zip_path);
        if (!p.has_extension()) return false;

        std::string ext = p.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c){ return std::tolower(c); });

        // If it's a split part (.z01, .z02, etc.), it's definitely a split archive
        if (is_split_zip_part(zip_path)) {
            return true;
        }

        // If it's a .zip file, check if .z01 exists
        if (ext != ".zip") return false;

        fs::path z01_path = p;
        z01_path.replace_extension(".z01");
        return fs::exists(z01_path);
    }

    // Build 7z extraction arguments (shared between 7z format and split ZIP)
    void build_7z_extract_args(std::vector<std::string>& args,
                               const std::string& source_path,
                               const std::string& target_dir_path,
                               const std::string& password) {
        args.push_back("x");
        if (!password.empty()) {
            args.push_back("-p" + password);
        }
        args.push_back(fs::absolute(source_path).string());
        args.push_back("-o" + fs::absolute(target_dir_path).string());
        args.push_back("-y");
    }

#ifdef _WIN32
    std::string quote_argument_for_windows(const std::string& arg) {
        if (arg.empty()) {
            return "\"\"";
        }
        if (arg.find_first_of(" \t\n\v\"") == std::string::npos) {
            return arg;
        }

        std::string quoted_arg;
        quoted_arg.push_back('"');
        for (auto it = arg.begin(); ; ++it) {
            unsigned int backslash_count = 0;
            while (it != arg.end() && *it == '\\') {
                ++it;
                ++backslash_count;
            }

            if (it == arg.end()) {
                quoted_arg.append(backslash_count * 2, '\\');
                break;
            }

            if (*it == '"') {
                quoted_arg.append(backslash_count * 2 + 1, '\\');
                quoted_arg.push_back(*it);
            } else {
                quoted_arg.append(backslash_count, '\\');
                quoted_arg.push_back(*it);
            }
        }
        quoted_arg.push_back('"');
        return quoted_arg;
    }
#endif
    
    int execute_command(const std::string& tool, const std::vector<std::string>& args, const std::string& working_dir = "") {
        std::string full_command = tool;
        for(const auto& arg : args) full_command += " " + arg;

#ifdef _WIN32
        PROCESS_INFORMATION piProcInfo;
        STARTUPINFOA siStartInfo;
        ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));
        ZeroMemory(&siStartInfo, sizeof(STARTUPINFOA));
        siStartInfo.cb = sizeof(STARTUPINFOA);
        siStartInfo.hStdError = GetStdHandle(STD_ERROR_HANDLE);
        siStartInfo.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
        siStartInfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
        siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

        std::string command_line = quote_argument_for_windows(tool);
        for (const auto& arg : args) {
            command_line += " " + quote_argument_for_windows(arg);
        }

        // CreateProcessA can modify the command line buffer, so we need a writable copy
        std::vector<char> cmd_line_buf(command_line.begin(), command_line.end());
        cmd_line_buf.push_back('\0'); // Ensure null termination

        BOOL bSuccess = CreateProcessA(
            NULL,
            cmd_line_buf.data(),
            NULL, NULL, TRUE, 0, NULL,
            working_dir.empty() ? NULL : working_dir.c_str(),
            &siStartInfo, &piProcInfo
        );

        if (!bSuccess) {
            error::throw_error(error::ErrorCode::OPERATION_FAILED, {{"COMMAND", full_command}, {"EXIT_CODE", "CreateProcess_failed: " + std::to_string(GetLastError())}});
        }
        
        WaitForSingleObject(piProcInfo.hProcess, INFINITE);
        DWORD exit_code;
        GetExitCodeProcess(piProcInfo.hProcess, &exit_code);

        CloseHandle(piProcInfo.hProcess);
        CloseHandle(piProcInfo.hThread);

#else
        pid_t pid = fork();
        if (pid == -1) {
            error::throw_error(error::ErrorCode::OPERATION_FAILED, {{"COMMAND", full_command}, {"EXIT_CODE", "fork_failed"}});
        }

        if (pid == 0) { // Child process
            if (!working_dir.empty()) {
                if (chdir(working_dir.c_str()) != 0) {
                    perror("chdir failed in child");
                    _exit(127);
                }
            }

            std::vector<char*> c_args;
            c_args.push_back(const_cast<char*>(tool.c_str()));
            for (const auto& arg : args) {
                c_args.push_back(const_cast<char*>(arg.c_str()));
            }
            c_args.push_back(nullptr);

            execvp(c_args[0], c_args.data());
            perror("execvp failed");
            _exit(127);
        }

        int status;
        waitpid(pid, &status, 0);
        int exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
#endif
        if (exit_code != 0) {
            std::cerr << std::endl;
        }
        return exit_code;
    }

    // Archive verification function
    bool verify_archive(const std::string& archive_path, file_type::FileType format) {
        std::string tool;
        std::vector<std::string> args;
        
        switch (format) {
            case file_type::FileType::ARCHIVE_TAR:
            case file_type::FileType::ARCHIVE_TAR_GZ:
            case file_type::FileType::ARCHIVE_TAR_BZ2:
            case file_type::FileType::ARCHIVE_TAR_XZ:
                tool = "tar";
                args = {"-tf", archive_path};
                break;
            case file_type::FileType::ARCHIVE_ZIP:
                tool = "unzip";
                args = {"-t", archive_path};
                break;
            case file_type::FileType::ARCHIVE_7Z:
                tool = "7z";
                args = {"t", archive_path};
                break;
            default:
                return true; // Skip verification for unsupported formats
        }
        
        if (!is_tool_available(tool)) return false;
        
        // Redirect output to avoid cluttering the console
        int result = execute_command(tool, args);
        return result == 0;
    }

    namespace {
        bool is_descendant_or_same(const fs::path& base, const fs::path& target) {
            std::error_code ec;
            fs::path relative = fs::relative(target, base, ec);
            if (ec) return false;
            if (relative.empty()) return true;
            for (const auto& part : relative) {
                if (part == "..") {
                    return false;
                }
            }
            return true;
        }

        fs::path determine_common_base(const std::vector<fs::path>& paths) {
            if (paths.empty()) return fs::current_path();
            fs::path base = paths.front().parent_path();
            if (base.empty()) base = paths.front();
            while (!base.empty()) {
                bool all_descendants = true;
                for (const auto& p : paths) {
                    if (!is_descendant_or_same(base, p)) {
                        all_descendants = false;
                        break;
                    }
                }
                if (all_descendants) return base;
                base = base.parent_path();
            }
            fs::path fallback = paths.front().root_path();
            if (fallback.empty()) fallback = fs::current_path();
            return fallback;
        }

        uintmax_t calculate_sources_size(const std::vector<fs::path>& canonical_sources) {
            uintmax_t total = 0;
            for (const auto& p : canonical_sources) {
                std::error_code ec;
                if (fs::is_directory(p, ec)) {
                    total += progress::calculate_directory_size(p.string());
                } else {
                    auto sz = fs::file_size(p, ec);
                    if (!ec) total += sz;
                }
            }
            return total;
        }
    } // namespace

    void compress(const std::vector<CompressionSource>& sources, const std::string& target_path_str, 
                 file_type::FileType target_format, const std::string& password,
                 const args::Options& options = {}) {
        if (sources.empty()) {
            error::throw_error(error::ErrorCode::MISSING_ARGS, {{"ADDITIONAL_INFO", "No sources provided for compression"}});
        }

        std::vector<fs::path> canonical_sources;
        canonical_sources.reserve(sources.size());
        std::vector<bool> is_directory_flags;
        is_directory_flags.reserve(sources.size());

        for (const auto& src : sources) {
            fs::path path_input(src.path);
            if (!fs::exists(path_input)) {
                error::throw_error(error::ErrorCode::INVALID_SOURCE, {{"PATH", src.path}});
            }
            fs::path canonical = fs::weakly_canonical(path_input);
            canonical_sources.push_back(canonical);
            is_directory_flags.push_back(fs::is_directory(canonical));
        }

        bool single_contents_mode = (sources.size() == 1 && sources.front().include_contents && is_directory_flags.front());
        fs::path base_dir;
        std::vector<std::string> items_to_archive;
        items_to_archive.reserve(sources.size());
        
        // Start progress tracking
        if (options.benchmark) {
            progress::start_operation();
            progress::set_original_size(calculate_sources_size(canonical_sources));
            progress::set_thread_count(options.thread_count > 0 ? options.thread_count : 1);
        }
        
        if (options.verbose && options.thread_count > 1) {
            std::cout << i18n::get("threads_info", {{"COUNT", std::to_string(options.thread_count)}}) << std::endl;
        }

        if (single_contents_mode) {
            base_dir = canonical_sources.front();
            items_to_archive.push_back(".");
        } else {
            base_dir = determine_common_base(canonical_sources);
            for (size_t idx = 0; idx < canonical_sources.size(); ++idx) {
                const auto& canonical = canonical_sources[idx];
                std::error_code ec;
                fs::path relative = fs::relative(canonical, base_dir, ec);
                if (ec || relative.empty() || relative == ".") {
                    fs::path fallback = canonical.filename();
                    if (fallback.empty()) {
                        fallback = canonical;
                    }
                    relative = fallback;
                }
                items_to_archive.push_back(relative.string());
            }
        }

        std::string tool;
        std::vector<std::string> args;
        std::string working_dir_for_cmd = base_dir.empty() ? "" : base_dir.string();
        if (working_dir_for_cmd.empty()) {
            working_dir_for_cmd = fs::current_path().string();
        }

        switch (target_format) {
            case file_type::FileType::ARCHIVE_TAR:
            case file_type::FileType::ARCHIVE_TAR_GZ:
            case file_type::FileType::ARCHIVE_TAR_BZ2:
            case file_type::FileType::ARCHIVE_TAR_XZ:
                if (!password.empty()) std::cout << i18n::get("warning_tar_password") << std::endl;
                tool = "tar";
                if (!is_tool_available(tool)) error::throw_error(error::ErrorCode::TOOL_NOT_FOUND, {{"TOOL_NAME", tool}});
                {
                    std::string flags;
                    if (target_format == file_type::FileType::ARCHIVE_TAR) flags = "-cf";
                    if (target_format == file_type::FileType::ARCHIVE_TAR_GZ) flags = "-czf";
                    if (target_format == file_type::FileType::ARCHIVE_TAR_BZ2) flags = "-cjf";
                    if (target_format == file_type::FileType::ARCHIVE_TAR_XZ) flags = "-cJf";
                    args = {flags, fs::absolute(target_path_str).string()};
                    args.insert(args.end(), items_to_archive.begin(), items_to_archive.end());
                }
                break;
            case file_type::FileType::ARCHIVE_ZIP:
                tool = "zip";
                if (!is_tool_available(tool)) error::throw_error(error::ErrorCode::TOOL_NOT_FOUND, {{"TOOL_NAME", tool}});
                if (!password.empty()) args.insert(args.end(), {"-P", password});
                if (options.compression_level > 0) {
                    args.push_back("-" + std::to_string(options.compression_level));
                }
                args.push_back("-r"); // Recurse into directories
                args.push_back(fs::absolute(target_path_str).string());
                args.insert(args.end(), items_to_archive.begin(), items_to_archive.end());
                break;
            case file_type::FileType::ARCHIVE_7Z:
                tool = "7z";
                if (!is_tool_available(tool)) error::throw_error(error::ErrorCode::TOOL_NOT_FOUND, {{"TOOL_NAME", tool}});
                args.push_back("a"); // Add to archive
                if (!password.empty()) args.push_back("-p" + password);
                if (options.compression_level > 0) {
                    args.push_back("-mx=" + std::to_string(options.compression_level));
                }
                args.push_back(fs::absolute(target_path_str).string());
                args.insert(args.end(), items_to_archive.begin(), items_to_archive.end());
                break;
            case file_type::FileType::ARCHIVE_LZ4:
                tool = "lz4";
                if (!is_tool_available(tool)) error::throw_error(error::ErrorCode::TOOL_NOT_FOUND, {{"TOOL_NAME", tool}});
                if (options.compression_level > 0) {
                    args.push_back("-" + std::to_string(options.compression_level));
                }
                args.push_back("-r"); // Recursive
                if (items_to_archive.size() != 1) {
                    error::throw_error(error::ErrorCode::UNKNOWN_FORMAT, {{"INFO", "Multiple sources are not supported for lz4 compression."}});
                }
                args.push_back(items_to_archive.front());
                args.push_back(fs::absolute(target_path_str).string());
                break;
            case file_type::FileType::ARCHIVE_ZSTD:
                tool = "zstd";
                if (!is_tool_available(tool)) error::throw_error(error::ErrorCode::TOOL_NOT_FOUND, {{"TOOL_NAME", tool}});
                if (options.compression_level > 0) {
                    args.push_back("-" + std::to_string(options.compression_level));
                }
                args.push_back("-r"); // Recursive
                if (items_to_archive.size() != 1) {
                    error::throw_error(error::ErrorCode::UNKNOWN_FORMAT, {{"INFO", "Multiple sources are not supported for zstd compression."}});
                }
                args.push_back(items_to_archive.front());
                args.push_back("-o");
                args.push_back(fs::absolute(target_path_str).string());
                break;
            case file_type::FileType::ARCHIVE_XAR:
                tool = "xar";
                if (!is_tool_available(tool)) error::throw_error(error::ErrorCode::TOOL_NOT_FOUND, {{"TOOL_NAME", tool}});
                args.push_back("-cf");
                args.push_back(fs::absolute(target_path_str).string());
                args.insert(args.end(), items_to_archive.begin(), items_to_archive.end());
                break;
            default:
                error::throw_error(error::ErrorCode::UNKNOWN_FORMAT, {{"INFO", "Unsupported target format for compression."}});
        }
        
        std::cout << i18n::get("compressing") << std::endl;
        int result = execute_command(tool, args, working_dir_for_cmd);
        if (result != 0) {
            error::throw_error(error::ErrorCode::OPERATION_FAILED, {{"COMMAND", tool}, {"EXIT_CODE", std::to_string(result)}});
        }
        
        // End progress tracking and collect stats
        if (options.benchmark) {
            progress::end_operation();
            std::error_code ec;
            if (fs::exists(target_path_str)) {
                auto size = fs::file_size(target_path_str, ec);
                if (!ec) {
                    progress::set_compressed_size(size);
                }
            }
        }
        
        // Verify archive integrity if requested
        if (options.verify) {
            std::cout << i18n::get("verifying") << std::endl;
            if (verify_archive(target_path_str, target_format)) {
                std::cout << i18n::get("verification_success") << std::endl;
            } else {
                std::cout << i18n::get("verification_failed") << std::endl;
            }
        }
        
        std::cout << i18n::get("operation_complete") << std::endl;
        
        // Print performance statistics
        if (options.benchmark || options.verbose) {
            progress::print_stats(options.verbose, options.benchmark);
        }
    }

    void compress(const std::string& source_path_str, const std::string& target_path_str, 
                 file_type::FileType target_format, const std::string& password,
                 const args::Options& options = {}) {
        bool has_trailing_slash = !source_path_str.empty() && (source_path_str.back() == '/' || source_path_str.back() == '\\');
        CompressionSource src{source_path_str, has_trailing_slash};
        compress({src}, target_path_str, target_format, password, options);
    }
    
    void decompress(const std::string& source_path, const std::string& target_dir_path, 
                   file_type::FileType source_type, const std::string& password,
                   const args::Options& options = {}) {
        if (!fs::exists(target_dir_path)) {
            try { fs::create_directories(target_dir_path); }
            catch (const fs::filesystem_error& e) { error::throw_error(error::ErrorCode::INVALID_TARGET, {{"PATH", target_dir_path}, {"REASON", e.what()}}); }
        }

        std::string tool;
        std::vector<std::string> args;

        switch (source_type) {
            case file_type::FileType::ARCHIVE_TAR:
            case file_type::FileType::ARCHIVE_TAR_GZ:
            case file_type::FileType::ARCHIVE_TAR_BZ2:
            case file_type::FileType::ARCHIVE_TAR_XZ:
                if (!password.empty()) std::cout << i18n::get("warning_tar_password") << std::endl;
                tool = "tar";
                if (!is_tool_available(tool)) error::throw_error(error::ErrorCode::TOOL_NOT_FOUND, {{"TOOL_NAME", tool}});
                {
                    std::string flags;
                    if (source_type == file_type::FileType::ARCHIVE_TAR) flags = "-xf";
                    if (source_type == file_type::FileType::ARCHIVE_TAR_GZ) flags = "-xzf";
                    if (source_type == file_type::FileType::ARCHIVE_TAR_BZ2) flags = "-xjf";
                    if (source_type == file_type::FileType::ARCHIVE_TAR_XZ) flags = "-xJf";
                    args = {flags, fs::absolute(source_path).string(), "-C", fs::absolute(target_dir_path).string()};
                }
                break;
            case file_type::FileType::ARCHIVE_ZIP:
                if (is_split_zip(source_path)) {
                    // Split ZIP archives require 7z (unzip doesn't support them)
                    tool = "7z";
                    if (!is_tool_available(tool)) {
                        // Provide specific error for split ZIP
                        throw error::HitpagException(error::ErrorCode::TOOL_NOT_FOUND,
                            i18n::get("error_split_zip_requires_7z"));
                    }

                    // If user provided a split part (.z01, .z02), find the main .zip file
                    std::string actual_source = source_path;
                    if (is_split_zip_part(source_path)) {
                        actual_source = find_split_zip_main(source_path);
                        if (actual_source.empty()) {
                            error::throw_error(error::ErrorCode::INVALID_SOURCE,
                                {{"PATH", fs::path(source_path).replace_extension(".zip").string()},
                                 {"REASON", i18n::get("error_split_zip_main_not_found",
                                     {{"PATH", fs::path(source_path).replace_extension(".zip").string()}})}});
                        }
                    }

                    if (options.verbose) {
                        std::cout << i18n::get("info_split_zip_detected") << std::endl;
                    }

                    build_7z_extract_args(args, actual_source, target_dir_path, password);
                } else {
                    tool = "unzip";
                    if (!is_tool_available(tool)) error::throw_error(error::ErrorCode::TOOL_NOT_FOUND, {{"TOOL_NAME", tool}});
                    if (!password.empty()) args.insert(args.end(), {"-P", password});
                    args.insert(args.end(), {"-o", fs::absolute(source_path).string(), "-d", fs::absolute(target_dir_path).string()});
                }
                break;
            case file_type::FileType::ARCHIVE_RAR:
                tool = "unrar";
                if (!is_tool_available(tool)) error::throw_error(error::ErrorCode::TOOL_NOT_FOUND, {{"TOOL_NAME", "unrar"}});
                args.push_back("x"); // eXtract with full paths
                if (!password.empty()) args.push_back("-p" + password);
                args.insert(args.end(), {"-o+", fs::absolute(source_path).string(), fs::absolute(target_dir_path).string()}); // -o+: overwrite existing
                break;
            case file_type::FileType::ARCHIVE_7Z:
                tool = "7z";
                if (!is_tool_available(tool)) error::throw_error(error::ErrorCode::TOOL_NOT_FOUND, {{"TOOL_NAME", tool}});
                build_7z_extract_args(args, source_path, target_dir_path, password);
                break;
            case file_type::FileType::ARCHIVE_LZ4:
                tool = "lz4";
                if (!is_tool_available(tool)) error::throw_error(error::ErrorCode::TOOL_NOT_FOUND, {{"TOOL_NAME", tool}});
                args.push_back("-d"); // Decompress
                args.push_back(fs::absolute(source_path).string());
                args.push_back(fs::absolute(target_dir_path).string());
                break;
            case file_type::FileType::ARCHIVE_ZSTD:
                tool = "zstd";
                if (!is_tool_available(tool)) error::throw_error(error::ErrorCode::TOOL_NOT_FOUND, {{"TOOL_NAME", tool}});
                args.push_back("-d"); // Decompress
                args.push_back(fs::absolute(source_path).string());
                args.push_back("-o");
                args.push_back(fs::absolute(target_dir_path).string());
                break;
            case file_type::FileType::ARCHIVE_XAR:
                tool = "xar";
                if (!is_tool_available(tool)) error::throw_error(error::ErrorCode::TOOL_NOT_FOUND, {{"TOOL_NAME", tool}});
                args.push_back("-xf");
                args.push_back(fs::absolute(source_path).string());
                args.push_back("-C");
                args.push_back(fs::absolute(target_dir_path).string());
                break;
            default:
                error::throw_error(error::ErrorCode::UNKNOWN_FORMAT, {{"INFO", "Unsupported source format for decompression."}});
        }
        
        std::cout << i18n::get("decompressing") << std::endl;
        int result = execute_command(tool, args);
        if (result != 0) {
            error::throw_error(error::ErrorCode::OPERATION_FAILED, {{"COMMAND", tool}, {"EXIT_CODE", std::to_string(result)}});
        }
        std::cout << i18n::get("operation_complete") << std::endl;
    }
}

/**
 * @brief Interactive mode user interface module.
 * 
 * Guides the user through the compression/decompression process with a series of prompts.
 */
namespace interactive {
    /**
     * @brief Helper to get trimmed input from the user.
     * 
     * [MODIFIED] Now handles EOF (end-of-file) from stdin gracefully by throwing
     * an exception, preventing infinite loops if the input stream is closed.
     * @return The trimmed user input string.
     * @throws std::runtime_error if the input stream is closed (EOF).
     */
    std::string get_input() {
        std::string input;
        if (!std::getline(std::cin, input)) {
            // If getline fails, it's likely due to EOF (e.g., Ctrl+D).
            // Throw an exception to allow the main loop to catch it and exit gracefully.
            throw std::runtime_error(i18n::get("error_input_stream_closed"));
        }
        if (!input.empty()) {
            input.erase(0, input.find_first_not_of(" \t\n\r"));
            input.erase(input.find_last_not_of(" \t\n\r") + 1);
        }
        return input;
    }

    /**
     * @brief Securely reads a password from the terminal without echoing characters.
     * 
     * Provides visual feedback ('*') for each character typed.
     * Implemented for both Windows and POSIX systems.
     * @param prompt The message to display to the user.
     * @return The entered password.
     */
    std::string get_password_interactively(std::string_view prompt) {
        std::cout << prompt << std::flush;
        std::string password;
#ifdef _WIN32
        char ch;
        while ((ch = _getch()) != '\r') { // Enter key
            if (ch == '\b') { // Backspace
                if (!password.empty()) {
                    password.pop_back();
                    std::cout << "\b \b";
                }
            } else {
                password += ch;
                std::cout << '*';
            }
        }
#else
        termios oldt, newt;
        if (tcgetattr(STDIN_FILENO, &oldt) != 0) {
            // Failed to get terminal attributes, fallback to regular input
            std::getline(std::cin, password);
            return password;
        }
        
        newt = oldt;
        newt.c_lflag &= ~(ECHO | ICANON); // Disable echoing and canonical mode
        if (tcsetattr(STDIN_FILENO, TCSANOW, &newt) != 0) {
            // Failed to set terminal attributes, fallback to regular input
            std::getline(std::cin, password);
            return password;
        }
        
        // Use RAII to ensure terminal settings are restored
        struct TerminalRestorer {
            const termios& old_settings;
            TerminalRestorer(const termios& old) : old_settings(old) {}
            ~TerminalRestorer() { tcsetattr(STDIN_FILENO, TCSANOW, &old_settings); }
        } restorer(oldt);
        
        char ch;
        while (read(STDIN_FILENO, &ch, 1) == 1 && ch != '\n') {
            if (ch == 127 || ch == '\b') { // Handle backspace/delete
                if (!password.empty()) {
                    password.pop_back();
                    std::cout << "\b \b" << std::flush;
                }
            } else {
                password += ch;
                std::cout << '*' << std::flush;
            }
        }
        // Terminal will be restored automatically by destructor
#endif
        std::cout << std::endl;
        return password;
    }
    
    // Gets a valid integer choice from the user within a specified range.
    int get_choice(int min_val, int max_val) { 
        while (true) {
            std::cout << "> ";
            std::string input = get_input();
            try {
                int choice = std::stoi(input);
                if (choice >= min_val && choice <= max_val) return choice;
            } catch (...) {}
            std::cout << i18n::get("invalid_choice") << std::endl;
        }
    }
    
    // Gets a yes/no confirmation from the user.
    bool get_confirmation(std::string_view prompt_key, const std::map<std::string, std::string>& placeholders = {}) {
        std::cout << i18n::get(prompt_key, placeholders);
        while (true) {
            std::string input = get_input();
            if (!input.empty()) {
                char choice = static_cast<char>(std::tolower(static_cast<unsigned char>(input[0])));
                if (choice == 'y') return true;
                if (choice == 'n') return false;
            }
            std::cout << i18n::get("invalid_choice") << " (y/n): ";
        }
    }
    
    // Data-driven menu item structure for format selection.
    struct MenuItem {
        std::string key;
        file_type::FileType type;
        bool supports_password;
    };

    void run(args::Options& options) { 
        std::cout << i18n::get("interactive_mode") << std::endl;
        
        if (options.source_path.empty()) {
            std::cout << "Please enter source path: ";
            options.source_path = get_input();
        }
        
        file_type::FileType source_type = file_type::recognize_source_type(options.source_path);
        
        std::cout << "Source: " << options.source_path << " (" << file_type::get_file_type_string(source_type) << ")" << std::endl;
        
        file_type::OperationType op_type = (source_type == file_type::FileType::DIRECTORY || source_type == file_type::FileType::REGULAR_FILE)
            ? file_type::OperationType::COMPRESS : file_type::OperationType::DECOMPRESS;
        
        std::cout << "Detected operation: " << (op_type == file_type::OperationType::COMPRESS ? "Compress" : "Decompress") << ". Change? (y/n): ";
        std::string change_op_input = get_input();
        if (!change_op_input.empty() && std::tolower(static_cast<unsigned char>(change_op_input[0])) == 'y') {
            std::cout << i18n::get("ask_operation") << std::endl << i18n::get("operation_compress") << std::endl << i18n::get("operation_decompress") << std::endl;
            op_type = (get_choice(1, 2) == 1) ? file_type::OperationType::COMPRESS : file_type::OperationType::DECOMPRESS;
        }
        
        file_type::FileType target_format = file_type::FileType::UNKNOWN;
        
        if (op_type == file_type::OperationType::COMPRESS) {
            const std::vector<MenuItem> formats = {
                {"format_tar_gz", file_type::FileType::ARCHIVE_TAR_GZ, false},
                {"format_zip", file_type::FileType::ARCHIVE_ZIP, true},
                {"format_7z", file_type::FileType::ARCHIVE_7Z, true},
                {"format_tar", file_type::FileType::ARCHIVE_TAR, false},
                {"format_tar_bz2", file_type::FileType::ARCHIVE_TAR_BZ2, false},
                {"format_tar_xz", file_type::FileType::ARCHIVE_TAR_XZ, false},
                {"format_lz4", file_type::FileType::ARCHIVE_LZ4, false},
                {"format_zstd", file_type::FileType::ARCHIVE_ZSTD, false},
                {"format_xar", file_type::FileType::ARCHIVE_XAR, false}
            };
            std::cout << i18n::get("ask_format") << std::endl;
            for(size_t i = 0; i < formats.size(); ++i) std::cout << i+1 << ". " << i18n::get(formats[i].key) << std::endl;
            int choice = get_choice(1, formats.size());
            const auto& selected_format = formats[choice - 1];
            target_format = selected_format.type;

            std::cout << "Please enter target archive path: ";
            options.target_path = get_input();
            if (options.target_path.empty()) error::throw_error(error::ErrorCode::INVALID_TARGET, {{"REASON", "Target path cannot be empty"}});

            if (selected_format.supports_password && options.password.empty()) {
                if (get_confirmation("ask_set_password")) {
                    while (true) {
                        std::string p1 = get_password_interactively(i18n::get("enter_password"));
                        std::string p2 = get_password_interactively(i18n::get("confirm_password"));
                        if (p1 == p2) { options.password = p1; break; }
                        else { std::cout << i18n::get("password_mismatch") << std::endl; }
                    }
                }
            }
        } else { // Decompress
            if (options.password.empty()) {
                 if (get_confirmation("ask_has_password")) {
                    options.password = get_password_interactively(i18n::get("enter_password"));
                 }
            }
            std::cout << "Please enter target directory (default: './'): ";
            options.target_path = get_input();
            if (options.target_path.empty()) options.target_path = ".";
        }

        const auto interactive_input_adapter = []() { return get_input(); };
        const auto interactive_output_adapter = [](const std::string& message) { std::cout << message << std::flush; };
        const auto interactive_error_adapter = [](const std::string& message) { std::cerr << message << std::flush; };

        if (!target_path::resolve_existing_target(options.target_path, interactive_input_adapter, interactive_output_adapter, interactive_error_adapter)) {
            std::cout << i18n::get("operation_canceled") << std::endl;
            return;
        }
        
        bool delete_source = get_confirmation("ask_delete_source", {{"SOURCE_PATH", options.source_path}});
        
        if (op_type == file_type::OperationType::COMPRESS) {
            operation::compress(options.source_path, options.target_path, target_format, options.password, options);
        } else { 
            operation::decompress(options.source_path, options.target_path, source_type, options.password, options);
        }
        
        if (delete_source) {
            std::cout << "Deleting source: " << options.source_path << std::endl;
            std::error_code ec;
            fs::remove_all(options.source_path, ec);
            if (ec) std::cerr << "Warning: Failed to delete source '" << options.source_path << "': " << ec.message() << std::endl;
            else std::cout << "Source deleted." << std::endl;
        }
    }
}

/**
 * @brief Main entry point of the application.
 * 
 * Orchestrates the entire workflow:
 * 1. Parses command-line arguments.
 * 2. Handles help and version flags.
 * 3. Dispatches to interactive or command-line mode.
 * 4. Catches all exceptions for graceful error reporting and exit.
 */
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

        if (options.password_prompt) {
            options.password = interactive::get_password_interactively(i18n::get("enter_password"));
        }
        
        if (options.interactive_mode) {
            interactive::run(options); 
        } else {
            const auto cli_input_adapter = []() { return cli_io::get_input(); };
            const auto cli_output_adapter = [](const std::string& message) { std::cout << message << std::flush; };
            const auto cli_error_adapter = [](const std::string& message) { std::cerr << message << std::flush; };

            if (options.source_paths.size() > 1) {
                // Ensure target is not the same as any source.
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

                operation::compress(compression_sources, options.target_path, target_type, options.password, options);
            } else {
                // Prevent operating on the same file/directory
                if (fs::exists(options.source_path) && fs::exists(options.target_path)) {
                    std::error_code ec;
                    if (fs::equivalent(options.source_path, options.target_path, ec) && !ec) {
                        error::throw_error(error::ErrorCode::SAME_PATH);
                    }
                }

                file_type::RecognitionResult result = file_type::recognize(options.source_path, options.target_path);
                
                // Override target format if manually specified
                if (!options.force_format.empty()) {
                    file_type::FileType forced_type = file_type::parse_format_string(options.force_format);
                    if (forced_type == file_type::FileType::UNKNOWN) {
                        error::throw_error(error::ErrorCode::UNKNOWN_FORMAT, {{"INFO", "Invalid format specified: " + options.force_format}});
                    }
                    
                    // For compression, override the target format
                    if (result.operation == file_type::OperationType::COMPRESS) {
                        result.target_type_hint = forced_type;
                    } else {
                        // For decompression, override the source type detection
                        result.source_type = forced_type;
                    }
                }
                
                // Check if we need format specification for compression
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
                    operation::compress(options.source_path, options.target_path, result.target_type_hint, options.password, options);
                } else if (result.operation == file_type::OperationType::DECOMPRESS) {
                    operation::decompress(options.source_path, options.target_path, result.source_type, options.password, options);
                }
            }
        }
        
        std::cout << i18n::get("goodbye") << std::endl;

    } catch (const error::HitpagException& e) {
        std::cerr << e.what() << std::endl;
        return static_cast<int>(e.code()); 
    } catch (const std::exception& e) {
        // This will catch standard exceptions, including the one thrown by get_input() on EOF.
        std::cerr << "An unexpected error occurred: " << e.what() << std::endl;
        return static_cast<int>(error::ErrorCode::UNKNOWN_ERROR);
    } catch (...) {
        std::cerr << "An unknown, non-standard error occurred." << std::endl;
        return static_cast<int>(error::ErrorCode::UNKNOWN_ERROR);
    }
    
    return 0;
}
