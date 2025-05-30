# Hirmux `hitpag` Compilation and Usage Guide

## [Hitmux Official Website https://hitmux.top](https://hitmux.top)

[English](https://github.com/Caokai674674/hitpag/blob/main/README.md) [中文](https://github.com/Caokai674674/hitpag/blob/main/README_zh.md) [日本語](https://github.com/Caokai674674/hitpag/blob/main/README_ja.md) [Deutsch](https://github.com/Caokai674674/hitpag/blob/main/README_de.md) [한국어](https://github.com/Caokai674674/hitpag/blob/main/README_ko.md)


## Introduction

In a Linux environment, handling compressed files often requires remembering various commands and parameters for tools like `tar`, `gzip`, `unzip`, `7z`. This can be a significant challenge for new users or those who don't frequently work with archives. **`hitpag` was created to solve this problem.**

Hitmux `hitpag` is an intelligent and user-friendly command-line tool developed by Hitmux, designed to simplify file compression and decompression operations on Linux. Its core philosophy revolves around **automatic recognition and user-friendly interaction**. Whether you need to decompress a downloaded archive or package local files for sharing, `hitpag` intelligently identifies the file type based on its extension and automatically invokes the underlying system tools for processing, saving you the hassle of memorizing complex commands. Additionally, it offers a helpful **interactive mode**, allowing you to easily complete operations through simple questions and answers, even if you're unfamiliar with command-line parameters.

---

## Compilation Instructions

### Environment Requirements

* **Operating System**: Linux (tested successfully on Ubuntu 22.04 and Debian 12)
* **Compiler**: GCC/G++ supporting C++17 standard
* **Build Tool**: CMake 3.10 or higher
* **Dependency Library**: `stdc++fs` (C++ filesystem library), usually installed automatically with GCC/G++

### Compilation Steps

1.  **Update your system and install build tools**:
    First, ensure your system is up-to-date and necessary compilation tools are installed:

    ```bash
    sudo apt update
    sudo apt install -y g++ cmake make
    ```

2.  **Navigate to project directory and create build folder**:
    Go to the root directory of the `hitpag` project, then create and enter the `build` folder:

    ```bash
    cd hitpag
    mkdir -p build
    cd build
    ```

3.  **Generate build files using CMake**:
    Run CMake to configure the project and generate the Makefile:

    ```bash
    cmake ..
    ```

4.  **Compile the project**:
    Execute the `make` command to compile `hitpag`:

    ```bash
    make
    ```

5.  ** (Optional) Install to system**:
    If you want `hitpag` to be runnable from any directory, you can install it to your system's PATH:

    ```bash
    sudo make install
    ```

---

## Usage Instructions

### Basic Usage

The basic syntax for the `hitpag` tool is very straightforward:

```bash
hitpag [options] source_path destination_path
```

### Command-line Options

* `-i`: **Interactive mode**. When you're unsure how to proceed, the tool will guide you step-by-step.
* `-h, --help`: Display help information.
* `-v`: Display version information.

### Usage Examples

1.  **Decompressing files**:
    `hitpag` automatically identifies the archive type and decompresses it.

    ```bash
    hitpag archive.tar.gz ./extracted_dir
    ```
    This command will decompress `archive.tar.gz` into the `./extracted_dir` directory.

2.  **Compressing files/directories**:
    `hitpag` automatically selects the compression format based on the target filename's extension.

    ```bash
    hitpag ./my_folder my_archive.zip
    ```
    This command will compress the `./my_folder` directory into `my_archive.zip`.

3.  **Interactive mode**:
    In interactive mode, `hitpag` will ask questions and wait for your input.

    ```bash
    hitpag -i big_file.rar
    ```
    You will be guided to select the decompression type, destination path, and more.

### Supported Compression Formats

`hitpag` supports the following mainstream compression/decompression formats by invoking existing system tools:

* `tar` (uncompressed)
* `tar.gz` / `tgz` (gzip compressed)
* `tar.bz2` / `tbz2` (bzip2 compressed)
* `tar.xz` / `txz` (xz compressed)
* `zip`
* `rar`
* `7z`

### Important Notes

1.  **Ensure corresponding compression/decompression tools are installed**:
    `hitpag` relies on the compression tools already installed on your system. Please run the following command to ensure they are all installed:

    ```bash
    sudo apt install -y tar gzip bzip2 xz-utils zip unzip rar unrar p7zip-full
    ```

2.  **Automatic directory creation**:
    When decompressing, if the specified destination directory does not exist, `hitpag` will automatically create it for you.

---

## Error Handling

When an operation fails, `hitpag` displays clear error messages in English to help you diagnose the problem. Common error messages include:

* `Error: Missing arguments`: Insufficient command-line arguments.
* `Error: Source path does not exist`: The specified source file or directory does not exist.
* `Error: Invalid destination path`: The destination path format is incorrect or inaccessible.
* `Error: Unrecognized file format`: Unable to identify the compression format of the specified file.
* `Error: Required tool not found`: The system lacks the compression/decompression tool needed to perform the operation.
* `Error: Operation failed`: The compression/decompression operation itself failed to execute.
* `Error: Permission denied`: You do not have sufficient permissions to access the file or directory.
* `Error: Insufficient disk space`: Not enough disk space to complete the operation.
