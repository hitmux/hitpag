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

namespace fs = std::filesystem;

namespace {
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

    ScopedTestDir tmp_root("/opt/hitpag/tmp/tui_smoke_test");
    if (!tmp_root.valid()) {
        std::cerr << "FAIL: unable to create test directory: " << tmp_root.error() << std::endl;
        return 1;
    }

    fs::path single_file = tmp_root.path() / "single.txt";
    ok &= expect(write_text_file(single_file, "hello from single-file archive\n"), "should create single-file input");

    ok &= test_tar_text_extraction(tmp_root.path());
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
