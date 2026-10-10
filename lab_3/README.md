# Lab 3: LIS3DSH 三軸加速度感測器與中斷同步 (EXTI & Semaphore)

## 📝 實驗摘要
本次實驗旨在將 **STM32F407 微控制器**、**LIS3DSH 晶載三軸加速度感測器** 與 **FreeRTOS 即時作業系統** 進行深度的軟硬體整合。透過 SPI 通訊協定配置感測器內部微型狀態機，結合外部硬體中斷（EXTI）與 FreeRTOS 二值訊號量（Binary Semaphore），實現高即時性事件通知與多工任務協同排程。

---

## ⚙️ 核心功能實作

### 1. SPI 通訊協定配置與感測器初始化
* **讀寫模式切換**：透過軟體控制 SPI 傳送位址的最高有效位元（`address |= 0x80`，即 MSB 設為 1），代表讀取暫存器指令。
* **微型狀態機配置**：初始化 LIS3DSH 內部狀態機，設定偵測搖晃事件的 G 值閾值（Threshold），並濾除 Z 軸常態地心引力干擾，確保觸發信號精準。

### 2. 硬體中斷與 FreeRTOS 同步機制
* **中斷腳位綁定**：將 LIS3DSH 的 `INT1` 中斷輸出腳位與 STM32 的 `PE0` 腳位連接，配置為外部中斷（EXTI）。
* **無縫環境切換 (Context Switch)**：
  * 在中斷服務常式（ISR）中呼叫 `xSemaphoreGiveFromISR` 釋放二值訊號量（Binary Semaphore）。
  * 搭配 `portYIELD_FROM_ISR` 宏，在中斷離開時立即切換上下文，喚醒高優先級任務，達到零延遲即時響應。

### 3. 多工任務排程架構
* **綠燈任務 (Green LED Task)**：週期性閃爍，代表系統正常運作的心跳指示燈（Heartbeat）。
* **橘燈警報任務 (Orange LED Alarm Task)**：平時處於阻塞狀態（Blocked）等待訊號量；一旦搖晃觸發中斷並給予 Semaphore，橘燈任務立即被喚醒並執行 5 秒警報閃爍。
* **CPU 資源釋放**：警報閃爍期間調用 `vTaskDelay` 主動出讓 CPU，確保綠燈任務在警報期間依然不受干擾地持續穩定運作。

---

## 💡 技術重點與除錯心得

1. **二值訊號量 (Binary Semaphore) vs 互斥鎖 (Mutex) 的核心差異**：
   * **Mutex**：具有「所有權（Ownership）」與「優先權繼承（Priority Inheritance）」機制，專門用於保護共享資源。
   * **Binary Semaphore**：沒有所有權概念，專門用於 ISR 與 Task 或 Task 與 Task 之間的「事件同步與通知」。中斷常式內**絕對不能使用 Mutex**，必須使用 Semaphore。
2. **RTOS 排程優勢 vs 傳統 Busy-Wait**：
   * 傳統單晶片 Super Loop 常使用 `while(HAL_GetTick()...)` 進行輪詢等待，浪費大量 CPU 週期且阻礙其他功能。
   * FreeRTOS 的 `vTaskDelay` 會將任務移入 Delayed List，讓出 CPU 給其他 Ready 任務，展現多工系統的真正並行效能。
3. **IDE 燒錄配置排錯**：
   * 釐清了編譯器輸出的二進位包裹檔（`.elf`，Artifact Name）與除錯器派件單（`.launch`）名稱必須完全一致的對應關係。