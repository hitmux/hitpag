# hitpag - Intelligent Compression Tool

**A C++17 command-line compression tool with an archive browser TUI.**

[![GitHub](https://img.shields.io/badge/GitHub-Hitmux/hitpag-blue)](https://github.com/Hitmux/hitpag)
[![Website](https://img.shields.io/badge/Website-hitmux.org-green)](https://hitmux.org)

[English](README.md) | [简体中文](README_zh.md)

---

## Highlights

- Detects original formats by extension/signature and probes additional containers with p7zip/7-Zip.
- Uses one command for compression, extraction, verification, and archive browsing.
- Includes a TUI archive browser for listing, searching, previewing, extracting, editing, and playing audio entries.
- Supports tar, gzip, bzip2, xz, zip, 7z, rar, lz4, zstd, and xar.
- Supports password-protected zip, 7z, and rar extraction where the underlying tool supports it.
- Uses FTXUI for the terminal interface; a vendored static FTXUI copy is included.

---

## Install

```bash
# Ubuntu/Debian build and archive dependencies
sudo apt install -y build-essential cmake make tar unrar gzip bzip2 xz-utils zip unzip p7zip-full lz4 zstd

# Optional audio playback and desktop fallback
sudo apt install -y mpv ffmpeg pulseaudio-utils alsa-utils xdg-utils

git clone https://github.com/Hitmux/hitpag.git
cd hitpag
mkdir -p build
cd build
cmake ..
make
sudo make install
```

FTXUI is resolved during CMake configuration. hitpag prefers a compatible system static FTXUI package and falls back to `third_party/ftxui/`.

```bash
cmake .. -DHITPAG_FORCE_VENDORED_FTXUI=ON
cmake .. -DHITPAG_FORCE_SYSTEM_FTXUI=ON
```

### Dependencies by platform

The archive tools are needed at runtime for the formats you use. Audio preview selects the first available player (`mpv`, `ffplay`, `termux-media-player`, `paplay`, `aplay`, or `xdg-open`).

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

`unrar` may require an additional Termux repository. `termux-api` is only needed when using Android-side Termux integrations.

---

## TUI Archive Browser

Open an archive directly in the TUI:

```bash
hitpag --tui archive.zip
hitpag archive.tar.gz
```

When a single supported archive path is provided without a target path, hitpag opens the TUI automatically.

Core TUI actions:

- `Up` / `Down`: move through entries.
- `Right` / `Enter`: open a directory or move into preview.
- `Left`: return to the parent directory or file list.
- `/`: search entries.
- `x`: extract the selected entry.
- `a`: play the selected audio entry with an available system or Termux player.
- `e`: edit the selected file through an external editor.
- `s`: configure the editor command.
- `?`: open help.
- `q` or `Esc`: quit or close the current dialog.

Audio preview uses the first available player from `mpv`, `ffplay`, `termux-media-player`, `paplay`, `aplay`, and `xdg-open`.

---

## CLI Usage

```bash
# Extract
hitpag archive.tar.gz ./output/
hitpag backup.zip ./extracted/
hitpag data.7z ./data/

# Compress
hitpag ./my_folder/ backup.zip
hitpag ./documents/ archive.tar.gz

# Password
hitpag -pMySecret secure.7z ./sensitive/
hitpag -p encrypted.zip ./output/

# Performance and verification
hitpag -l9 -t8 --benchmark data.tar.xz ./large_files/
hitpag --verify ./documents/ archive.zip
```

---

## Filtering

```bash
hitpag --include='*.cpp' --include='*.h' code.7z ./project/
hitpag --exclude='*.tmp' --exclude='node_modules/*' clean.tar.gz ./project/
```

---

## Supported Formats

| Format | Compress | Extract | Password | Notes |
|--------|----------|---------|----------|-------|
| tar, tar.gz, tar.bz2, tar.xz, tar.zst | yes | yes | no | Unix archive formats |
| zip | yes | yes | yes | Split zip extraction uses 7z |
| 7z | yes | yes | yes | High compression ratio |
| rar | no | yes | yes | Extraction only |
| lz4 | yes | yes | no | Single-file compression |
| zstd | yes | yes | no | Single-file compression |
| xar | yes | yes | no | macOS archive format |


The original rows above keep their existing compression/extraction backends. The additional formats below use **p7zip/7-Zip** (`7z`, then `7zz`, then `7za` on PATH). `7za` supports fewer formats; a full `7z`/`7zz` installation is recommended. “yes*” means that the installed backend must include the corresponding reader/codec; it does not promise support for every variant.

| Additional format / extensions | Compress | Extract / TUI | Password | Notes |
|--------------------------------|----------|---------------|----------|-------|
| gzip (`gz`, `gzip`) | yes* | yes* | no | One regular file, not a directory; distinct from tar.gz |
| bzip2 (`bz2`, `bzip2`) | yes* | yes* | no | One regular file; distinct from tar.bz2 |
| xz | yes* | yes* | no | One regular file; distinct from tar.xz |
| WIM (`wim`) | yes* | yes* | no | Files/directories; codec support varies |
| WIM variants (`swm`, `esd`, `ppkg`) | no | yes* | no | Does not create split WIM or installation packages |
| ZIP containers (`zipx`, `jar`, `xpi`, `odt`, `ods`, `docx`, `xlsx`, `pptx`, `epub`, `ipa`, `apk`, `appx`, `msix`) | yes* | yes* | yes* | Creates a ZIP container only; no application packaging, manifests, or signing |
| OVA (`ova`) | yes* | yes* | no | Creates a TAR container only; no virtual appliance metadata |
| Lizard (`liz`), LZ5 (`lz5`) | yes* | yes* | no | Optional p7zip codecs; one regular file |
| compressed SWF (`swf`, `--format=swfc`) | yes* | yes* | no | Creation requires one valid uncompressed SWF file |
| AR (`ar`, `a`, `deb`, `lib`) | no | yes* | no | Includes Debian package containers; nested archives need a second extraction |
| ARJ (`arj`) | no | yes* | backend-dependent | Reader capabilities apply |
| CAB (`cab`) | no | yes* | no | Cabinet archives |
| CHM / HXS (`chm`, `chi`, `chq`, `chw`, `hxs`, `hxi`, `hxr`, `hxq`, `hxw`, `lit`) | no | yes* | no | Help/document containers |
| Compound (`msi`, `msp`, `doc`, `xls`, `ppt`) | no | yes* | no | Lists embedded streams, not decrypted document contents |
| CPIO (`cpio`), RPM (`rpm`) | no | yes* | no | Package/archive readers |
| DMG (`dmg`) | no | yes* | backend-dependent | Supported disk image variants only |
| ISO / UDF (`iso`, `udf`, `img`) | no | yes* | no | IMG format is detected by the backend |
| APFS, APM, GPT, MBR (`apfs`, `apm`, `gpt`, `mbr`) | no | yes* | no | APFS requires a newer backend |
| Ext, FAT, HFS, NTFS (`ext`, `ext2`, `ext3`, `ext4`, `fat`, `hfs`, `hfsx`, `ntfs`, `img`) | no | yes* | no | Filesystem images; no mounting |
| CramFS / SquashFS (`cramfs`, `squashfs`) | no | yes* | no | Filesystem images |
| QCOW / VDI / VHD / VHDX / VMDK (`qcow`, `qcow2`, `qcow2c`, `vdi`, `vhd`, `vhdx`, `vmdk`) | no | yes* | no | VHDX requires a backend with its reader |
| LZH (`lzh`, `lha`) | no | yes* | backend-dependent | Reader capabilities apply |
| LZMA / LZMA86 / PPMd / lzip (`lzma`, `lzma86`, `pmd`, `lz`) | no | yes* | no | Standalone compressed streams |
| Unix compress / MsLZ (`z`, `mslz`) | no | yes* | no | Standalone compressed streams |
| RAR first volume (`r00`) | no | yes* | yes* | Keep all volumes together; normal `.rar` keeps its original backend |
| Split (`001`) | no | yes* | inherited | Open the first volume with all parts in one directory |
| PE / NSIS / SFX (`exe`, `dll`, `sys`, `nsis`) | no | yes* | backend-dependent | Inspects supported sections/payloads; never runs the executable |
| ELF / Mach-O / TE / Mub (`elf`, `macho`, `te`, `mub`) | no | yes* | no | Executable containers/sections |
| FLV / SWF (`flv`, uncompressed `swf`) | no | yes* | no | Container contents, not media playback/transcoding |
| IHex / UEFI (`ihex`, `scap`, `uefif`) | no | yes* | no | Supported firmware containers |
| XAR aliases (`pkg`, `xip`) | no | yes* | no | Normal `.xar` keeps its original backend |
| Compressed TAR aliases (`tpz`, `taz`, `tliz`, `tlz`, `tlz4`, `tlz5`, `tzstd`) | no | yes* | no | Extracts the outer stream; open the resulting TAR separately |

Unknown extensions and extensionless files are also probed with the installed backend, so additional readers can work without adding another extension here. Known original formats keep their original dispatch. An EXE filename alone does not guarantee readable contents: proprietary/encrypted installers or BIOS packages may remain unsupported. Empty/corrupt archives and unsupported codecs can fail; hitpag does not run installers to unpack them. The general upstream format list is available on the [7-Zip website](https://www.7-zip.org/); run `7z i` (or `7zz i`) to inspect your installed build.

For added formats, one archive argument opens the TUI, a directory target extracts, and an archive-extension target creates a new archive. Single-file streams require exactly one regular input file; archive a directory as TAR first. TUI editing does **not** write changes back into the added formats. Names absent from stream metadata are synthesized for browsing/selected extraction; full CLI extraction uses the backend's naming.

```bash
hitpag BIOS-8JCN56WW.exe          # Browse if the backend can read this EXE
hitpag disk.iso extracted        # Extract into a directory
hitpag document.txt document.xz  # Create a single-file XZ stream
hitpag folder backup.wim         # Create a WIM archive
hitpag movie.swf compressed.swf  # Compress a valid uncompressed SWF
hitpag --format=gzip input.txt output.gz
```

---

## Command Reference

| Option | Description |
|--------|-------------|
| `-i` | Interactive CLI mode |
| `--tui` | TUI archive browser |
| `-p[password]` | Password; prompts when omitted |
| `-l[1-9]` | Compression level |
| `-t[count]` | Thread count |
| `--format=TYPE` | Force archive type |
| `--verbose` | Detailed output |
| `--benchmark` | Performance statistics |
| `--verify` | Verify archive integrity |
| `--include=PATTERN` | Include matching paths |
| `--exclude=PATTERN` | Exclude matching paths |

---

## Troubleshooting

| Problem | Solution |
|---------|----------|
| Format not recognized | Use `--format=TYPE` |
| Runtime tool not found | Install the corresponding package, such as `p7zip-full`, `unrar`, `lz4`, or `zstd` |
| Split zip extraction fails | Install `p7zip-full` |
| TUI does not open automatically | Use `hitpag --tui archive.zip` |

---

## Contributing

- [Issues](https://github.com/Hitmux/hitpag/issues)
- [Pull Requests](https://github.com/Hitmux/hitpag/pulls)

## License

[GNU Affero General Public License v3.0](LICENSE)

---

**Developer**: [Hitmux](https://hitmux.org)
