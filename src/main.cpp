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
// Version: 1.1.0
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

// [NEW] Application constants for easy maintenance and display.
constexpr std::string_view APP_VERSION = "1.1.0";
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
        
        // Help messages
        {"usage", "Usage: hitpag [options] [--] SOURCE_PATH TARGET_PATH"},
        {"help_options", "Options:"},
        {"help_i", "  -i              Interactive mode"},
        {"help_p", "  -p[password]    Encrypt/Decrypt with a password. If password is not attached, prompts for it."},
        {"help_h", "  -h, --help      Display help information"},
        {"help_v", "  -v, --version   Display version information"},
        {"help_examples", "Examples:"},
        {"help_example1", "  hitpag arch.tar.gz ./extracted_dir    # Decompress arch.tar.gz to extracted_dir"},
        {"help_example2", "  hitpag ./my_folder my_archive.zip     # Compress my_folder to my_archive.zip (creates my_folder inside zip)"},
        {"help_example_new_path", "  hitpag ./my_folder/ my_archive.zip    # Compress contents of my_folder (no root folder in zip)"},
        {"help_example3", "  hitpag -i big_file.rar .              # Interactive decompression of big_file.rar to current directory"},
        {"help_example4", "  hitpag -pmysecret my_docs.7z ./docs  # Encrypt ./docs into my_docs.7z with password 'mysecret'"},
        {"help_example5", "  hitpag -p secret.zip .                # Decompress secret.zip, will prompt for password"},
        
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
        {"operation_complete", "Operation complete"},
        {"operation_canceled", "Operation canceled"},
        {"warning_tar_password", "Warning: Password protection is not supported for tar formats. The password will be ignored."},
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
        std::string target_path;
        std::string password;
        bool password_prompt = false;
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
            } else {
                 error::throw_error(error::ErrorCode::MISSING_ARGS, {{"ADDITIONAL_INFO", "Unknown option: " + opt}});
            }
        }
        
        if (i < args_vec.size()) options.source_path = args_vec[i++];
        if (i < args_vec.size()) options.target_path = args_vec[i++];

        if (i < args_vec.size()) {
            error::throw_error(error::ErrorCode::MISSING_ARGS, {{"ADDITIONAL_INFO", "Too many arguments"}});
        }
        
        if (!options.interactive_mode && !options.show_help && !options.show_version) {
            if (options.source_path.empty()) error::throw_error(error::ErrorCode::MISSING_ARGS, {{"ADDITIONAL_INFO", "Source path missing"}});
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
            {"-i", "help_i"}, {"-p", "help_p"}, {"-h", "help_h"}, {"-v", "help_v"}
        };
        for(const auto& opt : help_options) std::cout << i18n::get(opt.key) << std::endl;

        std::cout << std::endl << i18n::get("help_examples") << std::endl;
        const std::vector<std::string> example_keys = {
            "help_example1", "help_example2", "help_example_new_path", "help_example3", "help_example4", "help_example5"
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
        ARCHIVE_TAR_XZ, ARCHIVE_ZIP, ARCHIVE_RAR, ARCHIVE_7Z, UNKNOWN
    };
    enum class OperationType { COMPRESS, DECOMPRESS, UNKNOWN };
    
    struct RecognitionResult {
        FileType source_type = FileType::UNKNOWN;
        FileType target_type_hint = FileType::UNKNOWN; 
        OperationType operation = OperationType::UNKNOWN;
    };
    
    FileType recognize_by_extension(const std::string& path_str) {
        fs::path p(path_str);
        if (!p.has_extension()) return FileType::UNKNOWN;
        std::string ext = p.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c){ return std::tolower(c); });

        if (ext == ".tar") return FileType::ARCHIVE_TAR;
        if (ext == ".zip") return FileType::ARCHIVE_ZIP;
        if (ext == ".rar") return FileType::ARCHIVE_RAR;
        if (ext == ".7z") return FileType::ARCHIVE_7Z;
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
        std::array<char, 8> header{}; 
        file.read(header.data(), header.size());
        if(file.gcount() < 4) return FileType::UNKNOWN; 

        if (header[0] == 0x50 && header[1] == 0x4B) return FileType::ARCHIVE_ZIP; // PK..
        if (header[0] == 0x52 && header[1] == 0x61 && header[2] == 0x72 && header[3] == 0x21) return FileType::ARCHIVE_RAR; // Rar!
        if (header[0] == 0x37 && header[1] == 0x7A && header[2] == (char)0xBC && header[3] == (char)0xAF) return FileType::ARCHIVE_7Z; // 7z..
        if (header[0] == (char)0x1F && header[1] == (char)0x8B) return FileType::ARCHIVE_TAR_GZ; // Gzip magic
        if (header[0] == 0x42 && header[1] == 0x5A && header[2] == 0x68) return FileType::ARCHIVE_TAR_BZ2; // BZh
        if (header[0] == (char)0xFD && header[1] == 0x37 && header[2] == 0x7A && header[3] == 0x58 && file.gcount() >= 6 && header[4] == 0x5A && header[5] == 0x00) return FileType::ARCHIVE_TAR_XZ; // .7zX...

        // TAR archives have "ustar" at byte offset 257
        file.clear(); 
        file.seekg(257);
        std::array<char, 6> tar_header{}; 
        file.read(tar_header.data(), tar_header.size());
        if (file.gcount() >= 5 && std::string(tar_header.data(), 5) == "ustar") return FileType::ARCHIVE_TAR;
        
        return FileType::UNKNOWN;
    }

    // Helper function to robustly determine the type of a source path.
    FileType recognize_source_type(const std::string& source_path_str) {
        if (!fs::exists(source_path_str)) {
            error::throw_error(error::ErrorCode::INVALID_SOURCE, {{"PATH", source_path_str}});
        }
        
        if (fs::is_directory(source_path_str)) return FileType::DIRECTORY;
        
        if (fs::is_regular_file(source_path_str)) {
            FileType type = recognize_by_extension(source_path_str);
            if (type == FileType::UNKNOWN) {
                type = recognize_by_header(source_path_str);
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
            if (target_is_archive) result.operation = OperationType::COMPRESS;
            else error::throw_error(error::ErrorCode::UNKNOWN_FORMAT, {{"INFO", "Target for compression must have a recognized archive extension."}});
        } else { // Source is an archive
            result.operation = OperationType::DECOMPRESS;
            if (fs::exists(target_path_str) && !fs::is_directory(target_path_str)) {
                error::throw_error(error::ErrorCode::INVALID_TARGET, {{"PATH", target_path_str}, {"REASON", "Target for decompression must be a directory."}});
            }
        }
        
        if (result.operation == OperationType::UNKNOWN) error::throw_error(error::ErrorCode::UNKNOWN_FORMAT);
        return result;
    }

    std::string get_file_type_string(FileType type) {
        static const std::map<FileType, std::string> type_map = {
            {FileType::REGULAR_FILE, "Regular File"}, {FileType::DIRECTORY, "Directory"},
            {FileType::ARCHIVE_TAR, "TAR Archive"}, {FileType::ARCHIVE_TAR_GZ, "TAR.GZ Archive"},
            {FileType::ARCHIVE_TAR_BZ2, "TAR.BZ2 Archive"}, {FileType::ARCHIVE_TAR_XZ, "TAR.XZ Archive"},
            {FileType::ARCHIVE_ZIP, "ZIP Archive"}, {FileType::ARCHIVE_RAR, "RAR Archive"},
            {FileType::ARCHIVE_7Z, "7Z Archive"}, {FileType::UNKNOWN, "Unknown Type"}
        };
        auto it = type_map.find(type);
        return it != type_map.end() ? it->second : "Unknown";
    }
}

/**
 * @brief Operation dispatch and execution module.
 * 
 * Handles the construction and secure execution of external command-line tools.
 */
namespace operation {
    bool is_tool_available(std::string_view tool) {
        #ifdef _WIN32
            std::string command = "where " + std::string(tool) + " > nul 2>&1";
        #else
            std::string command = "command -v " + std::string(tool) + " > /dev/null 2>&1";
        #endif
        return system(command.c_str()) == 0;
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

        BOOL bSuccess = CreateProcessA(
            NULL,
            &command_line[0],
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

    void compress(const std::string& source_path_str, const std::string& target_path_str, file_type::FileType target_format, const std::string& password) {
        fs::path source_path(source_path_str);
        fs::path canonical_source = fs::weakly_canonical(source_path);
        
        // Logic to handle compressing directory contents vs the directory itself.
        // This is determined by a trailing slash on the source path.
        fs::path item_to_archive;
        fs::path base_dir;
        
        bool is_dir = fs::is_directory(canonical_source);
        bool has_trailing_slash = !source_path_str.empty() && (source_path_str.back() == '/' || source_path_str.back() == '\\');

        if (is_dir && has_trailing_slash) {
            // User provided "my_dir/", so compress its contents.
            // The base directory for the command is the source directory itself.
            base_dir = canonical_source;
            // The item to archive is everything inside it ('.').
            item_to_archive = ".";
        } else {
            // User provided "my_dir" or "my_file.txt", compress the item itself.
            // The base directory is the parent of the source.
            item_to_archive = canonical_source.filename();
            if (item_to_archive.empty() || item_to_archive == ".") {
                item_to_archive = fs::path(source_path_str).filename();
            }
            base_dir = canonical_source.parent_path();
        }
        
        if (base_dir.empty()) base_dir = ".";

        std::string tool;
        std::vector<std::string> args;
        std::string working_dir_for_cmd = base_dir.string();

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
                    // tar's -C is the most reliable way to handle this.
                    args = {flags, fs::absolute(target_path_str).string(), "-C", base_dir.string(), item_to_archive.string()};
                    working_dir_for_cmd = ""; // Let tar's -C handle the directory change.
                }
                break;
            case file_type::FileType::ARCHIVE_ZIP:
                tool = "zip";
                if (!is_tool_available(tool)) error::throw_error(error::ErrorCode::TOOL_NOT_FOUND, {{"TOOL_NAME", tool}});
                if (!password.empty()) args.insert(args.end(), {"-P", password});
                args.push_back("-r"); // Recurse into directories
                args.push_back(fs::absolute(target_path_str).string());
                args.push_back(item_to_archive.string());
                break;
            case file_type::FileType::ARCHIVE_7Z:
                tool = "7z";
                if (!is_tool_available(tool)) error::throw_error(error::ErrorCode::TOOL_NOT_FOUND, {{"TOOL_NAME", tool}});
                args.push_back("a"); // Add to archive
                if (!password.empty()) args.push_back("-p" + password);
                args.push_back(fs::absolute(target_path_str).string());
                args.push_back(item_to_archive.string());
                break;
            default:
                error::throw_error(error::ErrorCode::UNKNOWN_FORMAT, {{"INFO", "Unsupported target format for compression."}});
        }
        
        std::cout << i18n::get("compressing") << std::endl;
        int result = execute_command(tool, args, working_dir_for_cmd);
        if (result != 0) {
            error::throw_error(error::ErrorCode::OPERATION_FAILED, {{"COMMAND", tool}, {"EXIT_CODE", std::to_string(result)}});
        }
        std::cout << i18n::get("operation_complete") << std::endl;
    }
    
    void decompress(const std::string& source_path, const std::string& target_dir_path, file_type::FileType source_type, const std::string& password) {
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
                tool = "unzip";
                if (!is_tool_available(tool)) error::throw_error(error::ErrorCode::TOOL_NOT_FOUND, {{"TOOL_NAME", tool}});
                if (!password.empty()) args.insert(args.end(), {"-P", password});
                args.insert(args.end(), {"-o", fs::absolute(source_path).string(), "-d", fs::absolute(target_dir_path).string()}); // -o: overwrite
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
                args.push_back("x"); // eXtract with full paths
                if (!password.empty()) args.push_back("-p" + password);
                args.push_back(fs::absolute(source_path).string());
                args.push_back("-o" + fs::absolute(target_dir_path).string()); // -o: output directory
                args.push_back("-y"); // Assume Yes to all queries
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
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ECHO | ICANON); // Disable echoing and canonical mode
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
        
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
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt); // Restore terminal settings
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
                {"format_tar_xz", file_type::FileType::ARCHIVE_TAR_XZ, false}
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

        if (fs::exists(options.target_path) && !fs::is_directory(options.target_path)) {
            if (!get_confirmation("ask_overwrite", {{"TARGET_PATH", options.target_path}})) {
                std::cout << i18n::get("operation_canceled") << std::endl;
                return;
            }
        }
        
        bool delete_source = get_confirmation("ask_delete_source", {{"SOURCE_PATH", options.source_path}});
        
        if (op_type == file_type::OperationType::COMPRESS) {
            operation::compress(options.source_path, options.target_path, target_format, options.password);
        } else { 
            operation::decompress(options.source_path, options.target_path, source_type, options.password);
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
            // Prevent operating on the same file/directory
            if (fs::exists(options.source_path) && fs::exists(options.target_path)) {
                std::error_code ec;
                if (fs::equivalent(options.source_path, options.target_path, ec) && !ec) {
                    error::throw_error(error::ErrorCode::SAME_PATH);
                }
            }

            file_type::RecognitionResult result = file_type::recognize(options.source_path, options.target_path);
            
            if (result.operation == file_type::OperationType::COMPRESS) {
                operation::compress(options.source_path, options.target_path, result.target_type_hint, options.password);
            } else if (result.operation == file_type::OperationType::DECOMPRESS) {
                operation::decompress(options.source_path, options.target_path, result.source_type, options.password);
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
