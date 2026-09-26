// Copyright (C) 2025 Hitmux
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#include "include/process.h"
#include <algorithm>
#include <array>
#include <cerrno>
#include <fstream>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#endif

namespace process {
#ifdef _WIN32
    static std::string quote_windows(const std::string& value) {
        std::string result = "\"";
        size_t slashes = 0;
        for (char c : value) {
            if (c == '\\') { ++slashes; continue; }
            result.append(slashes * (c == '"' ? 2 : 1), '\\');
            slashes = 0;
            if (c == '"') result += '\\';
            result += c;
        }
        result.append(slashes * 2, '\\');
        return result + '"';
    }
#endif
#ifdef _WIN32
    static CommandResult run_command_capture_windows(const std::vector<std::string>& cmd, size_t max_output, std::ostream* output = nullptr) {
        CommandResult result;
        if (cmd.empty()) return result;

        std::string tool = cmd[0];
        std::string args_str;
        for (size_t i = 1; i < cmd.size(); i++) {
            args_str += " " + quote_windows(cmd[i]);
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
        HANDLE input = CreateFileA("NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, &sa, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        si.hStdInput = input;
        si.dwFlags |= STARTF_USESTDHANDLES;

        PROCESS_INFORMATION pi;
        ZeroMemory(&pi, sizeof(pi));

        std::string full_cmd = quote_windows(tool) + args_str;
        std::vector<char> cmd_buf(full_cmd.begin(), full_cmd.end());
        cmd_buf.push_back('\0');

        if (CreateProcessA(NULL, cmd_buf.data(), NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
            CloseHandle(hWrite);
            std::array<char, 4096> buffer;
            DWORD bytes_read;
            while (ReadFile(hRead, buffer.data(), static_cast<DWORD>(buffer.size()), &bytes_read, NULL) && bytes_read > 0) {
                if (output) output->write(buffer.data(), bytes_read);
                else result.stdout_output.append(buffer.data(), std::min(static_cast<size_t>(bytes_read), max_output - result.stdout_output.size()));
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

        if (input != INVALID_HANDLE_VALUE) CloseHandle(input);
        return result;
    }

    static int run_command_status_windows(const std::vector<std::string>& cmd) {
        return run_command_capture_windows(cmd, 0).exit_code;
    }
#else
    static CommandResult run_command_capture_posix(const std::vector<std::string>& cmd, size_t max_output, std::ostream* output = nullptr) {
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
            while (true) {
                bytes_read = read(pipefd[0], buffer.data(), buffer.size());
                if (bytes_read < 0 && errno == EINTR) continue;
                if (bytes_read <= 0) break;
                if (output) output->write(buffer.data(), bytes_read);
                else result.stdout_output.append(buffer.data(), std::min(static_cast<size_t>(bytes_read), max_output - result.stdout_output.size()));
            }
            close(pipefd[0]);

            int status = 0;
            pid_t waited;
            do { waited = waitpid(pid, &status, 0); } while (waited == -1 && errno == EINTR);
            if (waited == pid && WIFEXITED(status)) {
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
            int status = 0;
            pid_t waited;
            do { waited = waitpid(pid, &status, 0); } while (waited == -1 && errno == EINTR);
            if (waited == pid && WIFEXITED(status)) {
                return WEXITSTATUS(status);
            }
            return -1;
        }
        return -1;
    }
#endif

    CommandResult run_command_capture(const std::vector<std::string>& cmd, size_t max_output) {
#ifdef _WIN32
        return run_command_capture_windows(cmd, max_output);
#else
        return run_command_capture_posix(cmd, max_output);
#endif
    }

    bool run_command_to_file(const std::vector<std::string>& cmd, const std::string& path) {
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        if (!output) return false;
#ifdef _WIN32
        auto result = run_command_capture_windows(cmd, 0, &output);
#else
        auto result = run_command_capture_posix(cmd, 0, &output);
#endif
        output.flush();
        return result.exit_code == 0 && output.good();
    }

    int run_command_status(const std::vector<std::string>& cmd) {
#ifdef _WIN32
        return run_command_status_windows(cmd);
#else
        return run_command_status_posix(cmd);
#endif
    }

}
