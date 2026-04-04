// Copyright (C) 2025 Hitmux
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#pragma once

#include <string>
#include <map>
#include <stdexcept>

namespace error {
    enum class ErrorCode {
        SUCCESS = 0,
        MISSING_ARGS = 1,
        INVALID_SOURCE = 2,
        INVALID_TARGET = 3,
        SAME_PATH = 4,
        UNKNOWN_FORMAT = 5,
        TOOL_NOT_FOUND = 6,
        OPERATION_FAILED = 7,
        PERMISSION_DENIED = 8,
        NOT_ENOUGH_SPACE = 9,
        UNKNOWN_ERROR = 99
    };

    class HitpagException : public std::runtime_error {
    private:
        ErrorCode code_;
    public:
        HitpagException(ErrorCode code, const std::string& message)
            : std::runtime_error(message), code_(code) {}
        ErrorCode code() const { return code_; }
    };

    void throw_error(ErrorCode code, const std::map<std::string, std::string>& placeholders = {});
}
