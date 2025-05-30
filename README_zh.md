# Hirmux `hitpag` 编译与使用说明

##  [Hitmux 官网 https://hitmux.top](https://hitmux.top)

[English](https://github.com/Caokai674674/hitpag/blob/main/README.md) [中文](https://github.com/Caokai674674/hitpag/blob/main/README_zh.md) [日本語](https://github.com/Caokai674674/hitpag/blob/main/README_ja.md) [Deutsch](https://github.com/Caokai674674/hitpag/blob/main/README_de.md) [한국어](https://github.com/Caokai674674/hitpag/blob/main/README_ko.md)

## 介绍

在 Linux 环境中，处理压缩文件常常需要记忆 `tar`, `gzip`, `unzip`, `7z` 等一系列工具的不同命令和参数，这对于新手或不经常处理压缩包的用户来说，无疑是个不小的挑战。**`hitpag` 正是为了解决这一痛点而生**。

Hitmux `hitpag` 是一款由 Hitmux 开发的智能且用户友好的命令行工具，它旨在简化 Linux 上的文件压缩和解压操作。它的核心理念是**自动化识别与人性化交互**。无论您是需要解压一个下载的压缩包，还是将本地文件打包分享，`hitpag` 都能根据文件后缀名智能识别文件类型，并自动调用系统底层的相应工具进行处理，省去了您记忆复杂命令的烦恼。此外，它还提供了贴心的**交互模式**，让您在不熟悉命令行参数的情况下，也能通过简单的问答轻松完成操作。

### 在 `/src` 目录下，我们同时提供了 `hitpag` 中文版的源代码！替换mian.cpp文件即可编译。

---

## 编译说明

### 环境要求

* **操作系统**：Linux（已在 Ubuntu 22.04 和 Debian 12 上测试成功）
* **编译器**：支持 C++17 标准的 GCC/G++
* **构建工具**：CMake 3.10 或更高版本
* **依赖库**：`stdc++fs`（C++文件系统库），通常随 GCC/G++ 自动安装

### 编译步骤

1.  **更新系统并安装编译工具**：
    首先，确保您的系统是最新的，并安装了必要的编译工具：

    ```bash
    sudo apt update
    sudo apt install -y g++ cmake make
    ```

2.  **进入项目目录并创建构建文件夹**：
    导航到 `hitpag` 项目的根目录，然后创建并进入 `build` 文件夹：

    ```bash
    cd hitpag
    mkdir -p build
    cd build
    ```

3.  **使用 CMake 生成构建文件**：
    运行 CMake 来配置项目并生成 Makefile：

    ```bash
    cmake ..
    ```

4.  **编译项目**：
    执行 `make` 命令来编译 `hitpag`：

    ```bash
    make
    ```

5.  **（可选）安装到系统**：
    如果您希望 `hitpag` 可以在任何目录下直接运行，可以将其安装到系统路径中：

    ```bash
    sudo make install
    ```

---

## 使用说明

### 基本用法

`hitpag` 工具的基本语法非常简洁：

```
hitpag [选项] 源路径 目标路径
```

### 命令行选项

* `-i`：**交互模式**。当您不确定如何操作时，工具会引导您完成每一步。
* `-h, --help`：显示帮助信息。
* `-v`：显示版本信息。

### 使用示例

1.  **解压文件**：
    `hitpag` 会自动识别压缩包类型并解压。
    ```bash
    hitpag archive.tar.gz ./extracted_dir
    ```
    此命令会将 `archive.tar.gz` 解压到 `./extracted_dir` 目录。

2.  **压缩文件/目录**：
    `hitpag` 会根据目标文件名后缀自动选择压缩格式。
    ```bash
    hitpag ./my_folder my_archive.zip
    ```
    此命令会将 `./my_folder` 目录压缩成 `my_archive.zip` 文件。

3.  **交互模式**：
    在交互模式下，`hitpag` 会提问并等待您的输入。
    ```bash
    hitpag -i big_file.rar
    ```
    您将被引导选择解压操作类型、目标路径等。

### 支持的压缩格式

`hitpag` 通过调用系统现有工具，支持以下主流压缩/解压格式：

* `tar` (无压缩)
* `tar.gz` / `tgz` (gzip 压缩)
* `tar.bz2` / `tbz2` (bzip2 压缩)
* `tar.xz` / `txz` (xz 压缩)
* `zip`
* `rar`
* `7z`

### 注意事项

1.  **确保已安装相应的压缩/解压工具**：
    `hitpag` 依赖于您系统中已安装的压缩工具。请运行以下命令确保它们都已安装：

    ```bash
    sudo apt install -y tar gzip bzip2 xz-utils zip unzip rar unrar p7zip-full
    ```

2.  **自动创建目录**：
    在解压操作时，如果指定的目标目录不存在，`hitpag` 会自动为您创建。


---

## 错误处理

当操作失败时，`hitpag` 会显示清晰的中文错误信息，帮助您诊断问题。常见错误信息包括：

* `错误: 缺少参数`：命令行参数不足。
* `错误: 源路径不存在`：指定的源文件或目录不存在。
* `错误: 目标路径无效`：目标路径格式不正确或无法访问。
* `错误: 无法识别的文件格式`：无法识别指定文件的压缩格式。
* `错误: 未找到所需的工具`：系统中缺少执行操作所需的压缩/解压工具。
* `错误: 操作失败`：压缩/解压操作本身执行失败。
* `错误: 权限被拒绝`：没有足够的权限访问文件或目录。
* `错误: 磁盘空间不足`：磁盘空间不足以完成操作。
