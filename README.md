# Hirmux `hitpag` 2.0 - Advanced Compression Tool

## [Hitmux Official Website https://hitmux.top](https://hitmux.top)

## Introduction

**`hitpag` 2.0** is a major upgrade to the intelligent command-line compression tool. Building on the core philosophy of **automatic recognition and user-friendly interaction**, version 2.0 introduces powerful new features including compression level control, multi-threading support, file filtering, performance benchmarking, and support for modern compression formats.

Whether you're compressing large datasets with optimal settings, filtering specific file types, or benchmarking compression performance, `hitpag` 2.0 provides enterprise-grade functionality while maintaining its signature ease of use.

---

## Compilation Instructions

### Environment Requirements

*   **Operating System**: Linux (tested on Ubuntu 22.04 and Debian 12)
*   **Compiler**: GCC/G++ supporting the C++17 standard
*   **Build Tool**: CMake 3.10 or higher
*   **Dependencies**: `stdc++fs`, `pthread` (C++ filesystem and threading libraries)

### Compilation Steps

1.  **Update system and install build tools**:
    ```bash
    sudo apt update
    sudo apt install -y g++ cmake make
    ```

2.  **Navigate to project directory and build**:
    ```bash
    cd hitpag
    mkdir -p build
    cd build
    cmake ..
    make
    ```

3.  **(Optional) Install to system**:
    ```bash
    sudo make install
    ```

### System Dependencies

Install all supported compression tools:

```bash
# Essential compression tools
sudo apt install -y tar gzip bzip2 xz-utils zip unzip rar unrar p7zip-full

# Modern compression formats (optional)
sudo apt install -y lz4 zstd xar
```

---

## Usage Instructions

### Basic Syntax

```bash
hitpag [options] source_path destination_path
```

### New Command-line Options (Version 2.0)

*   **`-i`**: Interactive mode with enhanced format selection
*   **`-p[password]`**: Password encryption/decryption
*   **`-l[level]`**: Compression level (1-9, format-dependent defaults)
*   **`-t[threads]`**: Thread count for parallel processing (auto-detect if not specified)
*   **`--verbose`**: Detailed progress and operation information
*   **`--benchmark`**: Performance statistics including compression ratios and timing
*   **`--verify`**: Archive integrity verification after compression
*   **`--exclude=PATTERN`**: Exclude files/directories matching regex pattern
*   **`--include=PATTERN`**: Include only files/directories matching regex pattern
*   **`-h, --help`**: Display comprehensive help information
*   **`-v, --version`**: Show version information
*   **Note**: There is a difference between `hitpag xxx/ file.zip` and `hitpag xxx file.zip`, which should not be difficult to understand. The same is true for `hitapg file.zip xxx/` and `hitpag file.zip xxx/`.

### Advanced Usage Examples

1.  **High-performance compression with optimal settings**:
    ```bash
    hitpag -l9 -t4 --benchmark large_dataset.tar.gz ./data/
    ```
    Compress with maximum compression level using 4 threads and show performance statistics.

2.  **Filtered compression with file type selection**:
    ```bash
    hitpag --include='*.cpp' --include='*.h' --exclude='build/*' source_code.7z ./project/
    ```
    Compress only C++ source files, excluding build directories.

3.  **Encrypted compression with verification**:
    ```bash
    hitpag -pmysecret --verify --verbose confidential.zip ./sensitive_data/
    ```
    Password-encrypt with integrity verification and detailed output.

4.  **Modern compression formats**:
    ```bash
    hitpag -l6 -t8 fast_archive.lz4 ./temp_files/
    hitpag -l9 small_archive.zstd ./documents/
    ```
    Use LZ4 for speed or Zstandard for optimal compression.

5.  **Batch processing with performance monitoring**:
    ```bash
    hitpag --benchmark --exclude='*.tmp' --exclude='*.log' backup.tar.xz ./work_folder/
    ```
    Create compressed backup excluding temporary files with performance analysis.

### Supported Compression Formats

`hitpag` 2.0 supports all traditional formats plus modern compression algorithms:

**Traditional formats:**
*   `tar` (uncompressed)
*   `tar.gz` / `tgz` (gzip compression)
*   `tar.bz2` / `tbz2` (bzip2 compression)
*   `tar.xz` / `txz` (xz compression)
*   `zip` (supports passwords and compression levels)
*   `rar` (decompression only)
*   `7z` (supports passwords and compression levels)

**Modern formats (new in 2.0):**
*   `lz4` (ultra-fast compression/decompression)
*   `zst` / `zstd` (Facebook's Zstandard - excellent compression/speed balance)
*   `xar` (macOS archive format)

### Performance Features

#### Multi-threading Support
Automatically detects CPU cores and utilizes parallel processing for supported formats:
```bash
hitpag -t8 archive.tar.gz ./large_directory/  # Use 8 threads explicitly
hitpag -t archive.tar.gz ./large_directory/   # Auto-detect optimal thread count
```

#### Compression Level Control
Fine-tune compression vs. speed trade-offs:
```bash
hitpag -l1 fast.zip ./files/      # Fastest compression
hitpag -l6 balanced.7z ./files/   # Balanced (default for most formats)
hitpag -l9 smallest.tar.xz ./files/ # Maximum compression
```

#### File Filtering
Advanced pattern matching for selective compression:
```bash
# Include only specific file types
hitpag --include='*.jpg' --include='*.png' photos.zip ./images/

# Exclude temporary and build files
hitpag --exclude='*.tmp' --exclude='node_modules/*' --exclude='build/*' clean_backup.tar.gz ./project/

# Complex filtering combinations
hitpag --include='src/*' --exclude='*.o' --exclude='*.obj' source_only.7z ./project/
```

#### Performance Benchmarking
Comprehensive performance analysis:
```bash
hitpag --benchmark --verbose optimized.tar.xz ./data/
```
Outputs:
- Compression time
- Original vs compressed size
- Compression ratio percentage
- Thread utilization information

---

## Error Handling

Enhanced error reporting with detailed diagnostics:

*   **`Error: Compression level must be between 1-9`**: Invalid compression level specified
*   **`Error: Thread count must be positive`**: Invalid thread count parameter
*   **`Error: Required tool not found: lz4`**: Missing compression utility
*   **`Filtering files: included 150, excluded 45`**: File filtering summary
*   **`Archive verification failed`**: Integrity check unsuccessful

---

## Version 2.0 Changelog

### New Features
- **Multi-threading support** for parallel compression/decompression
- **Compression level control** (1-9) for all supported formats
- **File filtering system** with regex pattern matching
- **Performance benchmarking** with detailed statistics
- **Archive verification** for integrity checking
- **Modern compression formats**: LZ4, Zstandard, XAR
- **Enhanced verbose mode** with detailed progress information

### Improvements
- **Modular architecture** with organized namespaces
- **Better error handling** with specific error codes
- **International message system** for easy localization
- **Enhanced interactive mode** with new format options
- **Improved help system** with comprehensive examples

### Technical Enhancements
- **C++17 threading support** using `std::thread`
- **Regex-based filtering** using `std::regex`
- **Performance monitoring** with `std::chrono`
- **Memory-efficient processing** for large files
- **Cross-platform compatibility** improvements

---

## Migration from 1.x

Version 2.0 maintains full backward compatibility with 1.x command syntax. All existing scripts and workflows will continue to work unchanged, while new features can be adopted incrementally.

```bash
# 1.x syntax still works
hitpag archive.tar.gz ./extracted_dir

# Enhanced 2.0 syntax adds powerful options
hitpag --benchmark --verbose -l9 -t4 archive.tar.gz ./extracted_dir
```
