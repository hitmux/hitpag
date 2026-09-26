# hitpag - 智能压缩工具

**带 TUI 归档浏览器的 C++17 命令行压缩工具。**

[![GitHub](https://img.shields.io/badge/GitHub-Hitmux/hitpag-blue)](https://github.com/Hitmux/hitpag)
[![Website](https://img.shields.io/badge/Website-hitmux.org-green)](https://hitmux.org)

[English](README.md) | [简体中文](README_zh.md)

---

## 主要特性

- 通过扩展名、文件签名识别已有格式，并使用 p7zip/7-Zip 探测更多归档和容器。
- 一条命令覆盖压缩、解压、验证和归档浏览。
- 内置 TUI 归档浏览器，可列表、搜索、预览、提取、编辑和播放音频归档条目。
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


上表已有格式继续使用原来的压缩、读取和解压后端。下表新增格式使用 **p7zip/7-Zip**，依次查找 PATH 中的 `7z`、`7zz`、`7za`。`7za` 支持的格式较少，建议安装完整的 `7z` 或 `7zz`。`yes*` 表示需要本机后端包含对应的读取器或编解码器，不代表兼容该格式的所有变体。

| 新增格式 / 扩展名 | 压缩 | 解压 / TUI | 密码 | 说明 |
|------------------|------|------------|------|------|
| gzip (`gz`, `gzip`) | yes* | yes* | no | 仅一个普通文件，不能直接压缩目录；区别于 tar.gz |
| bzip2 (`bz2`, `bzip2`) | yes* | yes* | no | 仅一个普通文件；区别于 tar.bz2 |
| xz | yes* | yes* | no | 仅一个普通文件；区别于 tar.xz |
| WIM (`wim`) | yes* | yes* | no | 支持文件、目录；编解码器取决于后端 |
| WIM 变体 (`swm`, `esd`, `ppkg`) | no | yes* | no | 不创建分卷 WIM 或安装包 |
| ZIP 容器 (`zipx`, `jar`, `xpi`, `odt`, `ods`, `docx`, `xlsx`, `pptx`, `epub`, `ipa`, `apk`, `appx`, `msix`) | yes* | yes* | yes* | 仅创建 ZIP 容器，不生成应用清单、签名或完整应用包 |
| OVA (`ova`) | yes* | yes* | no | 仅创建 TAR 容器，不生成虚拟机描述信息 |
| Lizard (`liz`)、LZ5 (`lz5`) | yes* | yes* | no | p7zip 可选编解码器；仅一个普通文件 |
| 压缩 SWF (`swf`、`--format=swfc`) | yes* | yes* | no | 创建时必须输入一个有效的未压缩 SWF 文件 |
| AR (`ar`, `a`, `deb`, `lib`) | no | yes* | no | 包括 Debian 包容器；嵌套归档需要再次解压 |
| ARJ (`arj`) | no | yes* | 取决于后端 | 以读取器能力为准 |
| CAB (`cab`) | no | yes* | no | Cabinet 归档 |
| CHM / HXS (`chm`, `chi`, `chq`, `chw`, `hxs`, `hxi`, `hxr`, `hxq`, `hxw`, `lit`) | no | yes* | no | 帮助文件、文档容器 |
| Compound (`msi`, `msp`, `doc`, `xls`, `ppt`) | no | yes* | no | 列出内部数据流，不解密文档内容 |
| CPIO (`cpio`)、RPM (`rpm`) | no | yes* | no | 归档、软件包读取 |
| DMG (`dmg`) | no | yes* | 取决于后端 | 仅支持后端可读取的磁盘映像变体 |
| ISO / UDF (`iso`, `udf`, `img`) | no | yes* | no | IMG 的实际格式交给后端识别 |
| APFS、APM、GPT、MBR (`apfs`, `apm`, `gpt`, `mbr`) | no | yes* | no | APFS 需要较新的后端 |
| Ext、FAT、HFS、NTFS (`ext`, `ext2`, `ext3`, `ext4`, `fat`, `hfs`, `hfsx`, `ntfs`, `img`) | no | yes* | no | 读取文件系统映像，不挂载映像 |
| CramFS / SquashFS (`cramfs`, `squashfs`) | no | yes* | no | 文件系统映像 |
| QCOW / VDI / VHD / VHDX / VMDK (`qcow`, `qcow2`, `qcow2c`, `vdi`, `vhd`, `vhdx`, `vmdk`) | no | yes* | no | VHDX 需要后端包含相应读取器 |
| LZH (`lzh`, `lha`) | no | yes* | 取决于后端 | 以读取器能力为准 |
| LZMA / LZMA86 / PPMd / lzip (`lzma`, `lzma86`, `pmd`, `lz`) | no | yes* | no | 独立压缩流 |
| Unix compress / MsLZ (`z`, `mslz`) | no | yes* | no | 独立压缩流 |
| RAR 首分卷 (`r00`) | no | yes* | yes* | 所有分卷放在同一目录；普通 `.rar` 保留原后端 |
| Split (`001`) | no | yes* | 继承内部格式 | 从首分卷打开，所有分卷放在同一目录 |
| PE / NSIS / SFX (`exe`, `dll`, `sys`, `nsis`) | no | yes* | 取决于后端 | 读取受支持的节区或内嵌归档，绝不运行可执行文件 |
| ELF / Mach-O / TE / Mub (`elf`, `macho`, `te`, `mub`) | no | yes* | no | 可执行文件容器、节区 |
| FLV / SWF (`flv`、未压缩 `swf`) | no | yes* | no | 读取容器内容，不是媒体转码功能 |
| IHex / UEFI (`ihex`, `scap`, `uefif`) | no | yes* | no | 后端支持的固件容器 |
| XAR 别名 (`pkg`, `xip`) | no | yes* | no | 普通 `.xar` 保留原后端 |
| 压缩 TAR 别名 (`tpz`, `taz`, `tliz`, `tlz`, `tlz4`, `tlz5`, `tzstd`) | no | yes* | no | 先解开外层压缩流，再单独打开生成的 TAR |

未知扩展名和无扩展名文件也会交给后端探测，因此后端新增的读取器不一定需要在此继续添加扩展名。已有格式保留原来的分派方式。文件名为 EXE 不代表一定能够解包：专有或加密安装程序、BIOS 包仍可能不受支持。空归档、损坏归档或缺少编解码器可能导致操作失败；hitpag 不会通过运行安装程序来解包。通用格式列表可参考 [7-Zip 官方网站](https://www.7-zip.org/)，本机实际能力请执行 `7z i` 或 `7zz i` 查看。

新增格式传入一个归档路径时打开 TUI，目标为目录时解压，目标带归档扩展名时创建新归档。单文件压缩流只接受一个普通文件；多个文件或目录请先打包为 TAR。TUI 编辑功能**不会回写新增格式**。没有存储文件名的压缩流会在浏览和单项提取时生成显示名称；CLI 完整解压使用后端的命名规则。

```bash
hitpag BIOS-8JCN56WW.exe          # 后端可读取此 EXE 时进入浏览界面
hitpag disk.iso extracted        # 解压到目录
hitpag document.txt document.xz  # 创建单文件 XZ 压缩流
hitpag folder backup.wim         # 创建 WIM 归档
hitpag movie.swf compressed.swf  # 压缩有效的未压缩 SWF
hitpag --format=gzip input.txt output.gz
```

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
