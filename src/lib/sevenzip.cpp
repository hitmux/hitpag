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
#include <chrono>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
#include <set>

namespace fs = std::filesystem;

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
        std::string lower(std::string value) {
            std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return std::tolower(c); });
            return value;
        }

        // Format names this project knows how to drive for creation. Whether the installed
        // backend can actually write one of them is a separate question and is probed below:
        // p7zip can create SWFc, lizard, lz5, lz4 and zstd, while current 7-Zip builds can
        // create none of them.
        const std::set<std::string>& creatable_candidates() {
            static const std::set<std::string> value = {
                "gzip", "bzip2", "xz", "wim", "SWFc", "lizard", "lz5",
                "zip", "tar", "lz4", "zstd"
            };
            return value;
        }

        // Defined after executable(); declared here because can_create() is defined above it.
        bool backend_supports_creation(const std::string& format);
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
        // Split/installable variants and compressed TAR aliases need additional workflows.
        static const std::set<std::string> read_only_aliases = {
            "swm", "esd", "ppkg", "tpz", "taz", "tliz", "tlz", "tlz4", "tlz5", "tzstd"
        };
        if (read_only_aliases.count(name)) return "";
        return normalize_format(name);
    }

    bool can_create(const std::string& format) {
        if (creatable_candidates().count(format) == 0) return false;
        return backend_supports_creation(format);
    }

    bool single_file_format(const std::string& format) {
        return can_create(format) && format != "wim" && format != "zip" && format != "tar";
    }

    bool supports_password(const std::string& format) { return format == "zip"; }

    std::string executable() {
        for (const char* tool : {"7z", "7zz", "7za"}) {
            if (operation::is_tool_available(tool)) return tool;
        }
        return "";
    }

    std::string require_executable() {
        std::string tool = executable();
        if (tool.empty()) error::throw_error(error::ErrorCode::TOOL_NOT_FOUND, {{"TOOL_NAME", "7z (or 7zz / 7za)"}});
        return tool;
    }

    namespace {
        enum class ProbeOutcome {
            Supported,
            Unsupported,
            Inconclusive,
        };

        // Failure text that reliably means "this build cannot write that format at all": 7-Zip
        // reports E_NOTIMPL for codecs it no longer ships (SWFc, zstd) and rejects a format name
        // it does not know with an unsupported-archive-type error. Every other failure - a codec
        // that dislikes the probe payload, for instance - must not be read as "unsupported", or
        // a format the installed backend can really write would be blocked by mistake.
        bool is_missing_format_error(const std::string& output) {
            static const char* const markers[] = {
                "E_NOTIMPL",
                "Not implemented",
                "Unsupported archive type",
                "Unsupported method",
            };
            for (const char* marker : markers) {
                if (output.find(marker) != std::string::npos) return true;
            }
            return false;
        }

        // Learns whether the installed backend can really write a format by writing a throwaway
        // archive with it. Parsing `7z i` is not an option: p7zip and 7-Zip lay the format table
        // out differently, and the columns move with the locale, so a real attempt is the only
        // version- and locale-independent answer. Anything that is not a clear yes or no is
        // reported as inconclusive so a format the backend can write is never blocked by mistake.
        ProbeOutcome probe_creation(const std::string& format) {
            const std::string tool = executable();
            if (tool.empty()) return ProbeOutcome::Inconclusive;

            std::error_code ec;
            const fs::path root = fs::temp_directory_path(ec);
            if (ec) return ProbeOutcome::Inconclusive;

            const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
            const fs::path dir = root / ("hitpag-format-probe-" + std::to_string(stamp) + "-" + lower(format));

            fs::remove_all(dir, ec);
            fs::create_directories(dir, ec);
            if (ec) return ProbeOutcome::Inconclusive;

            const fs::path payload = dir / "payload.bin";
            const fs::path archive = dir / ("probe." + lower(format));

            bool wrote_payload = false;
            {
                std::ofstream output(payload, std::ios::binary | std::ios::trunc);
                if (output) {
                    output.put('x');
                    wrote_payload = output.good();
                }
            }

            ProbeOutcome outcome = ProbeOutcome::Inconclusive;
            if (wrote_payload) {
                // Mirrors the real creation command: -spd keeps wildcard characters literal and
                // no -mx level is forced, because some codecs reject a forced copy level.
                const auto result = process::run_command_capture(
                    {tool, "a", "-t" + format, "-spd", "--", archive.string(), payload.string()});

                std::error_code size_ec;
                const auto size = fs::file_size(archive, size_ec);
                const bool produced_archive = !size_ec && size > 0;

                if (result.exit_code == 0 && produced_archive) {
                    outcome = ProbeOutcome::Supported;
                } else if (result.exit_code != 0 && !produced_archive &&
                           is_missing_format_error(result.stdout_output)) {
                    outcome = ProbeOutcome::Unsupported;
                }
            }

            fs::remove_all(dir, ec);
            return outcome;
        }

        bool backend_supports_creation(const std::string& format) {
            static std::map<std::string, bool> cache;
            const auto cached = cache.find(format);
            if (cached != cache.end()) return cached->second;

            // Inconclusive keeps the historical behaviour of trusting the candidate list.
            const bool supported = probe_creation(format) != ProbeOutcome::Unsupported;
            cache[format] = supported;
            return supported;
        }
    }

    bool is_stream_format(const std::string& format) {
        static const std::set<std::string> streams = {
            "gzip", "bzip2", "xz", "lzma", "lzma86", "Ppmd", "Z", "MsLZ",
            "lizard", "lz5", "lz4", "lzip", "zstd", "SWFc"
        };
        return streams.count(format) != 0;
    }

    std::string probe_format(const std::string& path, const std::string& password) {
        const std::string tool = executable();
        if (tool.empty()) return "";
        // Never execute an EXE. Ask the archive reader to inspect its contents.
        auto result = process::run_command_capture(
            {tool, "l", "-slt", "-sccUTF-8", password.empty() ? "-p-" : "-p" + password, "--", std::filesystem::absolute(path).string()}, 65536);
        std::istringstream lines(result.stdout_output);
        std::string line;
        while (std::getline(lines, line)) {
            if (line.rfind("----------", 0) == 0) break;
            // Encrypted headers may fail to list without a password but still identify the container.
            if (line.rfind("Type = ", 0) == 0 && line.size() > 7) {
                std::string format = line.substr(7);
                if (!format.empty() && format.back() == '\r') format.pop_back();
                return format;
            }
        }
        return "";
    }

    bool recognizes(const std::string& path) { return !probe_format(path).empty(); }
}
