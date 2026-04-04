// Copyright (C) 2025 Hitmux
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#include "include/operation.h"
#include "include/error.h"
#include "include/i18n.h"

#include <filesystem>
#include <iostream>
#include <string_view>
#include <algorithm>
#include <cctype>

#ifdef _WIN32
#include <windows.h>
#include <process.h>
#else
#include <cstdio>
#include <unistd.h>
#include <sys/wait.h>
#endif

namespace fs = std::filesystem;

namespace operation {
    bool is_tool_available(std::string_view tool) {
#ifdef _WIN32
        std::string command = "where " + std::string(tool) + " > nul 2>&1";
#else
        std::string command = "command -v " + std::string(tool) + " > /dev/null 2>&1";
#endif
        return system(command.c_str()) == 0;
    }

    bool is_split_zip_part(const std::string& path) {
        fs::path p(path);
        if (!p.has_extension()) return false;
        std::string ext = p.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c){ return std::tolower(c); });
        return file_type::is_split_zip_extension(ext);
    }

    std::string find_split_zip_main(const std::string& any_part_path) {
        fs::path p(any_part_path);
        fs::path main_zip = p;
        main_zip.replace_extension(".zip");
        if (fs::exists(main_zip)) {
            return main_zip.string();
        }
        return "";
    }

    bool is_split_zip(const std::string& zip_path) {
        fs::path p(zip_path);
        if (!p.has_extension()) return false;
        std::string ext = p.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c){ return std::tolower(c); });
        if (is_split_zip_part(zip_path)) return true;
        if (ext != ".zip") return false;
        fs::path z01_path = p;
        z01_path.replace_extension(".z01");
        return fs::exists(z01_path);
    }

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
        if (arg.empty()) return "\"\"";
        if (arg.find_first_of(" \t\n\v\"") == std::string::npos) return arg;

        std::string quoted_arg;
        quoted_arg.push_back('"');
        for (auto it = arg.begin(); ; ++it) {
            unsigned int backslash_count = 0;
            while (it != arg.end() && *it == '\\') { ++it; ++backslash_count; }
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
        for (const auto& arg : args) full_command += " " + arg;

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

        std::vector<char> cmd_line_buf(command_line.begin(), command_line.end());
        cmd_line_buf.push_back('\0');

        BOOL bSuccess = CreateProcessA(NULL, cmd_line_buf.data(), NULL, NULL, TRUE, 0, NULL,
            working_dir.empty() ? NULL : working_dir.c_str(), &siStartInfo, &piProcInfo);

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

        if (pid == 0) {
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
                return true;
        }

        if (!is_tool_available(tool)) return false;
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
                if (part == "..") return false;
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

        uintmax_t calculate_sources_size(const std::vector<fs::path>& canonical_sources, progress::ProgressTracker& tracker) {
            uintmax_t total = 0;
            for (const auto& p : canonical_sources) {
                std::error_code ec;
                if (fs::is_directory(p, ec)) {
                    total += tracker.calculate_directory_size(p.string());
                } else {
                    auto sz = fs::file_size(p, ec);
                    if (!ec) total += sz;
                }
            }
            return total;
        }
    }

    void compress(const std::vector<CompressionSource>& sources, const std::string& target_path_str,
                  file_type::FileType target_format, const std::string& password,
                  const args::Options& options, progress::ProgressTracker& tracker) {
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

        if (options.benchmark) {
            tracker.start_operation();
            tracker.set_original_size(calculate_sources_size(canonical_sources, tracker));
            tracker.set_thread_count(options.thread_count > 0 ? options.thread_count : 1);
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
                    if (fallback.empty()) fallback = canonical;
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
                args.push_back("-r");
                args.push_back(fs::absolute(target_path_str).string());
                args.insert(args.end(), items_to_archive.begin(), items_to_archive.end());
                break;
            case file_type::FileType::ARCHIVE_7Z:
                tool = "7z";
                if (!is_tool_available(tool)) error::throw_error(error::ErrorCode::TOOL_NOT_FOUND, {{"TOOL_NAME", tool}});
                args.push_back("a");
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
                args.push_back("-r");
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
                args.push_back("-r");
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

        if (options.benchmark) {
            tracker.end_operation();
            std::error_code ec;
            if (fs::exists(target_path_str)) {
                auto size = fs::file_size(target_path_str, ec);
                if (!ec) tracker.set_compressed_size(size);
            }
        }

        if (options.verify) {
            std::cout << i18n::get("verifying") << std::endl;
            if (verify_archive(target_path_str, target_format)) {
                std::cout << i18n::get("verification_success") << std::endl;
            } else {
                std::cout << i18n::get("verification_failed") << std::endl;
            }
        }

        std::cout << i18n::get("operation_complete") << std::endl;

        if (options.benchmark || options.verbose) {
            tracker.print_stats(options.verbose, options.benchmark);
        }
    }

    void compress(const std::string& source_path_str, const std::string& target_path_str,
                  file_type::FileType target_format, const std::string& password,
                  const args::Options& options, progress::ProgressTracker& tracker) {
        bool has_trailing_slash = !source_path_str.empty() && (source_path_str.back() == '/' || source_path_str.back() == '\\');
        CompressionSource src{source_path_str, has_trailing_slash};
        compress({src}, target_path_str, target_format, password, options, tracker);
    }

    void decompress(const std::string& source_path, const std::string& target_dir_path,
                    file_type::FileType source_type, const std::string& password,
                    const args::Options& options, progress::ProgressTracker& tracker) {
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
                    tool = "7z";
                    if (!is_tool_available(tool)) {
                        throw error::HitpagException(error::ErrorCode::TOOL_NOT_FOUND,
                            i18n::get("error_split_zip_requires_7z"));
                    }
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
                args.push_back("x");
                if (!password.empty()) args.push_back("-p" + password);
                args.insert(args.end(), {"-o+", fs::absolute(source_path).string(), fs::absolute(target_dir_path).string()});
                break;
            case file_type::FileType::ARCHIVE_7Z:
                tool = "7z";
                if (!is_tool_available(tool)) error::throw_error(error::ErrorCode::TOOL_NOT_FOUND, {{"TOOL_NAME", tool}});
                build_7z_extract_args(args, source_path, target_dir_path, password);
                break;
            case file_type::FileType::ARCHIVE_LZ4:
                tool = "lz4";
                if (!is_tool_available(tool)) error::throw_error(error::ErrorCode::TOOL_NOT_FOUND, {{"TOOL_NAME", tool}});
                args.push_back("-d");
                args.push_back(fs::absolute(source_path).string());
                args.push_back(fs::absolute(target_dir_path).string());
                break;
            case file_type::FileType::ARCHIVE_ZSTD:
                tool = "zstd";
                if (!is_tool_available(tool)) error::throw_error(error::ErrorCode::TOOL_NOT_FOUND, {{"TOOL_NAME", tool}});
                args.push_back("-d");
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
