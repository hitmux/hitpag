// Copyright (C) 2025 Hitmux
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#include "include/sevenzip.h"
#include "include/operation.h"
#include "include/process.h"
#include "include/error.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <map>
#include <mutex>
#include <set>
#include <sstream>

namespace sevenzip {
    namespace {
        const std::map<std::string, std::string> extensions = {
            {"apfs", "APFS"}, {"apm", "APM"}, {"ar", "Ar"}, {"a", "Ar"}, {"deb", "Ar"}, {"lib", "Ar"},
            {"arj", "Arj"}, {"bz2", "bzip2"}, {"bzip2", "bzip2"}, {"cab", "Cab"},
            {"chm", "Chm"}, {"chi", "Chm"}, {"chq", "Chm"}, {"chw", "Chm"},
            {"hxs", "Hxs"}, {"hxi", "Hxs"}, {"hxr", "Hxs"}, {"hxq", "Hxs"}, {"hxw", "Hxs"}, {"lit", "Hxs"},
            {"msi", "Compound"}, {"msp", "Compound"}, {"doc", "Compound"}, {"xls", "Compound"}, {"ppt", "Compound"},
            {"cpio", "Cpio"}, {"cramfs", "CramFS"}, {"dmg", "Dmg"}, {"elf", "ELF"},
            {"ext", "Ext"}, {"ext2", "Ext"}, {"ext3", "Ext"}, {"ext4", "Ext"}, {"img", "auto"},
            {"fat", "FAT"}, {"flv", "FLV"}, {"gz", "gzip"}, {"gzip", "gzip"},
            {"gpt", "GPT"}, {"mbr", "MBR"}, {"hfs", "HFS"}, {"hfsx", "HFS"},
            {"ihex", "IHex"}, {"iso", "Iso"}, {"lzh", "Lzh"}, {"lha", "Lzh"},
            {"lzma", "lzma"}, {"lzma86", "lzma86"}, {"macho", "MachO"}, {"mslz", "MsLZ"},
            {"mub", "Mub"}, {"nsis", "Nsis"}, {"ntfs", "NTFS"},
            {"exe", "PE"}, {"dll", "PE"}, {"sys", "PE"}, {"te", "TE"}, {"pmd", "Ppmd"},
            {"qcow", "QCOW"}, {"qcow2", "QCOW"}, {"qcow2c", "QCOW"},
            {"r00", "Rar"}, {"rpm", "Rpm"}, {"001", "Split"}, {"squashfs", "SquashFS"},
            {"swf", "SWFc"}, {"udf", "Udf"}, {"scap", "UEFIc"}, {"uefif", "UEFIf"},
            {"vdi", "VDI"}, {"vhd", "VHD"}, {"vhdx", "VHDX"}, {"vmdk", "VMDK"},
            {"wim", "wim"}, {"swm", "wim"}, {"esd", "wim"}, {"ppkg", "wim"},
            {"pkg", "Xar"}, {"xip", "Xar"}, {"xz", "xz"}, {"z", "Z"},
            {"zipx", "zip"}, {"jar", "zip"}, {"xpi", "zip"}, {"odt", "zip"}, {"ods", "zip"},
            {"docx", "zip"}, {"xlsx", "zip"}, {"pptx", "zip"}, {"epub", "zip"}, {"ipa", "zip"},
            {"apk", "zip"}, {"appx", "zip"}, {"msix", "zip"},
            {"liz", "lizard"}, {"lz", "lzip"}, {"lz5", "lz5"},
            {"ova", "tar"}, {"tpz", "gzip"}, {"taz", "Z"}, {"tliz", "lizard"},
            {"tlz", "lzip"}, {"tlz4", "lz4"}, {"tlz5", "lz5"}, {"tzstd", "zstd"}
        };

        struct BackendCacheEntry {
            std::string backend;
            std::string format;
        };
        std::map<std::string, BackendCacheEntry> backend_cache;
        std::mutex backend_cache_mutex;

        std::string lower(std::string value) {
            std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
                return static_cast<char>(std::tolower(c));
            });
            return value;
        }

        std::string cache_key(const std::string& path, const std::string& password) {
            if (path.empty()) return {};
            std::error_code ec;
            const std::string normalized = std::filesystem::absolute(path, ec).lexically_normal().string();
            return ec ? std::string{} : normalized + "\n" + password;
        }

        std::string parse_type(const std::string& output) {
            std::istringstream lines(output);
            std::string line;
            while (std::getline(lines, line)) {
                if (!line.empty() && line.back() == '\r') line.pop_back();
                if (line.rfind("Type = ", 0) == 0 && line.size() > 7) {
                    return line.substr(7);
                }
            }
            return {};
        }
    }

    std::string format_for_extension(const std::string& extension) {
        std::string ext = lower(extension);
        if (!ext.empty() && ext.front() == '.') ext.erase(0, 1);
        auto it = extensions.find(ext);
        return it == extensions.end() ? "" : it->second;
    }

    std::string normalize_format(const std::string& format) {
        const std::string value = lower(format);
        if (value == "p7zip" || value == "auto") return "auto";
        for (const auto& entry : extensions) {
            if (lower(entry.second) == value) return entry.second;
        }
        return format_for_extension(value);
    }

    std::string creation_format(const std::string& extension_or_format) {
        std::string name = lower(extension_or_format);
        if (!name.empty() && name.front() == '.') name.erase(0, 1);
        static const std::set<std::string> read_only_aliases = {
            "swm", "esd", "ppkg", "tpz", "taz", "tliz", "tlz", "tlz4", "tlz5", "tzstd"
        };
        if (read_only_aliases.count(name)) return "";
        return normalize_format(name);
    }

    std::vector<CreationFormat> creation_formats() {
        return {
            {"gzip stream (7-Zip)", "gzip", false},
            {"bzip2 stream (7-Zip)", "bzip2", false},
            {"XZ stream (7-Zip)", "xz", false},
            {"WIM archive (7-Zip)", "wim", false},
            {"ZIP container (JAR/Office aliases)", "zip", true},
            {"OVA TAR container (7-Zip)", "tar", false},
            {"Lizard stream (7-Zip)", "lizard", false},
            {"LZ5 stream (7-Zip)", "lz5", false},
            {"compressed SWF (7-Zip)", "SWFc", false}
        };
    }

    bool can_create(const std::string& format) {
        for (const auto& entry : creation_formats()) {
            if (entry.format == format) return true;
        }
        return format == "lz4" || format == "zstd";
    }

    bool single_file_format(const std::string& format) {
        return can_create(format) && format != "wim" && format != "zip" && format != "tar";
    }

    bool supports_password(const std::string& format) {
        for (const auto& entry : creation_formats()) {
            if (entry.format == format) return entry.supports_password;
        }
        return false;
    }

    std::vector<std::string> executables() {
        std::vector<std::string> result;
        for (const char* tool : {"7z", "7zz", "7za"}) {
            if (operation::is_tool_available(tool)) result.emplace_back(tool);
        }
        return result;
    }

    std::vector<std::string> backend_candidates(const std::string& path, const std::string& password) {
        std::vector<std::string> result;
        const std::string key = cache_key(path, password);
        if (!key.empty()) {
            std::lock_guard<std::mutex> lock(backend_cache_mutex);
            auto it = backend_cache.find(key);
            if (it != backend_cache.end() && !it->second.backend.empty() &&
                operation::is_tool_available(it->second.backend.c_str())) {
                result.push_back(it->second.backend);
            }
        }
        for (const auto& tool : executables()) {
            if (std::find(result.begin(), result.end(), tool) == result.end()) result.push_back(tool);
        }
        return result;
    }

    void remember_backend(const std::string& path, const std::string& password, const std::string& tool) {
        const std::string key = cache_key(path, password);
        if (key.empty() || tool.empty()) return;
        std::lock_guard<std::mutex> lock(backend_cache_mutex);
        backend_cache[key].backend = tool;
    }

    void remember_probe(const std::string& path, const std::string& password,
                        const std::string& tool, const std::string& format) {
        const std::string key = cache_key(path, password);
        if (key.empty()) return;
        std::lock_guard<std::mutex> lock(backend_cache_mutex);
        auto& entry = backend_cache[key];
        entry.backend = tool;
        entry.format = format;
    }

    std::string cached_format(const std::string& path, const std::string& password) {
        const std::string key = cache_key(path, password);
        if (key.empty()) return {};
        std::lock_guard<std::mutex> lock(backend_cache_mutex);
        auto it = backend_cache.find(key);
        return it == backend_cache.end() ? std::string{} : it->second.format;
    }

    std::string executable() {
        const auto candidates = executables();
        return candidates.empty() ? std::string{} : candidates.front();
    }

    std::string require_executable() {
        const std::string tool = executable();
        if (tool.empty()) {
            error::throw_error(error::ErrorCode::TOOL_NOT_FOUND, {{"TOOL_NAME", "7z (or 7zz / 7za)"}});
        }
        return tool;
    }

    bool is_stream_format(const std::string& format) {
        static const std::set<std::string> streams = {
            "gzip", "bzip2", "xz", "lzma", "lzma86", "ppmd", "z", "mslz",
            "lizard", "lz5", "lz4", "lzip", "zstd", "swfc"
        };
        return streams.count(lower(format)) != 0;
    }

    std::string probe_format(const std::string& path, const std::string& password) {
        const std::string known = cached_format(path, password);
        if (!known.empty()) return known;

        const auto candidates = backend_candidates(path, password);
        for (const auto& tool : candidates) {
            auto result = process::run_command_capture(
                {tool, "l", "-slt", "-sccUTF-8", password.empty() ? "-p-" : "-p" + password,
                 "--", std::filesystem::absolute(path).string()}, 65536);
            if (result.exit_code != 0) continue;
            const std::string format = parse_type(result.stdout_output);
            if (!format.empty()) {
                remember_probe(path, password, tool, format);
                return format;
            }
        }
        return {};
    }

    bool recognizes(const std::string& path) { return !probe_format(path).empty(); }
}
