# hitpag - 智能压缩工具

**带 TUI 归档浏览器的 C++17 命令行压缩工具。**

[![GitHub](https://img.shields.io/badge/GitHub-Hitmux/hitpag-blue)](https://github.com/Hitmux/hitpag)
[![Website](https://img.shields.io/badge/Website-hitmux.org-green)](https://hitmux.org)

[English](README.md) | [简体中文](README_zh.md)

---

## 主要特性

- 通过文件签名识别归档格式，不依赖文件扩展名。
- 一条命令覆盖压缩、解压、验证和归档浏览。
- 内置 TUI 归档浏览器，可列表、搜索、预览、提取、编辑和播放音频归档条目。
- 图片条目（PNG、JPEG、GIF、BMP、WebP 等）可直接在预览面板中以真彩色块渲染。
- 支持 tar、gzip、bzip2、xz、zip、7z、rar、lz4、zstd 和 xar。
- 在底层工具支持的范围内，支持 zip、7z 和 rar 的密码解压。
- TUI 基于 FTXUI；仓库内包含 vendored static FTXUI。

---

## 安装

```bash
# Ubuntu/Debian 构建和归档依赖
sudo apt install -y build-essential cmake make tar unrar gzip bzip2 xz-utils zip unzip p7zip-full lz4 zstd

# 可选的音频播放和桌面打开工具
sudo apt install -y mpv ffmpeg pulseaudio-utils alsa-utils xdg-utils

git clone https://github.com/Hitmux/hitpag.git
cd hitpag
mkdir -p build
cd build
cmake ..
make
sudo make install
```

CMake 配置时会解析 FTXUI。hitpag 优先使用兼容的系统静态 FTXUI，找不到时回退到 `third_party/ftxui/`。

```bash
cmake .. -DHITPAG_FORCE_VENDORED_FTXUI=ON
cmake .. -DHITPAG_FORCE_SYSTEM_FTXUI=ON
```

图片预览不需要额外运行时依赖：仓库内置的单头文件 `third_party/stb/stb_image.h`（public domain）负责解码 PNG、JPEG、GIF、BMP、TGA、PSD、PNM 与 WebP 条目。

### 各平台依赖

归档工具按实际使用的格式安装。音频预览会选择第一个可用的播放器：`mpv`、`ffplay`、`termux-media-player`、`paplay`、`aplay` 或 `xdg-open`。

**Arch Linux**

```bash
sudo pacman -S --needed base-devel cmake tar gzip bzip2 xz zip unzip p7zip lz4 zstd unrar
sudo pacman -S --needed mpv ffmpeg alsa-utils xdg-utils
```

**Termux**

```bash
pkg update
pkg install clang cmake make tar gzip bzip2 xz zip unzip p7zip lz4 zstd
pkg install mpv ffmpeg termux-api
```

`unrar` 可能需要额外的 Termux 软件源。只有使用 Android 侧 Termux 集成功能时才需要 `termux-api`。

---

## TUI 归档浏览器

直接打开归档：

```bash
hitpag --tui archive.zip
hitpag archive.tar.gz
```

当只传入一个受支持的归档路径且没有目标路径时，hitpag 会自动进入 TUI。

常用快捷键：

- `Up` / `Down`：移动条目。
- `Right` / `Enter`：打开目录，或进入预览。
- `Left`：返回父目录或文件列表。
- `/`：搜索条目。
- `x`：提取当前条目。
- `a`：使用可用的系统或 Termux 播放器播放当前音频条目。
- `e`：用外部编辑器编辑当前文件。
- `s`：配置编辑器命令。
- `?`：打开帮助。
- `q` 或 `Esc`：退出或关闭当前对话框。

音频预览会依次尝试 `mpv`、`ffplay`、`termux-media-player`、`paplay`、`aplay` 和 `xdg-open`。

图片预览使用真彩色半格字符（`▀`）绘制，并且始终缩放到预览面板范围内，因此会跟随当前终端尺寸自适应，不会溢出或换行。

---

## CLI 用法

```bash
# 解压
hitpag archive.tar.gz ./output/
hitpag backup.zip ./extracted/
hitpag data.7z ./data/

# 压缩
hitpag ./my_folder/ backup.zip
hitpag ./documents/ archive.tar.gz

# 密码
hitpag -pMySecret secure.7z ./sensitive/
hitpag -p encrypted.zip ./output/

# 性能和验证
hitpag -l9 -t8 --benchmark data.tar.xz ./large_files/
hitpag --verify ./documents/ archive.zip
```

---

## 文件过滤

```bash
hitpag --include='*.cpp' --include='*.h' code.7z ./project/
hitpag --exclude='*.tmp' --exclude='node_modules/*' clean.tar.gz ./project/
```

---

## 支持的格式

| 格式 | 压缩 | 解压 | 密码 | 说明 |
|------|------|------|------|------|
| tar, tar.gz, tar.bz2, tar.xz, tar.zst | yes | yes | no | Unix 归档格式 |
| zip | yes | yes | yes | 分卷 zip 解压使用 7z |
| 7z | yes | yes | yes | 高压缩率 |
| rar | no | yes | yes | 仅支持解压 |
| lz4 | yes | yes | no | 单文件压缩 |
| zstd | yes | yes | no | 单文件压缩 |
| xar | yes | yes | no | macOS 归档格式 |

---

## 命令参考

| 选项 | 说明 |
|------|------|
| `-i` | 交互式 CLI 模式 |
| `--tui` | TUI 归档浏览器 |
| `-p[password]` | 密码；省略时交互输入 |
| `-l[1-9]` | 压缩级别 |
| `-t[count]` | 线程数 |
| `--format=TYPE` | 强制指定归档类型 |
| `--verbose` | 输出详细信息 |
| `--benchmark` | 输出性能统计 |
| `--verify` | 验证归档完整性 |
| `--include=PATTERN` | 只包含匹配路径 |
| `--exclude=PATTERN` | 排除匹配路径 |

---

## 问题排查

| 问题 | 处理方式 |
|------|----------|
| 格式无法识别 | 使用 `--format=TYPE` |
| 找不到运行时工具 | 安装对应包，例如 `p7zip-full`、`unrar`、`lz4` 或 `zstd` |
| 分卷 zip 解压失败 | 安装 `p7zip-full` |
| TUI 没有自动打开 | 使用 `hitpag --tui archive.zip` |

---

## 贡献

- [提交问题](https://github.com/Hitmux/hitpag/issues)
- [提交 PR](https://github.com/Hitmux/hitpag/pulls)

## 许可证

[GNU Affero General Public License v3.0](LICENSE)

---

**开发者**: [Hitmux](https://hitmux.org)
