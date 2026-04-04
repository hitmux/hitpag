// Copyright (C) 2025 Hitmux
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#pragma once

#include <string>
#include <functional>

namespace target_path {
    using InputFn = std::function<std::string()>;
    using OutputFn = std::function<void(const std::string&)>;

    bool resolve_existing_target(std::string& target_path,
                                 const InputFn& input_fn,
                                 const OutputFn& output_fn,
                                 const OutputFn& error_fn);
}
