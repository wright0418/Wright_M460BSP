# M460_MyProject 修改記錄

## 2025年11月10日 - 初始版本

### 建立的檔案
- ✅ `.vscode/c_cpp_properties.json` - C/C++ IntelliSense 配置
- ✅ `.vscode/launch.json` - 除錯啟動配置
- ✅ `.vscode/tasks.json` - 編譯/燒錄任務配置
- ✅ `main.c` - LED 閃爍範例主程式 (含詳細中文註解)
- ✅ `M460_MyProject.csolution.yml` - CMSIS 解決方案配置
- ✅ `M460_MyProject.cproject.yml` - CMSIS 專案配置
- ✅ `vcpkg-configuration.json` - vcpkg 工具鏈配置
- ✅ `README.md` - 完整的使用說明文件
- ✅ `CHANGELOG.md` - 本文件

### 編譯環境修正

#### 問題 1: UART 時鐘選擇巨集錯誤
**錯誤訊息**:
```
error: 'CLK_CLKSEL2_UART0SEL_HIRC' undeclared
```

**原因**: M460 系列 UART0 的時鐘選擇使用 `CLKSEL1` 暫存器,不是 `CLKSEL2`

**修正**: 
- 將 `CLK_CLKSEL2_UART0SEL_HIRC` 改為 `CLK_CLKSEL1_UART0SEL_HIRC`
- 檔案: `main.c` 第 30 行

**參考文件**: 
- `Library/StdDriver/inc/clk.h` 第 159 行
- `Library/Device/Nuvoton/m460/Include/clk_reg.h` 第 1873 行

#### 問題 2: GPIO 驅動未包含
**錯誤訊息**:
```
undefined reference to `GPIO_SetMode'
```

**原因**: 專案使用了 GPIO 功能 (LED 控制),但未包含 `gpio.c` 驅動模組

**修正**: 
- 在 `M460_MyProject.cproject.yml` 的 Library 群組中新增 `gpio.c`
- 新增行: `- file: ../../Library/StdDriver/src/gpio.c`

**影響範圍**: 所有使用 GPIO 功能的程式

### 編譯驗證結果

所有編譯配置均測試通過:

#### GCC 10.3.1 編譯器
- ✅ Debug 版本 - 編譯成功
- ✅ Release 版本 - 編譯成功
- 輸出檔案:
  - `out/M460_MyProject/GNUC/debug/M460_MyProject.elf`
  - `out/M460_MyProject/GNUC/debug/M460_MyProject.bin`
  - `out/M460_MyProject/GNUC/release/M460_MyProject.elf`
  - `out/M460_MyProject/GNUC/release/M460_MyProject.bin`

#### ARM Compiler 6.24.0 (AC6)
- ✅ Debug 版本 - 編譯成功
- ✅ Release 版本 - 編譯成功
- 程式大小 (Debug):
  - Code: 4340 bytes
  - RO-data: 404 bytes
  - RW-data: 8 bytes
  - ZI-data: 7764 bytes
- 輸出檔案:
  - `out/M460_MyProject/ARMCLANG/debug/M460_MyProject.axf`
  - `out/M460_MyProject/ARMCLANG/debug/M460_MyProject.bin`
  - `out/M460_MyProject/ARMCLANG/release/M460_MyProject.axf`
  - `out/M460_MyProject/ARMCLANG/release/M460_MyProject.bin`

### 專案特色

1. **雙編譯器支援**: 完整支援 AC6 和 GCC 兩種編譯器
2. **路徑正確性**: 所有引用路徑已根據 `Project/M460_MyProject/` 位置調整
3. **完整註解**: 所有設定檔和程式碼均包含詳細的中文註解
4. **範例程式**: 包含 LED 閃爍和 UART 輸出的完整範例
5. **VSCode 整合**: 完整的編譯、除錯、燒錄任務配置

### 已包含的驅動模組

- `clk.c` - 時鐘控制驅動
- `gpio.c` - GPIO 控制驅動 (LED 控制)
- `uart.c` - UART 通訊驅動 (printf 輸出)
- `sys.c` - 系統控制驅動
- `retarget.c` - 標準輸出重定向

### 下一步建議

1. ✅ 編譯環境已完成設定並驗證
2. 🔄 建議測試硬體燒錄和除錯功能
3. 🔄 可根據需求新增其他驅動模組 (SPI, I2C, ADC 等)
4. 🔄 可修改 `main.c` 實作具體應用功能

### 技術細節

#### 編譯指令
```bash
# GCC Debug
cbuild M460_MyProject.csolution.yml --context .debug+GNUC

# AC6 Debug
cbuild M460_MyProject.csolution.yml --context .debug+ARMCLANG

# GCC Release
cbuild M460_MyProject.csolution.yml --context .release+GNUC

# AC6 Release
cbuild M460_MyProject.csolution.yml --context .release+ARMCLANG
```

#### 清理輸出
```bash
# 清理所有編譯輸出
Remove-Item -Path "tmp" -Recurse -Force
Remove-Item -Path "out" -Recurse -Force
```

---

**建立者**: GitHub Copilot  
**日期**: 2025年11月10日  
**專案版本**: 1.0.0  
**狀態**: ✅ 編譯環境已完成配置並驗證
