# MyRTOSPrj - FreeRTOS Blinky 範例專案

本專案由 `SampleCode/FreeRTOS/Blinky` 範例複製並調整為 CMSIS-Toolbox 專案結構。

## 專案結構

```
MyRTOSPrj/
├── main.c                      # 主程式（包含 FreeRTOS 任務與系統初始化）
├── ParTest.c                   # LED 控制函式（PH.4, PH.5, PH.6）
├── FreeRTOSConfig.h            # FreeRTOS 核心配置
├── RTOSDemo.csolution.yml      # CMSIS Solution 設定檔
├── RTOSDemo.cproject.yml       # CMSIS Project 設定檔
├── vcpkg-configuration.json    # 工具鏈自動安裝配置
└── .vscode/
    ├── launch.json             # VSCode 除錯設定
    └── tasks.json              # VSCode 任務設定
```

## 建置專案

### 使用 GCC 編譯器
```bash
cbuild RTOSDemo.csolution.yml --context .debug+GNUC
```

### 使用 ARM Compiler 6 (MDK)
```bash
cbuild RTOSDemo.csolution.yml --context .debug+ARMCLANG
```

### 輸出檔案位置
- **GCC**: `out/RTOSDemo/GNUC/debug/RTOSDemo.elf`
- **AC6**: `out/RTOSDemo/ARMCLANG/debug/RTOSDemo.axf`

## 功能說明

### 硬體設定
- **MCU**: Nuvoton M467HJHAE (Cortex-M4F @ 200MHz)
- **LED**: PH.4, PH.5, PH.6 (紅、黃、綠 LED)
- **UART0**: PB12 (RXD), PB13 (TXD) @ 115200 bps
- **除錯器**: CMSIS-DAP (透過 pyOCD)

### FreeRTOS 任務
1. **Check Task**: 每 5 秒檢查 Queue 運作狀態
2. **LED Flash Tasks**: 控制 LED 閃爍
3. **Polled Queue Tasks**: 佇列輪詢測試

### 時鐘配置
- **核心時鐘**: 200 MHz
- **PCLK0/PCLK1**: HCLK/2 (100 MHz)
- **UART0**: HIRC (12 MHz)
- **Timer0**: HIRC (12 MHz)

## 燒錄與除錯

### VSCode 任務
- **CMSIS Erase**: 清除 Flash
- **CMSIS Load**: 燒錄程式到 MCU
- **CMSIS Run**: 啟動 GDB Server 進行除錯

### 命令列操作
```bash
# 燒錄程式
pyocd load --probe cmsisdap: --cbuild-run RTOSDemo+GNUC.cbuild-run.yml

# 啟動除錯伺服器
pyocd gdbserver --probe cmsisdap: --cbuild-run RTOSDemo+GNUC.cbuild-run.yml
```

### VSCode 除錯
1. 在 VSCode 中開啟 `RTOSDemo.csolution.yml`
2. 選擇編譯器（ARMCLANG 或 GNUC）
3. 按 F5 開始除錯

## 預期輸出

連接 UART0 (115200-8-N-1) 應看到：
```
Toggle LED_R/Y/G(PH.4~PH.6)
FreeRTOS is starting ...
Check Task is running ...
```

LED (PH.4, PH.5, PH.6) 會以不同模式閃爍。

## FreeRTOS 配置重點

### 記憶體配置
- **Heap Size**: 16 KB (`configTOTAL_HEAP_SIZE`)
- **Minimal Stack**: 120 words (`configMINIMAL_STACK_SIZE`)
- **Memory Scheme**: heap_2.c (允許記憶體釋放，但不合併碎片)

### 核心設定
- **Preemption**: 啟用（搶占式排程）
- **Tick Rate**: 1000 Hz (1 ms per tick)
- **Max Priorities**: 5
- **Stack Overflow Check**: Level 2 (Keil), 0 (IAR)

### 關鍵中斷優先級
- **Kernel Interrupt**: 最低優先級 (0xF0)
- **Max Syscall Priority**: 0x50 (優先級 5 以下才能呼叫 FreeRTOS API)

## 修改與擴展

### 新增任務
在 `main.c` 中使用 `xTaskCreate()`:
```c
xTaskCreate(vYourTask, "TaskName", STACK_SIZE, NULL, PRIORITY, NULL);
```

### 新增驅動
在 `RTOSDemo.cproject.yml` 的 `groups: - group: lib` 中新增：
```yaml
- file: ../../Library/StdDriver/src/<module>.c
```

並在 `add-path` 中確認 `../../Library/StdDriver/inc` 已包含。

### 調整 LED
修改 `ParTest.c` 中的 `vParTestToggleLED()` 函式。

## 常見問題

### 編譯錯誤
- 確認 `vcpkg-configuration.json` 已正確安裝工具鏈
- 檢查所有 `../../` 相對路徑是否正確

### 燒錄失敗
- 確認 CMSIS-DAP 除錯器已連接
- 執行 `pyocd list` 確認設備可見

### FreeRTOS 無法啟動
- 檢查 Heap 大小是否足夠
- 確認 `configMINIMAL_STACK_SIZE` 設定合理

## 參考資料

- [FreeRTOS 官方文件](https://www.freertos.org)
- [Nuvoton M460 BSP 指南](../../.github/copilot-instructions.md)
- [CMSIS-Toolbox 使用說明](https://github.com/Open-CMSIS-Pack/cmsis-toolbox)

---

**建立日期**: 2025-11-11  
**基於範例**: `SampleCode/FreeRTOS/Blinky`  
**目標硬體**: NuMaker-M467HJ 開發板
