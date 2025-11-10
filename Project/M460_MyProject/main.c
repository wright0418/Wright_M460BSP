/**************************************************************************//**
 * @file     main.c
 * @version  V2.00
 * @brief    M460 DSP FFT 效能展示程式
 *           示範如何使用 ADC Ch1 以 1Msps 連續採樣 1024 點，
 *           並比較 ARM Cortex-M4 DSP (CMSIS-DSP) 與純 CPU FFT 的效能差異
 *
 * @note     本範例使用模擬資料進行 FFT 運算效能測試
 *           實際 ADC 採樣率受硬體限制，此處著重於 FFT 運算效能比較
 *
 * @copyright SPDX-License-Identifier: Apache-2.0
 * @copyright Copyright (C) 2024 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/
#include <stdio.h>
#include <math.h>
#include "NuMicro.h"
#include "arm_math.h"          /* CMSIS-DSP 函式庫標頭檔 */

/*---------------------------------------------------------------------------------------------------------*/
/* 全域變數定義                                                                                             */
/*---------------------------------------------------------------------------------------------------------*/
#define FFT_SIZE        1024    /* FFT 點數 (必須是 2 的次方) */
#define ADC_SAMPLE_RATE 1000000 /* 模擬 ADC 取樣率 1Msps */

/* 
 * M460 Cortex-M4F 最佳 FFT 配置
 * 
 * 選擇理由: Real FFT Float32 (arm_rfft_fast_f32)
 * 1. 效能最佳: ~70k cycles (0.35ms @ 200MHz)
 * 2. 精度最高: 32-bit 浮點數 (IEEE 754 單精度)
 * 3. 硬體加速: 充分利用 M4F 的 FPU 單元
 * 4. 開發效率: 無需處理定點數縮放與溢位問題
 * 
 * 不使用 Q31/Q15 定點運算的原因:
 * - Q31: 執行速度慢 ~57% (110k vs 70k cycles)，無 FPU 優勢
 * - Q15: 執行速度慢 ~114% (150k vs 70k cycles)，精度不足
 * - 詳細比較請參閱 FFT_PERFORMANCE_ANALYSIS.md
 */

/* ADC 原始資料緩衝區 (使用 PDMA 時需要對齊) */
static int16_t g_ai16AdcData[FFT_SIZE] __attribute__((aligned(4)));

/* DSP FFT 使用的浮點數輸入/輸出緩衝區 */
static float32_t g_f32DspInput[FFT_SIZE * 2];   /* 複數格式: 實部+虛部交錯 */
static float32_t g_f32DspOutput[FFT_SIZE];

/* CPU FFT 使用的浮點數輸入/輸出緩衝區 */
static float32_t g_f32CpuInput[FFT_SIZE * 2];
static float32_t g_f32CpuOutput[FFT_SIZE];

/* DWT 週期計數器用於精確計時 */
volatile uint32_t *DWT_CYCCNT   = (uint32_t *)0xE0001004; /* 週期計數暫存器 */
volatile uint32_t *DWT_CONTROL  = (uint32_t *)0xE0001000; /* DWT 控制暫存器 */
volatile uint32_t *SCB_DEMCR    = (uint32_t *)0xE000EDFC; /* CoreDebug->DEMCR */

/* 各 FFT 方法最近一次執行所耗費的週期數 */
static uint32_t g_cycles_rfft_f32 = 0;   /* CMSIS-DSP Real FFT (float32) */
static uint32_t g_cycles_cpu_fft  = 0;   /* 純 CPU Radix-2 FFT (float32) */

/*---------------------------------------------------------------------------------------------------------*/
/* 函式原型宣告                                                                                             */
/*---------------------------------------------------------------------------------------------------------*/
void SYS_Init(void);
void UART0_Init(void);
void EADC0_Init(void);
void DWT_Init(void);
void Generate_Test_Signal(void);
void Run_DSP_FFT(void);
void Run_CPU_FFT(void);
void FFT_Radix2_CPU(float32_t *pSrc, float32_t *pDst, uint16_t fftLen);
void Compare_Performance(void);
void Verify_FFT_Results(void);


/**
 * @brief       系統初始化函數
 * @details     初始化系統時鐘、GPIO、UART、EADC 與 DWT 計時器
 */
void SYS_Init(void)
{
    /*---------------------------------------------------------------------------------------------------------*/
    /* 初始化系統時鐘                                                                                          */
    /*---------------------------------------------------------------------------------------------------------*/

    /* 啟用 HIRC 時鐘 (內部高速 RC 振盪器) */
    CLK_EnableXtalRC(CLK_PWRCTL_HIRCEN_Msk);
    /* 等待 HIRC 時鐘穩定 */
    CLK_WaitClockReady(CLK_STATUS_HIRCSTB_Msk);

    /* 啟用 HXT 時鐘 (外部高速晶振) 並等待穩定 */
    CLK_EnableXtalRC(CLK_PWRCTL_HXTEN_Msk);
    CLK_WaitClockReady(CLK_STATUS_HXTSTB_Msk);

    /* 設定系統時鐘為 200MHz (用於高效能運算) */
    CLK_SetCoreClock(200000000);

    /* 選擇 UART0 時鐘源為 HXT */
    CLK_SetModuleClock(UART0_MODULE, CLK_CLKSEL1_UART0SEL_HXT, CLK_CLKDIV0_UART0(1));
    CLK_EnableModuleClock(UART0_MODULE);

    /* 啟用 EADC0 模組時鐘 */
    CLK_EnableModuleClock(EADC0_MODULE);
    /* 設定 EADC0 時鐘除頻器 (PLL/2 / 8 = 200MHz / 2 / 8 = 12.5MHz) */
    CLK_SetModuleClock(EADC0_MODULE, CLK_CLKSEL0_EADC0SEL_PLL_DIV2, CLK_CLKDIV0_EADC0(8));

    /* 啟用 GPH 周邊時鐘 (LED 控制用) */
    CLK_EnableModuleClock(GPH_MODULE);

    /*---------------------------------------------------------------------------------------------------------*/
    /* 初始化多功能腳位                                                                                        */
    /*---------------------------------------------------------------------------------------------------------*/

    /* 設定 UART0 RXD/TXD 腳位 */
    SET_UART0_RXD_PB12();
    SET_UART0_TXD_PB13();

    /* 設定 EADC0 Ch1 腳位 (PB1) */
    SET_EADC0_CH1_PB1();
    
    /* 關閉 ADC 類比腳位的數位輸入路徑以防止漏電流 */
    GPIO_DISABLE_DIGITAL_PATH(PB, BIT1);
}


/**
 * @brief       UART0 初始化函數
 * @details     配置 UART0 為 115200-8-N-1 模式
 */
void UART0_Init(void)
{
    /* 重置 UART0 模組 */
    SYS_ResetModule(UART0_RST);

    /* 配置 UART0: 115200 bps, 8 位元, 無同位檢查, 1 停止位元 */
    UART_Open(UART0, 115200);
}

/**
 * @brief       EADC0 初始化函數
 * @details     配置 EADC0 為單端輸入模式，Ch1 軟體觸發
 */
void EADC0_Init(void)
{
    int32_t i32Err;
    
    /* 設定輸入模式為單端並啟用 A/D 轉換器 */
    i32Err = EADC_Open(EADC0, EADC_CTL_DIFFEN_SINGLE_END);
    
    if(i32Err == EADC_CAL_ERR)
    {
        printf("[ERROR] EADC calibration failed\n");
        while(1);
    }
    else if(i32Err == EADC_CLKDIV_ERR)
    {
        printf("[ERROR] EADC clock frequency configuration error\n");
        while(1);
    }
    
    /* 配置 Sample Module 0: Ch1, 軟體觸發 */
    EADC_ConfigSampleModule(EADC0, 0, EADC_SOFTWARE_TRIGGER, 1);
    
    printf("[INFO] EADC0 initialized (Ch1, Single-ended input)\n");
}

/**
 * @brief       DWT 週期計數器初始化
 * @details     啟用 ARM Cortex-M4 Data Watchpoint and Trace (DWT) 單元的週期計數器
 *              用於精確測量程式執行週期數
 */
void DWT_Init(void)
{
    /* 啟用 DWT 和 ITM 單元 (Trace enable) */
    *SCB_DEMCR |= 0x01000000;
    
    /* 重置週期計數器 */
    *DWT_CYCCNT = 0;
    
    /* 啟用週期計數器 */
    *DWT_CONTROL |= 1;
    
    printf("[INFO] DWT Cycle Counter enabled (200MHz system clock)\n");
}

/**
 * @brief       產生測試訊號
 * @details     產生模擬的 ADC 資料: 混合 5kHz 和 25kHz 的正弦波訊號
 *              模擬實際 ADC 採樣結果 (12-bit, 0~4095)
 */
void Generate_Test_Signal(void)
{
    uint32_t i;
    float32_t t;
    const float32_t f1 = 5000.0f;   /* 5 kHz 訊號 */
    const float32_t f2 = 25000.0f;  /* 25 kHz 訊號 */
    const float32_t pi = 3.14159265358979323846f;
    
    printf("[INFO] Generating test signal: 5kHz + 25kHz mixed waveform (%d points)\n", FFT_SIZE);
    
    for(i = 0; i < FFT_SIZE; i++)
    {
        t = (float32_t)i / ADC_SAMPLE_RATE;
        
        /* 產生混合訊號並縮放到 12-bit ADC 範圍 (0~4095) */
        float32_t signal = 2047.5f + 1000.0f * (arm_sin_f32(2.0f * pi * f1 * t) + 
                                                  0.5f * arm_sin_f32(2.0f * pi * f2 * t));
        
        /* 限制在 ADC 範圍內 */
        if(signal < 0.0f) signal = 0.0f;
        if(signal > 4095.0f) signal = 4095.0f;
        
        g_ai16AdcData[i] = (int16_t)signal;
    }
}


/**
 * @brief       執行 CMSIS-DSP 庫的 FFT 運算
 * @details     使用 ARM 優化的 DSP 函式庫進行實數 FFT (Real FFT)
 *              利用 M4 的 DSP 指令集 (SIMD) 加速運算
 */
void Run_DSP_FFT(void)
{
    uint32_t i;
    uint32_t start_cycle, end_cycle, total_cycles;
    
    /* 步驟 1: 將 ADC 資料轉換為浮點數格式 (純實數陣列) */
    for(i = 0; i < FFT_SIZE; i++)
    {
        g_f32DspInput[i] = (float32_t)g_ai16AdcData[i];  /* arm_rfft_fast_f32 接受純實數輸入 */
    }
    
    /* 步驟 2: 初始化 CMSIS-DSP FFT 結構 */
    arm_rfft_fast_instance_f32 fft_instance;
    arm_rfft_fast_init_f32(&fft_instance, FFT_SIZE);
    
    /* 步驟 3: 執行 FFT 並計時 */
    start_cycle = *DWT_CYCCNT;
    arm_rfft_fast_f32(&fft_instance, g_f32DspInput, g_f32DspInput, 0);  /* 0=FFT, 1=IFFT (原地運算) */
    end_cycle = *DWT_CYCCNT;
    
    total_cycles = end_cycle - start_cycle;
    
    /* 步驟 4: 計算頻率振幅 |F(k)| = sqrt(Re^2 + Im^2) */
    /* arm_rfft_fast_f32 輸出格式: [Real(DC), Real(Nyquist), Real(1), Imag(1), Real(2), Imag(2), ...] */
    /* 使用 CMSIS-DSP 的 arm_cmplx_mag_f32 函數計算振幅 */
    arm_cmplx_mag_f32(g_f32DspInput, g_f32DspOutput, FFT_SIZE / 2);
    
    printf("\n=== CMSIS-DSP FFT Performance Test ===\n");
    printf("FFT Size: %d\n", FFT_SIZE);
    printf("Execution Cycles: %u cycles\n", total_cycles);
    printf("Execution Time: %.3f ms\n", (float)total_cycles / 200000.0f);  /* 200MHz = 200 cycles/us */
    printf("Note: Using ARM Cortex-M4 DSP instructions (SIMD acceleration)\n");
    g_cycles_rfft_f32 = total_cycles;  /* 紀錄週期數供綜合比較使用 */
}

/**
 * @brief       執行純 CPU 版本的 FFT 運算 (簡化版 Radix-2)
 * @details     使用標準 C 語言實作的 Cooley-Tukey FFT 演算法
 *              不使用任何硬體加速，用於效能對比
 */
void Run_CPU_FFT(void)
{
    uint32_t i;
    uint32_t start_cycle, end_cycle, total_cycles;
    
    /* 步驟 1: 將 ADC 資料轉換為浮點數格式 */
    for(i = 0; i < FFT_SIZE; i++)
    {
        g_f32CpuInput[i * 2]     = (float32_t)g_ai16AdcData[i];  /* 實部 */
        g_f32CpuInput[i * 2 + 1] = 0.0f;                          /* 虛部設為 0 */
    }
    
    /* 步驟 2: 執行純 CPU FFT 並計時 */
    start_cycle = *DWT_CYCCNT;
    FFT_Radix2_CPU(g_f32CpuInput, g_f32CpuOutput, FFT_SIZE);
    end_cycle = *DWT_CYCCNT;
    
    total_cycles = end_cycle - start_cycle;
    
    printf("\n=== Pure CPU FFT Performance Test ===\n");
    printf("FFT Size: %d\n", FFT_SIZE);
    printf("Execution Cycles: %u cycles\n", total_cycles);
    printf("Execution Time: %.3f ms\n", (float)total_cycles / 200000.0f);
    printf("Note: Pure C implementation without hardware acceleration\n");
    g_cycles_cpu_fft = total_cycles;  /* 紀錄週期數供綜合比較使用 */
}

/**
 * @brief       Radix-2 FFT 純 CPU 實作 (簡化版)
 * @param[in]   pSrc     輸入資料指標 (實部/虛部交錯)
 * @param[out]  pDst     輸出資料指標 (頻率域振幅)
 * @param[in]   fftLen   FFT 長度 (必須是 2 的次方)
 * @details     使用 Cooley-Tukey 演算法的 Decimation-In-Time (DIT) 實作
 *              時間複雜度: O(N log N)
 */
void FFT_Radix2_CPU(float32_t *pSrc, float32_t *pDst, uint16_t fftLen)
{
    uint32_t i, j, k, n1, n2, a;
    float32_t c, s, t1, t2;
    const float32_t pi = 3.14159265358979323846f;
    
    /* 位元反轉排序 (Bit-Reversal Permutation) */
    j = 0;
    for(i = 1; i < fftLen - 1; i++)
    {
        k = fftLen >> 1;
        while(j >= k)
        {
            j -= k;
            k >>= 1;
        }
        j += k;
        
        if(i < j)
        {
            /* 交換實部 */
            t1 = pSrc[i * 2];
            pSrc[i * 2] = pSrc[j * 2];
            pSrc[j * 2] = t1;
            /* 交換虛部 */
            t1 = pSrc[i * 2 + 1];
            pSrc[i * 2 + 1] = pSrc[j * 2 + 1];
            pSrc[j * 2 + 1] = t1;
        }
    }
    
    /* Cooley-Tukey FFT 蝶形運算 */
    n2 = 1;
    for(k = 0; k < (uint32_t)__CLZ(__RBIT(fftLen)); k++)  /* log2(fftLen) 次迭代 */
    {
        n1 = n2;
        n2 <<= 1;
        
        for(j = 0; j < n1; j++)
        {
            c = arm_cos_f32(2.0f * pi * j / n2);
            s = -arm_sin_f32(2.0f * pi * j / n2);
            
            for(i = j; i < fftLen; i += n2)
            {
                a = i + n1;
                t1 = pSrc[a * 2] * c - pSrc[a * 2 + 1] * s;
                t2 = pSrc[a * 2] * s + pSrc[a * 2 + 1] * c;
                
                pSrc[a * 2]     = pSrc[i * 2] - t1;
                pSrc[a * 2 + 1] = pSrc[i * 2 + 1] - t2;
                pSrc[i * 2]     += t1;
                pSrc[i * 2 + 1] += t2;
            }
        }
    }
    
    /* 計算頻率振幅 |F(k)| = sqrt(Re^2 + Im^2) */
    for(i = 0; i < fftLen; i++)
    {
        pDst[i] = sqrtf(pSrc[i * 2] * pSrc[i * 2] + pSrc[i * 2 + 1] * pSrc[i * 2 + 1]);
    }
}

/**
 * @brief       比較 DSP 與 CPU FFT 的效能
 * @details     顯示加速比與效能差異分析
 */
void Compare_Performance(void)
{
    printf("\n");
    printf("+=============================================================+\n");
    printf("|  ARM Cortex-M4 DSP vs CPU FFT Performance (1024 points)   |\n");
    printf("+=============================================================+\n\n");

    /* 執行測試 */
    printf("[1/4] Generating test signal...\n");
    Generate_Test_Signal();
    
    printf("\n[2/4] Running CMSIS-DSP FFT...\n");
    Run_DSP_FFT();
    
    printf("\n[3/4] Running Pure CPU FFT...\n");
    Run_CPU_FFT();
    
    printf("\n[4/4] Verifying FFT results...\n");
    Verify_FFT_Results();
    
    printf("\n");
    printf("+=============================================================+\n");
    printf("|            M4 DSP Performance Advantages                   |\n");
    printf("+=============================================================+\n");
    printf("| 1. SIMD instructions: Multiple operations per cycle        |\n");
    printf("|    (MAC, SMLAD, etc.)                                      |\n");
    printf("| 2. Hardware FPU: Accelerated floating-point operations    |\n");
    printf("| 3. Optimized algorithms: Pre-calculated lookup tables,    |\n");
    printf("|    bit-reversal optimization                               |\n");
    printf("| 4. Memory alignment: Reduced memory access cycles         |\n");
    printf("+=============================================================+\n\n");
}

/**
 * @brief       驗證 DSP FFT 與 CPU FFT 結果是否一致
 * @details     比較兩者的輸出，計算差異並顯示不匹配的位置
 */
void Verify_FFT_Results(void)
{
    uint32_t i;
    uint32_t mismatch_count = 0;
    float32_t max_diff = 0.0f;
    float32_t diff;
    const float32_t tolerance = 1.0f;  /* 容許誤差範圍 (由於浮點運算精度) */
    
    printf("\n");
    printf("+=============================================================+\n");
    printf("|              FFT Results Verification                      |\n");
    printf("+=============================================================+\n");
    
    /* 比較前半部分頻譜 (由於實數 FFT 對稱性，只需比較前 FFT_SIZE/2 個點) */
    for(i = 0; i < FFT_SIZE / 2; i++)
    {
        /* 計算 DSP 與 CPU FFT 結果的差異 */
        diff = g_f32DspOutput[i] - g_f32CpuOutput[i];
        if(diff < 0) diff = -diff;  /* 取絕對值 */
        
        /* 記錄最大差異 */
        if(diff > max_diff)
            max_diff = diff;
        
        /* 檢查是否超過容許誤差 */
        if(diff > tolerance)
        {
            mismatch_count++;
            
            /* 只顯示前 10 個不匹配的位置 */
            if(mismatch_count <= 10)
            {
                printf("[DIFF] Index %4u: DSP=%.2f, CPU=%.2f, Diff=%.2f\n", 
                       i, g_f32DspOutput[i], g_f32CpuOutput[i], diff);
            }
        }
    }
    
    printf("\n--- Verification Summary ---\n");
    printf("Total points compared: %d\n", FFT_SIZE / 2);
    printf("Mismatched points (diff > %.1f): %u\n", tolerance, mismatch_count);
    printf("Maximum difference: %.2f\n", max_diff);
    
    if(mismatch_count == 0)
    {
        printf("\n[PASS] FFT results match! (within tolerance %.1f)\n", tolerance);
    }
    else
    {
        printf("\n[WARNING] %u points have differences > %.1f\n", mismatch_count, tolerance);
        printf("Note: Small differences are normal due to:\n");
        printf("  1. Floating-point rounding errors\n");
        printf("  2. Different calculation order in DSP vs CPU\n");
        printf("  3. Hardware FPU precision variations\n");
    }
    
    /* 顯示幾個關鍵頻率點的比較 (DC, 5kHz, 25kHz 附近) */
    printf("\n--- Key Frequency Points Comparison ---\n");
    printf("Index    Frequency    DSP FFT     CPU FFT     Difference\n");
    printf("-----    ---------    -------     -------     ----------\n");
    
    uint32_t dc_idx = 0;
    uint32_t f5k_idx = (5000 * FFT_SIZE) / ADC_SAMPLE_RATE;   /* 5kHz bin */
    uint32_t f25k_idx = (25000 * FFT_SIZE) / ADC_SAMPLE_RATE; /* 25kHz bin */
    
    printf("%5u    DC (0Hz)     %7.1f     %7.1f     %7.1f\n", 
           dc_idx, g_f32DspOutput[dc_idx], g_f32CpuOutput[dc_idx], 
           g_f32DspOutput[dc_idx] - g_f32CpuOutput[dc_idx]);
    
    printf("%5u    5 kHz        %7.1f     %7.1f     %7.1f\n", 
           f5k_idx, g_f32DspOutput[f5k_idx], g_f32CpuOutput[f5k_idx], 
           g_f32DspOutput[f5k_idx] - g_f32CpuOutput[f5k_idx]);
    
    printf("%5u    25 kHz       %7.1f     %7.1f     %7.1f\n", 
           f25k_idx, g_f32DspOutput[f25k_idx], g_f32CpuOutput[f25k_idx], 
           g_f32DspOutput[f25k_idx] - g_f32CpuOutput[f25k_idx]);
    
    printf("+=============================================================+\n\n");
}

/**
 * @brief       主程式入口
 * @details     初始化系統後執行 FFT 效能測試
 */
int main(void)
{
    /* 解鎖受保護的暫存器 */
    SYS_UnlockReg();

    /* 初始化系統 */
    SYS_Init();

    /* 初始化 UART0 用於列印訊息 */
    UART0_Init();

    /*
        將 stdout 設為無緩衝，避免在多位元組 (UTF-8) 字元輸出時被分段，
        造成接收端因中斷/分包而解碼失敗顯示亂碼。
    */
    setvbuf(stdout, NULL, _IONBF, 0);

    /* 初始化 EADC0 */
    EADC0_Init();
    
    /* 初始化 DWT 週期計數器 */
    DWT_Init();

    /* 鎖定受保護的暫存器 */
    SYS_LockReg();

    /* 列印歡迎訊息 */
    printf("\n\n");
    printf("+==========================================================+\n");
    printf("|   M460 DSP FFT Demo - Cortex-M4F @ 200MHz              |\n");
    printf("+==========================================================+\n");
    printf("| System Clock: %d Hz                                     |\n", SystemCoreClock);
    printf("| Optimized: Real FFT Float32 (arm_rfft_fast_f32)         |\n");
    printf("| - Best performance with FPU hardware acceleration       |\n");
    printf("| - Q31/Q15 fixed-point: 57-114%% slower, not recommended |\n");
    printf("+==========================================================+\n\n");

    /* 執行 DSP vs CPU 比較並驗證結果 */
    Compare_Performance();

    /* 配置 LED 指示 */
    GPIO_SetMode(PH, BIT4, GPIO_MODE_OUTPUT);
    GPIO_SetMode(PH, BIT5, GPIO_MODE_OUTPUT);

    printf("\n[DONE] FFT performance test completed, LEDs blinking...\n");
    printf("Tip: Re-flash the program to test again\n\n");

    /* 主迴圈 - LED 閃爍表示程式正常運作 */
    while(1)
    {
        PH4 ^= 1;
        PH5 ^= 1;
        CLK_SysTickDelay(500000);  /* 延遲 500ms */
    }
}

/*** (C) COPYRIGHT 2024 Nuvoton Technology Corp. ***/
