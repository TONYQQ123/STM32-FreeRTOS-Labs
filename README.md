# 114-2 嵌入式作業系統分析與實作 (Embedded OS Labs)

本儲存庫記錄了 114-2 學期「嵌入式作業系統分析與實作」課程的 FreeRTOS 實驗專案、原始程式碼與期末專案成果。每個 Lab 及期末專案皆獨立為完整的 STM32CubeIDE 專案。

---

## 👨‍💻 作者資訊與開發環境
* **作者:** 余東蓁 (NE6141018)
* **期末專題組員:** 余東蓁、江詠翔
* **硬體平台:** STM32F407 開發板 (STM32F4-Discovery)
* **作業系統:** FreeRTOS
* **開發環境:** STM32CubeIDE (C 語言)

---

## 🚀 實驗與專題進度總覽 (Overview)

- [x] **[Lab 1: FreeRTOS 基礎應用與多任務排程](./lab_1)**
  * 實作 LED 狀態機與按鈕偵測 Task，學習 Queue 通訊機制、軟體防彈跳 (Debounce) 與任務掛起/恢復。
- [x] **[Lab 2: Task Monitor 任務監控器實作](./lab_2)**
  * 深入 FreeRTOS 核心，透過修改 `tasks.c` 遍歷 ReadyList 與 DelayedList，經 UART 輸出任務狀態與 TCB 資訊，並驗證 Tick Count 溢位機制。
- [x] **[Lab 3: LIS3DSH 三軸加速度感測器與中斷同步](./lab_3)**
  * 透過 SPI 通訊配置 LIS3DSH 感測器狀態機，利用 EXTI 硬體中斷觸發 Binary Semaphore，並透過 `portYIELD_FROM_ISR` 達成無縫 Context Switch 驅動警報任務。
- [x] **[Lab 4: FreeRTOS 記憶體管理剖析與動態區塊合併 (heap_2.c)](./lab_4)**
  * 深度修改 `heap_2.c`，實作空閒記憶體鏈結串列遍歷輸出 (`vPrintFreeList`) 與實體相鄰記憶體合併機制 (`Memory Merge`)，改善 Best-Fit 配置下的記憶體碎片化。
- [x] **[Final Project: Topic-Based Pub-Sub IPC on FreeRTOS](./final_project)**
  * 在單晶片微控制器上實作解耦的發布/訂閱（Publish-Subscribe）訊息中介器（Broker），整合 DHT11 溫濕度計、傾斜感測器、OLED 螢幕與多色 LED 警報子系統。

---

## 🛠️ 如何使用本專案

1. 將本 Repository 下載或 Clone 至本機：
   ```bash
   git clone https://github.com/TONYQQ123/STM32-FreeRTOS-Labs.git
   ```
2. 開啟 **STM32CubeIDE**。
3. 點選選單 `File` -> `Import` -> `Existing Projects into Workspace`。
4. 點選 `Browse`，選擇欲執行的專案資料夾（例如 `lab_1`、`lab_2`、`lab_3`、`lab_4` 或 `final_project`）。
5. 勾選專案後點選 `Finish` 完成匯入，即可編譯（Build）並燒錄至開發板執行。