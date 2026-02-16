# hitpag - The Intelligent Compression Tool

**Smart, powerful, and easy-to-use command-line compression tool**

[![GitHub](https://img.shields.io/badge/GitHub-Hitmux/hitpag-blue)](https://github.com/Hitmux/hitpag)
[![Website](https://img.shields.io/badge/Website-hitmux.org-green)](https://hitmux.org)

[English](README.md) | [简体中文](README_zh.md)

---

## Why hitpag?

- **🧠 Smart Recognition** - Automatically detects file format by magic number, not just extension
- **⚡ One Command** - No need to remember different tools for different formats
- **📦 All Formats** - tar, gzip, bzip2, xz, zip, 7z, rar, lz4, zstd, xar
- **🔐 Password Support** - Encryption for zip and 7z formats
- **🚀 Multi-threaded** - Parallel compression for better performance

---

## Quick Start

### Install
```bash
# Ubuntu/Debian
sudo apt install -y tar unrar gzip bzip2 xz-utils zip unzip p7zip-full lz4 zstd g++ cmake make

# Build
git clone https://github.com/Hitmux/hitpag.git
cd hitpag && mkdir build && cd build && cmake .. && make
sudo make install  # Optional
```

### Basic Usage
```bash
# Decompress - just point to the archive
hitpag archive.tar.gz ./output/
hitpag backup.zip ./extracted/
hitpag data.7z ./data/

# Compress - specify source and target
hitpag ./my_folder/ backup.zip
hitpag ./documents/ archive.tar.gz

# With password
hitpag -pMySecret secure.7z ./sensitive/
hitpag -p encrypted.zip ./output/  # Prompts for password
```

That's it! hitpag figures out the rest.

---

## Advanced Usage

### Performance Options
```bash
# Multi-threaded compression with benchmarking
hitpag -l9 -t8 --benchmark data.tar.xz ./large_files/

# Ultra-fast compression (LZ4)
hitpag --format=lz4 temp.lz4 ./temp_data/

# High-efficiency compression (Zstandard)
hitpag --format=zstd archive.zstd ./documents/
```

### File Filtering
```bash
# Include only specific files
hitpag --include='*.cpp' --include='*.h' code.7z ./project/

# Exclude files
hitpag --exclude='*.tmp' --exclude='node_modules/*' clean.tar.gz ./project/
```

### Other Options
```bash
hitpag -i                    # Interactive mode
hitpag --verbose archive.7z  # Detailed output
hitpag --verify data.tar.gz  # Verify after compression
hitpag --format=rar unknown  # Force specific format
```

---

## Supported Formats

| Format | Compress | Decompress | Password | Notes |
|--------|----------|------------|----------|-------|
| tar, tar.gz, tar.bz2, tar.xz | ✅ | ✅ | ❌ | Classic Unix formats |
| zip | ✅ | ✅ | ✅ | Including split archives (.z01, .z02, ...) |
| 7z | ✅ | ✅ | ✅ | Best compression ratio |
| rar | ❌ | ✅ | ✅ | Decompress only |
| lz4 | ✅ | ✅ | ❌ | Ultra-fast speed |
| zstd | ✅ | ✅ | ❌ | Best speed/ratio balance |
| xar | ✅ | ✅ | ❌ | macOS native |

---

## Command Reference

| Option | Description |
|--------|-------------|
| `-i` | Interactive mode |
| `-p[password]` | Password (prompts if not provided) |
| `-l[1-9]` | Compression level |
| `-t[count]` | Thread count |
| `--format=TYPE` | Force format |
| `--verbose` | Detailed output |
| `--benchmark` | Performance stats |
| `--verify` | Integrity check |
| `--include=PATTERN` | Include files |
| `--exclude=PATTERN` | Exclude files |

---

## Troubleshooting

| Problem | Solution |
|---------|----------|
| Format not recognized | Use `--format=TYPE` to specify |
| Permission denied | Check file/directory permissions |
| Tool not found | Install required tool (p7zip-full, unrar, etc.) |
| Split ZIP fails | Install p7zip-full (`sudo apt install p7zip-full`) |

---

## Contributing

- 📝 [Issues](https://github.com/Hitmux/hitpag/issues)
- 🔧 [Pull Requests](https://github.com/Hitmux/hitpag/pulls)

## License

[GNU Affero General Public License v3.0](LICENSE)

---

**Developer**: [Hitmux](https://hitmux.org)
