# M460_MyProject

這是一個基於 Nuvoton M460 系列微控制器的 VSCode 專案範本,支援 ARM Compiler 6 (MDK) 和 GCC 編譯器。

## 專案結構

```
M460_MyProject/
├── .vscode/                           # VSCode 配置目錄
│   ├── c_cpp_properties.json          # C/C++ IntelliSense 配置
│   ├── launch.json                    # 除錯配置
│   └── tasks.json                     # 任務配置 (建置、燒錄等)
├── main.c                             # 主程式檔案
├── M460_MyProject.csolution.yml       # CMSIS 解決方案配置
├── M460_MyProject.cproject.yml        # CMSIS 專案配置
├── vcpkg-configuration.json           # vcpkg 工具鏈配置
└── README.md                          # 本說明文件
```

## 功能特色

- ✅ 支援 **ARM Compiler 6 (AC6/MDK)** 編譯器
- ✅ 支援 **GCC (ARM-NONE-EABI-GCC)** 編譯器
- ✅ 使用 **CMSIS-Toolbox** 進行專案管理
- ✅ 整合 **VSCode** 開發環境
- ✅ 包含基本的 LED 閃爍範例程式
- ✅ 支援 UART 串列埠輸出 (115200 bps)
- ✅ 完整的除錯支援 (透過 CMSIS-DAP)

## 系統需求

### 必要工具

1. **Visual Studio Code**
   - 下載: https://code.visualstudio.com/

2. **VSCode 擴展**
   - Arm CMSIS csolution
   - Cortex-Debug
   - C/C++

3. **CMSIS-Toolbox**
   - 透過 vcpkg 自動安裝 (請參考下方說明)

4. **編譯器 (擇一或兩者都安裝)**
   - **ARM Compiler 6**: 版本 >= 6.19.0
     - 透過 vcpkg 自動安裝,或
     - 從 Keil MDK 安裝
   - **ARM GCC**: 版本 >= 10.3.1
     - 透過 vcpkg 自動安裝

5. **燒錄/除錯工具**
   - pyOCD (用於燒錄和除錯)
   - 安裝: `pip install pyocd`

### 硬體需求

- Nuvoton M467HJHAE 開發板或相容板卡
- CMSIS-DAP 除錯器 (通常整合在開發板上)

## 快速開始

### 1. 安裝 vcpkg 工具鏈

首次開啟專案時,VSCode 會自動提示安裝必要的工具鏈。或者手動執行:

```bash
# 在專案目錄下執行
vcpkg activate
```

這將自動安裝:
- CMake
- Ninja
- ARM Compiler 6
- CMSIS-Toolbox
- ARM GCC

### 2. 開啟專案

1. 啟動 VSCode
2. 開啟本專案資料夾: `File > Open Folder` → 選擇 `M460_MyProject`
3. VSCode 會自動識別 CMSIS 專案配置

### 3. 選擇編譯器和建置類型

在 VSCode 的狀態列底部,您會看到:
- **Compiler**: 選擇 `ARMCLANG` (AC6) 或 `GNUC` (GCC)
- **Build Type**: 選擇 `debug` 或 `release`

### 4. 建置專案

有多種方式可以建置專案:

#### 方法 1: 使用 CMSIS 擴展
- 點擊 VSCode 側邊欄的 CMSIS 圖示
- 點擊 "Build" 按鈕

#### 方法 2: 使用命令面板
- 按 `Ctrl+Shift+P`
- 輸入 "CMSIS: Build"

#### 方法 3: 使用命令列
```bash
# 使用 AC6 編譯器建置 Debug 版本
cbuild M460_MyProject.csolution.yml --context .debug+ARMCLANG

# 使用 GCC 編譯器建置 Debug 版本
cbuild M460_MyProject.csolution.yml --context .debug+GNUC

# 使用 AC6 編譯器建置 Release 版本
cbuild M460_MyProject.csolution.yml --context .release+ARMCLANG
```

### 5. 燒錄程式

#### 使用 VSCode 任務
- 按 `Ctrl+Shift+P`
- 輸入 "Tasks: Run Task"
- 選擇 "CMSIS Load" 或 "CMSIS Load+Run"

#### 使用命令列
```bash
# 載入程式到開發板
pyocd load --probe cmsisdap: --cbuild-run <cbuild-run.yml>
```

### 6. 除錯程式

1. 按 `F5` 或點擊 VSCode 除錯面板的 "Start Debugging"
2. 選擇 "CMSIS_DAP@pyOCD (launch)" 配置
3. 程式會自動燒錄並在 `main()` 函數停止

## 程式說明

### main.c

這是一個簡單的 LED 閃爍程式,主要功能包括:

1. **系統初始化**
   - 啟用內部高速 RC 振盪器 (HIRC)
   - 配置 UART0 時鐘
   - 設定 GPIO 腳位

2. **UART 通訊**
   - 鮑率: 115200 bps
   - 格式: 8-N-1
   - 用於輸出除錯訊息

3. **LED 控制**
   - 使用 PH.4 和 PH.5 控制 LED
   - 閃爍週期: 約 500ms

4. **主迴圈**
   - 切換 LED 狀態
   - 透過 UART 輸出計數值
   - 延遲 500ms

## 編譯器配置說明

### ARM Compiler 6 (AC6)

- **標準**: C99
- **優化**: O0 (除錯模式)
- **函式庫**: MicroLib (減少程式碼大小)
- **除錯格式**: DWARF-4

關鍵編譯選項:
```yaml
C: 
  - -std=c99                # C99 標準
  - -ffunction-sections     # 函數獨立節區 (便於移除未使用程式碼)
  - -fdata-sections         # 資料獨立節區
  - -D__MICROLIB           # 使用 MicroLib
  - -o0                     # 無優化 (便於除錯)
  - -gdwarf-4              # DWARF-4 除錯格式
```

### GCC (ARM-NONE-EABI-GCC)

- **標準**: GNU11
- **優化**: 預設
- **函式庫**: newlib-nano (減少程式碼大小)
- **除錯格式**: DWARF-4

關鍵編譯選項:
```yaml
C: 
  - -std=gnu11              # GNU11 標準
  - --specs=nano.specs      # 使用 newlib-nano
  - -ffunction-sections     # 函數獨立節區
  - -fdata-sections         # 資料獨立節區
  - -mthumb                 # Thumb 指令集
  - -gdwarf-4              # DWARF-4 除錯格式

Link:
  - --gc-sections           # 移除未使用的節區
```

## 路徑配置

所有路徑都是相對於專案根目錄 (`Project/M460_MyProject/`) 的相對路徑:

```yaml
包含路徑:
  - ../../Library/Device/Nuvoton/m460/Include    # M460 設備標頭檔
  - ../../Library/StdDriver/inc                  # 標準驅動標頭檔
  - ../../Library/CMSIS/Include                  # CMSIS 標頭檔

源文件:
  - ../../Library/Device/Nuvoton/m460/Source/    # 啟動文件和系統初始化
  - ../../Library/StdDriver/src/                 # 標準驅動源文件

連結腳本:
  - ../../Library/Device/Nuvoton/m460/Source/GCC/gcc_arm.ld  # GCC 連結腳本
```

## 新增源文件

如果您想新增更多的源文件或驅動程式,請編輯 `M460_MyProject.cproject.yml`:

```yaml
groups:
  - group: User
    files:
      - file: main.c
      - file: your_new_file.c       # 新增您的檔案

  - group: Library
    files:
      - file: ../../Library/StdDriver/src/clk.c
      - file: ../../Library/StdDriver/src/gpio.c    # GPIO 驅動 (已包含)
      - file: ../../Library/StdDriver/src/uart.c    # UART 驅動 (已包含)
      # ... 新增其他驅動檔案
```

### 已包含的驅動模組

本專案已預設包含以下驅動模組:
- **clk.c** - 時鐘控制驅動
- **gpio.c** - GPIO 控制驅動 (用於 LED 控制)
- **uart.c** - UART 通訊驅動 (用於 printf 輸出)
- **sys.c** - 系統控制驅動
- **retarget.c** - 標準輸出重定向 (printf 支援)

### 編譯驗證狀態

專案已通過以下編譯配置測試:

| 編譯器 | Debug 版本 | Release 版本 | 程式大小 (Debug) |
|--------|-----------|-------------|-----------------|
| **GCC 10.3.1** | ✅ 成功 | ✅ 成功 | ~4KB Code |
| **AC6 6.24.0** | ✅ 成功 | ✅ 成功 | 4340 bytes Code, 404 bytes RO-data |

輸出檔案位置:
```
out/M460_MyProject/
├── ARMCLANG/
│   ├── debug/
│   │   ├── M460_MyProject.axf    # AC6 Debug ELF 檔
│   │   ├── M460_MyProject.bin    # AC6 Debug 二進位檔
│   │   └── M460_MyProject.map    # AC6 Debug 記憶體映射檔
│   └── release/
│       ├── M460_MyProject.axf
│       ├── M460_MyProject.bin
│       └── M460_MyProject.map
└── GNUC/
    ├── debug/
    │   ├── M460_MyProject.elf    # GCC Debug ELF 檔
    │   ├── M460_MyProject.bin    # GCC Debug 二進位檔
    │   └── M460_MyProject.map    # GCC Debug 記憶體映射檔
    └── release/
        ├── M460_MyProject.elf
        ├── M460_MyProject.bin
        └── M460_MyProject.map
```

## 常見問題

### Q1: 找不到編譯器?
**A**: 確保已執行 `vcpkg activate` 來安裝工具鏈,或手動設定編譯器路徑。

### Q2: 建置失敗?
**A**: 檢查:
1. CMSIS-Toolbox 是否正確安裝
2. 編譯器是否可用
3. 路徑配置是否正確

### Q3: 編譯錯誤 `CLK_CLKSEL2_UART0SEL_HIRC` 未定義?
**A**: M460 系列使用 `CLK_CLKSEL1_UART0SEL_HIRC` (注意是 CLKSEL**1** 不是 CLKSEL2)。此問題已在專案中修正。

### Q4: 連結錯誤 `undefined reference to GPIO_SetMode`?
**A**: 需要在 `M460_MyProject.cproject.yml` 中新增 `gpio.c` 驅動:
```yaml
- group: Library
  files:
    - file: ../../Library/StdDriver/src/gpio.c
```
此驅動已預設包含在專案中。

### Q5: 無法燒錄程式?
**A**: 確保:
1. pyOCD 已安裝: `pip install pyocd`
2. 開發板已連接
3. CMSIS-DAP 驅動已安裝

### Q6: IntelliSense 無法正常工作?
**A**: 檢查 `.vscode/c_cpp_properties.json` 中的包含路徑是否正確。注意:IntelliSense 錯誤不影響實際編譯。

## 參考資源

- [Nuvoton M460 系列技術手冊](https://www.nuvoton.com/products/microcontrollers/arm-cortex-m4-mcus/m460-ethernet-crypto-series/)
- [CMSIS-Toolbox 文件](https://github.com/Open-CMSIS-Pack/cmsis-toolbox)
- [ARM Compiler 6 參考手冊](https://developer.arm.com/documentation/dui0774/latest/)
- [GNU ARM Embedded Toolchain](https://developer.arm.com/Tools%20and%20Software/GNU%20Toolchain)

## 授權

本專案使用 Apache-2.0 授權。

---

**建立日期**: 2025年11月10日  
**適用於**: Nuvoton M460 系列微控制器  
**VSCode**: 需要 CMSIS csolution 擴展
