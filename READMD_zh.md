# Hirmux `hitpag` 2.0 - 高级压缩工具

## [Hitmux 官方网站 https://hitmux.top](https://hitmux.top)

[English](README.md)

## 简介

**`hitpag` 2.0** 是这款智能命令行压缩工具的一次重大升级。在**自动识别和用户友好交互**的核心理念基础上，2.0 版本引入了强大的新功能，包括压缩级别控制、多线程支持、文件过滤、性能基准测试以及对现代压缩格式的支持。

无论您是需要使用最佳设置压缩大型数据集、过滤特定文件类型，还是对压缩性能进行基准测试，`hitpag` 2.0 都能提供企业级功能，同时保持其标志性的易用性。

---

## 编译说明

### 环境要求

*   **操作系统**: Linux (已在 Ubuntu 22.04 和 Debian 12 上测试)
*   **编译器**: 支持 C++17 标准的 GCC/G++
*   **构建工具**: CMake 3.10 或更高版本
*   **依赖项**: `stdc++fs`, `pthread` (C++ 文件系统和线程库)

### 编译步骤

1.  **更新系统并安装构建工具**:
    ```bash
    sudo apt update
    sudo apt install -y g++ cmake make
    ```

2.  **进入项目目录并构建**:
    ```bash
    cd hitpag
    mkdir -p build
    cd build
    cmake ..
    make
    ```

3.  **（可选）安装到系统**:
    ```bash
    sudo make install
    ```

### 系统依赖

安装所有支持的压缩工具：

```bash
# 基础压缩工具
sudo apt install -y tar gzip bzip2 xz-utils zip unzip rar unrar p7zip-full

# 现代压缩格式（可选）
sudo apt install -y lz4 zstd xar
```

---

## 使用说明

### 基本语法

```bash
hitpag [选项] 源路径 目标路径
```

### 新增命令行选项（2.0 版本）

*   **`-i`**: 交互模式，提供增强的格式选择
*   **`-p[密码]`**: 密码加密/解密
*   **`-l[级别]`**: 压缩级别（1-9，默认值取决于格式）
*   **`-t[线程数]`**: 并行处理的线程数（若未指定则自动检测）
*   **`--verbose`**: 显示详细的进度和操作信息
*   **`--benchmark`**: 显示性能统计信息，包括压缩率和耗时
*   **`--verify`**: 压缩后进行归档完整性验证
*   **`--exclude=PATTERN`**: 排除匹配正则表达式模式的文件/目录
*   **`--include=PATTERN`**: 仅包含匹配正则表达式模式的文件/目录
*   **`-h, --help`**: 显示全面的帮助信息
*   **`-v, --version`**: 显示版本信息
*   **注意**：`hitpag xxx/ file.zip` 和 `hitpag xxx file.zip` 之间存在差异，这应该不难理解。同理，`hitpag file.zip xxx/` 和 `hitpag file.zip xxx` 也是如此。

### 高级用法示例

1.  **使用最佳设置进行高性能压缩**:
    ```bash
    hitpag -l9 -t4 --benchmark large_dataset.tar.gz ./data/
    ```
    使用最高压缩级别和 4 个线程进行压缩，并显示性能统计信息。

2.  **通过文件类型选择进行过滤压缩**:
    ```bash
    hitpag --include='*.cpp' --include='*.h' --exclude='build/*' source_code.7z ./project/
    ```
    仅压缩 C++ 源文件，并排除构建目录。

3.  **带验证的加密压缩**:
    ```bash
    hitpag -pmysecret --verify --verbose confidential.zip ./sensitive_data/
    ```
    使用密码加密，并进行完整性验证和详细输出。

4.  **使用现代压缩格式**:
    ```bash
    hitpag -l6 -t8 fast_archive.lz4 ./temp_files/
    hitpag -l9 small_archive.zstd ./documents/
    ```
    使用 LZ4 追求速度，或使用 Zstandard 实现最佳压缩。

5.  **带性能监控的批量处理**:
    ```bash
    hitpag --benchmark --exclude='*.tmp' --exclude='*.log' backup.tar.xz ./work_folder/
    ```
    创建压缩备份，排除临时文件，并进行性能分析。

### 支持的压缩格式

`hitpag` 2.0 支持所有传统格式以及现代压缩算法：

**传统格式：**
*   `tar` (未压缩)
*   `tar.gz` / `tgz` (gzip 压缩)
*   `tar.bz2` / `tbz2` (bzip2 压缩)
*   `tar.xz` / `txz` (xz 压缩)
*   `zip` (支持密码和压缩级别)
*   `rar` (仅支持解压)
*   `7z` (支持密码和压缩级别)

**现代格式（2.0 新增）：**
*   `lz4` (超快速压缩/解压)
*   `zst` / `zstd` (Facebook 的 Zstandard - 卓越的压缩/速度平衡)
*   `xar` (macOS 归档格式)

### 性能特性

#### 多线程支持
自动检测 CPU 核心数，并为支持的格式利用并行处理：
```bash
hitpag -t8 archive.tar.gz ./large_directory/  # 明确使用 8 个线程
hitpag -t archive.tar.gz ./large_directory/   # 自动检测最佳线程数
```

#### 压缩级别控制
微调压缩率与速度之间的平衡：
```bash
hitpag -l1 fast.zip ./files/      # 最快压缩
hitpag -l6 balanced.7z ./files/   # 平衡（多数格式的默认值）
hitpag -l9 smallest.tar.xz ./files/ # 最大压缩
```

#### 文件过滤
通过高级模式匹配进行选择性压缩：```bash
# 仅包含特定文件类型
hitpag --include='*.jpg' --include='*.png' photos.zip ./images/

# 排除临时文件和构建文件
hitpag --exclude='*.tmp' --exclude='node_modules/*' --exclude='build/*' clean_backup.tar.gz ./project/

# 复杂的过滤组合
hitpag --include='src/*' --exclude='*.o' --exclude='*.obj' source_only.7z ./project/
```

#### 性能基准测试
全面的性能分析：
```bash
hitpag --benchmark --verbose optimized.tar.xz ./data/
```
输出内容：
- 压缩时间
- 原始大小与压缩后大小
- 压缩率百分比
- 线程利用率信息

---

## 错误处理

增强的错误报告，提供详细的诊断信息：

*   **`Error: Compression level must be between 1-9`**: 错误：压缩级别必须在 1-9 之间
*   **`Error: Thread count must be positive`**: 错误：线程数必须为正数
*   **`Error: Required tool not found: lz4`**: 错误：未找到所需的工具：lz4
*   **`Filtering files: included 150, excluded 45`**: 文件过滤摘要：包含 150 个，排除 45 个
*   **`Archive verification failed`**: 归档验证失败

---

## 2.0 版本更新日志

### 新功能
- **多线程支持**，实现并行压缩/解压
- **压缩级别控制** (1-9)，适用于所有支持的格式
- **文件过滤系统**，支持正则表达式模式匹配
- **性能基准测试**，提供详细统计数据
- **归档验证**，用于完整性检查
- **现代压缩格式**：LZ4, Zstandard, XAR
- **增强的详细模式**，提供详细的进度信息

### 改进
- **采用有组织命名空间的模块化架构**
- **更完善的错误处理**，提供具体的错误代码
- **国际化消息系统**，便于本地化
- **增强的交互模式**，提供新的格式选项
- **改进的帮助系统**，包含全面的示例

### 技术增强
- **使用 `std::thread` 实现 C++17 线程支持**
- **使用 `std::regex` 实现基于正则表达式的过滤**
- **使用 `std::chrono` 进行性能监控**
- **针对大文件的高效内存处理**
- **跨平台兼容性改进**

---

## 从 1.x 版本迁移

2.0 版本完全向后兼容 1.x 版本的命令语法。所有现有的脚本和工作流程将继续正常工作，同时可以逐步采用新功能。

```bash
# 1.x 版本的语法仍然有效
hitpag archive.tar.gz ./extracted_dir

# 增强的 2.0 语法增加了强大的选项
hitpag --benchmark --verbose -l9 -t4 archive.tar.gz ./extracted_dir
```
