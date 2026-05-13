# hitpag - Intelligent Compression Tool

**A C++17 command-line compression tool with an archive browser TUI.**

[![GitHub](https://img.shields.io/badge/GitHub-Hitmux/hitpag-blue)](https://github.com/Hitmux/hitpag)
[![Website](https://img.shields.io/badge/Website-hitmux.org-green)](https://hitmux.org)

[English](README.md) | [简体中文](README_zh.md)

---

## Highlights

- Detects archive formats by file signature instead of filename extension.
- Uses one command for compression, extraction, verification, and archive browsing.
- Includes a TUI archive browser for listing, searching, previewing, extracting, and editing archive entries.
- Supports tar, gzip, bzip2, xz, zip, 7z, rar, lz4, zstd, and xar.
- Supports password-protected zip, 7z, and rar extraction where the underlying tool supports it.
- Uses FTXUI for the terminal interface; a vendored static FTXUI copy is included.

---

## Install

```bash
# Ubuntu/Debian runtime and build dependencies
sudo apt install -y tar unrar gzip bzip2 xz-utils zip unzip p7zip-full lz4 zstd g++ cmake make

git clone https://github.com/Hitmux/hitpag.git
cd hitpag
mkdir -p build
cd build
cmake ..
make
sudo make install
```

FTXUI is resolved during CMake configuration. hitpag prefers a compatible system static FTXUI package and falls back to `third_party/ftxui/`.

```bash
cmake .. -DHITPAG_FORCE_VENDORED_FTXUI=ON
cmake .. -DHITPAG_FORCE_SYSTEM_FTXUI=ON
```

---

## TUI Archive Browser

Open an archive directly in the TUI:

```bash
hitpag --tui archive.zip
hitpag archive.tar.gz
```

When a single supported archive path is provided without a target path, hitpag opens the TUI automatically.

Core TUI actions:

- `Up` / `Down`: move through entries.
- `Right` / `Enter`: open a directory or move into preview.
- `Left`: return to the parent directory or file list.
- `/`: search entries.
- `x`: extract the selected entry.
- `e`: edit the selected file through an external editor.
- `s`: configure the editor command.
- `?`: open help.
- `q` or `Esc`: quit or close the current dialog.

---

## CLI Usage

```bash
# Extract
hitpag archive.tar.gz ./output/
hitpag backup.zip ./extracted/
hitpag data.7z ./data/

# Compress
hitpag ./my_folder/ backup.zip
hitpag ./documents/ archive.tar.gz

# Password
hitpag -pMySecret secure.7z ./sensitive/
hitpag -p encrypted.zip ./output/

# Performance and verification
hitpag -l9 -t8 --benchmark data.tar.xz ./large_files/
hitpag --verify ./documents/ archive.zip
```

---

## Filtering

```bash
hitpag --include='*.cpp' --include='*.h' code.7z ./project/
hitpag --exclude='*.tmp' --exclude='node_modules/*' clean.tar.gz ./project/
```

---

## Supported Formats

| Format | Compress | Extract | Password | Notes |
|--------|----------|---------|----------|-------|
| tar, tar.gz, tar.bz2, tar.xz, tar.zst | yes | yes | no | Unix archive formats |
| zip | yes | yes | yes | Split zip extraction uses 7z |
| 7z | yes | yes | yes | High compression ratio |
| rar | no | yes | yes | Extraction only |
| lz4 | yes | yes | no | Single-file compression |
| zstd | yes | yes | no | Single-file compression |
| xar | yes | yes | no | macOS archive format |

---

## Command Reference

| Option | Description |
|--------|-------------|
| `-i` | Interactive CLI mode |
| `--tui` | TUI archive browser |
| `-p[password]` | Password; prompts when omitted |
| `-l[1-9]` | Compression level |
| `-t[count]` | Thread count |
| `--format=TYPE` | Force archive type |
| `--verbose` | Detailed output |
| `--benchmark` | Performance statistics |
| `--verify` | Verify archive integrity |
| `--include=PATTERN` | Include matching paths |
| `--exclude=PATTERN` | Exclude matching paths |

---

## Troubleshooting

| Problem | Solution |
|---------|----------|
| Format not recognized | Use `--format=TYPE` |
| Runtime tool not found | Install the corresponding package, such as `p7zip-full`, `unrar`, `lz4`, or `zstd` |
| Split zip extraction fails | Install `p7zip-full` |
| TUI does not open automatically | Use `hitpag --tui archive.zip` |

---

## Contributing

- [Issues](https://github.com/Hitmux/hitpag/issues)
- [Pull Requests](https://github.com/Hitmux/hitpag/pulls)

## License

[GNU Affero General Public License v3.0](LICENSE)

---

**Developer**: [Hitmux](https://hitmux.org)
