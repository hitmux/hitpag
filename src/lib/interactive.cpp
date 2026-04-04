// Copyright (C) 2025 Hitmux
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#include "include/interactive.h"
#include "include/i18n.h"
#include "include/error.h"
#include "include/file_type.h"
#include "include/operation.h"
#include "include/util.h"
#include "include/target_path.h"

#include <iostream>
#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <cctype>
#include <filesystem>
#include <functional>

#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

namespace fs = std::filesystem;

namespace interactive {
    std::string get_input() {
        std::string input;
        if (!std::getline(std::cin, input)) {
            throw std::runtime_error(i18n::get("error_input_stream_closed"));
        }
        if (!input.empty()) {
            input.erase(0, input.find_first_not_of(" \t\n\r"));
            input.erase(input.find_last_not_of(" \t\n\r") + 1);
        }
        return input;
    }

    std::string get_password_interactively(std::string_view prompt) {
        std::cout << prompt << std::flush;
        std::string password;
#ifdef _WIN32
        char ch;
        while ((ch = _getch()) != '\r') {
            if (ch == '\b') {
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
            std::getline(std::cin, password);
            return password;
        }

        newt = oldt;
        newt.c_lflag &= ~(ECHO | ICANON);
        if (tcsetattr(STDIN_FILENO, TCSANOW, &newt) != 0) {
            std::getline(std::cin, password);
            return password;
        }

        struct TerminalRestorer {
            const termios& old_settings;
            TerminalRestorer(const termios& old) : old_settings(old) {}
            ~TerminalRestorer() { tcsetattr(STDIN_FILENO, TCSANOW, &old_settings); }
        } restorer(oldt);

        char ch;
        while (read(STDIN_FILENO, &ch, 1) == 1 && ch != '\n') {
            if (ch == 127 || ch == '\b') {
                if (!password.empty()) {
                    password.pop_back();
                    std::cout << "\b \b" << std::flush;
                }
            } else {
                password += ch;
                std::cout << '*' << std::flush;
            }
        }
#endif
        std::cout << std::endl;
        return password;
    }

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

    struct MenuItem {
        std::string key;
        file_type::FileType type;
        bool supports_password;
    };

    void run(args::Options& options, progress::ProgressTracker& tracker) {
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
            for (size_t i = 0; i < formats.size(); ++i) std::cout << i+1 << ". " << i18n::get(formats[i].key) << std::endl;
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
        } else {
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
            operation::compress(options.source_path, options.target_path, target_format, options.password, options, tracker);
        } else {
            operation::decompress(options.source_path, options.target_path, source_type, options.password, options, tracker);
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
