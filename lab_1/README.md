# Lab 1: FreeRTOS 基礎應用與多任務排程

## 📝 實驗摘要
本次實驗旨在 STM32F407 開發板上實作 FreeRTOS 多工作業系統的基礎應用，主要透過兩個獨立的 Task 來控制 LED 狀態機與按鈕偵測。

## ⚙️ 核心功能實作
本實驗包含兩個主要的 Tasks，並透過 Queue 進行跨任務通訊：

* **LED Task (`vLEDTask`) - 狀態機切換**
  * **狀態 S1**: 紅、橘、綠燈各亮 1 秒輪流循環。
  * **狀態 S2**: 橘燈每 0.5 秒閃爍。
  * **技術亮點**: 捨棄傳統的 Delay，利用 `xQueueReceive` 配合 Timeout 機制，達成在等待燈號切換的同時，也能即時接收按鈕狀態變更的要求。

* **Button Task (`vButtonTask`) - 按鈕偵測與防彈跳**
  * 負責偵測板載藍色按鈕 (PA0)，並實作了 20ms 的軟體防彈跳 (Debounce) 機制。
  * 利用 `xTaskGetTickCount()` 計算按壓時間：
    * **長按 (>1秒)**: 呼叫 `vTaskSuspend` 或 `vTaskResume` 來凍結/恢復 LED Task。
    * **短按 (<1秒)**: 透過 Queue (`xQueueSend`) 將狀態切換指令發送給 LED Task。

## DEMO


https://github.com/user-attachments/assets/4bc6b0b2-8c21-4377-a79d-1572de1ef722

