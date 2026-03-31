# 114-2 嵌入式作業系統分析與實作 (Embedded OS Labs)

本儲存庫記錄了 114-2 學期「嵌入式作業系統分析與實作」課程的 FreeRTOS 實驗專案與程式碼。每個 Lab 皆獨立為一個完整的 STM32CubeIDE 專案。

## 👨‍💻 作者資訊與開發環境
* **作者:** 余東蓁 (NE6141018)
* **硬體平台:** STM32F407 開發板
* **作業系統:** FreeRTOS
* **開發環境:** STM32CubeIDE (C 語言)

---

## 🚀 實驗進度總覽 (Labs Overview)

- [V] **[Lab 1: FreeRTOS 基礎應用與多任務排程](./lab_1)**
  * 實作 LED 狀態機與按鈕偵測 Task，學習 Queue 通訊機制與軟體防彈跳 (Debounce) 處理。
- [ ] **Lab 2: (待完成)**
- [ ] **Lab 3: (待完成)**
- [ ] **Lab 4: (待完成)**

---

## 🛠️ 如何使用本專案
1. 將本 Repository 下載或 Clone 至本機。
2. 開啟 STM32CubeIDE，選擇 `File` -> `Import` -> `Existing Projects into Workspace`。
3. 選擇對應的 Lab 資料夾（例如 `lab1`）即可匯入並編譯燒錄。