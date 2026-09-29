// Copyright (C) 2025 Hitmux
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#include "include/tui_image.h"

#include <algorithm>
#include <cmath>
#include <limits>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#define STBI_NO_LINEAR
#define STBI_NO_HDR
#include "stb_image.h"

namespace tui::image {
    Image decode(const std::string& bytes) {
        Image image;
        if (bytes.empty() ||
            bytes.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
            return image;
        }

        // stb_image materialises the whole bitmap, so refuse absurd canvases before decoding
        // instead of letting a decompression bomb exhaust memory. The probe is advisory: when
        // it cannot read the header the regular load below still gets a chance.
        constexpr unsigned long long MAX_DECODE_PIXELS = 40ull * 1000ull * 1000ull;

        const unsigned char* payload = reinterpret_cast<const unsigned char*>(bytes.data());
        const int payload_size = static_cast<int>(bytes.size());

        int probe_width = 0;
        int probe_height = 0;
        if (stbi_info_from_memory(payload, payload_size, &probe_width, &probe_height, nullptr) != 0 &&
            probe_width > 0 && probe_height > 0 &&
            static_cast<unsigned long long>(probe_width) * static_cast<unsigned long long>(probe_height) > MAX_DECODE_PIXELS) {
            return image;
        }

        int width = 0;
        int height = 0;
        unsigned char* decoded = stbi_load_from_memory(payload, payload_size, &width, &height, nullptr, 4);

        if (decoded == nullptr) {
            return image;
        }
        if (width <= 0 || height <= 0) {
            stbi_image_free(decoded);
            return image;
        }

        image.width = width;
        image.height = height;
        image.pixels.resize(static_cast<std::size_t>(width) * static_cast<std::size_t>(height));

        for (std::size_t i = 0; i < image.pixels.size(); ++i) {
            const unsigned char* source = decoded + i * 4;
            const unsigned int alpha = source[3];
            if (alpha == 255) {
                image.pixels[i] = Rgb{source[0], source[1], source[2]};
                continue;
            }
            // Composite translucent pixels over black so cut-out artwork does not show
            // whatever colour happens to sit under the transparent areas.
            image.pixels[i] = Rgb{
                static_cast<std::uint8_t>(source[0] * alpha / 255),
                static_cast<std::uint8_t>(source[1] * alpha / 255),
                static_cast<std::uint8_t>(source[2] * alpha / 255),
            };
        }

        stbi_image_free(decoded);
        return image;
    }

    Image fit_to_cells(const Image& source, int max_columns, int max_rows) {
        if (!source.valid() || max_columns < 1 || max_rows < 1) {
            return Image{};
        }

        const int budget_width = max_columns;
        const int budget_height = max_rows * 2;

        const double scale = std::min(static_cast<double>(budget_width) / source.width,
                                      static_cast<double>(budget_height) / source.height);

        int target_width = static_cast<int>(std::lround(source.width * scale));
        // The height has to be even because every character cell carries two pixels.
        int target_height = static_cast<int>(std::lround(source.height * scale));
        target_height -= target_height % 2;

        target_width = std::clamp(target_width, 1, budget_width);
        target_height = std::clamp(target_height, 2, budget_height);

        Image result;
        result.width = target_width;
        result.height = target_height;
        result.pixels.assign(static_cast<std::size_t>(target_width) * static_cast<std::size_t>(target_height),
                             Rgb{});

        for (int y = 0; y < target_height; ++y) {
            const int source_y0 = static_cast<int>(static_cast<long long>(y) * source.height / target_height);
            const int source_y1 = std::max(source_y0 + 1,
                                           static_cast<int>(static_cast<long long>(y + 1) * source.height / target_height));

            for (int x = 0; x < target_width; ++x) {
                const int source_x0 = static_cast<int>(static_cast<long long>(x) * source.width / target_width);
                const int source_x1 = std::max(source_x0 + 1,
                                               static_cast<int>(static_cast<long long>(x + 1) * source.width / target_width));

                unsigned long long red = 0;
                unsigned long long green = 0;
                unsigned long long blue = 0;
                unsigned int count = 0;

                for (int sy = source_y0; sy < source_y1 && sy < source.height; ++sy) {
                    for (int sx = source_x0; sx < source_x1 && sx < source.width; ++sx) {
                        const Rgb& pixel = source.at(sx, sy);
                        red += pixel.r;
                        green += pixel.g;
                        blue += pixel.b;
                        ++count;
                    }
                }

                if (count == 0) {
                    continue;
                }

                result.pixels[static_cast<std::size_t>(y) * target_width + x] = Rgb{
                    static_cast<std::uint8_t>(red / count),
                    static_cast<std::uint8_t>(green / count),
                    static_cast<std::uint8_t>(blue / count),
                };
            }
        }

        return result;
    }
}
