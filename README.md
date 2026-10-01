# VYM Legacy (View Your Mind 1.12.2)

[![License: GPL-2.0](https://img.shields.io/badge/License-GPL%202.0-blue.svg)](LICENSE.txt)
[![Platform: Linux](https://img.shields.io/badge/Platform-Linux%20Mint%20%7C%20Ubuntu-green.svg)]()
[![Toolkit: Qt4](https://img.shields.io/badge/Toolkit-Qt%204.8-orange.svg)]()

**VYM Legacy** is a modernized, standalone packaging of the classic **VYM (View Your Mind) 1.12.2**, fully patched and optimized to compile and run reliably on modern Linux systems, including **Linux Mint 22.3** and **Ubuntu 24.04 LTS (Noble Numbat)**.

To prevent any conflict with modern `vym` packages (version 2.6+) provided by standard distribution repositories, this project is packaged under the name **`vym-legacy`**, allowing both versions to coexist side-by-side with zero collision.

---

## Key Patches & Improvements

1. **GCC 13 & C++17 Standard Compatibility**:
   - Fixed invalid boolean-to-pointer conversions in `editxlinkdialog.cpp` (`xlo = NULL;`).
   - Fixed invalid boolean return in pointer-returning function in `linkablemapobj.cpp` (`return NULL;`).
   - Added `#include <unistd.h>` in `mainwindow.cpp` for POSIX `sleep()` declaration.

2. **Qt4 ELF Copy Relocation Fix (Critical Crash Fix)**:
   - Modern GCC toolchains default to position-independent executables with PC-relative data addressing, causing an ELF `R_X86_64_COPY` relocation on `QCoreApplication::self`.
   - In standard Qt4 libraries, `libQtCore` writes to its internal `self` while `libQtGui` queries the uninitialized copy in the main executable's `.bss`, resulting in an abort: `QPixmap: Must construct a QApplication before a QPaintDevice`.
   - Added `-fPIC` to `vym.pro` compiler flags, enforcing Global Offset Table (`R_X86_64_GLOB_DAT`) resolution across all Qt modules and eliminating the crash completely.

3. **Font Size & High-DPI Display Enhancements**:
   - Automatically increased default UI font from tiny 9pt to readable **11pt**.
   - Added command-line parameter `-fs, --fontsize <pt>` (e.g. `vym-legacy --fontsize 13`).
   - Added environment variable support: `VYM_FONT_SIZE` (for UI) and `VYM_NODE_FONT_SIZE` (for mindmap node hierarchy).
   - Increased default node font hierarchy from 16/12/10 pt to **18/14/12 pt** for modern screens.

4. **Complete Traditional Chinese (繁體中文) Support**:
   - Fixed translation filename typo (`vym-zh_TW.ts` -> `vym_zh_TW.ts`) and compiled `vym_zh_TW.qm` (405 translations).

5. **Distribution & Desktop Integration**:
   - Isolated binary: `/usr/bin/vym-legacy`
   - Isolated resources: `/usr/share/vym-legacy/`
   - Desktop entry: `vym-legacy.desktop` with localized descriptions.
   - High-resolution hicolor icon set (16x16, 48x48, 128x128).
   - MIME database registration for `*.vym` mindmap files.
   - UNIX manual page (`man vym-legacy`).

---

## Prerequisites (Linux Mint 22.3 / Ubuntu 24.04)

Ensure the Qt4 build environment and dependencies are installed:

```bash
sudo apt update
sudo apt install build-essential qt4-qmake qt4-dev-tools libqt4-dev \
                 libqt4-qt3support libqt4-xml libqt4-network libqtgui4 libqtcore4 \
                 zip unzip xsltproc shared-mime-info
```

---

## Building from Source

```bash
# 1. Generate Makefile using Qt4 qmake
qmake-qt4 PREFIX=/usr DOCDIR=/usr/share/doc/vym-legacy vym.pro

# 2. Compile
make -j$(nproc)

# 3. Test run locally
./vym-legacy --version
./vym-legacy --fontsize 13

# 4. Install system-wide (optional)
sudo make install
```

---

## Building Debian (.deb) Package

To create an installable `.deb` package:

```bash
# Run the packaging script from the parent directory:
../package_vym_deb.sh
```

This generates `vym-legacy_1.12.2-1+mint22.3_amd64.deb`.

Install it with:
```bash
sudo dpkg -i vym-legacy_1.12.2-1+mint22.3_amd64.deb
sudo apt-get install -f
```

---

## Usage

```bash
# Launch GUI
vym-legacy

# Launch with custom font size
vym-legacy --fontsize 13

# View options
vym-legacy --help
```

---

## License

This software is released under the GNU General Public License version 2 (GPL-2.0). See [LICENSE.txt](LICENSE.txt) for details.
Original Author: Uwe Drechsel <vym@InSilmaril.de>
