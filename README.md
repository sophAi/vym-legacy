# VYM Legacy (View Your Mind 1.12.2)

[![License: GPL-2.0](https://img.shields.io/badge/License-GPL%202.0-blue.svg)](LICENSE.txt)
[![Platform: Linux](https://img.shields.io/badge/Platform-Linux%20Mint%20%7C%20Ubuntu%20%7C%20Debian-green.svg)]()
[![Toolkit: Qt4](https://img.shields.io/badge/Toolkit-Qt%204.8-orange.svg)]()

**VYM Legacy** 是經典心智圖工具 **VYM (View Your Mind) 1.12.2** 的現代化相容版本，經完整修補與最佳化，可在現代 Linux 系統（包括 **Linux Mint 22+ / 23+**、**Ubuntu 24.04 LTS+** 等）上穩定編譯與執行。

為了避免與軟體庫中由官方提供的 `vym`（版本 2.6+）產生檔案與設定衝突，本專案統一以 **`vym-legacy`** 名稱打包，兩者可在同一系統中**完全並存且互不干擾**。

---

## 🌟 主要特色與修復 (Key Patches & Improvements)

1. **獨立共存（Zero Conflicts）**：
   - 執行檔名稱重新命名為 `/usr/bin/vym-legacy`。
   - 獨立的桌面捷徑檔 `vym-legacy.desktop`（顯示名稱：**VYM Legacy** / **心智圖工具 (舊版 1.12.2)**）。
   - 獨立的應用程式圖示（Hicolor 多尺寸圖標）、共用資源目錄（`/usr/share/vym-legacy/`）與 Man 手冊（`man vym-legacy`）。
   - 註冊獨立的 MIME 關聯類型（`application/x-vym`）。

2. **GCC 13+ & C++17 標準相容性修復**：
   - 修復 `editxlinkdialog.cpp` 中以 `bool` 賦值指標型態之錯誤（改為 `xlo = NULL;`）。
   - 修復 `linkablemapobj.cpp` 中回傳指標函式回傳 `false` 之錯誤（改為 `return NULL;`）。
   - 在 `mainwindow.cpp` 引入 POSIX `<unistd.h>` 標頭檔以支援 `sleep()`。

3. **Qt4 ELF Copy Relocation 崩潰關鍵修正（關鍵穩定性修復）**：
   - 現代 GCC 預設開啟 PIE（Position Independent Executable）與 PC 相對定址，導致主程式在存取 `QCoreApplication::self` 時產生 ELF `R_X86_64_COPY` relocation。
   - 此重定位會造成 `libQtCore` 寫入其內部全域變數，而 `libQtGui` 卻讀取主程式 `.bss` 區段中未初始化的零值，引發 SIGABRT 崩潰（`QPixmap: Must construct a QApplication before a QPaintDevice`）。
   - 在 `vym.pro` 中加入 `-fPIC` 編譯參數，強制所有模組透過 GOT（全域位移表，`R_X86_64_GLOB_DAT`）進行符號解析，徹底根除此崩潰問題。

4. **高解析度螢幕與字體縮放支援**：
   - 將預設 UI 字體由過小的 9pt 調升為清晰易讀的 **11pt**。
   - 新增命令列參數 `-fs, --fontsize <pt>`（例如 `vym-legacy --fontsize 13`）。
   - 支援環境變數控制：
     - `VYM_FONT_SIZE`：控制全域介面字體大小。
     - `VYM_NODE_FONT_SIZE`：控制心智圖節點字體基準大小。
   - 將節點層級預設字體由 16/12/10 pt 自動調整為更適合高解析度螢幕的 **18/14/12 pt**。

5. **完整繁體中文語系支援**：
   - 修正語系檔案命名，並重新編譯產生包含 405 條完整譯文的繁體中文語系檔（`lang/vym_zh_TW.qm`）。

---

## 🛠️ 環境準備（作業系統升級後 / 全新安裝）

由於 Qt4 在現代 Ubuntu / Linux Mint 官方基礎套件庫中已退役，若在**作業系統升級後**或全新安裝的環境中，需要透過社群維護的 Qt4 PPA 取得相依套件：

### 1. 新增 Qt4 PPA

```bash
# 支援 Linux Mint 21 / 22 / 23 及 Ubuntu 20.04 / 22.04 / 24.04 (Noble) 等
sudo add-apt-repository -y ppa:rock-core/qt4
```

> **提示**：若剛完成 Linux Mint / Ubuntu 發行版升級（如 Mint 21 -> 22 或 22 -> 23），系統升級程序通常會自動停用第三方 PPA。請重新執行上述指令加入或啟用該 PPA。

### 2. 安裝編譯與打包工具

```bash
sudo apt update
sudo apt install -y build-essential dpkg-dev qt4-qmake qt4-linguist-tools \
                    libqt4-dev libqt4-qt3support libqt4-xml libqt4-network \
                    libqtgui4 libqtcore4 zip unzip xsltproc shared-mime-info
```

---

## 📦 一鍵自動化編譯與打包 (.deb)

專案根目錄中已內建自動化打包腳本 [`build_deb.sh`](build_deb.sh)。

### 執行打包

直接在專案根目錄下執行：

```bash
cd vym-legacy
./build_deb.sh
```

### 腳本自動處理流程：
1. **依賴環境檢查**：自動驗證編譯工具與套件庫完整性，若缺少將清楚列出並提供安裝指令。
2. **自動偵測發行版與架構**：自動讀取 `/etc/os-release`，動態生成版本標籤（例如 `mint22.3`、`noble` 等）與硬體架構（如 `amd64`）。
3. **語系檔編譯**：透過 `lrelease-qt4` 自動編譯多國語言翻譯檔（`*.ts` -> `*.qm`）。
4. **原始碼編譯**：使用 Qt4 qmake 與 `make -j$(nproc)` 進行多核心編譯。
5. **桌面環境整合**：自動部屬桌面選單捷徑、多解析度應用程式圖示、MIME 檔案關聯與 UNIX Man 手冊。
6. **動態函式庫相依性計算**：透過 `dpkg-shlibdeps` 精準計算目標系統的執行期相依性（Depends）。
7. **產生標準 .deb 套件**：產出符合 Debian 規範的安裝檔，例如 `vym-legacy_1.12.2-1+mint22.3_amd64.deb`。

---

## 🚀 安裝、執行與移除

### 1. 安裝 .deb 套件

```bash
# 使用 apt 直接安裝本機產生的 deb 檔（推薦，會自動解析相依性）
sudo apt install ./vym-legacy_*.deb

# 或使用傳統 dpkg 指令安裝：
# sudo dpkg -i vym-legacy_*.deb
# sudo apt-get install -f
```

### 2. 啟動程式

* **應用程式選單**：
  在桌面選單的「辦公 (Office)」或「圖形 (Graphics)」類別中點選 **VYM Legacy**。
* **終端機啟動**：
  ```bash
  # 預設啟動
  vym-legacy

  # 開啟特定心智圖檔案
  vym-legacy my_mindmap.vym

  # 自訂介面字體大小（例如 13pt）
  vym-legacy --fontsize 13

  # 查看所有支援選項
  vym-legacy --help
  ```

### 3. 移除套件

```bash
sudo apt remove vym-legacy
```

---

## 💻 手動編譯與測試（開發者選項）

若僅需在本地端進行編譯除錯，不進行打包，可直接執行標準 qmake 流程：

```bash
# 1. 產生 Makefile
qmake-qt4 PREFIX=/usr DOCDIR=/usr/share/doc/vym-legacy vym.pro

# 2. 編譯
make -j$(nproc)

# 3. 本地執行測試
./vym-legacy
```

---

## 📄 授權條款 (License)

本軟體遵循 GNU General Public License version 2 (GPL-2.0) 授權條款。詳情請參閱 [LICENSE.txt](LICENSE.txt)。  
原作者：Uwe Drechsel <vym@InSilmaril.de>  
專案網站：http://www.insilmaril.de/vym/
