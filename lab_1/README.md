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

## 💡 遇到的挑戰與學習心得
* **變數可見範圍 (Scope)**: 釐清了 Handle 與 Queue 不能宣告在 `main()` 內的區域變數，否則其他 Task 會無法存取。改為全域變數後成功解決通訊問題。
* **軟體防彈跳機制**: 實作並理解了「先偵測 20ms 再次確認」的雙重確認機制，有效過濾硬體金屬彈跳雜訊與短暫干擾。
* **硬體除錯經驗**: 學會排解 ST-LINK 連線問題 ("Unable to launch") 以及從 Console 報錯行號來精準抓蟲。