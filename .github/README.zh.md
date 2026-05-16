# SIC/XE Linking Loader (C++17)

[English](../README.md) | [繁體中文](README.zh.md)

這是一個以 C++17 實作的 SIC/XE Linking Loader。  
專案可讀取一個或多個 object 檔、執行載入與重定位，並輸出結構化報告。

## Build

請在專案根目錄（`/Users/jeff/Desktop/Projects/linking-loader`）執行：

```bash
cmake -S . -B build
cmake --build build
```

### Build 產物位置

- CMake 建置目錄：`build/`
- 可執行檔位置：`build/linking_loader`

## Execute

### 基本執行

```bash
./build/linking_loader prog1.obj
```

### 完整 CLI 形式

```bash
./build/linking_loader <obj1> [obj2 ...] [--load-address 0x1000] [--dump-start 0x1000] [--dump-length 64] [--report output.txt]
```

### 參數說明

- `obj1 obj2 ...`：依序載入的一個或多個 object 檔
- `--load-address`：基底載入位址（預設：`0x1000`）
- `--dump-start`：報告中記憶體 dump 的起始位址（預設：`0x1000`）
- `--dump-length`：dump 長度（bytes）（預設：`64`）
- `--report`：輸出報告檔路徑（預設：`output.txt`）

### 執行後會看到什麼

成功時，終端機會顯示一行摘要：

```text
Linking completed. Entry=0x1000. Report written to: output.txt
```

詳細資料會寫入報告檔（預設 `output.txt`），包含：

- linking 摘要
- symbol table
- memory window dump

## 專案檔案

- `CMakeLists.txt`：建置設定
- `main.cpp`：CLI 參數處理、整體執行流程、報告檔輸出
- `linking_loader.hpp`：類別介面與模組大綱
- `linking_loader.cpp`：linking loader 完整實作
- `prog1.obj`：範例 object input

## 以 `linking_loader.hpp` 看模組大綱

`linking_loader.hpp` 是整個 loader 的結構總覽。

### 1) 核心資料模型

- `TextRecord`：一筆 text record（`T`），包含位址與 hex payload
- `ModificationRecord`：一筆 relocation record（`M`），包含位址、長度、運算子、symbol
- `ObjectFileData`：單一 object 檔解析後的聚合結果（`H/T/M/E`）

### 2) 內部狀態

- `memory_`：模擬記憶體映像（`std::vector<std::uint8_t>`）
- `symtab_`：symbol 到位址的對應表
- `symbol_order_`：symbol 輸出順序（保持穩定）
- `reloc_info_`：位址正規化後的 relocation 工作清單
- `entry_point_`：最終解析出的程式進入點

### 3) 對外 API

- `reset()`：清空 loader 狀態
- `linking_load(...)`：執行多檔 linking + relocation 主流程
- `display_symbol_table(std::ostream&)`：將 symbol table 寫到任意輸出串流
- `display_memory(std::ostream&, ...)`：將記憶體視窗 dump 寫到任意輸出串流

### 4) 私有 helper

Header 將 parsing、loading、relocation、數值轉換清楚分區，讓呼叫邊界明確。

## 深入 `linking_loader.cpp`

## Parsing 層（`H/T/M/E`）

- `parse_header`, `parse_text`, `parse_modification`, `parse_end`：解析固定欄寬記錄。
- `slice_field(...)`：用 offset/length 做安全切片。
- `parse_hex_u32` / `parse_hex_byte`：驗證 hex 合法性與數值範圍。

關鍵行為：

- Header 提供 section 名稱 / 起點 / 長度。
- Text record 提供要寫入記憶體的 byte 資料。
- Modification record 定義 relocation（`+` / `-`）目標欄位。
- End record 可指定 section 進入點。

## 檔案聚合層

- `read_object_file(...)` 逐行讀檔。
- 依首字元分派到 `H` / `T` / `M` / `E`。
- 產生單一 `ObjectFileData` 結構。

這層把 I/O 與解碼跟記憶體寫入分離，讓流程更清楚。

## 載入層

- `load_program(...)` 計算每筆 text record 的實際位址：
  - `actual = load_base + record_addr - section_start`
- 將 hex pair 轉成 byte 寫進 `memory_`。

這樣可以保留 object 相對位址，同時支援 relocation 後的基底載入。

## 重定位層

- `linking_load(...)` 先跨檔案收集 symbols 與 relocation 清單。
- 每筆 relocation 位址先正規化為最終記憶體座標。
- `apply_relocations(...)` 解析 symbol 位址並更新目標欄位。

### 數值重定位細節

- `read_signed_be(...)` 讀取 N-byte signed big-endian（`1..8 bytes`）。
- 依 relocation 規則做 `+symbol` 或 `-symbol`。
- `write_signed_be(...)` 以 big-endian 寫回結果。

這是 linker 的核心：把符號參照轉成最終可執行位址。

## Loader 全流程

`linking_load(...)` 主要流程：

1. `reset` 清空狀態
2. 逐一處理 object 檔：
   - 解析 records
   - 以目前載入位址註冊 section symbol
   - 正規化並暫存 modification records
   - 將 text bytes 載入記憶體
   - 若有 entry 且尚未設定，記錄 entry point
   - 依 section 長度推進下一個載入位址
3. 套用 relocation 清單
4. 若無 entry，fallback 到 base load address
5. 回傳最終 entry point

## 輸出策略

Loader 模組本身用 `std::ostream` 輸出，不直接綁死 `stdout`。  
`main.cpp` 決定輸出到哪裡。
