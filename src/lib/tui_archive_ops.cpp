// Copyright (C) 2025 Hitmux
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#include "include/tui_archive_ops.h"
#include "include/operation.h"

#include <cstdio>
#include <array>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <cctype>

#ifndef _WIN32
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#endif

namespace fs = std::filesystem;

namespace tui::archive_ops {

    static std::string trim_str(std::string s) {
        s.erase(0, s.find_first_not_of(" \t\n\r"));
        if (!s.empty()) s.erase(s.find_last_not_of(" \t\n\r") + 1);
        return s;
    }

    static std::string build_unrar_password_arg(const std::string& password) {
        return password.empty() ? "-p-" : "-p" + password;
    }

    static TextExtractionResult make_text_extraction_result(CommandResult result) {
        TextExtractionResult extraction;
        extraction.success = (result.exit_code == 0);
        extraction.content = extraction.success ? std::move(result.stdout_output) : "";
        extraction.empty_file = extraction.success && extraction.content.empty();
        return extraction;
    }

#ifdef _WIN32
    static CommandResult run_command_capture_windows(const std::vector<std::string>& cmd) {
        CommandResult result;
        if (cmd.empty()) return result;

        std::string tool = cmd[0];
        std::string args_str;
        for (size_t i = 1; i < cmd.size(); i++) {
            args_str += " " + cmd[i];
        }

        SECURITY_ATTRIBUTES sa;
        sa.nLength = sizeof(SECURITY_ATTRIBUTES);
        sa.bInheritHandle = TRUE;
        sa.lpSecurityDescriptor = NULL;

        HANDLE hRead, hWrite;
        if (!CreatePipe(&hRead, &hWrite, &sa, 0)) return result;
        if (!SetHandleInformation(hRead, HANDLE_FLAG_INHERIT, 0)) {
            CloseHandle(hWrite);
            CloseHandle(hRead);
            return result;
        }

        STARTUPINFOA si;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        si.hStdOutput = hWrite;
        si.hStdError = hWrite;
        si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
        si.dwFlags |= STARTF_USESTDHANDLES;

        PROCESS_INFORMATION pi;
        ZeroMemory(&pi, sizeof(pi));

        std::string full_cmd = tool + args_str;
        std::vector<char> cmd_buf(full_cmd.begin(), full_cmd.end());
        cmd_buf.push_back('\0');

        if (CreateProcessA(NULL, cmd_buf.data(), NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
            CloseHandle(hWrite);
            std::array<char, 4096> buffer;
            DWORD bytes_read;
            while (ReadFile(hRead, buffer.data(), static_cast<DWORD>(buffer.size()), &bytes_read, NULL) && bytes_read > 0) {
                result.stdout_output.append(buffer.data(), bytes_read);
            }
            CloseHandle(hRead);
            WaitForSingleObject(pi.hProcess, INFINITE);
            DWORD exit_code;
            GetExitCodeProcess(pi.hProcess, &exit_code);
            result.exit_code = static_cast<int>(exit_code);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        } else {
            CloseHandle(hWrite);
            CloseHandle(hRead);
        }

        return result;
    }

    static int run_command_status_windows(const std::vector<std::string>& cmd) {
        if (cmd.empty()) return -1;

        std::string tool = cmd[0];
        std::string args_str;
        for (size_t i = 1; i < cmd.size(); i++) {
            args_str += " " + cmd[i];
        }

        STARTUPINFOA si;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);

        PROCESS_INFORMATION pi;
        ZeroMemory(&pi, sizeof(pi));

        std::string full_cmd = tool + args_str;
        std::vector<char> cmd_buf(full_cmd.begin(), full_cmd.end());
        cmd_buf.push_back('\0');

        if (CreateProcessA(NULL, cmd_buf.data(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
            WaitForSingleObject(pi.hProcess, INFINITE);
            DWORD exit_code;
            GetExitCodeProcess(pi.hProcess, &exit_code);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            return static_cast<int>(exit_code);
        }
        return -1;
    }
#else
    static CommandResult run_command_capture_posix(const std::vector<std::string>& cmd) {
        CommandResult result;
        if (cmd.empty()) return result;

        std::vector<char*> argv;
        for (const auto& arg : cmd) {
            argv.push_back(const_cast<char*>(arg.c_str()));
        }
        argv.push_back(nullptr);

        int pipefd[2];
        if (pipe(pipefd) != 0) return result;

        pid_t pid = fork();
        if (pid == 0) {
            close(pipefd[0]);
            dup2(pipefd[1], STDOUT_FILENO);
            dup2(pipefd[1], STDERR_FILENO);
            close(pipefd[1]);

            int devnull = open("/dev/null", O_RDONLY);
            if (devnull >= 0) {
                dup2(devnull, STDIN_FILENO);
                close(devnull);
            }

            execvp(argv[0], argv.data());
            _exit(127);
        } else if (pid > 0) {
            close(pipefd[1]);
            std::array<char, 4096> buffer;
            ssize_t bytes_read;
            while ((bytes_read = read(pipefd[0], buffer.data(), buffer.size())) > 0) {
                result.stdout_output.append(buffer.data(), bytes_read);
            }
            close(pipefd[0]);

            int status;
            waitpid(pid, &status, 0);
            if (WIFEXITED(status)) {
                result.exit_code = WEXITSTATUS(status);
            } else {
                result.exit_code = -1;
            }
        } else {
            close(pipefd[0]);
            close(pipefd[1]);
        }

        return result;
    }

    static int run_command_status_posix(const std::vector<std::string>& cmd) {
        if (cmd.empty()) return -1;

        std::vector<char*> argv;
        for (const auto& arg : cmd) {
            argv.push_back(const_cast<char*>(arg.c_str()));
        }
        argv.push_back(nullptr);

        pid_t pid = fork();
        if (pid == 0) {
            int devnull = open("/dev/null", O_WRONLY);
            if (devnull >= 0) {
                dup2(devnull, STDOUT_FILENO);
                dup2(devnull, STDERR_FILENO);
                close(devnull);
            }
            devnull = open("/dev/null", O_RDONLY);
            if (devnull >= 0) {
                dup2(devnull, STDIN_FILENO);
                close(devnull);
            }

            execvp(argv[0], argv.data());
            _exit(127);
        } else if (pid > 0) {
            int status;
            waitpid(pid, &status, 0);
            if (WIFEXITED(status)) {
                return WEXITSTATUS(status);
            }
            return -1;
        }
        return -1;
    }
#endif

    CommandResult run_command_capture(const std::vector<std::string>& cmd) {
#ifdef _WIN32
        return run_command_capture_windows(cmd);
#else
        return run_command_capture_posix(cmd);
#endif
    }

    int run_command_status(const std::vector<std::string>& cmd) {
#ifdef _WIN32
        return run_command_status_windows(cmd);
#else
        return run_command_status_posix(cmd);
#endif
    }

    static bool is_tar_family(file_type::FileType type) {
        return type == file_type::FileType::ARCHIVE_TAR ||
               type == file_type::FileType::ARCHIVE_TAR_GZ ||
               type == file_type::FileType::ARCHIVE_TAR_BZ2 ||
               type == file_type::FileType::ARCHIVE_TAR_XZ ||
               type == file_type::FileType::ARCHIVE_TAR_ZSTD;
    }

    static std::string get_tar_flags(file_type::FileType type) {
        switch (type) {
            case file_type::FileType::ARCHIVE_TAR_GZ: return "-xzf";
            case file_type::FileType::ARCHIVE_TAR_BZ2: return "-xjf";
            case file_type::FileType::ARCHIVE_TAR_XZ: return "-xJf";
            case file_type::FileType::ARCHIVE_TAR_ZSTD: return "-xf";
            default: return "-xf";
        }
    }

    static std::vector<ArchiveEntry> list_tar(const std::string& archive_path, file_type::FileType type) {
        std::vector<ArchiveEntry> entries;
        std::string flags = "-tf";
        if (type == file_type::FileType::ARCHIVE_TAR_ZSTD) {
            flags = "--zstd -tf";
        } else if (type == file_type::FileType::ARCHIVE_TAR_GZ) {
            flags = "-tzf";
        } else if (type == file_type::FileType::ARCHIVE_TAR_BZ2) {
            flags = "-tjf";
        } else if (type == file_type::FileType::ARCHIVE_TAR_XZ) {
            flags = "-tJf";
        }

        auto result = run_command_capture({"tar", flags, archive_path});
        if (result.exit_code != 0) return entries;

        std::istringstream stream(result.stdout_output);
        std::string line;
        while (std::getline(stream, line)) {
            line = trim_str(line);
            if (line.empty()) continue;

            ArchiveEntry entry;
            entry.path = line;
            if (!line.empty() && line.back() == '/') {
                entry.is_directory = true;
                entry.path.pop_back();
            }
            entries.push_back(entry);
        }
        return entries;
    }

    static std::vector<ArchiveEntry> list_7z(const std::string& archive_path, const std::string& password) {
        std::vector<ArchiveEntry> entries;
        std::vector<std::string> cmd = {"7z", "l", "-slt", archive_path};
        if (!password.empty()) {
            cmd.insert(cmd.begin() + 2, "-p" + password);
        }

        auto result = run_command_capture(cmd);
        if (result.exit_code != 0) return entries;

        std::istringstream stream(result.stdout_output);
        std::string line;
        ArchiveEntry current;
        bool past_separator = false;

        while (std::getline(stream, line)) {
            std::string trimmed = trim_str(line);

            if (trimmed.find("----------") != std::string::npos && trimmed.size() >= 10 && trimmed.find_first_not_of('-') == std::string::npos) {
                if (!current.path.empty()) {
                    entries.push_back(current);
                }
                past_separator = true;
                current = ArchiveEntry{};
                continue;
            }

            if (!past_separator) continue;

            if (trimmed.empty()) {
                if (!current.path.empty()) {
                    entries.push_back(current);
                    current = ArchiveEntry{};
                }
                continue;
            }

            size_t eq_pos = trimmed.find(" = ");
            if (eq_pos == std::string::npos) continue;

            std::string key = trim_str(trimmed.substr(0, eq_pos));
            std::string value = trim_str(trimmed.substr(eq_pos + 3));

            if (key == "Path") {
                if (!current.path.empty()) {
                    entries.push_back(current);
                    current = ArchiveEntry{};
                }
                current.path = value;
            } else if (key == "Folder") {
                current.is_directory = (value == "+");
            } else if (key == "Size") {
                try { current.size = std::stoull(value); } catch (...) {}
            } else if (key == "Packed Size") {
                try { current.compressed_size = std::stoull(value); } catch (...) {}
            } else if (key == "Modified") {
                current.modified = value;
            } else if (key == "Method") {
                current.method = value;
            } else if (key == "CRC") {
                try { current.crc = std::stoul(value, nullptr, 16); } catch (...) {}
            }
        }

        if (past_separator && !current.path.empty()) {
            entries.push_back(current);
        }
        return entries;
    }

    static std::vector<ArchiveEntry> list_unzip(const std::string& archive_path, const std::string& password) {
        std::vector<ArchiveEntry> entries;
        std::vector<std::string> cmd = {"unzip", "-l", archive_path};
        if (!password.empty()) {
            cmd.insert(cmd.begin() + 2, "-P");
            cmd.insert(cmd.begin() + 3, password);
        }

        auto result = run_command_capture(cmd);
        if (result.exit_code != 0) return entries;

        std::istringstream stream(result.stdout_output);
        std::string line;
        bool in_listing = false;
        bool past_dash_line = false;

        while (std::getline(stream, line)) {
            if (line.find("Archive:") != std::string::npos) continue;

            if (line.find("-------") != std::string::npos) {
                if (!past_dash_line) {
                    past_dash_line = true;
                    in_listing = true;
                } else {
                    break;
                }
                continue;
            }

            if (!in_listing || line.empty()) continue;

            std::istringstream ls(line);
            uint64_t size_val = 0, compressed_val = 0;
            std::string date_str, time_str, name;
            if (ls >> size_val >> date_str >> time_str >> name) {
                if (name.empty()) continue;
                ArchiveEntry entry;
                entry.path = name;
                entry.size = size_val;
                entry.compressed_size = compressed_val;
                entry.modified = date_str + " " + time_str;
                if (!name.empty() && name.back() == '/') {
                    entry.is_directory = true;
                    entry.path.pop_back();
                }
                entries.push_back(entry);
            }
        }
        return entries;
    }

    static std::vector<ArchiveEntry> list_xar(const std::string& archive_path) {
        std::vector<ArchiveEntry> entries;
        auto result = run_command_capture({"xar", "-tf", archive_path});
        if (result.exit_code != 0) return entries;

        std::istringstream stream(result.stdout_output);
        std::string line;
        while (std::getline(stream, line)) {
            line = trim_str(line);
            if (line.empty()) continue;

            ArchiveEntry entry;
            entry.path = line;
            if (!line.empty() && line.back() == '/') {
                entry.is_directory = true;
                entry.path.pop_back();
            }
            entries.push_back(entry);
        }
        return entries;
    }

    static std::vector<ArchiveEntry> list_rar(const std::string& archive_path, const std::string& password) {
        std::vector<ArchiveEntry> entries;
        auto result = run_command_capture({"unrar", "lb", build_unrar_password_arg(password), archive_path});
        if (result.exit_code != 0) return entries;

        std::istringstream stream(result.stdout_output);
        std::string line;
        while (std::getline(stream, line)) {
            line = trim_str(line);
            if (line.empty()) continue;

            ArchiveEntry entry;
            entry.path = line;
            if (!line.empty() && line.back() == '/') {
                entry.is_directory = true;
                entry.path.pop_back();
            }
            entries.push_back(entry);
        }
        return entries;
    }

    static std::vector<ArchiveEntry> list_single_file_archive(const std::string& archive_path, file_type::FileType type) {
        std::vector<ArchiveEntry> entries;
        fs::path p(archive_path);
        std::string stem = p.stem().string();
        if (stem.empty()) stem = p.filename().string();

        ArchiveEntry entry;
        entry.path = stem;
        entry.is_directory = false;

        std::error_code ec;
        if (fs::exists(archive_path, ec)) {
            entry.size = fs::file_size(archive_path, ec);
        }

        entries.push_back(entry);
        return entries;
    }

    std::vector<ArchiveEntry> list_archive(const std::string& archive_path, file_type::FileType type, const std::string& password) {
        std::vector<ArchiveEntry> entries;

        switch (type) {
            case file_type::FileType::ARCHIVE_TAR:
            case file_type::FileType::ARCHIVE_TAR_GZ:
            case file_type::FileType::ARCHIVE_TAR_BZ2:
            case file_type::FileType::ARCHIVE_TAR_XZ:
            case file_type::FileType::ARCHIVE_TAR_ZSTD:
                entries = list_tar(archive_path, type);
                break;

            case file_type::FileType::ARCHIVE_7Z:
                if (operation::is_tool_available("7z")) {
                    entries = list_7z(archive_path, password);
                }
                break;

            case file_type::FileType::ARCHIVE_RAR:
                if (operation::is_tool_available("unrar")) {
                    entries = list_rar(archive_path, password);
                } else if (operation::is_tool_available("7z")) {
                    entries = list_7z(archive_path, password);
                }
                break;

            case file_type::FileType::ARCHIVE_ZIP:
                if (operation::is_tool_available("7z")) {
                    entries = list_7z(archive_path, password);
                } else if (operation::is_tool_available("unzip")) {
                    entries = list_unzip(archive_path, password);
                }
                break;

            case file_type::FileType::ARCHIVE_XAR:
                if (operation::is_tool_available("xar")) {
                    entries = list_xar(archive_path);
                }
                break;

            case file_type::FileType::ARCHIVE_LZ4:
                if (operation::is_tool_available("lz4")) {
                    entries = list_single_file_archive(archive_path, type);
                }
                break;

            case file_type::FileType::ARCHIVE_ZSTD:
                if (operation::is_tool_available("zstd")) {
                    entries = list_single_file_archive(archive_path, type);
                }
                break;

            default:
                break;
        }

        std::sort(entries.begin(), entries.end(), [](const ArchiveEntry& a, const ArchiveEntry& b) {
            if (a.is_directory != b.is_directory) return a.is_directory > b.is_directory;
            return a.path < b.path;
        });

        return entries;
    }

    static CommandResult extract_tar_command(const std::string& archive_path, const std::string& entry_path, file_type::FileType type) {
        std::vector<std::string> cmd = {"tar", "-xf", archive_path, "-O", entry_path};
        if (type == file_type::FileType::ARCHIVE_TAR_ZSTD) {
            cmd = {"tar", "--zstd", "-xf", archive_path, "-O", entry_path};
        } else if (type == file_type::FileType::ARCHIVE_TAR_GZ) {
            cmd = {"tar", "-xzf", archive_path, "-O", entry_path};
        } else if (type == file_type::FileType::ARCHIVE_TAR_BZ2) {
            cmd = {"tar", "-xjf", archive_path, "-O", entry_path};
        } else if (type == file_type::FileType::ARCHIVE_TAR_XZ) {
            cmd = {"tar", "-xJf", archive_path, "-O", entry_path};
        }
        return run_command_capture(cmd);
    }

    static CommandResult extract_7z_command(const std::string& archive_path, const std::string& entry_path, const std::string& password) {
        std::vector<std::string> cmd = {"7z", "e", "-so", archive_path, entry_path};
        if (!password.empty()) {
            cmd.insert(cmd.begin() + 2, "-p" + password);
        }
        return run_command_capture(cmd);
    }

    static CommandResult extract_unzip_command(const std::string& archive_path, const std::string& entry_path, const std::string& password) {
        std::vector<std::string> cmd = {"unzip", "-p", archive_path, entry_path};
        if (!password.empty()) {
            cmd.insert(cmd.begin() + 2, "-P");
            cmd.insert(cmd.begin() + 3, password);
        }
        return run_command_capture(cmd);
    }

    static CommandResult extract_xar_command(const std::string& archive_path, const std::string& entry_path) {
        return run_command_capture({"xar", "-xf", archive_path, "-O", entry_path});
    }

    static CommandResult extract_rar_command(const std::string& archive_path, const std::string& entry_path, const std::string& password) {
        return run_command_capture({"unrar", "p", "-inul", build_unrar_password_arg(password), archive_path, entry_path});
    }

    static CommandResult extract_lz4_command(const std::string& archive_path) {
        return run_command_capture({"lz4", "-d", "-f", archive_path, "-"});
    }

    static CommandResult extract_zstd_command(const std::string& archive_path) {
        return run_command_capture({"zstd", "-d", "-f", "-c", archive_path});
    }

    TextExtractionResult extract_text(const std::string& archive_path, const std::string& entry_path, file_type::FileType type, const std::string& password) {
        switch (type) {
            case file_type::FileType::ARCHIVE_TAR:
            case file_type::FileType::ARCHIVE_TAR_GZ:
            case file_type::FileType::ARCHIVE_TAR_BZ2:
            case file_type::FileType::ARCHIVE_TAR_XZ:
            case file_type::FileType::ARCHIVE_TAR_ZSTD:
                return make_text_extraction_result(extract_tar_command(archive_path, entry_path, type));

            case file_type::FileType::ARCHIVE_7Z:
                if (operation::is_tool_available("7z")) {
                    return make_text_extraction_result(extract_7z_command(archive_path, entry_path, password));
                }
                break;

            case file_type::FileType::ARCHIVE_RAR:
                if (operation::is_tool_available("unrar")) {
                    return make_text_extraction_result(extract_rar_command(archive_path, entry_path, password));
                }
                if (operation::is_tool_available("7z")) {
                    return make_text_extraction_result(extract_7z_command(archive_path, entry_path, password));
                }
                break;

            case file_type::FileType::ARCHIVE_ZIP:
                if (operation::is_tool_available("7z")) {
                    return make_text_extraction_result(extract_7z_command(archive_path, entry_path, password));
                }
                if (operation::is_tool_available("unzip")) {
                    return make_text_extraction_result(extract_unzip_command(archive_path, entry_path, password));
                }
                break;

            case file_type::FileType::ARCHIVE_XAR:
                if (operation::is_tool_available("xar")) {
                    return make_text_extraction_result(extract_xar_command(archive_path, entry_path));
                }
                break;

            case file_type::FileType::ARCHIVE_LZ4:
                if (operation::is_tool_available("lz4")) {
                    return make_text_extraction_result(extract_lz4_command(archive_path));
                }
                break;

            case file_type::FileType::ARCHIVE_ZSTD:
                if (operation::is_tool_available("zstd")) {
                    return make_text_extraction_result(extract_zstd_command(archive_path));
                }
                break;

            default:
                break;
        }

        return TextExtractionResult{};
    }

    std::string extract_to_string(const std::string& archive_path, const std::string& entry_path, file_type::FileType type, const std::string& password) {
        switch (type) {
            case file_type::FileType::ARCHIVE_TAR:
            case file_type::FileType::ARCHIVE_TAR_GZ:
            case file_type::FileType::ARCHIVE_TAR_BZ2:
            case file_type::FileType::ARCHIVE_TAR_XZ:
            case file_type::FileType::ARCHIVE_TAR_ZSTD:
            {
                auto result = extract_tar_command(archive_path, entry_path, type);
                return result.exit_code == 0 ? result.stdout_output : "";
            }

            case file_type::FileType::ARCHIVE_7Z:
                if (operation::is_tool_available("7z")) {
                    auto result = extract_7z_command(archive_path, entry_path, password);
                    return result.exit_code == 0 ? result.stdout_output : "";
                }
                break;

            case file_type::FileType::ARCHIVE_RAR:
                if (operation::is_tool_available("unrar")) {
                    auto result = extract_rar_command(archive_path, entry_path, password);
                    return result.exit_code == 0 ? result.stdout_output : "";
                }
                if (operation::is_tool_available("7z")) {
                    auto result = extract_7z_command(archive_path, entry_path, password);
                    return result.exit_code == 0 ? result.stdout_output : "";
                }
                break;

            case file_type::FileType::ARCHIVE_ZIP:
                if (operation::is_tool_available("7z")) {
                    auto result = extract_7z_command(archive_path, entry_path, password);
                    return result.exit_code == 0 ? result.stdout_output : "";
                } else if (operation::is_tool_available("unzip")) {
                    auto result = extract_unzip_command(archive_path, entry_path, password);
                    return result.exit_code == 0 ? result.stdout_output : "";
                }
                break;

            case file_type::FileType::ARCHIVE_XAR:
                if (operation::is_tool_available("xar")) {
                    auto result = extract_xar_command(archive_path, entry_path);
                    return result.exit_code == 0 ? result.stdout_output : "";
                }
                break;

            case file_type::FileType::ARCHIVE_LZ4:
                if (operation::is_tool_available("lz4")) {
                    auto result = extract_lz4_command(archive_path);
                    return result.exit_code == 0 ? result.stdout_output : "";
                }
                break;

            case file_type::FileType::ARCHIVE_ZSTD:
                if (operation::is_tool_available("zstd")) {
                    auto result = extract_zstd_command(archive_path);
                    return result.exit_code == 0 ? result.stdout_output : "";
                }
                break;

            default:
                break;
        }
        return "";
    }

    static bool extract_single_tar(const std::string& archive_path, const std::string& entry_path, const std::string& output_dir, file_type::FileType type) {
        std::vector<std::string> cmd;
        if (type == file_type::FileType::ARCHIVE_TAR_ZSTD) {
            cmd = {"tar", "--zstd", "-xf", archive_path, "-C", output_dir, entry_path};
        } else if (type == file_type::FileType::ARCHIVE_TAR_GZ) {
            cmd = {"tar", "-xzf", archive_path, "-C", output_dir, entry_path};
        } else if (type == file_type::FileType::ARCHIVE_TAR_BZ2) {
            cmd = {"tar", "-xjf", archive_path, "-C", output_dir, entry_path};
        } else if (type == file_type::FileType::ARCHIVE_TAR_XZ) {
            cmd = {"tar", "-xJf", archive_path, "-C", output_dir, entry_path};
        } else {
            cmd = {"tar", "-xf", archive_path, "-C", output_dir, entry_path};
        }
        return run_command_status(cmd) == 0;
    }

    static bool extract_single_7z(const std::string& archive_path, const std::string& entry_path, const std::string& output_dir, const std::string& password) {
        std::vector<std::string> cmd = {"7z", "x", archive_path, entry_path, "-o" + output_dir, "-y"};
        if (!password.empty()) {
            cmd.insert(cmd.begin() + 2, "-p" + password);
        }
        return run_command_status(cmd) == 0;
    }

    static bool extract_single_unzip(const std::string& archive_path, const std::string& entry_path, const std::string& output_dir, const std::string& password) {
        std::vector<std::string> cmd = {"unzip", "-o", archive_path, entry_path, "-d", output_dir};
        if (!password.empty()) {
            cmd.insert(cmd.begin() + 2, "-P");
            cmd.insert(cmd.begin() + 3, password);
        }
        return run_command_status(cmd) == 0;
    }

    static bool extract_single_xar(const std::string& archive_path, const std::string& entry_path, const std::string& output_dir) {
        return run_command_status({"xar", "-xf", archive_path, "-C", output_dir, entry_path}) == 0;
    }

    static bool extract_single_rar(const std::string& archive_path, const std::string& entry_path, const std::string& output_dir, const std::string& password) {
        return run_command_status({"unrar", "x", build_unrar_password_arg(password), "-o+", archive_path, entry_path, output_dir}) == 0;
    }

    static bool extract_single_lz4(const std::string& archive_path, const std::string& output_dir) {
        fs::path p(archive_path);
        std::string out_name = p.stem().string();
        if (out_name.empty()) out_name = p.filename().string();
        fs::path out_path = fs::path(output_dir) / out_name;
        return run_command_status({"lz4", "-d", "-f", archive_path, out_path.string()}) == 0;
    }

    static bool extract_single_zstd(const std::string& archive_path, const std::string& output_dir) {
        fs::path p(archive_path);
        std::string out_name = p.stem().string();
        if (out_name.empty()) out_name = p.filename().string();
        fs::path out_path = fs::path(output_dir) / out_name;
        return run_command_status({"zstd", "-d", "-f", archive_path, "-o", out_path.string()}) == 0;
    }

    bool extract_single(const std::string& archive_path, const std::string& entry_path, const std::string& output_dir, file_type::FileType type, const std::string& password) {
        switch (type) {
            case file_type::FileType::ARCHIVE_TAR:
            case file_type::FileType::ARCHIVE_TAR_GZ:
            case file_type::FileType::ARCHIVE_TAR_BZ2:
            case file_type::FileType::ARCHIVE_TAR_XZ:
            case file_type::FileType::ARCHIVE_TAR_ZSTD:
                return extract_single_tar(archive_path, entry_path, output_dir, type);

            case file_type::FileType::ARCHIVE_7Z:
                if (operation::is_tool_available("7z")) {
                    return extract_single_7z(archive_path, entry_path, output_dir, password);
                }
                break;

            case file_type::FileType::ARCHIVE_RAR:
                if (operation::is_tool_available("unrar")) {
                    return extract_single_rar(archive_path, entry_path, output_dir, password);
                }
                if (operation::is_tool_available("7z")) {
                    return extract_single_7z(archive_path, entry_path, output_dir, password);
                }
                break;

            case file_type::FileType::ARCHIVE_ZIP:
                if (operation::is_tool_available("7z")) {
                    return extract_single_7z(archive_path, entry_path, output_dir, password);
                } else if (operation::is_tool_available("unzip")) {
                    return extract_single_unzip(archive_path, entry_path, output_dir, password);
                }
                break;

            case file_type::FileType::ARCHIVE_XAR:
                if (operation::is_tool_available("xar")) {
                    return extract_single_xar(archive_path, entry_path, output_dir);
                }
                break;

            case file_type::FileType::ARCHIVE_LZ4:
                if (operation::is_tool_available("lz4")) {
                    return extract_single_lz4(archive_path, output_dir);
                }
                break;

            case file_type::FileType::ARCHIVE_ZSTD:
                if (operation::is_tool_available("zstd")) {
                    return extract_single_zstd(archive_path, output_dir);
                }
                break;

            default:
                break;
        }
        return false;
    }

    bool is_text_content(const std::string& content) {
        if (content.empty()) return false;
        if (content.size() > 1024 * 1024) return false;

        size_t non_text = 0;
        for (unsigned char c : content) {
            if (c < 32 && c != '\n' && c != '\r' && c != '\t') {
                non_text++;
            }
        }
        return (non_text * 100 / content.size()) < 5;
    }
}
