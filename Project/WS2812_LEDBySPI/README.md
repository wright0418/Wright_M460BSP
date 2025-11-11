# WS2812_LEDBySPI 專案

## 專案說明
本專案為 Nuvoton M460 系列 MCU 的 WS2812 RGB LED 控制應用專案樣板。  
使用 SPI 模擬 WS2812 時序協定來驅動彩色 LED 燈條。

## WS2812 時序協定
WS2812 是一款智能 RGB LED，採用單總線通訊協定：
- **邏輯 1**: 高電平 0.8μs, 低電平 0.45μs
- **邏輯 0**: 高電平 0.4μs, 低電平 0.85μs  
- **Reset**: 低電平 > 50μs

## 專案結構
```
WS2812_LEDBySPI/
├── main.c                           # 主程式
├── WS2812_LEDBySPI.csolution.yml    # CMSIS Solution 配置
├── WS2812_LEDBySPI.cproject.yml     # CMSIS Project 配置
├── vcpkg-configuration.json         # 工具鏈依賴配置
├── .vscode/                         # VSCode 配置
│   ├── tasks.json                   # 建構/燒錄任務
│   └── launch.json                  # 除錯配置
└── out/                             # 建構輸出目錄
    └── WS2812_LEDBySPI/
        ├── ARMCLANG/debug/          # ARM Compiler 建構輸出
        └── GNUC/debug/              # GCC 建構輸出
```

## 編譯器支援
本專案支援兩種編譯器：
1. **ARM Compiler 6 (AC6/MDK)** - 版本 >= 5.19.0
2. **GNU ARM GCC** - 版本 >= 10.3.1

## 建構專案

### 使用 VSCode CMSIS Extension
1. 開啟 VSCode 並載入本專案資料夾
2. 在 CMSIS 面板選擇建構類型和編譯器
3. 點擊 Build 按鈕建構專案

### 使用命令列
**ARM Compiler (AC6/MDK) - 除錯版本:**
```bash
cbuild WS2812_LEDBySPI.csolution.yml --context .debug+ARMCLANG
```

**ARM Compiler (AC6/MDK) - 發布版本:**
```bash
cbuild WS2812_LEDBySPI.csolution.yml --context .release+ARMCLANG
```

**GCC - 除錯版本:**
```bash
cbuild WS2812_LEDBySPI.csolution.yml --context .debug+GNUC
```

**GCC - 發布版本:**
```bash
cbuild WS2812_LEDBySPI.csolution.yml --context .release+GNUC
```

## 燒錄與除錯

### 使用 VSCode 任務
在 VSCode 終端機執行任務：
- **燒錄程式**: `Terminal > Run Task > CMSIS Load`
- **清除 Flash**: `Terminal > Run Task > CMSIS Erase`
- **啟動除錯**: 按 `F5` 或使用除錯面板

### 使用命令列
**燒錄程式到 MCU:**
```bash
pyocd load --probe cmsisdap: --cbuild-run <project>.cbuild-run.yml
```

**清除 Flash:**
```bash
pyocd erase --probe cmsisdap: --chip --cbuild-run <project>.cbuild-run.yml
```

**啟動 GDB 伺服器進行除錯:**
```bash
pyocd gdbserver --probe cmsisdap: --cbuild-run <project>.cbuild-run.yml
```

## 硬體需求
- **MCU**: Nuvoton M467HJHAE (Cortex-M4F)
- **除錯器**: CMSIS-DAP 相容除錯器
- **時鐘源**: 外部高速晶振 (HXT) 或內部 RC 振盪器 (HIRC)
- **WS2812 LED**: 連接至 SPI MOSI 腳位

## 系統配置
- **核心時鐘**: 200 MHz
- **UART0**: 115200 bps, 8N1 (除錯輸出)
  - RXD: PB12
  - TXD: PB13
- **SPI**: 待配置 (用於 WS2812 控制)

## 開發指南

### 實現步驟
1. **配置 SPI 腳位**
   - 在 `SYS_Init()` 中設定 SPI 多功能腳位
   - 範例: `SET_SPI0_MOSI_PA0();`

2. **初始化 SPI 驅動**
   - 設定 SPI 時鐘頻率以匹配 WS2812 時序
   - 配置為主模式、全雙工、MSB 先行

3. **實現 WS2812 數據編碼**
   - 將 24-bit RGB 數據轉換為 SPI 時序
   - 每個 bit 需要多個 SPI bit 來模擬時序

4. **實現顏色控制函數**
   - 設定單個 LED 顏色
   - 批量設定多個 LED
   - 重新整理顯示

5. **新增動畫效果** (可選)
   - 彩虹循環
   - 呼吸燈效果
   - 流水燈效果

### 新增源檔案
如需新增源檔案，請編輯 `WS2812_LEDBySPI.cproject.yml` 的 `groups` 區段：

```yaml
- group: User
  files:
    - file: main.c
    - file: ws2812_driver.c    # 新增 WS2812 驅動程式碼
```

### 新增驅動模組
如需使用其他外設驅動，請在 `Library` 群組中新增：

```yaml
- group: Library
  files:
    - file: ../../Library/StdDriver/src/timer.c    # 新增 Timer 驅動
```

可用驅動清單請參考 `Library/StdDriver/inc/*.h`

## 除錯輸出注意事項
**重要**: 本 MCU 的 UART 限制，printf 可能會因為多位元組字元（中文）而產生亂碼。  
建議在程式碼中使用英文進行除錯輸出，避免顯示問題。

```c
// 關閉 stdio 緩衝以避免 UTF-8 字元分段
setvbuf(stdout, NULL, _IONBF, 0);

// 建議使用英文輸出
printf("LED Status: %d\n", status);
```

## 編譯輸出

### ARMCLANG (AC6) 編譯結果
```
Program Size: Code=4976 RO-data=232 RW-data=44 ZI-data=7768
Build summary: 1 succeeded, 0 failed
```

### GCC 編譯結果
```
Build summary: 1 succeeded, 0 failed
```

## 相關文件
- **M460 系列 BSP 指南**: `../../.github/copilot-instructions.md`
- **驅動 API 參考**: `../../Document/NuMicro M460 Series Driver Reference Guide.chm`
- **CMSIS 文件**: `../../Document/CMSIS.html`
- **範例程式碼**: `../../SampleCode/StdDriver/`

## 授權
SPDX-License-Identifier: Apache-2.0  
Copyright (C) 2021 Nuvoton Technology Corp. All rights reserved.

## 版本記錄
- **v1.0** (2025/11/11): 初始專案建立
  - 支援 ARMCLANG 和 GCC 編譯器
  - 基礎系統初始化
  - UART 除錯輸出
  - 新增 SPI、GPIO、PDMA 驅動準備用於 WS2812 控制
