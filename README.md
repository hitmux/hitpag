# hitpag 2.0 - Smart Compression Tool

**Intelligent, Powerful, User-friendly Next-generation Command-line Compression Tool**

[![GitHub](https://img.shields.io/badge/GitHub-Hitmux/hitpag-blue)](https://github.com/Hitmux/hitpag) 
[![Website](https://img.shields.io/badge/Website-hitmux.top-green)](https://hitmux.top)

[English](README.md) | [简体中文](README_zh.md)

---

## 🚀 Core Features

### 🧠 Intelligent File Type Recognition
- **Magic Number Detection**: Analyzes file headers to automatically identify real formats, independent of extensions
- **Handles Problem Files**: Supports files without extensions or with incorrect extensions
- **Format Override**: `--format` option for manual format specification when detection fails

```bash
# Auto-detect even without file extension
hitpag mystery_file ./output/

# Handle files with wrong extensions (actually 7z but named .zip)
hitpag fake.zip ./extracted/      # Auto-detects as 7z format

# Manual format specification
hitpag --format=rar problem_archive ./output/
```

### ⚡ High-Performance Compression
- **Multi-threading Support**: Auto-detects CPU cores for parallel compression speedup
- **Compression Level Control**: Fine-tune compression ratio vs speed with 1-9 levels
- **Modern Algorithm Support**: LZ4 ultra-fast, Zstandard high-efficiency

```bash
# Multi-threaded high-performance compression
hitpag -l9 -t8 --benchmark data.tar.gz ./large_files/

# Ultra-fast compression (suitable for temporary files)
hitpag --format=lz4 temp.lz4 ./temp_data/

# High-efficiency modern compression
hitpag --format=zstd archive.zstd ./documents/
```

### 🎯 Precise File Filtering
- **Regex Support**: Powerful include/exclude pattern matching
- **Combined Filtering**: Flexible multi-condition combinations for precise content control

```bash
# Compress only source code files
hitpag --include='*.cpp' --include='*.h' --include='*.py' code.7z ./project/

# Exclude temporary and build files
hitpag --exclude='*.tmp' --exclude='build/*' --exclude='node_modules/*' clean.tar.gz ./project/
```

### 📊 Performance Monitoring & Verification
- **Benchmarking**: Detailed statistics on compression ratio, time, thread utilization
- **Integrity Verification**: Automatic archive verification after compression
- **Verbose Output**: Real-time progress and operation information

```bash
# Performance analysis
hitpag --benchmark --verbose optimized.tar.xz ./data/

# Critical data compression with verification
hitpag --verify --benchmark important.7z ./critical_files/
```

### 🔐 Secure Encryption Protection
- **Password Protection**: Supports password encryption for ZIP and 7Z formats
- **Interactive Input**: Secure password input with no echo

```bash
# Password-protected compression
hitpag -pmysecret secure.7z ./sensitive_data/

# Interactive password input
hitpag -p confidential.zip ./private_files/
```

### 🎨 User-Friendly Interface
- **Interactive Mode**: Wizard-style operation for command-line beginners
- **Smart Detection**: Auto-detects operation type (compress/decompress)
- **Comprehensive Help**: Rich examples and documentation

```bash
# Launch interactive mode
hitpag -i

# Get help
hitpag --help
```

---

## 📦 Supported Formats

### Traditional Formats
- **TAR Series**: tar, tar.gz, tar.bz2, tar.xz
- **ZIP**: Full support including password protection
- **7Z**: Highest compression ratio with password support
- **RAR**: Decompression support

### Modern Formats
- **LZ4**: Ultra-fast compression/decompression for real-time scenarios
- **Zstandard (zstd)**: Facebook-developed, best balance of compression ratio and speed
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

### Build & Install
```bash
git clone https://github.com/Hitmux/hitpag.git
cd hitpag
mkdir build && cd build
cmake ..
make
sudo make install  # optional
```

### Basic Usage
```bash
# Auto-detect operation
hitpag archive.tar.gz ./extracted/    # decompress
hitpag ./folder/ backup.zip            # compress

# Advanced features
hitpag -l9 -t4 --benchmark big_data.tar.xz ./data/
hitpag --format=7z --verbose mystery_file ./output/
```

---

## 💡 Use Cases

### 🏢 Enterprise Backup
```bash
# Efficient backup excluding temporary files
hitpag --benchmark --exclude='*.tmp' --exclude='*.log' \
       -l9 -t8 enterprise_backup.tar.xz ./company_data/
```

### 👨‍💻 Development Project Compression
```bash
# Package only source code
hitpag --include='*.cpp' --include='*.h' --include='*.py' \
       --exclude='build/*' source_code.7z ./project/
```

### 🔒 Secure File Transfer
```bash
# Encrypted compression of sensitive files
hitpag -p --verify --benchmark secure_transfer.7z ./confidential/
```

### ⚡ Quick Temporary Compression
```bash
# Ultra-fast compression of temporary files
hitpag --format=lz4 temp_backup.lz4 ./temp_work/
```

### 🎯 Problem File Handling
```bash
# Handle files with wrong extensions or no extensions
hitpag downloaded_archive ./extracted/           # auto-detect format
hitpag --format=rar unknown_format ./output/     # force specific format
```

---

## 📝 Command Reference

### Basic Options
- **`-i`** - Interactive mode
- **`-p[password]`** - Password protection
- **`-l[1-9]`** - Compression level
- **`-t[count]`** - Thread count
- **`--format=type`** - Force format

### Advanced Options
- **`--verbose`** - Detailed output
- **`--benchmark`** - Performance statistics
- **`--verify`** - Integrity verification
- **`--exclude=pattern`** - Exclude files
- **`--include=pattern`** - Include files

### Example Commands
```bash
# Intelligent recognition
hitpag file.7z ./output/

# High-performance compression
hitpag -l9 -t8 --benchmark archive.tar.xz ./data/

# Precise filtering
hitpag --include='*.cpp' --exclude='build/*' code.7z ./project/

# Secure encryption
hitpag -p --verify secure.7z ./sensitive/

# Format specification
hitpag --format=zip --verbose unknown_file ./extracted/
```

---

## 🔍 Troubleshooting

### Common Issues
1. **Recognition Failed**: Use `--format` to specify manually
2. **Permission Error**: Check file/directory permissions
3. **Missing Tool**: Install corresponding compression tool
4. **Out of Memory**: Reduce thread count or compression level

### Error Codes
- `Error: Invalid format specified` - Format parameter error
- `Error: Required tool not found` - Missing compression tool
- `Error: Source path does not exist` - Source file not found

---

## 📈 Version History

### 2.0.2 - Latest Release
**Bug Fixes & Improvements**
- Enhanced file header detection with better boundary checking
- Improved memory safety and error handling
- Better terminal settings restoration for password input
- More robust parameter validation
- Fixed edge cases in file type recognition

### 2.0.0 - Major Update
**New Features**
- Intelligent file header detection system
- Manual format override (--format option)
- Multi-threaded parallel processing
- Modern compression format support (LZ4, Zstandard)
- File filtering system
- Performance benchmarking

**Improvements**
- Enhanced error handling
- Better user interface
- Memory safety enhancements
- Cross-platform compatibility

**Fixes**
- File header detection boundary checks
- Terminal settings restoration
- Parameter validation improvements

---

## 🤝 Contributing

Welcome to contribute code, report bugs, or suggest features!

- 📝 [Submit Issue](https://github.com/Hitmux/hitpag/issues)
- 🔧 [Submit PR](https://github.com/Hitmux/hitpag/pulls)
- 💬 [Discussions](https://github.com/Hitmux/hitpag/discussions)

---

## 📄 License

This project is licensed under [GNU Affero General Public License v3.0](LICENSE).

---

**Developer**: [Hitmux](https://hitmux.top) | **Project**: [GitHub](https://github.com/Hitmux/hitpag)