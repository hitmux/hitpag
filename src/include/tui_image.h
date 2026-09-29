// Copyright (C) 2025 Hitmux
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace tui::image {
    struct Rgb {
        std::uint8_t r = 0;
        std::uint8_t g = 0;
        std::uint8_t b = 0;
    };

    struct Image {
        int width = 0;
        int height = 0;
        std::vector<Rgb> pixels;

        bool valid() const {
            return width > 0 && height > 0 &&
                   pixels.size() == static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
        }

        const Rgb& at(int x, int y) const {
            return pixels[static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x)];
        }
    };

    // Decodes encoded image bytes (PNG, JPEG, GIF, BMP, ...) into plain RGB rows.
    // Returns an invalid Image when the payload is empty or cannot be decoded.
    Image decode(const std::string& bytes);

    // Rescales `source` so it fits a grid of `max_columns` x `max_rows` character cells.
    // A cell holds two vertically stacked pixels, so the pixel budget is
    // max_columns x (max_rows * 2). The aspect ratio is preserved, the resulting height is
    // always even, and the result never exceeds the requested cell budget.
    Image fit_to_cells(const Image& source, int max_columns, int max_rows);
}
