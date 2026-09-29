// Copyright (C) 2025 Hitmux
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#include <array>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <ftxui/dom/node.hpp>
#include <ftxui/screen/screen.hpp>
#include <ftxui/screen/terminal.hpp>

#include "include/file_type.h"
#include "include/tui_preview.h"

namespace fs = std::filesystem;

namespace {
    // Same rule as tui_smoke_test: keep scratch data in the operating system's temp directory,
    // which is the only location that is writable on Termux, Linux and Windows alike.
    fs::path test_work_dir() {
        std::error_code ec;
        const fs::path temp = fs::temp_directory_path(ec);
        if (!ec) {
            const fs::path root = temp / "hitpag-tests";
            fs::create_directories(root, ec);
            if (!ec) return root;
        }
        return fs::current_path();
    }

    bool expect(bool condition, const std::string& message) {
        if (!condition) {
            std::cerr << "FAIL: " << message << std::endl;
            return false;
        }
        return true;
    }

    std::string make_bmp24(int width, int height, const std::vector<std::array<std::uint8_t, 3>>& pixels) {
        const int row_stride = ((width * 3 + 3) / 4) * 4;
        const int pixel_bytes = row_stride * height;
        const int file_size = 54 + pixel_bytes;

        auto push_u16 = [](std::string& out, std::uint16_t value) {
            out.push_back(static_cast<char>(value & 0xFF));
            out.push_back(static_cast<char>((value >> 8) & 0xFF));
        };
        auto push_u32 = [](std::string& out, std::uint32_t value) {
            for (int i = 0; i < 4; ++i) {
                out.push_back(static_cast<char>((value >> (8 * i)) & 0xFF));
            }
        };

        std::string bmp;
        bmp.reserve(static_cast<std::size_t>(file_size));

        bmp += "BM";
        push_u32(bmp, static_cast<std::uint32_t>(file_size));
        push_u32(bmp, 0);
        push_u32(bmp, 54);

        push_u32(bmp, 40);
        push_u32(bmp, static_cast<std::uint32_t>(width));
        push_u32(bmp, static_cast<std::uint32_t>(height));
        push_u16(bmp, 1);
        push_u16(bmp, 24);
        push_u32(bmp, 0);
        push_u32(bmp, static_cast<std::uint32_t>(pixel_bytes));
        push_u32(bmp, 2835);
        push_u32(bmp, 2835);
        push_u32(bmp, 0);
        push_u32(bmp, 0);

        for (int y = height - 1; y >= 0; --y) {
            int written = 0;
            for (int x = 0; x < width; ++x) {
                const std::array<std::uint8_t, 3>& rgb = pixels[static_cast<std::size_t>(y) * width + x];
                bmp.push_back(static_cast<char>(rgb[2]));
                bmp.push_back(static_cast<char>(rgb[1]));
                bmp.push_back(static_cast<char>(rgb[0]));
                written += 3;
            }
            while (written < row_stride) {
                bmp.push_back('\0');
                ++written;
            }
        }

        return bmp;
    }

    // Renders the panel twice inside a `width` x `height` screen. The first pass records the
    // panel box through reflect(); the second one sizes the image blocks against that box.
    // The requirement of the second pass is what the panel really demands from the terminal,
    // so asserting it stays inside the screen proves the preview cannot wrap.
    ftxui::Requirement measure(tui::PreviewPanel& panel, int width, int height) {
        ftxui::Requirement requirement;
        for (int pass = 0; pass < 2; ++pass) {
            const ftxui::Element element = panel.render();
            ftxui::Screen screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(width),
                                                         ftxui::Dimension::Fixed(height));
            ftxui::Render(screen, element);
            requirement = element->requirement();
        }
        return requirement;
    }
}

int main() {
    bool ok = true;

    ftxui::Terminal::SetFallbackSize(ftxui::Dimensions{120, 40});

    fs::path work_dir = test_work_dir() / "tui_preview_layout_test_data";
    std::error_code ec;
    fs::remove_all(work_dir, ec);
    fs::create_directories(work_dir, ec);
    if (ec) {
        std::cerr << "FAIL: unable to create test directory: " << ec.message() << std::endl;
        return 1;
    }

    const std::vector<std::array<std::uint8_t, 3>> pixels = {
        {255, 0, 0}, {0, 255, 0}, {0, 0, 255},
        {255, 255, 0}, {0, 255, 255}, {255, 0, 255},
    };
    const std::string bmp = make_bmp24(3, 2, pixels);

    {
        std::ofstream output(work_dir / "wide.bmp", std::ios::binary | std::ios::trunc);
        output.write(bmp.data(), static_cast<std::streamsize>(bmp.size()));
    }

    const fs::path archive_path = work_dir / "images.tar";
    const int tar_status = std::system(
        (std::string("tar -cf ") + archive_path.string() + " -C " + work_dir.string() + " wide.bmp").c_str());
    ok &= expect(tar_status == 0, "tar should build the layout test archive");

    tui::PreviewPanel panel;
    panel.load(archive_path.string(), "wide.bmp", file_type::FileType::ARCHIVE_TAR, "");
    ok &= expect(panel.is_image_view(), "a BMP entry should switch the preview panel into image mode");
    ok &= expect(panel.has_content(), "an image preview should count as preview content");

    struct SizeCase {
        int width;
        int height;
    };

    const std::vector<SizeCase> sizes = {
        {120, 40},
        {60, 20},
        {40, 12},
        {24, 8},
        {12, 6},
    };

    for (const SizeCase& size : sizes) {
        const ftxui::Requirement requirement = measure(panel, size.width, size.height);
        const std::string label = std::to_string(size.width) + "x" + std::to_string(size.height);
        ok &= expect(requirement.min_x <= size.width,
                     "image preview should fit the panel width at " + label +
                         " (requested " + std::to_string(requirement.min_x) + ")");
        ok &= expect(requirement.min_y <= size.height,
                     "image preview should fit the panel height at " + label +
                         " (requested " + std::to_string(requirement.min_y) + ")");
    }

    fs::remove_all(work_dir, ec);

    if (!ok) {
        return 1;
    }

    std::cout << "tui_preview_layout_test passed" << std::endl;
    return 0;
}
