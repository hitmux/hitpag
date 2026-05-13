// Copyright (C) 2025 Hitmux
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#pragma once

#include <string>
#include "include/file_type.h"

namespace tui {
    class ExtractDialog {
    public:
        enum class Result {
            Success,
            Failure,
            Canceled,
        };

        bool show(const std::string& archive_path, const std::string& entry_path, file_type::FileType type, const std::string& password = "");
        const std::string& last_message() const { return message_; }
        bool last_success() const { return success_; }
        Result last_result() const { return result_; }

    private:
        std::string message_;
        bool success_ = false;
        Result result_ = Result::Canceled;
    };
}
