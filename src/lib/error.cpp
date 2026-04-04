// Copyright (C) 2025 Hitmux
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#include "include/error.h"
#include "include/i18n.h"

namespace error {
    void throw_error(ErrorCode code, const std::map<std::string, std::string>& placeholders) {
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
