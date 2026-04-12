# Lab 2: Task Monitor (任務監控器)

## 📝 實驗摘要
本實驗旨在掌握 FreeRTOS 內部任務管理的機制。透過實作一個 `TaskMonitor_App`，定期遍歷系統內部的 ReadyList、DelayedList 及 OverflowDelayedList，並將各任務的狀態資訊透過 UART 傳送到電腦端進行監控。

## ⚙️ 核心功能實作
本實驗共建立 4 個任務：
1. **Red_LED_App**: 控制紅燈閃爍。
2. **Green_LED_App**: 控制綠燈閃爍。
3. **Delay_App**: 執行延時操作以改變任務狀態。
4. **TaskMonitor_App**: 每隔一段時間呼叫 `Taskmonitor()` 函數。

### Taskmonitor() 實作細節
* **遍歷列表**: 存取 FreeRTOS 內部變數 `pxReadyTasksLists`、`xDelayedTaskList1` 與 `xDelayedTaskList2`。
* **TCB 資訊解析**: 從 `TCB_t` 結構中提取以下資訊並透過 UART 輸出：
  * **Task Name**: 任務名稱。
  * **Priority**: 基礎與目前的優先權。
  * **Stack Pointer**: 目前堆疊指針。
  * **Top of Stack**: 堆疊頂端位置。
  * **Task State**: 任務目前狀態（Ready, Blocked, etc.）。

## DEMO


https://github.com/user-attachments/assets/a071b8de-3805-47ab-a165-3b171dc1372a

