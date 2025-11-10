# Nuvoton M460 系列 BSP 專案指南

這是 Nuvoton M460 系列微控制器的 Board Support Package (BSP)，使用 CMSIS-Toolbox 建構系統。本指南協助 AI 編碼助手快速理解專案架構與開發工作流程。

## 注意:
- printf 不支援中文，使用英文輸出以避免亂碼。
## 專案架構總覽

### 關鍵目錄結構
```
├── Library/                          # 核心函式庫與驅動
│   ├── CMSIS/                        # ARM CMSIS v5.0 定義
│   ├── Device/Nuvoton/m460/          # M460 設備特定檔案 (啟動檔、系統初始化、連結腳本)
│   ├── StdDriver/                    # 外設標準驅動 (src/ 與 inc/)
│   ├── CryptoAccelerator/            # mbedTLS 加密硬體加速
│   └── UsbHostLib/                   # USB Host 函式庫
├── SampleCode/StdDriver/             # 200+ 外設驅動範例 (每個 IP 模組獨立目錄)
├── ThirdParty/                       # 第三方軟體整合 (FreeRTOS, lwIP, mbedTLS, FatFs)
└── Project/M460_MyProject/           # VSCode 專案範本 (CMSIS csolution 結構)
```

### 硬體目標
- **設備**: Nuvoton M467HJHAE (Cortex-M4F)
- **除錯器**: CMSIS-DAP (透過 pyOCD)
- **時鐘**: HXT (外部晶振) 或 HIRC (內部 RC 振盪器)

## 開發工作流程

### 1. 建構系統 (CMSIS-Toolbox)
專案使用 CMSIS Solution (`.csolution.yml`) 與 Project (`.cproject.yml`) 結構：

**支援的編譯器**:
- **ARM Compiler 6 (AC6/MDK)**: `cbuild M460_MyProject.csolution.yml --context .debug+ARMCLANG`
- **GCC (ARM-NONE-EABI-GCC)**: `cbuild M460_MyProject.csolution.yml --context .debug+GNUC`

**關鍵配置檔案**:
- `vcpkg-configuration.json`: 工具鏈自動安裝 (CMake, Ninja, AC6, GCC, CMSIS-Toolbox)
- `M460_MyProject.csolution.yml`: 定義編譯器選擇與建置類型 (debug/release)
- `M460_MyProject.cproject.yml`: 專案源檔案、路徑、編譯選項

### 2. 燒錄與除錯
**使用 VSCode 任務** (`.vscode/tasks.json`):
- `CMSIS Load`: 燒錄程式到 MCU
- `CMSIS Erase`: 清除 Flash
- `CMSIS Run`: 啟動 GDB 伺服器進行除錯

**命令列操作**:
```bash
# 燒錄程式
pyocd load --probe cmsisdap: --cbuild-run <project>.cbuild-run.yml

# 啟動除錯伺服器
pyocd gdbserver --probe cmsisdap: --cbuild-run <project>.cbuild-run.yml
```

### 3. 標準驅動使用模式
所有外設驅動遵循一致的模式 (參考 `Library/StdDriver/src/` 與 `SampleCode/StdDriver/`):

```c
#include "NuMicro.h"  // 包含所有 M460 外設標頭檔

void SYS_Init(void) {
    // 1. 解鎖受保護暫存器
    SYS_UnlockReg();
    
    // 2. 啟用時鐘源並等待穩定
    CLK_EnableXtalRC(CLK_PWRCTL_HXTEN_Msk);
    CLK_WaitClockReady(CLK_STATUS_HXTSTB_Msk);
    
    // 3. 設定模組時鐘 (使用 CLK_SetModuleClock)
    CLK_SetModuleClock(UART0_MODULE, CLK_CLKSEL1_UART0SEL_HXT, CLK_CLKDIV0_UART0(1));
    
    // 4. 啟用周邊模組時鐘
    CLK_EnableModuleClock(UART0_MODULE);
    
    // 5. 設定多功能腳位 (使用 SET_<MODULE>_<PIN>_<PORT> 巨集)
    SET_UART0_RXD_PB12();
    SET_UART0_TXD_PB13();
    
    // 6. 鎖定受保護暫存器
    SYS_LockReg();
}
```

**關鍵 API 模式**:
- **時鐘**: `CLK_EnableXtalRC()`, `CLK_WaitClockReady()`, `CLK_SetModuleClock()`
- **GPIO**: `GPIO_SetMode()`, `GPIO_GET_INT_FLAG()`, `GPIO_CLR_INT_FLAG()`
- **UART**: `UART_Open()`, `printf()` (透過 `retarget.c` 重新導向)
- **中斷**: 處理常式命名為 `<MODULE>_IRQHandler()` (參考 `startup_m460.S`)

### 4. 新增源檔案與驅動
編輯 `M460_MyProject.cproject.yml` 的 `groups` 區段:

```yaml
groups:
  - group: User
    files:
      - file: main.c
      - file: your_new_module.c  # 新增使用者程式碼
  
  - group: Library
    files:
      - file: ../../Library/StdDriver/src/clk.c
      - file: ../../Library/StdDriver/src/gpio.c
      - file: ../../Library/StdDriver/src/spi.c    # 新增需要的驅動
      # 可用驅動清單: Library/StdDriver/inc/*.h (45+ 外設模組)
```

**重要**: 所有路徑相對於 `Project/M460_MyProject/` 目錄。

### 5. 除錯輸出限制 (中文支援)
```c
// ⚠️ Nuvoton MCU UART 限制: 避免 UTF-8 多位元組字元分段
setvbuf(stdout, NULL, _IONBF, 0);  // 關閉 stdio 緩衝

printf("計數: %d\r\n", count);      // 允許使用中文，但需確保完整傳輸
// 注意: 在某些情況下 (除錯中斷、USB-Serial) 多位元組字元可能被拆分造成亂碼
```

## 第三方整合

### FreeRTOS
- 位置: `ThirdParty/FreeRTOS/`
- 範例: `SampleCode/FreeRTOS/Blinky/`, `TicklessIdle/`
- 配置: 需要設定 `FreeRTOSConfig.h` 與 `heap_x.c`

### lwIP (TCP/IP 協定堆疊)
- 位置: `ThirdParty/lwIP/`
- 範例: `SampleCode/NuMaker_M467HJ/lwIP/`, `LwIP_httpd_*/`, `LwIP_MQTT/`
- 需要: EMAC 外設驅動 (`Library/StdDriver/src/emac.c`)

### mbedTLS (加密)
- 位置: `ThirdParty/mbedtls-3.1.0/`
- 硬體加速: `Library/CryptoAccelerator/*_alt.c` (AES, SHA256, RSA, ECC)
- 範例: `SampleCode/Crypto/mbedTLS_*/`

### FatFs (檔案系統)
- 位置: `ThirdParty/FatFs/`
- 範例: `SampleCode/StdDriver/SDH_FATFS/`

## 專案特定慣例

### 編譯器差異
**ARM Compiler 6**:
- 使用 MicroLib (`--library_type=microlib`)
- C99 標準 (`-std=c99`)
- 啟動檔: `Library/Device/Nuvoton/m460/Source/ARM/startup_m460.S`

**GCC**:
- 使用 newlib-nano (`--specs=nano.specs`)
- GNU11 標準 (`-std=gnu11`)
- 連結腳本: `Library/Device/Nuvoton/m460/Source/GCC/gcc_arm.ld`
- 連結器選項必須包含 `--gc-sections` (移除未使用節區)

### 系統 Ticks 延遲 (無 RTOS)
```c
CLK_SysTickDelay(500000);  // 延遲約 500ms (使用 SysTick)
// 注意: 這是阻塞延遲，不建議用於 RTOS 環境
```

### 記憶體映射
- **Flash**: 0x00000000 (通常 512KB - 1MB，依晶片型號)
- **SRAM**: 0x20000000 (通常 256KB - 512KB)
- **Peripheral**: 0x40000000 起始
- **HBI HRAM**: 0x08000000 (HyperRAM 介面，若使用)

## 範例程式碼參考

查找特定功能實作時的優先順序:
1. **`SampleCode/StdDriver/<PERIPHERAL>/`**: 官方驅動範例 (每個外設獨立目錄)
2. **`Library/StdDriver/src/<module>.c`**: 驅動實作與 API 文件
3. **`Document/NuMicro M460 Series Driver Reference Guide.chm`**: 完整 API 參考手冊
4. **`Project/AGENTS.md`**: Nuvoton MCU 特定開發限制與 IP 使用指南

## 常見問題與注意事項

1. **受保護暫存器**: 系統控制相關暫存器需要 `SYS_UnlockReg()` 才能寫入，使用後必須 `SYS_LockReg()`
2. **時鐘設定**: 修改系統時鐘前必須先啟用目標時鐘源並等待穩定
3. **PDMA**: 使用 PDMA 時需正確配置 scatter-gather descriptor 與觸發源
4. **中斷優先級**: Cortex-M4 支援 0-255 優先級，數字越小優先級越高
5. **printf 效能**: `retarget.c` 逐字節輸出，大量列印會影響效能，考慮使用 UART PDMA

## 快速啟動檢查清單

建立新功能時:
- [ ] 從 `SampleCode/StdDriver/` 找到相關外設範例
- [ ] 複製 `SYS_Init()` 模式設定時鐘與多功能腳位
- [ ] 在 `M460_MyProject.cproject.yml` 新增所需驅動源檔案
- [ ] 使用 `cbuild` 建構並檢查錯誤
- [ ] 透過 `CMSIS Load` 任務燒錄至 MCU
- [ ] 使用 VSCode 除錯器 (F5) 設定中斷點驗證邏輯

---
**參考文件**: 
- 專案範本: `Project/M460_MyProject/README.md`
- MCU 限制: `Project/AGENTS.md`
- CMSIS 文件: `Document/CMSIS.html`
