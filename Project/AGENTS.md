# Nuvoton MCU 限制
- debug printf 不可以用中文，只可以用英文
- 沒有 RTOS 的使用下使用 system ticks 檢查時間做非阻塞寫法
- 有關於IP的使用 請參考 ../../Library/StdDriver/src , 以及 ../../Library/StdDriver/inc 的標頭檔案
- 有關於 IP 使用流程範例請參考 ../../SampleCode/StdDriver 目錄下的範例程式
- M4 DSP的使用請參考 CMSIS-DSP 的相關文件與範例程式 ../../Library/CMSIS/DSP_Lib
- FreeRTOS 的使用請參考 ../../ThirdParty/FreeRTOS目錄下的相關文件與範例程式
- Ethernet 的使用請參考 ../../ThirdParty/LwIP 目錄下的相關文件與範例程式
- 其他的第三方套件請參考 ../../ThirdParty 目錄下的相關文件與範例程式