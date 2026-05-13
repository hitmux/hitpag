// Copyright (C) 2025 Hitmux
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#pragma once

#include <string>
#include "include/tui_archive_ops.h"
#include <ftxui/dom/elements.hpp>

namespace tui {
    class DetailsPanel {
    public:
        void set_entry(const archive_ops::ArchiveEntry* entry);
        ftxui::Element render() const;

    private:
        const archive_ops::ArchiveEntry* current_entry_ = nullptr;
        std::string format_size(uint64_t size) const;
    };
}
