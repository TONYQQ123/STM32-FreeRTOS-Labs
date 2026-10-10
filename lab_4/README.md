# Lab 4: FreeRTOS 記憶體管理剖析與動態區塊合併 (heap_2.c)

## 📝 實驗摘要
本次實驗深入剖析並實作 FreeRTOS 核心記憶體管理（Memory Management）機制。實驗以官方的 `heap_2.c` 底層架構為基礎，探討動態記憶體分配以及長時間運作下的記憶體碎片化（Fragmentation）問題。透過親自走訪空閒鏈結串列並實作相鄰區塊合併機制，大幅改善記憶體利用效率。

---

## ⚙️ 核心功能實作

### Part 1: 記憶體鏈結串列視覺化觀察 (`vPrintFreeList`)
* **走訪 Free List**：在 `heap_2.c` 內部實作走訪空閒記憶體鏈結串列的函式，從起始節點（`xStart`）依序遍歷整個 Linked List。
* **節點數據擷取**：擷取並計算各空閒區塊的關鍵資訊：
  * **Start Address**：區塊起始實體位址。
  * **heapSTRUCT_SIZE**：結構體頭部大小。
  * **Block Size**：可用空間大小。
  * **End Address**：區塊結束位址。
* **UART 輸出**：將走訪結果格式化並透過 UART 傳送至終端機，動態監控系統堆積記憶體的分配變化。

### Part 2: 相鄰空閒區塊合併機制 (Memory Merge)
* **問題背景**：原生的 `heap_2.c` 使用最佳適應（Best-Fit）演算法分配記憶體，但在釋放（`vPortFree`）時不會將相鄰的空閒記憶體合併，極易產生外部碎片。
* **修改核心巨集**：修改 `prvInsertBlockIntoFreeList` 巨集：
  * **實體位址相鄰判定**：檢查釋放區塊的結束位址是否與相鄰空閒區塊的起始位址相連（`Address Continuity`）。
  * **區塊融合**：若實體位址連續，將相鄰區塊合併為單一較大連續區塊。
  * **遞增排序維護**：合併後嚴格維持 Free List 依區塊大小（Block Size）由小至大遞增排序（Ascending Order）。

### 驗證與測試
* 建立多個測試 Task，部分任務啟動後立即呼叫 `vTaskDelete(NULL)` 自行銷毀並釋放資源。
* 於終端機對比「無 Merge 機制」與「加入 Merge 機制」下的 Free List 狀態，證實碎裂區塊成功被重新融合成完整大區塊。

---

## 💡 技術重點與除錯心得

1. **巨集 (Macro) 展開中的指標與變數作用域**：
   * `prvInsertBlockIntoFreeList` 是一個巨集而非一般函式，在操作 `pxBlockHolding` 與 `pxBlockToInsert` 等指標時，展開後若變數處理不慎極易造成鏈結串列斷裂或引發 Hard Fault。
2. **專案 Include Path 設定**：
   * 當手動匯入自訂或官方樣板檔案時，若未在 STM32CubeIDE 的 `Properties -> C/C++ General -> Paths and Symbols` 中正確新增 FreeRTOS 標頭檔路徑，會引發 `fatal error: FreeRTOS.h: No such file or directory`。
3. **記憶體管理演算法權衡**：
   * 透過實驗深刻體會到 `heap_2` 適合任務重複建立但大小固定的場景；而具備自動位址排序與相鄰合併特性的 `heap_4` 則能徹底對抗碎片化，理解了作業系統底層演進的必然性。