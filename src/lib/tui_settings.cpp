// Copyright (C) 2025 Hitmux
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#include "include/tui_settings.h"
#include "include/i18n.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

namespace tui::settings {
    namespace {
        fs::path config_directory() {
#ifdef _WIN32
            if (const char* appdata = std::getenv("APPDATA")) {
                return fs::path(appdata) / "hitpag";
            }
            if (const char* userprofile = std::getenv("USERPROFILE")) {
                return fs::path(userprofile) / ".hitpag";
            }
#else
            if (const char* xdg = std::getenv("XDG_CONFIG_HOME")) {
                return fs::path(xdg) / "hitpag";
            }
            if (const char* home = std::getenv("HOME")) {
                return fs::path(home) / ".config" / "hitpag";
            }
#endif
            return fs::current_path() / ".hitpag";
        }
    }

    fs::path config_path() {
        return config_directory() / "tui_settings.conf";
    }

    TuiSettings load() {
        TuiSettings settings;
        std::ifstream input(config_path());
        if (!input) {
            return settings;
        }

        std::string line;
        while (std::getline(input, line)) {
            if (line.rfind("editor_command=", 0) == 0) {
                settings.editor_command = line.substr(std::string("editor_command=").size());
            }
        }

        return settings;
    }

    bool save(const TuiSettings& settings, std::string* error_message) {
        std::error_code ec;
        fs::create_directories(config_directory(), ec);
        if (ec) {
            if (error_message) {
                *error_message = i18n::get("tui_settings_create_dir_failed", {{"REASON", ec.message()}});
            }
            return false;
        }

        std::ofstream output(config_path(), std::ios::trunc);
        if (!output) {
            if (error_message) {
                *error_message = i18n::get("tui_settings_open_failed");
            }
            return false;
        }

        output << "editor_command=" << settings.editor_command << '\n';
        return true;
    }
}
