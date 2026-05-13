// Copyright (C) 2025 Hitmux
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#include "include/tui_editor.h"

#include "include/operation.h"
#include "include/tui_archive_ops.h"
#include "include/i18n.h"

#include <cstdlib>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>

#ifdef _WIN32
#include <process.h>
#else
#include <unistd.h>
#endif

namespace fs = std::filesystem;

namespace tui::editor {
    namespace {
        std::string shell_quote(const std::string& value) {
#ifdef _WIN32
            std::string quoted = "\"";
            for (char ch : value) {
                if (ch == '"') {
                    quoted += '\\';
                }
                quoted += ch;
            }
            quoted += "\"";
            return quoted;
#else
            std::string quoted = "'";
            for (char ch : value) {
                if (ch == '\'') {
                    quoted += "'\"'\"'";
                } else {
                    quoted += ch;
                }
            }
            quoted += "'";
            return quoted;
#endif
        }

        std::string replace_all(std::string haystack, const std::string& needle, const std::string& replacement) {
            size_t pos = 0;
            while ((pos = haystack.find(needle, pos)) != std::string::npos) {
                haystack.replace(pos, needle.size(), replacement);
                pos += replacement.size();
            }
            return haystack;
        }

        std::string read_file_binary(const fs::path& path) {
            std::ifstream input(path, std::ios::binary);
            if (!input) {
                return "";
            }
            std::ostringstream buffer;
            buffer << input.rdbuf();
            return buffer.str();
        }

        std::string build_editor_command(const std::string& editor_template, const fs::path& file_path) {
            std::string quoted_file = shell_quote(file_path.string());
            if (editor_template.find("$file") != std::string::npos) {
                return replace_all(editor_template, "$file", quoted_file);
            }
            return editor_template + " " + quoted_file;
        }

        fs::path make_unique_temp_dir(const std::string& prefix) {
            fs::path base = fs::temp_directory_path();
#ifdef _WIN32
            int process_id = _getpid();
#else
            int process_id = static_cast<int>(::getpid());
#endif
            for (int attempt = 0; attempt < 100; ++attempt) {
                fs::path candidate = base / (prefix + std::to_string(process_id) + "-" + std::to_string(attempt));
                std::error_code ec;
                if (fs::create_directories(candidate, ec)) {
                    return candidate;
                }
            }
            throw std::runtime_error("Failed to create temporary directory");
        }

        bool is_path_inside(const fs::path& root, const fs::path& candidate) {
            std::error_code ec;
            fs::path root_canonical = fs::weakly_canonical(root, ec);
            if (ec) return false;
            fs::path candidate_canonical = fs::weakly_canonical(candidate, ec);
            if (ec) return false;

            fs::path relative = fs::relative(candidate_canonical, root_canonical, ec);
            if (ec) return false;
            if (relative.empty()) return true;
            for (const auto& part : relative) {
                if (part == "..") {
                    return false;
                }
            }
            return true;
        }

        bool supports_archive_rebuild(file_type::FileType type, const std::string& archive_path) {
            if (type == file_type::FileType::ARCHIVE_RAR) {
                return false;
            }
            if (type == file_type::FileType::ARCHIVE_ZIP && operation::is_split_zip(archive_path)) {
                return false;
            }

            switch (type) {
                case file_type::FileType::ARCHIVE_TAR:
                case file_type::FileType::ARCHIVE_TAR_GZ:
                case file_type::FileType::ARCHIVE_TAR_BZ2:
                case file_type::FileType::ARCHIVE_TAR_XZ:
                case file_type::FileType::ARCHIVE_TAR_ZSTD:
                case file_type::FileType::ARCHIVE_ZIP:
                case file_type::FileType::ARCHIVE_7Z:
                case file_type::FileType::ARCHIVE_XAR:
                    return true;
                default:
                    return false;
            }
        }

        bool replace_archive_file(const fs::path& original_archive, const fs::path& rebuilt_archive, std::string* error_message) {
            fs::path backup_archive = original_archive;
            backup_archive += ".hitpagbak";

            std::error_code ec;
            fs::remove(backup_archive, ec);
            ec.clear();

            fs::rename(original_archive, backup_archive, ec);
            if (ec) {
                if (error_message) {
                    *error_message = i18n::get("tui_editor_backup_failed", {{"REASON", ec.message()}});
                }
                return false;
            }

            ec.clear();
            fs::rename(rebuilt_archive, original_archive, ec);
            if (ec) {
                std::error_code restore_ec;
                fs::rename(backup_archive, original_archive, restore_ec);
                if (error_message) {
                    *error_message = i18n::get("tui_editor_replace_failed", {{"REASON", ec.message()}});
                }
                return false;
            }

            fs::remove(backup_archive, ec);
            return true;
        }

        bool rebuild_archive_with_modified_file(const editor::EditSession& session, std::string* error_message) {
            fs::path rebuild_root = make_unique_temp_dir("hitpag-rebuild-");
            fs::path extracted_root = rebuild_root / "content";
            fs::create_directories(extracted_root);

            args::Options rebuild_options = session.options;
            rebuild_options.benchmark = false;
            rebuild_options.verify = false;
            rebuild_options.verbose = false;

            progress::ProgressTracker tracker;

            try {
                operation::decompress(session.archive_path, extracted_root.string(), session.type, session.password, rebuild_options, tracker);

                fs::path target_file = extracted_root / fs::path(session.entry_path);
                if (!is_path_inside(extracted_root, target_file)) {
                    if (error_message) {
                        *error_message = i18n::get("tui_editor_escape_root", {{"PATH", session.entry_path}});
                    }
                    fs::remove_all(rebuild_root);
                    return false;
                }

                std::error_code ec;
                fs::create_directories(target_file.parent_path(), ec);
                if (ec) {
                    if (error_message) {
                        *error_message = i18n::get("tui_editor_create_dirs_failed", {{"REASON", ec.message()}});
                    }
                    fs::remove_all(rebuild_root);
                    return false;
                }

                ec.clear();
                fs::copy_file(session.temp_file, target_file, fs::copy_options::overwrite_existing, ec);
                if (ec) {
                    if (error_message) {
                        *error_message = i18n::get("tui_editor_copy_failed", {{"REASON", ec.message()}});
                    }
                    fs::remove_all(rebuild_root);
                    return false;
                }

                fs::path archive_path_fs(session.archive_path);
                fs::path rebuilt_archive = archive_path_fs.parent_path() /
                    (archive_path_fs.filename().string() + ".hitpag-editing");
                fs::remove(rebuilt_archive, ec);

                operation::CompressionSource source{extracted_root.string(), true};
                operation::compress({source}, rebuilt_archive.string(), session.type, session.password, rebuild_options, tracker);

                bool replaced = replace_archive_file(archive_path_fs, rebuilt_archive, error_message);
                fs::remove_all(rebuild_root);
                return replaced;
            } catch (const std::exception& ex) {
                if (error_message) {
                    *error_message = i18n::get("tui_editor_rebuild_failed", {{"REASON", ex.what()}});
                }
                std::error_code cleanup_ec;
                fs::remove_all(rebuild_root, cleanup_ec);
                return false;
            }
        }

        void cleanup_session_paths(editor::EditSession& session) {
            if (!session.temp_root.empty()) {
                std::error_code cleanup_ec;
                fs::remove_all(session.temp_root, cleanup_ec);
            }
            session.temp_root.clear();
            session.temp_file.clear();
        }
    }

    EditSession start_edit_session(const std::string& archive_path,
                                   const std::string& entry_path,
                                   file_type::FileType type,
                                   const std::string& password,
                                   const args::Options& options,
                                   const settings::TuiSettings& settings) {
        editor::EditSession session;
        if (!settings.has_editor_command()) {
            session.message = i18n::get("tui_editor_not_configured");
            session.outcome = EditSession::Outcome::Error;
            return session;
        }

        try {
            fs::path temp_root = make_unique_temp_dir("hitpag-edit-");
            bool extracted = archive_ops::extract_single(archive_path, entry_path, temp_root.string(), type, password);
            if (!extracted) {
                std::error_code cleanup_ec;
                fs::remove_all(temp_root, cleanup_ec);
                session.message = i18n::get("tui_editor_extract_failed");
                session.outcome = EditSession::Outcome::Error;
                return session;
            }

            fs::path temp_file = temp_root / fs::path(entry_path);
            if (!is_path_inside(temp_root, temp_file) || !fs::exists(temp_file)) {
                std::error_code cleanup_ec;
                fs::remove_all(temp_root, cleanup_ec);
                session.message = i18n::get("tui_editor_temp_path_invalid");
                session.outcome = EditSession::Outcome::Error;
                return session;
            }

            std::string original_content = read_file_binary(temp_file);
            std::string command = build_editor_command(settings.editor_command, temp_file);
            int editor_result = std::system(command.c_str());
            session.launched = true;
            session.archive_path = archive_path;
            session.entry_path = entry_path;
            session.password = password;
            session.type = type;
            session.options = options;
            session.temp_root = temp_root;
            session.temp_file = temp_file;

            if (editor_result != 0) {
                cleanup_session_paths(session);
                session.message = i18n::get("tui_editor_command_failed");
                session.outcome = EditSession::Outcome::Error;
                return session;
            }

            std::string edited_content = read_file_binary(temp_file);
            if (edited_content == original_content) {
                cleanup_session_paths(session);
                session.message = i18n::get("tui_editor_no_changes");
                session.outcome = EditSession::Outcome::NoChanges;
                return session;
            }

            session.file_changed = true;

            if (!supports_archive_rebuild(type, archive_path)) {
                session.keep_temp_files = true;
                session.outcome = EditSession::Outcome::TemporaryOnlyKept;
                session.message = i18n::get("tui_editor_temp_kept", {{"PATH", temp_root.string()}});
                return session;
            }

            session.write_back_supported = true;
            session.outcome = EditSession::Outcome::WriteBackPending;
            session.message = i18n::get("tui_editor_write_back_prompt");
            return session;
        } catch (const std::exception& ex) {
            session.message = ex.what();
            session.outcome = EditSession::Outcome::Error;
            cleanup_session_paths(session);
            return session;
        }
    }

    EditResult write_back_session(EditSession& session) {
        editor::EditResult result;
        result.launched = session.launched;
        result.file_changed = session.file_changed;

        if (!session.pending_write_back()) {
            result.message = session.message.empty() ? i18n::get("tui_editor_no_pending_write_back") : session.message;
            return result;
        }

        std::string error_message;
        if (!rebuild_archive_with_modified_file(session, &error_message)) {
            result.message = error_message.empty() ? i18n::get("tui_editor_write_back_failed") : error_message;
            cleanup_session_paths(session);
            return result;
        }

        cleanup_session_paths(session);
        result.archive_updated = true;
        result.message = i18n::get("tui_editor_archive_updated");
        return result;
    }

    void discard_edit_session(EditSession& session) {
        cleanup_session_paths(session);
        session.file_changed = false;
        session.write_back_supported = false;
        session.keep_temp_files = false;
        session.outcome = EditSession::Outcome::None;
    }

    void keep_edit_session(EditSession& session) {
        session.file_changed = false;
        session.write_back_supported = false;
        session.keep_temp_files = false;
        session.outcome = EditSession::Outcome::None;
        session.archive_path.clear();
        session.entry_path.clear();
        session.password.clear();
        session.type = file_type::FileType::UNKNOWN;
        session.options = args::Options{};
    }
}
