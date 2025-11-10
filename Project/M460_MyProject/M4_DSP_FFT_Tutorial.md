# ARM Cortex-M4 DSP FFT 使用教學

## 📚 概述

本教學介紹如何在 Nuvoton M460 系列微控制器 (ARM Cortex-M4F) 上使用 CMSIS-DSP 函式庫進行高效能的快速傅立葉轉換 (FFT)。

### 關鍵優勢
- **5 倍加速**: DSP 優化的 FFT 比純 CPU 實作快 5.08 倍
- **硬體加速**: 利用 Cortex-M4 的 DSP 指令集 (SIMD) 和 FPU
- **記憶體效率**: 支援原地 (in-place) 運算
- **精度保證**: 與標準 FFT 演算法結果一致 (誤差 < 0.001%)

---

## 🎯 實測效能比較

### 測試條件
- **MCU**: Nuvoton M467HJHAE (Cortex-M4F @ 200MHz)
- **FFT 大小**: 1024 點 (實數 FFT)
- **測試訊號**: 5kHz + 25kHz 混合正弦波
- **採樣率**: 1 MHz
- **編譯器**: ARM Compiler 6 (AC6) -O2 最佳化

### 效能測試結果

| 實作方式 | 執行週期 | 執行時間 | 加速比 |
|---------|---------|---------|-------|
| **CMSIS-DSP FFT** | 70,081 cycles | 0.350 ms | **5.08x** |
| Pure CPU FFT | 355,854 cycles | 1.779 ms | 1.00x (baseline) |

#### 分析
- **DSP FFT**: 使用 ARM 優化的 SIMD 指令和 FPU 硬體加速
- **CPU FFT**: 標準 Cooley-Tukey Radix-2 演算法 (純 C 實作)
- **效能提升**: 5.08 倍 (節省 285,773 個時鐘週期)

### 結果驗證

| 頻率點 | DSP FFT 輸出 | CPU FFT 輸出 | 差異 |
|-------|-------------|-------------|------|
| DC (0 Hz) | 2,110,307.0 | 2,110,307.0 | 0.0 |
| 5 kHz | 497,524.9 | 497,520.2 | 4.7 (**0.0009%**) |
| 25 kHz | 131,353.5 | 131,352.3 | 1.3 (**0.0010%**) |

✅ **驗證結論**: 兩種實作結果高度一致,最大相對誤差 < 0.001%

---

## 📋 前置準備

### 1. 硬體需求
- Nuvoton M460 系列開發板 (或任何 Cortex-M4F MCU)
- CMSIS-DAP 除錯器

### 2. 軟體需求
- CMSIS-DSP 函式庫 (已包含在 BSP 中: `Library/CMSIS/DSP_Lib/`)
- ARM Compiler 6 或 GCC ARM Embedded

### 3. 專案配置

在 `M460_MyProject.cproject.yml` 中加入以下內容:

```yaml
groups:
  - group: CMSIS-DSP
    files:
      # Transform Functions (FFT 核心)
      - file: ../../Library/CMSIS/DSP_Lib/Source/TransformFunctions/arm_rfft_fast_f32.c
      - file: ../../Library/CMSIS/DSP_Lib/Source/TransformFunctions/arm_cfft_f32.c
      - file: ../../Library/CMSIS/DSP_Lib/Source/TransformFunctions/arm_cfft_radix8_f32.c
      - file: ../../Library/CMSIS/DSP_Lib/Source/TransformFunctions/arm_bitreversal.c
      - file: ../../Library/CMSIS/DSP_Lib/Source/TransformFunctions/arm_bitreversal2.S
      
      # Common Tables (查表資料)
      - file: ../../Library/CMSIS/DSP_Lib/Source/CommonTables/arm_common_tables.c
      
      # Fast Math (三角函數)
      - file: ../../Library/CMSIS/DSP_Lib/Source/FastMathFunctions/arm_cos_f32.c
      - file: ../../Library/CMSIS/DSP_Lib/Source/FastMathFunctions/arm_sin_f32.c
      
      # Complex Math (振幅計算)
      - file: ../../Library/CMSIS/DSP_Lib/Source/ComplexMathFunctions/arm_cmplx_mag_f32.c

define:
  - ARM_MATH_CM4        # 啟用 Cortex-M4 優化
  - __FPU_PRESENT       # 啟用 FPU 支援

add-path:
  - ../../Library/CMSIS/DSP_Lib/Source
  - ../../Library/CMSIS/Include
```

---

## 🚀 使用步驟

### 步驟 1: 引入標頭檔

```c
#include "NuMicro.h"
#include "arm_math.h"          // CMSIS-DSP 主要標頭檔
#include "arm_const_structs.h" // FFT 常數結構 (可選)
```

### 步驟 2: 宣告全域變數

```c
#define FFT_SIZE 1024  // FFT 大小 (必須是 2 的次方: 256, 512, 1024, 2048, 4096)

float32_t g_f32Input[FFT_SIZE];       // 輸入: 純實數陣列
float32_t g_f32Output[FFT_SIZE];      // 輸出: 複數交錯格式 (會被覆蓋)
float32_t g_f32Magnitude[FFT_SIZE/2]; // 頻譜振幅 (0 ~ Nyquist 頻率)
```

⚠️ **記憶體注意事項**:
- `g_f32Input[]` 和 `g_f32Output[]` **可以使用同一個陣列** (原地運算)
- 如果使用原地運算,輸入資料會被覆蓋
- 總記憶體需求: `FFT_SIZE * 4 bytes` (浮點數)

---

## 📥 輸入資料擺放 (重要!)

### ✅ 正確做法: 純實數陣列

```c
// 範例 1: 從 ADC 讀取資料
int16_t adc_data[FFT_SIZE];
// ... (ADC 採樣)

for(uint32_t i = 0; i < FFT_SIZE; i++)
{
    g_f32Input[i] = (float32_t)adc_data[i];  // 連續存放,不要跳過索引!
}
```

```c
// 範例 2: 產生測試訊號
for(uint32_t i = 0; i < FFT_SIZE; i++)
{
    float32_t t = (float32_t)i / SAMPLE_RATE;
    g_f32Input[i] = arm_sin_f32(2.0f * PI * 1000.0f * t);  // 1kHz 正弦波
}
```

### ❌ 錯誤做法: 複數交錯格式 (不要這樣做!)

```c
// ❌ 錯誤! arm_rfft_fast_f32 不接受這種格式
for(uint32_t i = 0; i < FFT_SIZE; i++)
{
    g_f32Input[i * 2]     = (float32_t)adc_data[i];  // 偶數索引
    g_f32Input[i * 2 + 1] = 0.0f;                    // 奇數索引填 0
}
// 這會導致 FFT 結果完全錯誤!
```

### 📌 為什麼不能用複數交錯格式?

- `arm_rfft_fast_f32` 專為**實數 FFT** 設計
- 它利用實數訊號的對稱性進行優化,**只需要輸入 N 個實數**
- 如果使用複數交錯格式,會浪費一半的記憶體,且結果錯誤

---

## 💻 完整程式範例

### 基本用法

```c
void Run_DSP_FFT(void)
{
    uint32_t i;
    
    /* 步驟 1: 準備輸入資料 (純實數陣列) */
    for(i = 0; i < FFT_SIZE; i++)
    {
        g_f32Input[i] = (float32_t)adc_data[i];
    }
    
    /* 步驟 2: 初始化 FFT 實例 */
    arm_rfft_fast_instance_f32 fft_instance;
    arm_rfft_fast_init_f32(&fft_instance, FFT_SIZE);
    
    /* 步驟 3: 執行 FFT (可原地運算) */
    arm_rfft_fast_f32(&fft_instance, g_f32Input, g_f32Output, 0);
    // 參數說明:
    // - &fft_instance: FFT 結構指標
    // - g_f32Input:    輸入陣列 (純實數)
    // - g_f32Output:   輸出陣列 (複數交錯格式) - 可與輸入相同
    // - 0:             FFT (正轉換); 1 = IFFT (反轉換)
    
    /* 步驟 4: 計算頻譜振幅 */
    arm_cmplx_mag_f32(g_f32Output, g_f32Magnitude, FFT_SIZE / 2);
    // 輸出: g_f32Magnitude[0] = DC, g_f32Magnitude[1] = 第1個頻率, ...
}
```

### 原地運算 (節省記憶體)

```c
void Run_DSP_FFT_InPlace(void)
{
    /* 使用同一個陣列作為輸入和輸出 */
    for(uint32_t i = 0; i < FFT_SIZE; i++)
    {
        g_f32Input[i] = (float32_t)adc_data[i];
    }
    
    arm_rfft_fast_instance_f32 fft_instance;
    arm_rfft_fast_init_f32(&fft_instance, FFT_SIZE);
    
    // 輸入和輸出使用同一個陣列
    arm_rfft_fast_f32(&fft_instance, g_f32Input, g_f32Input, 0);
    
    // 此時 g_f32Input[] 已被覆蓋為 FFT 結果 (複數格式)
    arm_cmplx_mag_f32(g_f32Input, g_f32Magnitude, FFT_SIZE / 2);
}
```

---

## 📊 輸出資料格式

### `arm_rfft_fast_f32` 的輸出格式

對於 N 點實數 FFT,輸出陣列包含 **N 個元素**,格式為:

```
[Real(0), Real(N/2), Real(1), Imag(1), Real(2), Imag(2), ..., Real(N/2-1), Imag(N/2-1)]
```

#### 特殊值:
- `output[0]` = **DC 分量** (直流分量,純實數)
- `output[1]` = **Nyquist 頻率** (採樣率的一半,純實數)
- `output[2], output[3]` = 第 1 個頻率的實部和虛部
- `output[4], output[5]` = 第 2 個頻率的實部和虛部
- ...以此類推

### 振幅計算

使用 `arm_cmplx_mag_f32` 自動處理這種格式:

```c
arm_cmplx_mag_f32(g_f32Output, g_f32Magnitude, FFT_SIZE / 2);
```

**輸出**:
- `g_f32Magnitude[0]` = DC 分量的振幅
- `g_f32Magnitude[k]` = 第 k 個頻率的振幅 = √(Re² + Im²)

### 頻率對應關係

```c
float32_t frequency_resolution = (float32_t)SAMPLE_RATE / FFT_SIZE;

for(uint32_t k = 0; k < FFT_SIZE / 2; k++)
{
    float32_t frequency = k * frequency_resolution;
    float32_t magnitude = g_f32Magnitude[k];
    
    printf("Bin %d: %.2f Hz, Magnitude = %.2f\n", k, frequency, magnitude);
}
```

**範例** (採樣率 1MHz, 1024 點 FFT):
- `g_f32Magnitude[0]` → 0 Hz (DC)
- `g_f32Magnitude[5]` → 5 × (1000000/1024) ≈ 4883 Hz
- `g_f32Magnitude[512]` → 512 × (1000000/1024) = 500 kHz (Nyquist)

---

## ⚠️ 常見錯誤與注意事項

### 1. ❌ 輸入格式錯誤

**錯誤**: 使用複數交錯格式
```c
// ❌ 不要這樣做!
g_f32Input[i * 2] = real_value;
g_f32Input[i * 2 + 1] = 0.0f;
```

**正確**: 使用純實數陣列
```c
// ✅ 正確做法
g_f32Input[i] = real_value;
```

---

### 2. ❌ FFT 大小不是 2 的次方

`arm_rfft_fast_f32` **只支援 2 的次方大小**:
- ✅ 支援: 32, 64, 128, 256, 512, 1024, 2048, 4096
- ❌ 不支援: 100, 500, 1000

---

### 3. ❌ 忘記初始化 FFT 實例

```c
// ❌ 錯誤: 未初始化
arm_rfft_fast_instance_f32 fft_instance;
arm_rfft_fast_f32(&fft_instance, input, output, 0);  // 可能崩潰!

// ✅ 正確: 必須先初始化
arm_rfft_fast_instance_f32 fft_instance;
arm_rfft_fast_init_f32(&fft_instance, FFT_SIZE);
arm_rfft_fast_f32(&fft_instance, input, output, 0);
```

---

### 4. ❌ 直接讀取複數輸出

**錯誤**:
```c
arm_rfft_fast_f32(&fft_instance, input, output, 0);
float32_t magnitude_k = output[k];  // ❌ 這不是振幅!
```

**正確**:
```c
arm_rfft_fast_f32(&fft_instance, input, output, 0);
arm_cmplx_mag_f32(output, magnitude, FFT_SIZE / 2);  // ✅ 先計算振幅
float32_t magnitude_k = magnitude[k];
```

---

### 5. ❌ 陣列大小不足

```c
// ❌ 錯誤: 輸出陣列太小
float32_t output[FFT_SIZE / 2];  // 只有一半大小!
arm_rfft_fast_f32(&fft_instance, input, output, 0);  // 記憶體溢出!

// ✅ 正確: 輸出陣列必須與輸入相同大小
float32_t output[FFT_SIZE];
arm_rfft_fast_f32(&fft_instance, input, output, 0);
```

---

### 6. ⚠️ 浮點數精度

- CMSIS-DSP 使用 `float32_t` (單精度浮點數)
- 精度約 **7 位有效數字**
- 如需更高精度,使用 `arm_rfft_fast_f64` (雙精度,較慢)

---

### 7. ⚠️ 記憶體對齊

為了獲得最佳效能,建議**記憶體對齊**:

```c
// 建議做法: 使用 __attribute__((aligned(4)))
__attribute__((aligned(4))) float32_t g_f32Input[FFT_SIZE];
__attribute__((aligned(4))) float32_t g_f32Output[FFT_SIZE];
```

---

## 🎓 進階技巧

### 1. 窗函數 (Windowing)

減少頻譜洩漏 (Spectral Leakage):

```c
// Hann Window
for(uint32_t i = 0; i < FFT_SIZE; i++)
{
    float32_t window = 0.5f - 0.5f * arm_cos_f32(2.0f * PI * i / (FFT_SIZE - 1));
    g_f32Input[i] = adc_data[i] * window;
}
```

### 2. 重疊處理 (Overlap Processing)

用於連續訊號分析:

```c
#define OVERLAP (FFT_SIZE / 2)

// 第一次 FFT
memcpy(buffer, adc_data, FFT_SIZE * sizeof(float32_t));
arm_rfft_fast_f32(&fft_instance, buffer, output, 0);

// 第二次 FFT (重疊 50%)
memcpy(buffer, &adc_data[OVERLAP], FFT_SIZE * sizeof(float32_t));
arm_rfft_fast_f32(&fft_instance, buffer, output, 0);
```

### 3. 零填充 (Zero Padding)

提高頻率解析度:

```c
#define SIGNAL_SIZE 512
#define FFT_SIZE 2048  // 4 倍零填充

for(uint32_t i = 0; i < SIGNAL_SIZE; i++)
    g_f32Input[i] = adc_data[i];

for(uint32_t i = SIGNAL_SIZE; i < FFT_SIZE; i++)
    g_f32Input[i] = 0.0f;  // 填充零

arm_rfft_fast_f32(&fft_instance, g_f32Input, g_f32Output, 0);
```

---

## 🔧 除錯技巧

### 1. 驗證輸入資料

```c
// 列印前 10 個輸入值
printf("Input data:\n");
for(uint32_t i = 0; i < 10; i++)
{
    printf("  [%d] = %.2f\n", i, g_f32Input[i]);
}
```

### 2. 檢查 DC 分量

```c
arm_rfft_fast_f32(&fft_instance, input, output, 0);
float32_t dc_value = output[0];

printf("DC component: %.2f\n", dc_value);
// DC 應該等於輸入訊號的平均值 × FFT_SIZE
```

### 3. 頻譜視覺化

```c
printf("Frequency Spectrum:\n");
printf("Index  Frequency(Hz)  Magnitude\n");
for(uint32_t k = 0; k < 20; k++)  // 顯示前 20 個頻率
{
    float32_t freq = k * SAMPLE_RATE / FFT_SIZE;
    printf("%5d  %10.2f  %10.2f\n", k, freq, g_f32Magnitude[k]);
}
```

---

## 📈 效能優化建議

### 1. 使用最佳化編譯選項

- **ARM Compiler 6**: `-O2` 或 `-O3`
- **GCC**: `-O2 -mfpu=fpv4-sp-d16 -mfloat-abi=hard`

### 2. 啟用 FPU

確保專案已啟用硬體 FPU:
```yaml
misc:
  - C:
    - -mfpu=fpv4-sp-d16
    - -mfloat-abi=hard
```

### 3. 避免頻繁初始化

```c
// ✅ 好做法: 只初始化一次
arm_rfft_fast_instance_f32 g_fft_instance;

void Init(void)
{
    arm_rfft_fast_init_f32(&g_fft_instance, FFT_SIZE);
}

void Process(void)
{
    arm_rfft_fast_f32(&g_fft_instance, input, output, 0);  // 直接使用
}
```

### 4. 使用原地運算

```c
// 節省 FFT_SIZE * 4 bytes 記憶體
arm_rfft_fast_f32(&fft_instance, g_f32Input, g_f32Input, 0);
```

---

## 📚 參考資源

### CMSIS-DSP 官方文檔
- [CMSIS-DSP 函式庫文檔](https://arm-software.github.io/CMSIS_5/DSP/html/index.html)
- [arm_rfft_fast_f32 API](https://arm-software.github.io/CMSIS_5/DSP/html/group__RealFFT.html)

### 本專案範例
- 完整程式碼: `Project/M460_MyProject/main.c`
- 專案配置: `Project/M460_MyProject/M460_MyProject.cproject.yml`
- 效能測試: 執行 `main()` 函數查看 DSP vs CPU 比較

### Nuvoton BSP 文件
- 驅動參考手冊: `Document/NuMicro M460 Series Driver Reference Guide.chm`
- CMSIS 文檔: `Document/CMSIS.html`

---

## 📝 總結

### ✅ 使用 CMSIS-DSP FFT 的優點

1. **效能**: 比純 C 實作快 **5 倍以上**
2. **精度**: 與標準演算法結果一致 (< 0.001% 誤差)
3. **易用**: API 簡單,只需 3 個函數呼叫
4. **優化**: 充分利用 Cortex-M4 DSP 指令和 FPU
5. **穩定**: ARM 官方維護,經過廣泛測試

### 🎯 關鍵要點

1. **輸入格式**: 必須是**純實數陣列**,不是複數交錯格式
2. **FFT 大小**: 必須是 **2 的次方**
3. **記憶體**: 支援**原地運算**,節省記憶體
4. **輸出格式**: 使用 `arm_cmplx_mag_f32` 計算振幅
5. **初始化**: 每個 FFT 大小只需**初始化一次**

### 🚀 立即開始

複製本專案的 `main.c` 範例,將 ADC 採樣資料替換為您的實際資料,即可開始使用高效能的 DSP FFT!

---

**版本**: v1.0  
**作者**: Nuvoton M460 BSP Project  
**日期**: 2025/11/10  
**測試平台**: M467HJHAE @ 200MHz
