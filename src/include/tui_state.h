// Copyright (C) 2025 Hitmux
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#pragma once

#include "include/tui_editor.h"

#include <string>

namespace tui {
    enum class UiMode {
        Browse,
        PreviewScroll,
        Search,
        ExtractDialog,
        EditorSettings,
        WriteBackConfirm,
        Busy,
        Alert,
        Help,
    };

    struct AlertState {
        std::string title;
        std::string message;
        bool is_error = false;
    };

    struct ExtractState {
        std::string entry_path;
        std::string output_directory = ".";
    };

    struct TuiState {
        UiMode mode = UiMode::Browse;
        UiMode return_mode = UiMode::Browse;
        bool quit = false;
        std::string status_message;
        AlertState alert;
        ExtractState extract;
        editor::EditSession pending_edit_session;
    };
}
