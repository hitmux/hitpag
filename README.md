# Hirmux `hitpag` Compilation and Usage Guide

## [Hitmux Official Website https://hitmux.top](https://hitmux.top)

## Introduction

In a Linux environment, handling compressed files often requires remembering various commands and parameters for tools like `tar`, `gzip`, `unzip`, and `7z`. This can be a significant challenge for new users or those who don't frequently work with archives. **`hitpag` was created to solve this problem.**

Hitmux `hitpag` is an intelligent and user-friendly command-line tool developed by Hitmux, designed to simplify file compression and decompression operations on Linux. Its core philosophy revolves around **automatic recognition and user-friendly interaction**. Whether you need to decompress a downloaded archive or package local files for sharing, `hitpag` intelligently identifies the file type based on its extension and automatically invokes the underlying system tools for processing, saving you the hassle of memorizing complex commands. Additionally, it offers a powerful **interactive mode** and **password support**, allowing you to easily complete operations like encrypted compression or decryption through simple questions and answers, even if you're unfamiliar with command-line parameters.

---

## Compilation Instructions

### Environment Requirements

*   **Operating System**: Linux (tested successfully on Ubuntu 22.04 and Debian 12)
*   **Compiler**: GCC/G++ supporting the C++17 standard
*   **Build Tool**: CMake 3.10 or higher
*   **Dependency Library**: `stdc++fs` (C++ filesystem library), usually installed automatically with GCC/G++

### Compilation Steps

1.  **Update your system and install build tools**:
    First, ensure your system is up-to-date and necessary compilation tools are installed:

    ```bash
    sudo apt update
    sudo apt install -y g++ cmake make
    ```

2.  **Navigate to the project directory and create a build folder**:
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

5.  **(Optional) Install to the system**:
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

*   `-i`: **Interactive mode**. When you're unsure how to proceed, the tool will guide you step-by-step.
*   `-p[password]`: **Use a password**. For encryption or decryption. If you use just `-p` without an attached password, the tool will prompt you to enter it securely.
*   `-h, --help`: Display help information.
*   `-v, --version`: Display version information.

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
    This command will compress the entire `./my_folder` directory into `my_archive.zip`.

3.  **Compressing only directory contents (Important Tip)**:
    If you add a trailing slash `/` to the source directory path, `hitpag` will compress only the files and subdirectories within it, not the directory itself.

    ```bash
    hitpag ./my_folder/ my_archive.zip
    ```
    After extraction, the root of `my_archive.zip` will contain the contents of `my_folder`, not the `my_folder` directory.

4.  **Encrypting an archive**:
    Use the `-p` option to set a password for the archive.

    ```bash
    hitpag -pmysecret ./docs my_docs.7z
    ```
    This command encrypts the `./docs` directory into `my_docs.7z` with the password `mysecret`.

5.  **Decrypting a file (Interactive Password Prompt)**:
    When decompressing an encrypted file, use the `-p` option to have the tool prompt for a password.

    ```bash
    hitpag -p secret.zip .
    ```
    This will display an `Enter password:` prompt for secure password entry.

6.  **Interactive mode**:
    In interactive mode, `hitpag` will guide you through all steps, including choosing the format, setting a password, and confirming overwrites.

    ```bash
    hitpag -i
    ```

### Supported Compression Formats

`hitpag` supports the following mainstream compression/decompression formats by invoking existing system tools:

*   `tar` (uncompressed)
*   `tar.gz` / `tgz` (gzip compressed)
*   `tar.bz2` / `tbz2` (bzip2 compressed)
*   `tar.xz` / `txz` (xz compressed)
*   `zip` (supports passwords)
*   `rar`
*   `7z` (supports passwords)

### Important Notes

1.  **Ensure corresponding compression/decompression tools are installed**:
    `hitpag` relies on the compression tools already installed on your system. Please run the following command to ensure they are all installed:

    ```bash
    sudo apt install -y tar gzip bzip2 xz-utils zip unzip rar unrar p7zip-full
    ```

2.  **Password Protection**:
    Currently, only **`zip`** and **`7z`** formats support password protection. Using the password option with `tar` variants (e.g., `.tar.gz`) will be ignored, and a warning will be displayed.

3.  **Automatic directory creation**:
    When decompressing, if the specified destination directory does not exist, `hitpag` will automatically create it for you.

---

## Error Handling

When an operation fails, `hitpag` displays clear error messages in English to help you diagnose the problem. Common error messages include:

*   `Error: Missing arguments`: Insufficient command-line arguments.
*   `Error: Source path does not exist`: The specified source file or directory does not exist.
*   `Error: Invalid destination path`: The destination path format is incorrect or inaccessible.
*   `Error: Unrecognized file format`: Unable to identify the compression format of the specified file.
*   `Error: Required tool not found`: The system lacks the compression/decompression tool needed to perform the operation.
*   `Error: Operation failed`: The compression/decompression operation itself failed to execute (this may be due to a wrong password).
*   `Error: Permission denied`: You do not have sufficient permissions to access the file or directory.
*   `Error: Insufficient disk space`: Not enough disk space to complete the operation.
