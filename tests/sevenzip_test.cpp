#include "include/file_type.h"
#include "include/sevenzip.h"
#include "include/tui_archive_ops.h"
#include "include/process.h"
#include "include/error.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <chrono>
#include <stdexcept>
#include <cstdlib>
#include <iterator>
#include <sstream>

namespace fs = std::filesystem;
using file_type::FileType;
namespace ops = tui::archive_ops;

static void require(bool ok, const std::string& message) {
    if (!ok) throw std::runtime_error(message);
}
static void write(const fs::path& path, const std::string& data) {
    std::ofstream file(path, std::ios::binary);
    file.write(data.data(), data.size());
    require(file.good(), "write " + path.string());
}
static std::string read(const fs::path& path) {
    std::ifstream file(path, std::ios::binary);
    require(file.good(), "read " + path.string());
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}
static void run(const std::vector<std::string>& cmd) {
    auto result = process::run_command_capture(cmd);
    require(result.exit_code == 0, cmd.front() + " failed: " + result.stdout_output);
}
struct TempDir {
    fs::path path = fs::current_path() / ("sevenzip_test_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    TempDir() { fs::create_directories(path); }
    ~TempDir() { std::error_code ec; fs::remove_all(path, ec); }
};

#ifndef _WIN32
struct ScopedPath {
    std::string original = std::getenv("PATH") ? std::getenv("PATH") : "";
    explicit ScopedPath(const std::string& path) { setenv("PATH", path.c_str(), 1); }
    ~ScopedPath() { setenv("PATH", original.c_str(), 1); }
};
#endif

static void check_archive(const fs::path& path, const std::string& payload, const std::string& cli) {
    require(file_type::recognize_source_type(path.string()) == FileType::ARCHIVE_P7ZIP, "recognize " + path.string());
    auto entries = ops::list_archive(path.string(), FileType::ARCHIVE_P7ZIP);
    auto member = entries.end();
    for (auto it = entries.begin(); it != entries.end(); ++it) if (!it->is_directory) { member = it; break; }
    require(member != entries.end(), "list member " + path.string());
    auto text = ops::extract_text(path.string(), member->path, FileType::ARCHIVE_P7ZIP);
    require(text.success && text.content == payload, "preview " + path.string());
    require(ops::extract_to_string(path.string(), member->path, FileType::ARCHIVE_P7ZIP) == payload, "binary preview " + path.string());
    fs::path selected = path.string() + "_selected";
    require(ops::extract_single(path.string(), member->path, selected.string(), FileType::ARCHIVE_P7ZIP), "selected extraction " + path.string());
    require(read(selected / member->path) == payload, "selected contents " + path.string());
    fs::path all = path.string() + "_all";
    run({cli, path.string(), all.string()});
    bool found = false;
    for (const auto& entry : fs::recursive_directory_iterator(all)) {
        if (entry.is_regular_file() && read(entry.path()) == payload) found = true;
    }
    require(found, "CLI extracted contents " + path.string());
}

static void append_le(std::string& data, unsigned value, int bytes) {
    for (int i = 0; i < bytes; ++i) { data += static_cast<char>(value & 255); value >>= 8; }
}
static void make_cab(const fs::path& path, const std::string& payload) {
    const std::string name = "payload.txt";
    unsigned data_offset = 44 + 16 + name.size() + 1;
    std::string data = "MSCF";
    append_le(data, 0, 4); append_le(data, data_offset + 8 + payload.size(), 4);
    append_le(data, 0, 4); append_le(data, 44, 4); append_le(data, 0, 4);
    append_le(data, 3, 1); append_le(data, 1, 1);
    append_le(data, 1, 2); append_le(data, 1, 2);
    append_le(data, 0, 2); append_le(data, 0, 2); append_le(data, 0, 2);
    append_le(data, data_offset, 4); append_le(data, 1, 2); append_le(data, 0, 2);
    append_le(data, payload.size(), 4); append_le(data, 0, 4);
    append_le(data, 0, 2); append_le(data, 0, 2); append_le(data, 0, 2); append_le(data, 32, 2);
    data += name; data += '\0';
    append_le(data, 0, 4); append_le(data, payload.size(), 2); append_le(data, payload.size(), 2);
    write(path, data + payload);
}

int main(int argc, char** argv) {
    try {
        require(argc == 2, "expected hitpag executable");
        const std::string cli = fs::absolute(argv[1]).string();
        // Original extension dispatch must not change.
        for (auto pair : {std::pair<const char*, FileType>{"a.zip", FileType::ARCHIVE_ZIP},
             {"a.7z", FileType::ARCHIVE_7Z}, {"a.rar", FileType::ARCHIVE_RAR},
             {"a.tar", FileType::ARCHIVE_TAR}, {"a.tgz", FileType::ARCHIVE_TAR_GZ},
             {"a.tbz2", FileType::ARCHIVE_TAR_BZ2}, {"a.tar.xz", FileType::ARCHIVE_TAR_XZ},
             {"a.tar.zst", FileType::ARCHIVE_TAR_ZSTD}, {"a.lz4", FileType::ARCHIVE_LZ4},
             {"a.zst", FileType::ARCHIVE_ZSTD}, {"a.xar", FileType::ARCHIVE_XAR}}) {
            require(file_type::recognize_by_extension(pair.first) == pair.second, pair.first);
        }
        TempDir temp;
        const std::string payload = "format round trip\nUnicode: 中文\n";
        fs::path source = temp.path / "-payload with spaces.txt";
        write(source, payload);
        fs::path text = temp.path / "long.txt";
        write(text, std::string(1024, 'a'));
        require(file_type::recognize_by_header(text.string()) == FileType::UNKNOWN, "plain text is not TAR");
        const std::string tool = sevenzip::executable();
        if (tool.empty()) { std::cout << "SKIP: 7z/7zz/7za not installed\n"; return 77; }
        for (const std::string ext : {"gz", "bz2", "xz", "wim", "jar", "ova"}) {
            fs::path archive = temp.path / ("archive with spaces." + ext);
            run({cli, source.string(), archive.string()});
            check_archive(archive, payload, cli);
            std::cout << "PASS: " << ext << '\n';
        }
        // Optional codecs depend on the p7zip build.
        for (const std::string ext : {"liz", "lz5"}) {
            fs::path archive = temp.path / ("optional." + ext);
            auto supported = process::run_command_capture({tool, "a", "-t" + (ext == "liz" ? std::string("lizard") : ext),
                (temp.path / ("reference." + ext)).string(), source.string()});
            if (supported.exit_code == 0) {
                run({cli, source.string(), archive.string()});
                check_archive(archive, payload, cli);
                std::cout << "PASS: " << ext << '\n';
            } else std::cout << "SKIP optional codec: " << ext << '\n';
        }
        fs::path cab = temp.path / "sample.CAB";
        make_cab(cab, payload);
        check_archive(cab, payload, cli);
        fs::path renamed = temp.path / "unknown extension.data";
        fs::copy_file(cab, renamed);
        check_archive(renamed, payload, cli);
        fs::path renamed_stream = temp.path / "renamed stream";
        fs::copy_file(temp.path / "archive with spaces.bz2", renamed_stream);
        check_archive(renamed_stream, payload, cli);
        // A synthetic SFX layout tests embedded ZIP detection without executing a program.
        fs::path exe = temp.path / "installer.EXE";
        write(exe, "MZ" + std::string(126, '\0') + read(temp.path / "archive with spaces.jar"));
        check_archive(exe, payload, cli);
        fs::path swf = temp.path / "movie.swf";
        std::string swf_data = "FWS"; swf_data += '\x09'; append_le(swf_data, 16, 4);
        swf_data.append("\x08\x00\x00\x0c\x01\x00\x00\x00", 8);
        write(swf, swf_data);
        fs::path compressed_swf = temp.path / "compressed.swf";
        run({cli, swf.string(), compressed_swf.string()});
        check_archive(compressed_swf, swf_data, cli);
        fs::path encrypted = temp.path / "encrypted.jar";
        run({cli, "--verify", "-psecret", source.string(), encrypted.string()});
        auto members = ops::list_archive(encrypted.string(), FileType::ARCHIVE_P7ZIP, "secret");
        require(!members.empty(), "encrypted listing");
        auto decoded = ops::extract_text(encrypted.string(), members.front().path, FileType::ARCHIVE_P7ZIP, "secret");
        require(decoded.success && decoded.content == payload, "encrypted preview");
        require(!ops::extract_text(encrypted.string(), members.front().path, FileType::ARCHIVE_P7ZIP, "wrong").success, "wrong password fails");
        fs::path corrupt = temp.path / "corrupt.cab";
        write(corrupt, "not an archive");
        bool failed = false;
        try { (void)ops::list_archive(corrupt.string(), FileType::ARCHIVE_P7ZIP); }
        catch (const error::HitpagException&) { failed = true; }
        require(failed, "corrupt archive reports a listing failure");
        for (const std::string ext : {"cab", "iso", "exe", "lzma", "swm"}) {
            fs::path output = temp.path / ("unsupported." + ext);
            require(process::run_command_capture({cli, source.string(), output.string()}).exit_code != 0, "reject read-only creation " + ext);
            require(!fs::exists(output), "read-only creation leaves no archive");
        }
        require(process::run_command_capture({cli, source.string(), text.string(), (temp.path / "multi.gz").string()}).exit_code != 0, "reject multi-file gzip");
        require(process::run_command_capture({cli, temp.path.string(), (temp.path / "directory.gz").string()}).exit_code != 0, "reject directory gzip");
#ifndef _WIN32
        fs::path wildcard = temp.path / "literal*.txt";
        write(wildcard, payload);
        write(temp.path / "literal-other.txt", "must not match the literal wildcard");
        fs::path wildcard_archive = temp.path / "literal.jar";
        run({cli, wildcard.string(), wildcard_archive.string()});
        require(ops::list_archive(wildcard_archive.string(), FileType::ARCHIVE_P7ZIP).size() == 1, "literal compression filenames");
        check_archive(wildcard_archive, payload, cli);
        fs::path padded = temp.path / "padded.txt ";
        write(padded, payload);
        fs::path padded_archive = temp.path / "padded.jar";
        run({cli, padded.string(), padded_archive.string()});
        check_archive(padded_archive, payload, cli);
        fs::path backend_path;
        std::istringstream paths(std::getenv("PATH") ? std::getenv("PATH") : "");
        std::string directory;
        while (std::getline(paths, directory, ':')) {
            if (fs::exists(fs::path(directory) / tool)) { backend_path = fs::absolute(fs::path(directory) / tool); break; }
        }
        fs::path fallback_dir = temp.path / "fallback-bin";
        fs::create_directory(fallback_dir);
        fs::create_symlink(backend_path, fallback_dir / "7zz");
        {
            ScopedPath path(fallback_dir.string());
            require(sevenzip::executable() == "7zz", "7zz fallback discovery");
            require(!ops::list_archive(cab.string(), FileType::ARCHIVE_P7ZIP).empty(), "7zz fallback listing");
        }
        {
            ScopedPath path("");
            require(file_type::recognize_source_type(text.string()) == FileType::REGULAR_FILE, "ordinary files without backend");
            bool missing = false;
            try { (void)ops::list_archive(cab.string(), FileType::ARCHIVE_P7ZIP); }
            catch (const error::HitpagException& e) { missing = e.code() == error::ErrorCode::TOOL_NOT_FOUND; }
            require(missing, "missing backend produces a tool error");
        }
#endif
        std::cout << "sevenzip_test passed\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "FAIL: " << e.what() << '\n';
        return 1;
    }
}
