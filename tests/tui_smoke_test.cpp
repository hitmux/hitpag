#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "include/args.h"
#include "include/error.h"
#include "include/i18n.h"
#include "include/operation.h"
#include "include/tui_archive_ops.h"
#include "include/tui_image.h"

namespace fs = std::filesystem;

namespace {
    // Tests have to write somewhere that exists on every platform. A hardcoded absolute path
    // only ever works on the machine the suite was written on, and on Termux neither /tmp nor
    // /opt exists, so the operating system's temp directory (TMPDIR, %TEMP%, /tmp) is the
    // reliable choice. The current directory is the fallback when the environment cannot
    // provide one.
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

    class ScopedTestDir {
    public:
        explicit ScopedTestDir(fs::path path) : path_(std::move(path)) {
            std::error_code ec;
            fs::remove_all(path_, ec);
            fs::create_directories(path_, ec);
            if (ec) {
                valid_ = false;
                error_ = ec.message();
            }
        }

        ~ScopedTestDir() {
            std::error_code ec;
            fs::remove_all(path_, ec);
        }

        const fs::path& path() const {
            return path_;
        }

        bool valid() const {
            return valid_;
        }

        const std::string& error() const {
            return error_;
        }

    private:
        fs::path path_;
        bool valid_ = true;
        std::string error_;
    };

    bool write_text_file(const fs::path& path, const std::string& content) {
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        if (!output) {
            return false;
        }
        output << content;
        return output.good();
    }

    bool expect(bool condition, const std::string& message) {
        if (!condition) {
            std::cerr << "FAIL: " << message << std::endl;
            return false;
        }
        return true;
    }

    bool expect_equal(const std::string& actual, const std::string& expected, const std::string& message) {
        if (actual != expected) {
            std::cerr << "FAIL: " << message << " (actual='" << actual << "', expected='" << expected << "')" << std::endl;
            return false;
        }
        return true;
    }

    args::Options parse_args(std::vector<std::string> values) {
        std::vector<char*> argv;
        argv.reserve(values.size());
        for (std::string& value : values) {
            argv.push_back(value.data());
        }
        return args::parse(static_cast<int>(argv.size()), argv.data());
    }

    bool test_tui_args() {
        bool ok = true;

        args::Options tui_options = parse_args({"hitpag", "--tui", "archive.zip"});
        ok &= expect(tui_options.tui_mode, "--tui should enable tui_mode");
        ok &= expect_equal(tui_options.source_path, "archive.zip", "--tui should keep the archive path in source_path");
        ok &= expect(tui_options.target_path.empty(), "--tui should not set target_path");
        ok &= expect(tui_options.source_paths.empty(), "--tui should not keep source_paths");

        bool rejected_extra_positional = false;
        try {
            (void)parse_args({"hitpag", "--tui", "archive.zip", "extra"});
        } catch (const error::HitpagException& ex) {
            rejected_extra_positional = ex.code() == error::ErrorCode::MISSING_ARGS &&
                                        std::string(ex.what()).find("--tui accepts at most one positional argument") != std::string::npos;
        }
        ok &= expect(rejected_extra_positional, "--tui should reject more than one positional argument");

        args::Options normal_options = parse_args({"hitpag", "input.txt", "out.zip"});
        ok &= expect(!normal_options.tui_mode, "normal parse should not enable tui_mode");
        ok &= expect_equal(normal_options.source_path, "input.txt", "normal parse should keep first source_path");
        ok &= expect_equal(normal_options.target_path, "out.zip", "normal parse should keep target_path");

        return ok;
    }

    bool test_tui_i18n_keys() {
        bool ok = true;
        const std::vector<std::string> keys = {
            "tui_settings_custom_command",
            "tui_settings_custom_command_not_set",
            "tui_settings_builtin_template_ready",
            "tui_settings_custom_command_empty",
            "tui_settings_custom_command_missing_file",
            "tui_settings_custom_command_ready",
            "tui_settings_custom_command_placeholder",
            "tui_settings_configure_editor",
            "tui_settings_editor_settings",
            "tui_settings_first_time_hint",
            "tui_settings_update_hint",
            "tui_settings_current_label",
            "tui_settings_selected_label",
            "tui_settings_rule_label",
            "tui_settings_check_label",
            "tui_settings_not_configured",
            "tui_settings_rule_text",
            "tui_settings_command_templates",
            "tui_settings_custom_command_help",
            "tui_settings_footer_hint",
            "tui_settings_create_dir_failed",
            "tui_settings_open_failed",
            "tui_editor_write_back_failed",
            "tui_image_label",
            "tui_image_too_large",
            "tui_preview_shortcut_hint",
            "tui_list_shortcut_hint",
            "tui_active_suffix",
            "tui_preview_directories_suffix",
            "tui_preview_files_suffix",
            "tui_preview_first_entries",
        };

        for (const std::string& key : keys) {
            ok &= expect(i18n::get(key) != "[" + key + "]", "TUI i18n key should exist: " + key);
        }
        ok &= expect_equal(
            i18n::get("tui_settings_saved_message", {{"COMMAND", "nano $file"}}),
            "Editor command saved: nano $file",
            "i18n placeholder replacement should work for settings save message");

        return ok;
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

        // BMP rows are stored bottom-up and in BGR order.
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

    bool test_image_preview_helpers() {
        bool ok = true;

        ok &= expect(tui::archive_ops::is_image_file("photo.PNG"), "image detection should be case-insensitive");
        ok &= expect(tui::archive_ops::is_image_file("nested/art.webp"), "image detection should work for nested paths");
        ok &= expect(!tui::archive_ops::is_image_file("notes.txt"), "text entries should not be treated as images");
        ok &= expect(!tui::archive_ops::is_image_file("archive.zip"), "archives should not be treated as images");

        ok &= expect(!tui::image::decode("").valid(), "empty payload should not decode");
        ok &= expect(!tui::image::decode("this is definitely not an image").valid(), "garbage payload should not decode");

        const std::vector<std::array<std::uint8_t, 3>> pixels = {
            {255, 0, 0}, {0, 255, 0},
            {0, 0, 255}, {255, 255, 255},
        };
        const tui::image::Image decoded = tui::image::decode(make_bmp24(2, 2, pixels));
        ok &= expect(decoded.valid(), "a well-formed BMP should decode");
        ok &= expect(decoded.width == 2 && decoded.height == 2, "decoded BMP should keep its dimensions");
        if (decoded.valid()) {
            ok &= expect(decoded.at(0, 0).r == 255 && decoded.at(0, 0).g == 0 && decoded.at(0, 0).b == 0,
                         "decoded BMP should preserve the top-left pixel");
            ok &= expect(decoded.at(1, 1).r == 255 && decoded.at(1, 1).g == 255 && decoded.at(1, 1).b == 255,
                         "decoded BMP should preserve the bottom-right pixel");
        }

        struct BudgetCase {
            int width;
            int height;
            int columns;
            int rows;
            bool fills_one_axis;
        };

        const std::vector<BudgetCase> cases = {
            {1920, 1080, 40, 12, true},
            {1080, 1920, 40, 12, true},
            {4, 4, 40, 12, true},
            {8000, 40, 30, 20, false},
            {40, 8000, 30, 20, false},
        };

        for (const BudgetCase& test_case : cases) {
            tui::image::Image source;
            source.width = test_case.width;
            source.height = test_case.height;
            source.pixels.assign(static_cast<std::size_t>(test_case.width) * test_case.height,
                                 tui::image::Rgb{10, 20, 30});

            const tui::image::Image fitted = tui::image::fit_to_cells(source, test_case.columns, test_case.rows);
            const std::string label = std::to_string(test_case.width) + "x" + std::to_string(test_case.height) +
                                      " into " + std::to_string(test_case.columns) + "x" + std::to_string(test_case.rows);

            ok &= expect(fitted.valid(), "fit_to_cells should produce a valid image: " + label);
            if (!fitted.valid()) {
                continue;
            }

            ok &= expect(fitted.width >= 1 && fitted.width <= test_case.columns,
                         "fitted width should stay within the cell budget: " + label);
            ok &= expect(fitted.height >= 2 && fitted.height <= test_case.rows * 2,
                         "fitted height should stay within the pixel budget: " + label);
            ok &= expect(fitted.height % 2 == 0, "fitted height should be even so pixels pair into cells: " + label);

            if (test_case.fills_one_axis) {
                ok &= expect(fitted.width == test_case.columns || fitted.height == test_case.rows * 2,
                             "fitted image should reach one edge of the budget: " + label);

                const double source_ratio = static_cast<double>(test_case.width) / test_case.height;
                const double fitted_ratio = static_cast<double>(fitted.width) / fitted.height;
                ok &= expect(std::fabs(source_ratio - fitted_ratio) <= source_ratio * 0.15 + 0.05,
                             "fitted image should keep its aspect ratio: " + label);
            }
        }

        ok &= expect(!tui::image::fit_to_cells(tui::image::Image{}, 40, 12).valid(),
                     "fit_to_cells should reject an invalid source");

        return ok;
    }

    bool test_image_archive_round_trip(const fs::path& tmp_root) {
        bool ok = true;

        const std::vector<std::array<std::uint8_t, 3>> pixels = {
            {255, 0, 0}, {0, 255, 0},
            {0, 0, 255}, {255, 255, 255},
        };
        const std::string bmp = make_bmp24(2, 2, pixels);

        fs::path bmp_path = tmp_root / "pixel.bmp";
        {
            std::ofstream output(bmp_path, std::ios::binary | std::ios::trunc);
            ok &= expect(static_cast<bool>(output), "should create the BMP test input");
            output.write(bmp.data(), static_cast<std::streamsize>(bmp.size()));
            ok &= expect(output.good(), "should write the BMP test input");
        }

        fs::path archive_path = tmp_root / "pixel.tar";
        int tar_status = std::system((std::string("tar -cf ") + archive_path.string() + " -C " + tmp_root.string() + " pixel.bmp").c_str());
        ok &= expect(tar_status == 0, "tar command should create the image test archive");

        const std::string extracted =
            tui::archive_ops::extract_to_string(archive_path.string(), "pixel.bmp", file_type::FileType::ARCHIVE_TAR, "");
        ok &= expect(extracted.size() == bmp.size(), "extraction should return the image bytes unchanged");

        const tui::image::Image decoded = tui::image::decode(extracted);
        ok &= expect(decoded.valid(), "an image extracted from an archive should still decode");
        if (decoded.valid()) {
            ok &= expect(decoded.width == 2 && decoded.height == 2, "archive round-trip should keep image dimensions");
        }

        return ok;
    }

    bool test_bounded_archive_extraction(const fs::path& tmp_root) {
        bool ok = true;
        const std::string payload(4096, 'x');
        fs::path payload_path = tmp_root / "bounded.bin";
        fs::path archive_path = tmp_root / "bounded.tar";

        ok &= expect(write_text_file(payload_path, payload), "should create bounded extraction input");
        int tar_status = std::system(
            (std::string("tar -cf ") + archive_path.string() + " -C " + tmp_root.string() + " bounded.bin").c_str());
        ok &= expect(tar_status == 0, "tar command should create bounded extraction archive");

        const std::string extracted = tui::archive_ops::extract_to_string(
            archive_path.string(), "bounded.bin", file_type::FileType::ARCHIVE_TAR, "", 1025);
        ok &= expect(extracted.size() == 1025, "bounded extraction should cap captured bytes");
        ok &= expect(extracted == std::string(1025, 'x'), "bounded extraction should preserve the captured prefix");
        return ok;
    }

    bool test_tar_text_extraction(const fs::path& tmp_root) {
        bool ok = true;
        fs::path empty_file = tmp_root / "empty.txt";
        fs::path archive_path = tmp_root / "empty.tar";

        ok &= expect(write_text_file(empty_file, ""), "should create empty input file");

        int tar_status = std::system((std::string("tar -cf ") + archive_path.string() + " -C " + tmp_root.string() + " empty.txt").c_str());
        ok &= expect(tar_status == 0, "tar command should create a test archive");

        std::vector<tui::archive_ops::ArchiveEntry> entries =
            tui::archive_ops::list_archive(archive_path.string(), file_type::FileType::ARCHIVE_TAR, "");
        ok &= expect(!entries.empty(), "list_archive should list tar entries");
        ok &= expect_equal(entries.front().path, "empty.txt", "list_archive should preserve tar entry path");

        tui::archive_ops::TextExtractionResult extraction =
            tui::archive_ops::extract_text(archive_path.string(), "empty.txt", file_type::FileType::ARCHIVE_TAR, "");

        ok &= expect(extraction.success, "extract_text should succeed for an existing tar entry");
        ok &= expect(extraction.empty_file, "extract_text should mark empty extracted content as empty_file");
        ok &= expect(extraction.content.empty(), "extract_text should return empty content for empty file");

        return ok;
    }

    bool test_audio_preview_guards(const fs::path& tmp_root) {
        bool ok = true;
        ok &= expect(tui::archive_ops::is_audio_file("track.MP3"), "audio detection should be case-insensitive");
        ok &= expect(!tui::archive_ops::is_audio_file("track.txt"), "text files should not be treated as audio");

        fs::path output_dir = tmp_root / "audio-preview-guard";
        std::error_code ec;
        fs::create_directories(output_dir, ec);
        ok &= expect(!ec, "should create audio preview guard directory");

        std::string extracted_path;
        ok &= expect(
            !tui::archive_ops::extract_preview_file(
                "missing.zip", "../outside.mp3", output_dir.string(), file_type::FileType::ARCHIVE_ZIP, "", extracted_path),
            "audio preview should reject an entry that escapes the output directory");
        ok &= expect(extracted_path.empty(), "rejected audio preview should not return a path");
        return ok;
    }

    bool test_single_file_archive(const fs::path& tmp_root,
                                  const std::string& tool,
                                  const std::string& command,
                                  const fs::path& archive_path,
                                  file_type::FileType type) {
        if (!operation::is_tool_available(tool)) {
            std::cout << "skip " << tool << " coverage: tool not available" << std::endl;
            return true;
        }

        bool ok = true;
        int status = std::system(command.c_str());
        ok &= expect(status == 0, tool + " command should create a test archive");

        std::vector<tui::archive_ops::ArchiveEntry> entries =
            tui::archive_ops::list_archive(archive_path.string(), type, "");
        ok &= expect(!entries.empty(), "list_archive should expose a synthetic entry for " + tool);

        tui::archive_ops::TextExtractionResult extraction =
            tui::archive_ops::extract_text(archive_path.string(), entries.empty() ? "" : entries.front().path, type, "");
        ok &= expect(extraction.success, "extract_text should succeed for " + tool);
        ok &= expect_equal(extraction.content, "hello from single-file archive\n", "extract_text should return decompressed " + tool + " content");

        fs::path output_dir = tmp_root / (tool + "-out");
        std::error_code ec;
        fs::create_directories(output_dir, ec);
        ok &= expect(!ec, "should create output directory for " + tool);
        ok &= expect(
            tui::archive_ops::extract_single(archive_path.string(), entries.empty() ? "" : entries.front().path, output_dir.string(), type, ""),
            "extract_single should succeed for " + tool);

        return ok;
    }
}

int main() {
    bool ok = true;

    ok &= test_tui_args();
    ok &= test_tui_i18n_keys();
    ok &= test_image_preview_helpers();

    ScopedTestDir tmp_root(test_work_dir() / "tui_smoke_test");
    if (!tmp_root.valid()) {
        std::cerr << "FAIL: unable to create test directory: " << tmp_root.error() << std::endl;
        return 1;
    }

    fs::path single_file = tmp_root.path() / "single.txt";
    ok &= expect(write_text_file(single_file, "hello from single-file archive\n"), "should create single-file input");

    ok &= test_tar_text_extraction(tmp_root.path());
    ok &= test_image_archive_round_trip(tmp_root.path());
    ok &= test_bounded_archive_extraction(tmp_root.path());
    ok &= test_audio_preview_guards(tmp_root.path());
    ok &= test_single_file_archive(
        tmp_root.path(),
        "lz4",
        "lz4 -z -f " + single_file.string() + " " + (tmp_root.path() / "single.txt.lz4").string(),
        tmp_root.path() / "single.txt.lz4",
        file_type::FileType::ARCHIVE_LZ4);
    ok &= test_single_file_archive(
        tmp_root.path(),
        "zstd",
        "zstd -q -f " + single_file.string() + " -o " + (tmp_root.path() / "single.txt.zst").string(),
        tmp_root.path() / "single.txt.zst",
        file_type::FileType::ARCHIVE_ZSTD);

    if (!ok) {
        return 1;
    }

    std::cout << "tui_smoke_test passed" << std::endl;
    return 0;
}
