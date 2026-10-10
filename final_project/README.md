# Final Project: Topic-Based Pub-Sub IPC on FreeRTOS

> **A decoupled message broker connecting sensors, display, and alarm subsystems on a single MCU**  

---

## 📌 專案簡介 (Overview)

在傳統的嵌入式 FreeRTOS 應用中，任務間通訊多半採用一對一的 Queue，導致任務之間產生強烈耦合（Publisher 必須明確知道每一個 Consumer，新增訂閱者就必須修改發布者程式碼）。

本專題在 STM32F407 單晶片微控制器上實作了一個**輕量級、基於主題（Topic-Based）的發布/訂閱（Publish-Subscribe）IPC 訊息中介器（Broker）**。透過中介層徹底解耦感測端、顯示介面與警報控制系統，實現一對多廣播、高可擴展性且不產生記憶體碎片的嵌入式架構。

---

## 🏛️ 三層系統架構 (Three-Layer Architecture)

系統嚴格遵循分層封裝原則，上層對下層的底層實作細節保持透明：

```
+--------------------------------------------------------------+
| 1. Application Layer                                         |
|    - pubsub.c (Broker 核心), uart_log.c, app_tasks.c         |
+--------------------------------------------------------------+
                               ▲
+--------------------------------------------------------------+
| 2. Driver Layer                                              |
|    - dht11.c (封裝 1-Wire 協定與 DWT 微秒計時)               |
+--------------------------------------------------------------+
                               ▲
+--------------------------------------------------------------+
| 3. Hardware Layer (CubeMX Generated)                         |
|    - gpio.c / i2c.c / spi.c / usart.c                        |
+--------------------------------------------------------------+
```

---

## ⚡ 核心設計決策 (Design Decisions)

### 1. 全靜態記憶體配置 (Static Allocation Everywhere)
* 主題表（Topic Table，上限 8 個）與訂閱者清單（Subscriber Instances，每主題上限 5 個）均為編譯期決定的固定陣列。
* 杜絕 Heap 碎片化風險，在燒錄前即能精確預估系統記憶體佔用。
* 訊息採值複製（Copy-by-value），每則訊息大小固定為 96 Bytes。

### 2. 非阻塞訊息派發 (Non-blocking Dispatch, Timeout = 0)
* 當某個訂閱者的 Queue 已滿時，Broker 會直接丟棄該訊息並累加丟失計數器（Drop Counter），**絕不阻塞發布者**。
* 確保 Publisher 執行時間具備確定性上限（Bounded Execution Time），慢速的 Subscriber 不會拖慢關鍵感測任務。

### 3. 雙鎖死鎖防範 (Deadlock Prevention: Two Locks, Never Overlapping)
* 系統中包含兩個互斥鎖：
  * `topic_mutex`：保護主題表與訂閱關係。
  * `log_mutex`：保護 UART 輸出，避免多任務列印字串交錯。
* **快速派發設計**：在持有 `topic_mutex` 時僅快速複製訂閱者清單，隨即**立刻釋放 `topic_mutex`**，後續耗時的 Message Dispatch 與 UART Logging 則在鎖外執行，極大化系統響應速度。
* 所有 `subscribe()` 均在排程器啟動前（Before Scheduler Starts）完成，執行期間訂閱名單為唯讀狀態，保證執行緒安全。

---

## 📡 主題 (Topics) 與資料流

系統配置 4 個核心主題（使用 4 / 8）：

| Topic | Publisher | Subscribers | 用途說明 |
| :--- | :--- | :--- | :--- |
| `temperature` | `sensor_task` | `ui_task`, `control_task` | OLED 溫度顯示與 LED 警報門檻判定 |
| `humidity` | `sensor_task` | `ui_task` | OLED 濕度列更新 |
| `tilt` | `sensor_task` | `ui_task`, `control_task` | OLED 傾斜狀態顯示與警報觸發 |
| `alarm` | `control_task` | `alarm_task` | 觸發紅藍雙色 LED 閃爍與 UART 警報警告 |

---

## 🧵 任務配置表 (Task Configurations)

| Task Name | 優先權 (Priority) | 堆疊大小 (Stack) | 職責 (Role) | 觸發機制 (Trigger) |
| :--- | :---: | :---: | :--- | :--- |
| `sensor_task` | 3 (最高) | 320 words | 讀取 DHT11 與傾斜感測器並 Publish | 週期性 (每 2 秒) |
| `ui_task` | 2 | 320 words | 刷新 SSD1315 OLED 螢幕顯示 | Queue 事件驅動 |
| `control_task` | 2 | 320 words | 檢查溫度與傾斜門檻，控制 LED 或發出 Alarm | Queue 事件驅動 |
| `alarm_task` | 2 | 320 words | 執行紅藍 LED 閃爍與 UART 警告輸出 | Queue 事件驅動 |
| `stats_task` | 1 (最低) | 256 words | 統計並輸出 Broker 傳輸與丟包數據 | 週期性 (每 30 秒) |

> 🔍 **為什麼任務堆疊配置 320 words？**  
> 由於各任務皆頻繁調用 `snprintf` 與 `uart_logf`，C 語言標準庫中的格式化字串輸出極度耗費堆疊空間，因此放大堆疊以避免 Stack Overflow。此外，訂閱者平時使用 `xQueueReceive(..., portMAX_DELAY)` 保持休眠，零浪費 CPU 效能。

---

## 🔌 硬體配置與腳位對應 (Hardware Map)

| 外設 (Peripheral) | 連接腳位 (Pins) | 通訊協定 / 備註 |
| :--- | :--- | :--- |
| **UART Log** | `PA2` (USART2_TX) | 115200 8N1，接至 USB-to-TTL 模組 |
| **DHT11 溫濕度** | `PC2` | 1-Wire 協定，利用 DWT 週期計數器實現微秒級延時 |
| **傾斜感測器** | `PC1` | 數位輸入 (Digital Input)，LOW 代表傾斜狀態 |
| **OLED 螢幕 (SSD1315)** | `PB6` (SCL) / `PB9` (SDA) | I2C1 介面 (位址 `0x3C`)，128×64 解析度，8×8 字元 |
| **板載 LEDs** | `PD12` ~ `PD15` | 綠 (Normal) / 橘 (Warn) / 紅 (Hot) / 藍 (Alarm) |

### 溫度警報門檻 (Temperature Thresholds)
* **>= 36 °C (ALARM)**：觸發 `alarm` 主題，紅燈與藍燈交替閃爍，UART 發出緊急警告。
* **>= 33 °C (HOT)**：點亮紅燈。
* **>= 28 °C (WARN)**：點亮橘燈。
* **< 28 °C (NORMAL)**：點亮綠燈（常態指示）。

---

## Demo

https://github.com/user-attachments/assets/4b68fe1b-42aa-48fe-8d87-9cc66bc72d44


