/*
 * FreeRTOS Kernel V10.0.0
 * Copyright (C) 2017 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software. If you wish to use our Amazon
 * FreeRTOS name, please do so in a fair use way that does not cause confusion.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

#include <stdio.h>
#include <string.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"

/* Demo application includes. */
#include "partest.h"
#include "flash.h"
#include "flop.h"
#include "integer.h"
#include "PollQ.h"
#include "semtest.h"
#include "dynamic.h"
#include "BlockQ.h"
#include "blocktim.h"
#include "countsem.h"
#include "GenQTest.h"
#include "QueueSet.h"
#include "recmutex.h"
#include "death.h"

/* Hardware and starter kit includes. */
#include "NuMicro.h"

/* FatFs includes for SD Card */
#include "ff.h"
#include "diskio.h"

/* Priorities for the demo application tasks. */
#define mainFLASH_TASK_PRIORITY             ( tskIDLE_PRIORITY + 1UL )
#define mainQUEUE_POLL_PRIORITY             ( tskIDLE_PRIORITY + 2UL )
#define mainSEM_TEST_PRIORITY               ( tskIDLE_PRIORITY + 1UL )
#define mainBLOCK_Q_PRIORITY                ( tskIDLE_PRIORITY + 2UL )
#define mainCREATOR_TASK_PRIORITY           ( tskIDLE_PRIORITY + 3UL )
#define mainFLOP_TASK_PRIORITY              ( tskIDLE_PRIORITY )
#define mainCHECK_TASK_PRIORITY             ( tskIDLE_PRIORITY + 3UL )

#define mainCHECK_TASK_STACK_SIZE           ( configMINIMAL_STACK_SIZE )

/* The time between cycles of the 'check' task. */
#define mainCHECK_DELAY                     ( ( portTickType ) 5000 / portTICK_RATE_MS )

/* The LED used by the check timer. */
#define mainCHECK_LED                       ( 3UL )

/* A block time of zero simply means "don't block". */
#define mainDONT_BLOCK                      ( 0UL )

/* The period after which the check timer will expire, in ms, provided no errors
have been reported by any of the standard demo tasks.  ms are converted to the
equivalent in ticks using the portTICK_RATE_MS constant. */
#define mainCHECK_TIMER_PERIOD_MS           ( 3000UL / portTICK_RATE_MS )

/* The period at which the check timer will expire, in ms, if an error has been
reported in one of the standard demo tasks.  ms are converted to the equivalent
in ticks using the portTICK_RATE_MS constant. */
#define mainERROR_CHECK_TIMER_PERIOD_MS     ( 200UL / portTICK_RATE_MS )

/* Set mainCREATE_SIMPLE_LED_FLASHER_DEMO_ONLY to 1 to create a simple demo.
Set mainCREATE_SIMPLE_LED_FLASHER_DEMO_ONLY to 0 to create a much more
comprehensive test application.  See the comments at the top of this file, and
the documentation page on the http://www.FreeRTOS.org web site for more
information. */
#define mainCREATE_SIMPLE_LED_FLASHER_DEMO_ONLY     0

/* SD Card Test Task Priorities */
#define mainLED_TOGGLE_TASK_PRIORITY            ( tskIDLE_PRIORITY + 2UL )
#define mainSDCARD_TASK_PRIORITY                ( tskIDLE_PRIORITY + 3UL )

/* Test buffer size */
#define TEST_BUFFER_SIZE                        ( 512 )

/* Card detect source for SD Card */
#define DEF_CARD_DETECT_SOURCE                  CardDetect_From_GPIO

#define CHECK_TEST

/*-----------------------------------------------------------*/

/*
 * Set up the hardware ready to run this demo.
 */
static void prvSetupHardware( void );

/* SD Card test tasks */
static void vSDCardTestTask( void *pvParameters );
static void vLEDToggleTask( void *pvParameters );

/* SD Card interrupt handler */
void SDH0_IRQHandler(void);

/* FatFs required function - Get current time */
unsigned long get_fattime(void);

/*-----------------------------------------------------------*/

#ifdef CHECK_TEST
static void vCheckTask( void *pvParameters );
#endif

int main(void)
{
    /* Configure the hardware ready to run the test. */
    prvSetupHardware();

#ifdef CHECK_TEST
    xTaskCreate( vCheckTask, "Check", mainCHECK_TASK_STACK_SIZE, NULL, mainCHECK_TASK_PRIORITY, NULL );
#endif

    /* Create LED Toggle test task */
    xTaskCreate( vLEDToggleTask, "LED", configMINIMAL_STACK_SIZE, NULL, mainLED_TOGGLE_TASK_PRIORITY, NULL );
    
    /* Create SD Card test task */
    xTaskCreate( vSDCardTestTask, "SDCard", configMINIMAL_STACK_SIZE * 8, NULL, mainSDCARD_TASK_PRIORITY, NULL );

    /* Start standard demo/test application tasks */
    vStartPolledQueueTasks( mainQUEUE_POLL_PRIORITY );

    /* The following function will only create more tasks and timers if
    mainCREATE_SIMPLE_LED_FLASHER_DEMO_ONLY is set to 0 (at the top of this
    file).  See the comments at the top of this file for more information. */
    //prvOptionallyCreateComprehensveTestApplication();

    printf("Toggle LED_R/Y/G(PH.4~PH.6)\n");
    printf("FreeRTOS is starting ...\n");
    printf("LED Toggle and SD Card test tasks created.\n");

    /* Start the scheduler. */
    vTaskStartScheduler();

    /* If all is well, the scheduler will now be running, and the following line
    will never be reached.  If the following line does execute, then there was
    insufficient FreeRTOS heap memory available for the idle and/or timer tasks
    to be created.  See the memory management section on the FreeRTOS web site
    for more details. */
    for( ;; );
}
/*-----------------------------------------------------------*/

static void prvSetupHardware( void )
{
    /* Unlock protected registers */
    SYS_UnlockReg();

    /* Set PCLK0 and PCLK1 to HCLK/2 */
    CLK->PCLKDIV = (CLK_PCLKDIV_APB0DIV_DIV2 | CLK_PCLKDIV_APB1DIV_DIV2);

    /* Enable HIRC and HXT clock */
    CLK_EnableXtalRC(CLK_PWRCTL_HIRCEN_Msk | CLK_PWRCTL_HXTEN_Msk);
    CLK_WaitClockReady(CLK_STATUS_HIRCSTB_Msk | CLK_STATUS_HXTSTB_Msk);

    /* Set core clock to 180MHz (與官方 HBI 範例相同，確保穩定) */
    CLK_SetCoreClock(180000000);

    /* Enable all GPIO clock */
    CLK->AHBCLK0 |= CLK_AHBCLK0_GPACKEN_Msk | CLK_AHBCLK0_GPBCKEN_Msk | CLK_AHBCLK0_GPCCKEN_Msk | CLK_AHBCLK0_GPDCKEN_Msk |
                    CLK_AHBCLK0_GPECKEN_Msk | CLK_AHBCLK0_GPFCKEN_Msk | CLK_AHBCLK0_GPGCKEN_Msk | CLK_AHBCLK0_GPHCKEN_Msk;
    CLK->AHBCLK1 |= CLK_AHBCLK1_GPICKEN_Msk | CLK_AHBCLK1_GPJCKEN_Msk;

    /* Select peripheral clock source */
    CLK_SetModuleClock(UART0_MODULE, CLK_CLKSEL1_UART0SEL_HIRC, CLK_CLKDIV0_UART0(1));
    CLK_SetModuleClock(TMR0_MODULE, CLK_CLKSEL1_TMR0SEL_HIRC, 0);
    
    /* Enable SDH0 module clock and set clock source as HCLK, divider as 4 */
    CLK_EnableModuleClock(SDH0_MODULE);
    CLK_SetModuleClock(SDH0_MODULE, CLK_CLKSEL0_SDH0SEL_HCLK, CLK_CLKDIV0_SDH0(4));

    /* Enable peripheral clock */
    CLK_EnableModuleClock(UART0_MODULE);
    CLK_EnableModuleClock(TMR0_MODULE);

    /*---------------------------------------------------------------------------------------------------------*/
    /* Init I/O Multi-function                                                                                 */
    /*---------------------------------------------------------------------------------------------------------*/
    /* Set multi-function pins for UART0 RXD and TXD */
    SET_UART0_RXD_PB12();
    SET_UART0_TXD_PB13();
    
    /* Configure PH.4, PH.5 and PH.6 as Output mode for LED */
    GPIO_SetMode(PH, BIT4|BIT5|BIT6, GPIO_MODE_OUTPUT);
    
    /* Initialize LED state - turn off all LEDs (set to HIGH, assuming active LOW) */
    PH4 = 0;  /* LED off */
    PH5 = 0;  /* LED off */
    PH6 = 0;  /* LED off */
    
    /* Setup SD Card multi-function pins */
    /* Card Detect: PD13 */
    SET_SD0_nCD_PD13();
    /* CLK: PE6 */
    SET_SD0_CLK_PE6();
    /* CMD: PE7 */
    SET_SD0_CMD_PE7();
    /* D0: PE2 */
    SET_SD0_DAT0_PE2();
    /* D1: PE3 */
    SET_SD0_DAT1_PE3();
    /* D2: PE4 */
    SET_SD0_DAT2_PE4();
    /* D3: PE5 */
    SET_SD0_DAT3_PE5();
    
    /* Enable SDH0 interrupt */
    NVIC_EnableIRQ(SDH0_IRQn);

    /* Lock protected registers */
    SYS_LockReg();

    /* Init UART to 115200-8n1 for print message */
    UART_Open(UART0, 115200);
    /* 關閉 stdout 緩衝, 避免大量 printf 導致阻塞 */
    setvbuf(stdout, NULL, _IONBF, 0);

    printf("\n\n");
    printf("+------------------------------------------+\n");
    printf("|  M460 SD Card Test (FreeRTOS)            |\n");
    printf("+------------------------------------------+\n");
}

/*-----------------------------------------------------------*/

void vApplicationMallocFailedHook( void )
{
    /* vApplicationMallocFailedHook() will only be called if
    configUSE_MALLOC_FAILED_HOOK is set to 1 in FreeRTOSConfig.h.  It is a hook
    function that will get called if a call to pvPortMalloc() fails.
    pvPortMalloc() is called internally by the kernel whenever a task, queue,
    timer or semaphore is created.  It is also called by various parts of the
    demo application.  If heap_1.c or heap_2.c are used, then the size of the
    heap available to pvPortMalloc() is defined by configTOTAL_HEAP_SIZE in
    FreeRTOSConfig.h, and the xPortGetFreeHeapSize() API function can be used
    to query the size of free heap space that remains (although it does not
    provide information on how the remaining heap might be fragmented). */
    taskDISABLE_INTERRUPTS();
    for( ;; );
}
/*-----------------------------------------------------------*/

void vApplicationIdleHook( void )
{
    /* vApplicationIdleHook() will only be called if configUSE_IDLE_HOOK is set
    to 1 in FreeRTOSConfig.h.  It will be called on each iteration of the idle
    task.  It is essential that code added to this hook function never attempts
    to block in any way (for example, call xQueueReceive() with a block time
    specified, or call vTaskDelay()).  If the application makes use of the
    vTaskDelete() API function (as this demo application does) then it is also
    important that vApplicationIdleHook() is permitted to return to its calling
    function, because it is the responsibility of the idle task to clean up
    memory allocated by the kernel to any task that has since been deleted. */
}
/*-----------------------------------------------------------*/

void vApplicationStackOverflowHook( xTaskHandle pxTask, signed char *pcTaskName )
{
    ( void ) pcTaskName;
    ( void ) pxTask;

    /* Run time stack overflow checking is performed if
    configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
    function is called if a stack overflow is detected. */
    taskDISABLE_INTERRUPTS();
    for( ;; );
}
/*-----------------------------------------------------------*/

void vApplicationTickHook( void )
{
    /* This function will be called by each tick interrupt if
    configUSE_TICK_HOOK is set to 1 in FreeRTOSConfig.h.  User code can be
    added here, but the tick hook is called from an interrupt context, so
    code must not attempt to block, and only the interrupt safe FreeRTOS API
    functions can be used (those that end in FromISR()).  */

#if ( mainCREATE_SIMPLE_BLINKY_DEMO_ONLY == 0 )
    {
        /* In this case the tick hook is used as part of the queue set test. */
        vQueueSetAccessQueueSetFromISR();
    }
#endif /* mainCREATE_SIMPLE_BLINKY_DEMO_ONLY */
}
/*-----------------------------------------------------------*/
#ifdef CHECK_TEST
static void vCheckTask( void *pvParameters )
{
    portTickType xLastExecutionTime;

    xLastExecutionTime = xTaskGetTickCount();

    printf("Check Task is running ...\n");

    for( ;; )
    {
        /* Perform this check every mainCHECK_DELAY milliseconds. */
        vTaskDelayUntil( &xLastExecutionTime, mainCHECK_DELAY );
        if( xArePollingQueuesStillRunning() != pdTRUE )
        {
            printf( "ERROR IN POLL Q\n" );
        }
    }
}
#endif

/*-----------------------------------------------------------*/
/* LED Toggle Task Implementation                           */
/*-----------------------------------------------------------*/
/* 
 * 獨立的 LED 閃爍任務
 * 循環點亮三顆 LED (PH.4, PH.5, PH.6)
 * 用於視覺化確認 FreeRTOS 排程器正常運作
 */
static void vLEDToggleTask( void *pvParameters )
{
    uint8_t ledIndex = 0;
    uint32_t loopCount = 0;
    ( void ) pvParameters;
    printf("[LED Task] Start (周期 500ms)\n");
    for( ;; )
    {
        /* 簡化輸出減少 UART 阻塞 */
        PH4 = 0; PH5 = 0; PH6 = 0;
        if( ledIndex == 0 )      PH4 = 1;
        else if( ledIndex == 1 ) PH5 = 1;
        else                     PH6 = 1;
        ledIndex = (ledIndex + 1) % 3;
        if( (loopCount++ % 40) == 0 ) /* 每 20 秒列印一次狀態 */
            printf("[LED Task] Alive, loop=%lu\n", (unsigned long)loopCount);
        vTaskDelay( pdMS_TO_TICKS( 500 ) );
    }
}

/*-----------------------------------------------------------*/
/* SD Card Test Task Implementation                         */
/*-----------------------------------------------------------*/
static void vSDCardTestTask( void *pvParameters )
{
    FATFS fs;           /* FatFs file system object */
    FIL file;           /* File object */
    FRESULT res;        /* FatFs function result */
    UINT bytesWritten, bytesRead;
    char testData[TEST_BUFFER_SIZE];
    char readBuffer[TEST_BUFFER_SIZE];
    uint32_t i;
    
    /* Remove compiler warning */
    ( void ) pvParameters;
    
    printf("[SD Card Task] Starting SD Card initialization...\n");
    
    /* Wait a bit for system stabilization */
    vTaskDelay( pdMS_TO_TICKS( 1000 ) );
    
    /* Initialize SD Card - SD initial state needs 300KHz clock output */
    if( SDH_Open_Disk(SDH0, DEF_CARD_DETECT_SOURCE) != 0 )
    {
        printf("[SD Card Task] ERROR: SD Card initialization failed!\n");
        printf("[SD Card Task] Please check if SD card is inserted.\n");
        /* Task will continue to retry */
    }
    else
    {
        printf("[SD Card Task] SD Card initialized successfully.\n");
        printf("[SD Card Task] Card capacity: %d KB\n", SDH_GET_CARD_CAPACITY(SDH0));
    }
    
    for( ;; )
    {
        /* Check if card is present */
        if( !SDH_CardDetection(SDH0) )
        {
            printf("[SD Card Task] No SD card detected. Waiting...\n");
            vTaskDelay( pdMS_TO_TICKS( 3000 ) );
            continue;
        }
        
        /* Mount the file system */
        printf("[SD Card Task] Mounting file system...\n");
        res = f_mount(&fs, "0:", 1);  /* Mount volume 0 */
        
        if( res != FR_OK )
        {
            printf("[SD Card Task] ERROR: Mount failed (res=%d)\n", res);
            vTaskDelay( pdMS_TO_TICKS( 5000 ) );
            continue;
        }
        
        printf("[SD Card Task] File system mounted successfully.\n");
        
        /* Prepare test data */
        for( i = 0; i < TEST_BUFFER_SIZE; i++ )
        {
            testData[i] = (char)('A' + (i % 26));
        }
        testData[TEST_BUFFER_SIZE - 1] = '\0';
        
        /* Create and write to test file */
        printf("[SD Card Task] Creating test file: test.txt\n");
        res = f_open(&file, "0:test.txt", FA_CREATE_ALWAYS | FA_WRITE);
        
        if( res == FR_OK )
        {
            printf("[SD Card Task] Writing test data...\n");
            res = f_write(&file, testData, strlen(testData), &bytesWritten);
            
            if( res == FR_OK )
            {
                printf("[SD Card Task] Write OK: %d bytes written\n", bytesWritten);
            }
            else
            {
                printf("[SD Card Task] ERROR: Write failed (res=%d)\n", res);
            }
            
            f_close(&file);
        }
        else
        {
            printf("[SD Card Task] ERROR: File open failed (res=%d)\n", res);
        }
        
        /* Read back the test file */
        printf("[SD Card Task] Reading test file...\n");
        res = f_open(&file, "0:test.txt", FA_READ);
        
        if( res == FR_OK )
        {
            res = f_read(&file, readBuffer, TEST_BUFFER_SIZE - 1, &bytesRead);
            
            if( res == FR_OK )
            {
                readBuffer[bytesRead] = '\0';
                printf("[SD Card Task] Read OK: %d bytes read\n", bytesRead);
                
                /* Verify data */
                if( memcmp(testData, readBuffer, bytesRead) == 0 )
                {
                    printf("[SD Card Task] SUCCESS: Data verification passed!\n");
                }
                else
                {
                    printf("[SD Card Task] ERROR: Data verification failed!\n");
                }
            }
            else
            {
                printf("[SD Card Task] ERROR: Read failed (res=%d)\n", res);
            }
            
            f_close(&file);
        }
        else
        {
            printf("[SD Card Task] ERROR: File open for read failed (res=%d)\n", res);
        }
        
        /* Unmount file system */
        f_mount(NULL, "0:", 0);
        
        printf("[SD Card Task] Test cycle completed. Waiting 10 seconds...\n\n");
        
        /* Wait before next test cycle */
        vTaskDelay( pdMS_TO_TICKS( 10000 ) );
    }
}

/*-----------------------------------------------------------*/
/* SD Card Interrupt Handler                                */
/*-----------------------------------------------------------*/
void SDH0_IRQHandler(void)
{
    unsigned int volatile isr;
    unsigned int volatile ier;

    /* FMI data abort interrupt */
    if( SDH0->GINTSTS & SDH_GINTSTS_DTAIF_Msk )
    {
        /* Reset all engine */
        SDH0->GCTL |= SDH_GCTL_GCTLRST_Msk;
    }

    /* SD interrupt status */
    isr = SDH0->INTSTS;
    ier = SDH0->INTEN;

    if( isr & SDH_INTSTS_BLKDIF_Msk )
    {
        /* Block down */
        SD0.DataReadyFlag = TRUE;
        SDH0->INTSTS = SDH_INTSTS_BLKDIF_Msk;
    }

    if( (ier & SDH_INTEN_CDIEN_Msk) && (isr & SDH_INTSTS_CDIF_Msk) )
    {
        /* Card detect interrupt */
        /* Delay to make sure got updated value from REG_SDISR */
        volatile int delay_i;
        for( delay_i = 0; delay_i < 0x500; delay_i++ );
        
        isr = SDH0->INTSTS;

        if( isr & SDH_INTSTS_CDSTS_Msk )
        {
            /* Card removed */
            SD0.IsCardInsert = FALSE;
            memset(&SD0, 0, sizeof(SDH_INFO_T));
        }

        SDH0->INTSTS = SDH_INTSTS_CDIF_Msk;
    }

    /* CRC error interrupt */
    if( isr & SDH_INTSTS_CRCIF_Msk )
    {
        SDH0->INTSTS = SDH_INTSTS_CRCIF_Msk;
    }
}

/*-----------------------------------------------------------*/
/* FatFs Required Function - Get Current Time               */
/*-----------------------------------------------------------*/
/* 
 * 這是 FatFs 模組需要的即時時鐘服務函式
 * 即使系統不支援 RTC，也必須回傳任何有效的時間
 * 此函式在唯讀配置中不需要
 * 
 * 回傳值格式 (32-bit):
 * bit31:25  Year from 1980 (0..127)
 * bit24:21  Month (1..12)
 * bit20:16  Day (1..31)
 * bit15:11  Hour (0..23)
 * bit10:5   Minute (0..59)
 * bit4:0    Second / 2 (0..29)
 */
unsigned long get_fattime(void)
{
    /* 回傳一個固定的時間戳記 */
    /* 2024/11/11 12:00:00 */
    return ((2024 - 1980) << 25) | (11 << 21) | (11 << 16) | (12 << 11) | (0 << 5) | (0 >> 1);
}

