/*************************************************************************//**
 * @file     main.c
 * @version  V1.00
 * @brief    WS2812 LED 驅動專案 - 使用 SPI 模擬 WS2812 時序協定
 *           WS2812 是一款智能 RGB LED，採用單總線通訊協定
 *           本專案使用 SPI 模擬其時序以驅動彩色 LED 燈條
 *
 * @note     WS2812 時序要求：
 *           - 邏輯 1: 高電平 0.8us, 低電平 0.45us
 *           - 邏輯 0: 高電平 0.4us, 低電平 0.85us  
 *           - Reset: 低電平 > 50us
 *
 * @copyright SPDX-License-Identifier: Apache-2.0
 * @copyright Copyright (C) 2021 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/
#include <stdio.h>
#include "NuMicro.h"


/**
 * @brief      系統初始化函數
 * @details    配置系統時鐘、UART0 用於除錯輸出
 *             初始化 SPI 和 GPIO 用於 WS2812 控制
 * @return     無
 */
void SYS_Init(void)
{
    /*---------------------------------------------------------------------------------------------------------*/
    /* 系統時鐘初始化                                                                                           */
    /*---------------------------------------------------------------------------------------------------------*/

    /* 啟用外部高速晶振 (HXT) */
    CLK_EnableXtalRC(CLK_PWRCTL_HXTEN_Msk);

    /* 等待 HXT 時鐘穩定 */
    CLK_WaitClockReady(CLK_STATUS_HXTSTB_Msk);

    /* 設定 PCLK0 和 PCLK1 為 HCLK/2 */
    CLK->PCLKDIV = (CLK_PCLKDIV_APB0DIV_DIV2 | CLK_PCLKDIV_APB1DIV_DIV2);

    /* 設定核心時鐘為 200MHz */
    CLK_SetCoreClock(200000000);

    /* 啟用 UART0 模組時鐘 */
    CLK_EnableModuleClock(UART0_MODULE);

    /* 選擇 UART0 時鐘源為 HIRC，分頻器設為 1 */
    CLK_SetModuleClock(UART0_MODULE, CLK_CLKSEL1_UART0SEL_HIRC, CLK_CLKDIV0_UART0(1));

    /*---------------------------------------------------------------------------------------------------------*/
    /* 多功能腳位配置                                                                                           */
    /*---------------------------------------------------------------------------------------------------------*/
    
    /* 設定 GPB 多功能腳位用於 UART0 RXD 和 TXD */
    SET_UART0_RXD_PB12();
    SET_UART0_TXD_PB13();
    
    /* TODO: 配置 SPI 多功能腳位用於 WS2812 控制 */
    /* 範例: SET_SPI0_MOSI_PA0(); */
}

/*
 * WS2812_LEDBySPI 專案樣板
 * 
 * 本專案為 M460 系列 MCU 的 WS2812 LED 控制應用樣板。
 * 使用者可基於此專案開發 WS2812 RGB LED 燈條控制應用，
 * 無需擔心 IAR/Keil/VSCode 專案設定問題。
 *
 * 本樣板應用使用外部晶振作為 HCLK 來源，並配置 UART0 輸出除錯訊息。
 * 使用者需要根據實際硬體設計進行額外的系統配置和 WS2812 驅動實現。
 * 
 * 實現建議：
 * 1. 使用 SPI 的 MOSI 腳位輸出數據到 WS2812
 * 2. 配置 SPI 時鐘頻率以匹配 WS2812 時序要求
 * 3. 可選使用 PDMA 實現無阻塞的 LED 數據傳輸
 */

int main()
{

    /* 解鎖受保護暫存器 */
    SYS_UnlockReg();

    /* 系統初始化 */
    SYS_Init();
    
    /* 初始化 UART0 為 115200-8n1 用於列印訊息 */
    UART_Open(UART0, 115200);
    
    /* 將 UART 連接到 PC，並開啟終端工具接收以下訊息 */
    /* 注意：printf 不支援中文輸出，請使用英文避免亂碼 */
    printf("\n\nCPU @ %dHz\n", SystemCoreClock);
    printf("WS2812_LEDBySPI Project Template\n");
    printf("System initialized successfully\n");

    /* TODO: 初始化 SPI 用於 WS2812 控制 */
    /* TODO: 實現 WS2812 數據編碼函數 */
    /* TODO: 實現顏色控制和動畫效果 */

    /* 主迴圈 */
    while(1)
    {
        /* TODO: 在此處添加 WS2812 LED 控制邏輯 */
        /* 範例：循環顯示不同顏色、呼吸燈效果等 */
    }

}
