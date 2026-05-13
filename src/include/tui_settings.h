// Copyright (C) 2025 Hitmux
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#pragma once

#include <filesystem>
#include <string>

namespace tui::settings {
    struct TuiSettings {
        std::string editor_command;

        bool has_editor_command() const { return !editor_command.empty(); }
    };

    TuiSettings load();
    bool save(const TuiSettings& settings, std::string* error_message = nullptr);
    std::filesystem::path config_path();
}
