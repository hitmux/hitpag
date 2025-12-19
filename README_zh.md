# hitpag - 智能压缩工具

**智能、强大、易用的命令行压缩工具**

[![GitHub](https://img.shields.io/badge/GitHub-Hitmux/hitpag-blue)](https://github.com/Hitmux/hitpag)
[![Website](https://img.shields.io/badge/Website-hitmux.org-green)](https://hitmux.org)

[English](README.md) | [简体中文](README_zh.md)

---

## 为什么选择 hitpag？

- **🧠 智能识别** - 通过文件头自动检测格式，不依赖扩展名
- **⚡ 一条命令** - 无需记忆不同格式对应的不同工具
- **📦 全格式支持** - tar, gzip, bzip2, xz, zip, 7z, rar, lz4, zstd, xar
- **🔐 密码保护** - 支持 zip 和 7z 格式加密
- **🚀 多线程** - 并行压缩，性能更强

---

## 快速开始

### 安装
```bash
# Ubuntu/Debian
sudo apt install -y tar gzip bzip2 xz-utils zip unzip p7zip-full lz4 zstd

# 构建
git clone https://github.com/Hitmux/hitpag.git
cd hitpag && mkdir build && cd build && cmake .. && make
sudo make install  # 可选
```

### 基本用法
```bash
# 解压 - 直接指向压缩包
hitpag archive.tar.gz ./output/
hitpag backup.zip ./extracted/
hitpag data.7z ./data/

# 压缩 - 指定源和目标
hitpag ./my_folder/ backup.zip
hitpag ./documents/ archive.tar.gz

# 带密码
hitpag -pMySecret secure.7z ./sensitive/
hitpag -p encrypted.zip ./output/  # 交互式输入密码
```

就这么简单！hitpag 会自动处理其余的事情。

---

## 高级用法

### 性能选项
```bash
# 多线程压缩并显示性能统计
hitpag -l9 -t8 --benchmark data.tar.xz ./large_files/

# 超快压缩 (LZ4)
hitpag --format=lz4 temp.lz4 ./temp_data/

# 高效压缩 (Zstandard)
hitpag --format=zstd archive.zstd ./documents/
```

### 文件过滤
```bash
# 只包含特定文件
hitpag --include='*.cpp' --include='*.h' code.7z ./project/

# 排除文件
hitpag --exclude='*.tmp' --exclude='node_modules/*' clean.tar.gz ./project/
```

### 其他选项
```bash
hitpag -i                    # 交互模式
hitpag --verbose archive.7z  # 详细输出
hitpag --verify data.tar.gz  # 压缩后验证
hitpag --format=rar unknown  # 强制指定格式
```

---

## 支持的格式

| 格式 | 压缩 | 解压 | 密码 | 说明 |
|------|------|------|------|------|
| tar, tar.gz, tar.bz2, tar.xz | ✅ | ✅ | ❌ | 经典 Unix 格式 |
| zip | ✅ | ✅ | ✅ | 支持分卷压缩包 (.z01, .z02, ...) |
| 7z | ✅ | ✅ | ✅ | 最高压缩率 |
| rar | ❌ | ✅ | ✅ | 仅支持解压 |
| lz4 | ✅ | ✅ | ❌ | 超快速度 |
| zstd | ✅ | ✅ | ❌ | 速度/压缩率最佳平衡 |
| xar | ✅ | ✅ | ❌ | macOS 原生格式 |

---

## 命令参考

| 选项 | 说明 |
|------|------|
| `-i` | 交互模式 |
| `-p[password]` | 密码（不提供则交互输入） |
| `-l[1-9]` | 压缩级别 |
| `-t[count]` | 线程数 |
| `--format=TYPE` | 强制指定格式 |
| `--verbose` | 详细输出 |
| `--benchmark` | 性能统计 |
| `--verify` | 完整性验证 |
| `--include=PATTERN` | 包含文件 |
| `--exclude=PATTERN` | 排除文件 |

---

## 问题排查

| 问题 | 解决方案 |
|------|----------|
| 格式无法识别 | 使用 `--format=TYPE` 指定 |
| 权限被拒绝 | 检查文件/目录权限 |
| 找不到工具 | 安装对应工具 (p7zip-full, unrar 等) |
| 分卷 ZIP 失败 | 安装 p7zip-full (`sudo apt install p7zip-full`) |

---

## 贡献

- 📝 [提交问题](https://github.com/Hitmux/hitpag/issues)
- 🔧 [提交 PR](https://github.com/Hitmux/hitpag/pulls)

## 许可证

[GNU Affero General Public License v3.0](LICENSE)

---

**开发者**: [Hitmux](https://hitmux.top)
