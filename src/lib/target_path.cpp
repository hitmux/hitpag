// Copyright (C) 2025 Hitmux
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#include "include/target_path.h"
#include "include/target_conflict.h"
#include "include/i18n.h"
#include "include/util.h"

#include <filesystem>
#include <vector>
#include <string>

namespace fs = std::filesystem;

namespace target_path {
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
    }

    bool resolve_existing_target(std::string& target_path,
                                 const InputFn& input_fn,
                                 const OutputFn& output_fn,
                                 const OutputFn& error_fn) {
        std::string rename_base = target_path;
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
                output_fn, input_fn, header, options_line, choice_prompt, invalid_choice_line
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

            while (true) {
                std::string default_candidate = generate_sequential_candidate(rename_base, suffix_counter);
                std::string candidate = target_conflict::prompt_new_path(
                    output_fn, input_fn,
                    i18n::get("target_exists_rename_prompt", {{"DEFAULT", default_candidate}}),
                    default_candidate
                );
                candidate = util::trim_copy(candidate);

                if (candidate.empty()) {
                    candidate = default_candidate;
                }

                if (candidate == target_path) {
                    error_fn(i18n::get("target_exists_same") + "\n");
                    if (candidate == default_candidate) ++suffix_counter;
                    continue;
                }

                if (fs::exists(candidate)) {
                    error_fn(i18n::get("target_exists_rename_conflict", {{"TARGET_PATH", candidate}}) + "\n");
                    if (candidate == default_candidate) {
                        ++suffix_counter;
                    } else {
                        rename_base = candidate;
                        suffix_counter = 1;
                    }
                    continue;
                }

                target_path = candidate;
                if (candidate == default_candidate) ++suffix_counter;
                else {
                    rename_base = candidate;
                    suffix_counter = 1;
                }
                break;
            }
        }
        return true;
    }
}
