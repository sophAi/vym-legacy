#!/usr/bin/env bash
# ==============================================================================
# Script: build_deb.sh
# Project: VYM Legacy (View Your Mind 1.12.2)
# Description: Automatically compiles and packages vym-legacy into a Debian (.deb)
#              package for Debian / Ubuntu / Linux Mint systems.
#              Designed to run smoothly across OS upgrades.
# ==============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

PACKAGE_NAME="vym-legacy"
RAW_VERSION="1.12.2"

# --- 1. Detect OS Distribution, Codename and Architecture ---
DIST_TAG="linux"
if [ -f /etc/os-release ]; then
    # shellcheck disable=SC1091
    . /etc/os-release
    if [ "${ID:-}" = "linuxmint" ] && [ -n "${VERSION_ID:-}" ]; then
        DIST_TAG="mint${VERSION_ID}"
    elif [ -n "${UBUNTU_CODENAME:-}" ]; then
        DIST_TAG="${UBUNTU_CODENAME}"
    elif [ -n "${VERSION_CODENAME:-}" ]; then
        DIST_TAG="${VERSION_CODENAME}"
    elif [ -n "${VERSION_ID:-}" ]; then
        DIST_TAG="${ID:-os}${VERSION_ID}"
    fi
fi

ARCH="$(dpkg --print-architecture 2>/dev/null || uname -m | sed 's/x86_64/amd64/')"
VERSION="${RAW_VERSION}-1+${DIST_TAG}"
OUTPUT_DEB="${1:-${SCRIPT_DIR}/${PACKAGE_NAME}_${VERSION}_${ARCH}.deb}"
BUILD_WORKSPACE="${SCRIPT_DIR}/build_deb_workspace"
STAGING_DIR="${BUILD_WORKSPACE}/staging"

echo "=========================================================="
echo "  VYM Legacy (${RAW_VERSION}) Debian (.deb) Package Builder"
echo "  Target OS     : ${PRETTY_NAME:-Linux} (Tag: ${DIST_TAG})"
echo "  Architecture  : ${ARCH}"
echo "  Package Name  : ${PACKAGE_NAME}"
echo "  Version       : ${VERSION}"
echo "  Output File   : ${OUTPUT_DEB}"
echo "=========================================================="

# --- 2. Check Build Prerequisites ---
echo "[1/6] Checking build prerequisites..."

REQUIRED_COMMANDS=(
    "qmake-qt4"
    "make"
    "g++"
    "dpkg-deb"
    "dpkg-shlibdeps"
    "lrelease-qt4"
    "gzip"
    "strip"
)

MISSING_COMMANDS=()
for cmd in "${REQUIRED_COMMANDS[@]}"; do
    if ! command -v "$cmd" >/dev/null 2>&1; then
        MISSING_COMMANDS+=("$cmd")
    fi
done

REQUIRED_PKGS=(
    "build-essential"
    "dpkg-dev"
    "qt4-qmake"
    "qt4-linguist-tools"
    "libqt4-dev"
    "libqt4-qt3support"
    "libqt4-xml"
    "libqt4-network"
    "libqtgui4"
    "libqtcore4"
    "zip"
    "unzip"
    "xsltproc"
    "shared-mime-info"
)

MISSING_PKGS=()
for pkg in "${REQUIRED_PKGS[@]}"; do
    if ! dpkg -s "$pkg" 2>/dev/null | grep -q "Status: install ok installed"; then
        MISSING_PKGS+=("$pkg")
    fi
done

if [ ${#MISSING_COMMANDS[@]} -gt 0 ] || [ ${#MISSING_PKGS[@]} -gt 0 ]; then
    echo "ERROR: Missing required build tools or packages:"
    if [ ${#MISSING_COMMANDS[@]} -gt 0 ]; then
        echo "  Missing commands: ${MISSING_COMMANDS[*]}"
    fi
    if [ ${#MISSING_PKGS[@]} -gt 0 ]; then
        echo "  Missing packages: ${MISSING_PKGS[*]}"
    fi
    echo ""
    echo "To install all required packages on Ubuntu / Linux Mint:"
    echo "  1. Add Qt4 PPA (if Qt4 is not provided by your distro repositories):"
    echo "     sudo add-apt-repository -y ppa:rock-core/qt4"
    echo "  2. Update and install packages:"
    echo "     sudo apt update"
    echo "     sudo apt install -y build-essential dpkg-dev qt4-qmake qt4-linguist-tools \\"
    echo "                         libqt4-dev libqt4-qt3support libqt4-xml libqt4-network \\"
    echo "                         libqtgui4 libqtcore4 zip unzip xsltproc shared-mime-info"
    exit 1
fi

echo "All build tools and dependencies verified."

# --- 3. Build Translations ---
echo "[2/6] Compiling translations (*.ts -> *.qm)..."
if [ -f "lang/vym-zh_TW.ts" ] && [ ! -f "lang/vym_zh_TW.ts" ]; then
    cp "lang/vym-zh_TW.ts" "lang/vym_zh_TW.ts"
fi
for ts in lang/*.ts; do
    [ -f "$ts" ] || continue
    lrelease-qt4 "$ts" >/dev/null 2>&1 || true
done

# --- 4. Configure & Compile ---
echo "[3/6] Configuring and compiling VYM Legacy ${RAW_VERSION}..."
qmake-qt4 PREFIX=/usr DOCDIR=/usr/share/doc/${PACKAGE_NAME} vym.pro
make -j"$(nproc)"

if [ ! -f "vym-legacy" ]; then
    echo "ERROR: Compilation failed, 'vym-legacy' binary not found."
    exit 1
fi

# --- 5. Prepare Staging Directory & Install Files ---
echo "[4/6] Installing files to staging directory..."
rm -rf "$BUILD_WORKSPACE"
mkdir -p "$STAGING_DIR"

make install INSTALL_ROOT="$STAGING_DIR"

# Install Desktop Entry
mkdir -p "$STAGING_DIR/usr/share/applications"
cat << 'EOF' > "$STAGING_DIR/usr/share/applications/vym-legacy.desktop"
[Desktop Entry]
Type=Application
Name=VYM Legacy
GenericName=Mind Mapping Tool (Legacy 1.12.2)
GenericName[zh_TW]=心智圖工具 (舊版 1.12.2)
GenericName[zh_CN]=思维导图工具 (旧版 1.12.2)
GenericName[de]=Mindmapping-Werkzeug (Legacy 1.12.2)
Comment=View Your Mind - mindmapping software (v1.12.2)
Comment[zh_TW]=腦力激盪與心智圖繪製工具 (v1.12.2 經典版)
Icon=vym-legacy
Exec=vym-legacy %F
Terminal=false
MimeType=application/x-vym;
Categories=Qt;KDE;Office;Graphics;
StartupNotify=true
EOF
chmod 644 "$STAGING_DIR/usr/share/applications/vym-legacy.desktop"

# Install Application Icons
mkdir -p "$STAGING_DIR/usr/share/pixmaps"
cp "icons/vym.png" "$STAGING_DIR/usr/share/pixmaps/vym-legacy.png"

mkdir -p "$STAGING_DIR/usr/share/icons/hicolor/16x16/apps"
cp "icons/vym-16x16.png" "$STAGING_DIR/usr/share/icons/hicolor/16x16/apps/vym-legacy.png"

mkdir -p "$STAGING_DIR/usr/share/icons/hicolor/48x48/apps"
cp "icons/vym.png" "$STAGING_DIR/usr/share/icons/hicolor/48x48/apps/vym-legacy.png"

mkdir -p "$STAGING_DIR/usr/share/icons/hicolor/128x128/apps"
cp "icons/vym-128x128.png" "$STAGING_DIR/usr/share/icons/hicolor/128x128/apps/vym-legacy.png"

# Install MIME Type Definition
mkdir -p "$STAGING_DIR/usr/share/mime/packages"
cat << 'EOF' > "$STAGING_DIR/usr/share/mime/packages/vym-legacy.xml"
<?xml version="1.0" encoding="UTF-8"?>
<mime-info xmlns="http://www.freedesktop.org/standards/shared-mime-info">
  <mime-type type="application/x-vym">
    <comment>VYM Mindmap</comment>
    <comment xml:lang="zh_TW">VYM 心智圖檔案</comment>
    <glob pattern="*.vym"/>
    <icon name="vym-legacy"/>
  </mime-type>
</mime-info>
EOF
chmod 644 "$STAGING_DIR/usr/share/mime/packages/vym-legacy.xml"

# Install UNIX Manual Page
mkdir -p "$STAGING_DIR/usr/share/man/man1"
cat << 'EOF' | gzip -9n > "$STAGING_DIR/usr/share/man/man1/vym-legacy.1.gz"
.TH VYM-LEGACY 1 "October 2026" "vym-legacy 1.12.2" "User Commands"
.SH NAME
vym-legacy \- View Your Mind (v1.12.2), a mindmapping tool
.SH SYNOPSIS
.B vym-legacy
[\fIOPTIONS\fR] [\fIFILE\fR...]
.SH DESCRIPTION
.B VYM Legacy
(View Your Mind 1.12.2) is a classic tool to generate and manipulate maps which show your
thoughts. Such maps can help you to improve your creativity and effectivity.
You can use them for time management, to organize tasks, to get an overview
over complex contexts, to sort your ideas etc.
.SH OPTIONS
.TP
.BR \-d ", " \-\-debug
Turn on debug mode.
.TP
.BR \-fs ", " \-\-fontsize " \fIPTI\fR"
Set application UI font size (points, default: 11).
.TP
.BR \-h ", " \-\-help
Show help message and exit.
.TP
.BR \-l ", " \-\-local
Run in local mode.
.TP
.BR \-q ", " \-\-quit
Quit immediately after loading maps.
.TP
.BR \-r ", " \-\-run " \fIFILE\fR"
Run script on load.
.TP
.BR \-t ", " \-\-test " \fIFILE\fR"
Run tests.
.TP
.BR \-v ", " \-\-version
Print version information and exit.
.SH SEE ALSO
http://www.insilmaril.de/vym/
.SH AUTHOR
Uwe Drechsel <vym@InSilmaril.de>
EOF
chmod 644 "$STAGING_DIR/usr/share/man/man1/vym-legacy.1.gz"

# Install Documentation & Copyright
mkdir -p "$STAGING_DIR/usr/share/doc/${PACKAGE_NAME}"
if [ -f "README.txt" ]; then
    cp "README.txt" "$STAGING_DIR/usr/share/doc/${PACKAGE_NAME}/"
fi
cat << 'EOF' > "$STAGING_DIR/usr/share/doc/${PACKAGE_NAME}/copyright"
Format: https://www.debian.org/doc/packaging-manuals/copyright-format/1.0/
Upstream-Name: vym
Upstream-Contact: Uwe Drechsel <vym@InSilmaril.de>
Source: http://www.insilmaril.de/vym/

Files: *
Copyright: (c) 2004-2009 Uwe Drechsel <vym@InSilmaril.de>
License: GPL-2.0-only
 On Debian and Ubuntu systems, the full text of the GNU General Public
 License version 2 can be found in the file `/usr/share/common-licenses/GPL-2'.
EOF
chmod 644 "$STAGING_DIR/usr/share/doc/${PACKAGE_NAME}/copyright"

RFC_DATE="$(date -R)"
cat << EOF | gzip -9n > "$STAGING_DIR/usr/share/doc/${PACKAGE_NAME}/changelog.Debian.gz"
${PACKAGE_NAME} (${VERSION}) ${DIST_TAG}; urgency=medium

  * Renamed package and executable to vym-legacy to coexist with repo vym.
  * Rebuilt and packaged for ${PRETTY_NAME:-Linux}.
  * Fixed C++17 compatibility issues:
    - editxlinkdialog.cpp: initialized XLinkObj* with NULL.
    - linkablemapobj.cpp: returned NULL instead of false.
    - mainwindow.cpp: included unistd.h for sleep().
  * Fixed Qt4 dynamic linking issue:
    - Added -fPIC to prevent copy relocations of QCoreApplication::self.
  * Added font size scaling (--fontsize option and VYM_FONT_SIZE / VYM_NODE_FONT_SIZE).
  * Added traditional Chinese translation (lang/vym_zh_TW.qm).
  * Added desktop entry, icons, MIME database support, and manual page.

 -- VYM Legacy Packager <vym@InSilmaril.de>  ${RFC_DATE}
EOF
chmod 644 "$STAGING_DIR/usr/share/doc/${PACKAGE_NAME}/changelog.Debian.gz"

# Strip binary to minimize size
strip --strip-unneeded "$STAGING_DIR/usr/bin/vym-legacy"

# Standardize permissions
find "$STAGING_DIR" -type d -exec chmod 755 {} +
find "$STAGING_DIR/usr/bin" -type f -exec chmod 755 {} +
find "$STAGING_DIR/usr/share" -type f -exec chmod 644 {} +
if [ -d "$STAGING_DIR/usr/share/${PACKAGE_NAME}/scripts" ]; then
    find "$STAGING_DIR/usr/share/${PACKAGE_NAME}/scripts" -type f -exec chmod 755 {} +
fi

# --- 6. Calculate Dependencies & Create Control File ---
echo "[5/6] Calculating dependencies and creating control metadata..."
mkdir -p "$STAGING_DIR/DEBIAN"

INSTALLED_SIZE=$(du -ks "$STAGING_DIR/usr" | cut -f1)

# Dynamically calculate library dependencies using dpkg-shlibdeps
mkdir -p "${BUILD_WORKSPACE}/debian"
touch "${BUILD_WORKSPACE}/debian/control"
cd "${BUILD_WORKSPACE}"

SHLIB_DEPS=$(dpkg-shlibdeps -O "${STAGING_DIR}/usr/bin/vym-legacy" 2>/dev/null | sed -n 's/^shlibs:Depends=//p' || true)

cd "$SCRIPT_DIR"

if [ -z "${SHLIB_DEPS}" ]; then
    echo "Note: Using fallback library dependency definitions."
    SHLIB_DEPS="libc6 (>= 2.34), libgcc-s1 (>= 3.0), libqt4-network (>= 4:4.5.3), libqt4-qt3support (>= 4:4.5.3), libqt4-xml (>= 4:4.5.3), libqtcore4 (>= 4:4.7.0~beta1), libqtgui4 (>= 4:4.5.3), libstdc++6 (>= 13.1)"
fi

FINAL_DEPS="${SHLIB_DEPS}, zip, unzip, xsltproc, shared-mime-info"

cat << EOF > "$STAGING_DIR/DEBIAN/control"
Package: ${PACKAGE_NAME}
Version: ${VERSION}
Section: editors
Priority: optional
Architecture: ${ARCH}
Maintainer: Uwe Drechsel <vym@InSilmaril.de>
Installed-Size: ${INSTALLED_SIZE}
Depends: ${FINAL_DEPS}
Recommends: xdg-utils
Homepage: http://www.insilmaril.de/vym/
Description: Mindmapping tool (View Your Mind) - Legacy 1.12.2 Edition
 VYM (View Your Mind) is a tool to generate and manipulate maps
 which show your thoughts. Such maps can help you to improve your
 creativity and effectivity. You can use them for time management,
 to organize tasks, to get an overview over complex contexts, to
 sort your ideas etc.
 .
 Mindmaps have many applications in personal, family, educational,
 and business situations. Possibilities include note-taking,
 brainstorming, summarizing, revising and general clarifying of thoughts.
 .
 This package provides the classic Qt4 version (1.12.2) named 'vym-legacy',
 allowing it to coexist without conflicts with the official 'vym' package.
EOF

# Maintainer Scripts
cat << 'EOF' > "$STAGING_DIR/DEBIAN/postinst"
#!/bin/sh
set -e

if [ "$1" = "configure" ]; then
    if command -v update-desktop-database >/dev/null 2>&1; then
        update-desktop-database -q /usr/share/applications || true
    fi
    if command -v update-mime-database >/dev/null 2>&1; then
        update-mime-database /usr/share/mime || true
    fi
    if command -v gtk-update-icon-cache >/dev/null 2>&1; then
        gtk-update-icon-cache -q -t -f /usr/share/icons/hicolor || true
    fi
fi

exit 0
EOF
chmod 755 "$STAGING_DIR/DEBIAN/postinst"

cat << 'EOF' > "$STAGING_DIR/DEBIAN/postrm"
#!/bin/sh
set -e

if [ "$1" = "remove" ] || [ "$1" = "purge" ]; then
    if command -v update-desktop-database >/dev/null 2>&1; then
        update-desktop-database -q /usr/share/applications || true
    fi
    if command -v update-mime-database >/dev/null 2>&1; then
        update-mime-database /usr/share/mime || true
    fi
    if command -v gtk-update-icon-cache >/dev/null 2>&1; then
        gtk-update-icon-cache -q -t -f /usr/share/icons/hicolor || true
    fi
fi

exit 0
EOF
chmod 755 "$STAGING_DIR/DEBIAN/postrm"

# --- 7. Build Debian (.deb) Package ---
echo "[6/6] Building Debian package into ${OUTPUT_DEB}..."
mkdir -p "$(dirname "$OUTPUT_DEB")"
dpkg-deb --build --root-owner-group "$STAGING_DIR" "$OUTPUT_DEB"

# Clean temporary workspace
rm -rf "$BUILD_WORKSPACE"

echo "=========================================================="
echo "  Build & Packaging Successful!"
echo "=========================================================="
echo "Package File: $OUTPUT_DEB"
ls -lh "$OUTPUT_DEB"
echo ""
echo "Package Information:"
dpkg-deb -I "$OUTPUT_DEB"
echo ""
echo "Installation Command:"
echo "  sudo apt install ./${OUTPUT_DEB##*/}"
echo "  # or: sudo dpkg -i ${OUTPUT_DEB##*/} && sudo apt-get install -f"
echo "=========================================================="
