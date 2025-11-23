# hitpag 2.0.3 - The Intelligent Compression Tool

**An intelligent, powerful, and easy-to-use next-generation command-line compression tool**

[![GitHub](https://img.shields.io/badge/GitHub-Hitmux/hitpag-blue)](https://github.com/Hitmux/hitpag)
[![Website](https://img.shields.io/badge/Website-hitmux.org-green)](https://hitmux.org)

[English](README.md) | [简体中文](README_zh.md)

---

## 🚀 Core Features

### 🧠 Intelligent File Type Recognition
- **Magic Number Detection**: Automatically identifies the true format by analyzing the file header, regardless of the extension.
- **Handles Problematic Files**: Supports files with no extension or incorrect extensions.
- **Format Override**: When auto-detection fails, the `--format` option can be used to manually specify the format.

```bash
# Auto-detects even without a file extension
hitpag mystery_file ./output/

# Handles files with incorrect extensions (it's actually a 7z file but named .zip)
hitpag fake.zip ./extracted/      # Automatically detected as 7z format

# Identifies compression format based on the suffix
hitpag example backup.tar # The 'example' folder will be inside the archive
hitpag example/ backup2.rat # The 'example' folder will not be inside the archive

# Force a specific format
hitpag --format=rar problem_archive ./output/
```

### ⚡ High-Performance Compression
- **Multi-threading Support**: Automatically detects the number of CPU cores to accelerate compression in parallel.
- **Compression Level Control**: Finely tune the balance between compression ratio and speed with levels 1-9.
- **Modern Algorithm Support**: Supports LZ4 for ultra-fast compression and Zstandard for high-efficiency compression.

```bash
# High-performance multi-threaded compression
hitpag -l9 -t8 --benchmark data.tar.gz ./large_files/

# Ultra-fast compression (suitable for temporary files)
hitpag --format=lz4 temp.lz4 ./temp_data/

# Efficient modern compression
hitpag --format=zstd archive.zstd ./documents/
```

### 🎯 Precise File Filtering
- **Regular Expression Support**: Powerful include/exclude pattern matching.
- **Combined Filtering**: Flexible combination of multiple conditions for precise control over content.

```bash
# Compress only source code files
hitpag --include='*.cpp' --include='*.h' --include='*.py' code.7z ./project/

# Exclude temporary and build files
hitpag --exclude='*.tmp' --exclude='build/*' --exclude='node_modules/*' clean.tar.gz ./project/
```

### 📊 Performance Monitoring & Verification
- **Benchmarking**: Provides detailed statistics on compression ratio, time taken, thread utilization, etc.
- **Integrity Verification**: Automatically verifies the integrity of the archive file after compression.
- **Verbose Output**: Displays real-time progress and operational information.

```bash
# Performance analysis
hitpag --benchmark --verbose optimized.tar.xz ./data/

# Compress and verify critical data
hitpag --verify --benchmark important.7z ./critical_files/
```

### 🔐 Secure Encryption Protection
- **Password Protection**: Supports password encryption for ZIP and 7Z formats.
- **Interactive Input**: Secure password entry method with no echo.

```bash
# Compress with a password
hitpag -pmysecret secure.7z ./sensitive_data/

# Interactive password input
hitpag -p confidential.zip ./private_files/
```

### 🎨 User-Friendly Interface
- **Interactive Mode**: Provides a wizard-like experience for command-line newcomers.
- **Smart Detection**: Automatically detects the operation type (compression/decompression).
- **Comprehensive Help Documentation**: Offers a wealth of examples and documentation.

```bash
# Start interactive mode
hitpag -i

# Get help
hitpag --help
```

---

## 📦 Supported Formats

### Legacy Formats
- **TAR Series**: tar, tar.gz, tar.bz2, tar.xz
- **ZIP**: Full support, including password protection
- **7Z**: Highest compression ratio, supports password protection
- **RAR**: Decompression support

### Modern Formats
- **LZ4**: Ultra-fast compression/decompression speeds, suitable for real-time scenarios
- **Zstandard (zstd)**: Developed by Facebook, offers the best balance between compression ratio and speed
- **XAR**: macOS native format

---

## 🛠️ Quick Start

### Install Dependencies
```bash
# Ubuntu/Debian
sudo apt install -y tar gzip bzip2 xz-utils zip unzip p7zip-full lz4 zstd

# Optional tools
sudo apt install -y rar unrar xar
```

### Build & Install```bash
git clone https://github.com/Hitmux/hitpag.git
cd hitpag
mkdir build && cd build
cmake ..
make
sudo make install  # Optional
```

### Basic Usage
```bash
# Auto-detect operation
hitpag archive.tar.gz ./extracted/    # Decompress
hitpag ./folder/ backup.zip            # Compress

# Advanced features
hitpag -l9 -t4 --benchmark big_data.tar.xz ./data/
hitpag --format=7z --verbose mystery_file ./output/
```

---

## 💡 Use Cases

### 🏢 Enterprise Backup
```bash
# High-efficiency backup, excluding temporary files
hitpag --benchmark --exclude='*.tmp' --exclude='*.log' \
       -l9 -t8 enterprise_backup.tar.xz ./company_data/
```

### 👨‍💻 Compressing Development Projects
```bash
# Package only the source code
hitpag --include='*.cpp' --include='*.h' --include='*.py' \
       --exclude='build/*' source_code.7z ./project/
```

### 🔒 Secure File Transfer
```bash
# Encrypt and compress sensitive files
hitpag -p --verify --benchmark secure_transfer.7z ./confidential/
```

### ⚡ Quick Temporary Compression
```bash
# Ultra-fast compression for temporary files
hitpag --format=lz4 temp_backup.lz4 ./temp_work/
```

### 🎯 Handling Problematic Files
```bash
# Handle files with incorrect or no extensions
hitpag downloaded_archive ./extracted/           # Auto-detect format
hitpag --format=rar unknown_format ./output/     # Force a specific format
```

---

## 📝 Command Reference

### Basic Options
- **`-i`** - Interactive mode
- **`-p[password]`** - Password protection
- **`-l[1-9]`** - Compression level
- **`-t[count]`** - Number of threads
- **`--format=type`** - Force a specific format

### Advanced Options
- **`--verbose`** - Verbose output
- **`--benchmark`** - Performance statistics
- **`--verify`** - Integrity verification
- **`--exclude=pattern`** - Exclude files
- **`--include=pattern`** - Include files

### Command Examples
```bash
# Smart recognition
hitpag file.7z ./output/

# High-performance compression
hitpag -l9 -t8 --benchmark archive.tar.xz ./data/

# Precise filtering
hitpag --include='*.cpp' --exclude='build/*' code.7z ./project/

# Secure encryption
hitpag -p --verify secure.7z ./sensitive/

# Specify format
hitpag --format=zip --verbose unknown_file ./extracted/
```

---

## 🔍 Troubleshooting

### Common Issues
1.  **Recognition Failure**: Use `--format` to manually specify the format.
2.  **Permission Errors**: Check the permissions of the file/directory.
3.  **Missing Tools**: Install the corresponding compression tool.
4.  **Insufficient Memory**: Reduce the number of threads or the compression level.

### Error Codes
- `Error: Invalid format specified` - Incorrect format parameter
- `Error: Required tool not found` - Missing compression tool
- `Error: Source path does not exist` - Source file does not exist

---

## 📈 Version History

### 2.0.3 - Latest Version
**New Features**
- Added an overwrite/cancel/rename confirmation flow with intelligent default suggestions (`_1`, `_2`, ...).
- CLI now supports compressing multiple sources in a single run (e.g., via shell wildcards) with automatic common-path handling.

**Improvements**
- Default rename suggestions now auto-increment when conflicts persist.
- Strengthened target path validation to prevent accidental self-overwrites.

### 2.0.2
**Bug Fixes & Improvements**
- Enhanced file header detection with improved boundary checks.
- Improved memory safety and error handling.
- Better restoration of terminal settings after password input.
- More robust parameter validation.
- Fixed edge cases in file type recognition.

### 2.0.0 - Major Update
**New Features**
- Intelligent file header detection system.
- Manual format override (`--format` option).
- Multi-threaded parallel processing.
- Support for modern compression formats (LZ4, Zstandard).
- File filtering system.
- Performance benchmarking.

**Improvements**
- Enhanced error handling.
- Better user interface.
- Increased memory safety.
- Cross-platform compatibility.

**Fixes**
- Boundary checks in file header detection.
- Terminal settings restoration.
- Improved parameter validation.

---

## 🤝 Contributing

Contributions, bug reports, and feature suggestions are welcome!

- 📝 [Submit Issue](https://github.com/Hitmux/hitpag/issues)
- 🔧 [Submit PR](https://github.com/Hitmux/hitpag/pulls)
- 💬 [Discussions](https://github.com/Hitmux/hitpag/discussions)

---

## 📄 License

This project is licensed under the [GNU Affero General Public License v3.0](LICENSE).

---

**Developer**: [Hitmux](https://hitmux.top)
