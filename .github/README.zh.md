# SIC/XE Linking Loader (C++17)

[English](../README.md) | [繁體中文](README.zh.md)

這是一個 C++ 版的兩階段 SIC/XE Linking Loader：
- Pass 1 建立 `ESTAB`
- Pass 2 載入 `T` record 並套用 `M` record 重定位

## Build
請在專案根目錄執行：

```bash
cmake -S . -B build
cmake --build build
```

可執行檔：
- `build/linking_loader`

## Run

### 基本執行（預設輸出到 root 的 `output.txt`）
```bash
./build/linking_loader 4000 PROGA.obj PROGB.obj PROGC.obj
```

### 指定輸出檔路徑
```bash
./build/linking_loader 4000 PROGA.obj PROGB.obj PROGC.obj --report reports/run1.txt
```

### 指定記憶體 dump 範圍
```bash
./build/linking_loader 4000 PROGA.obj PROGB.obj PROGC.obj --dump-start 0x4040 --dump-length 128
```

### 完整指令格式
```bash
./build/linking_loader <PROGADDR_HEX> <obj1> [obj2 ...] [--dump-start <addr>] [--dump-length <bytes>] [--report <path>]
```

參數說明：
- `PROGADDR_HEX`：十六進位載入起始位址（例如 `4000`）
- `obj1 obj2 ...`：同一次 linking 要處理的 object 檔
- `--dump-start`：報告中 memory dump 起點（預設：`PROGADDR`）
- `--dump-length`：dump 長度（bytes）（預設：整段 linked memory）
- `--report`：報告檔路徑（預設：`output.txt`）

## 測試
專案提供 C++ 測試，會對照 Python 參考實作與 C++ 輸出是否一致。

```bash
ctest --test-dir build --output-on-failure
```

## 專案結構
- `main.cpp`：CLI 解析與報告輸出
- `linking_loader.hpp/.cpp`：協調 pass1/pass2 主流程
- `passers.hpp/.cpp`：兩個 pass 的實作
  - `Passer1::build_estab(...)`
  - `Passer2::load_and_relocate(...)`
- `util.hpp/.cpp`：共用 parsing / hex 工具
- `test/test_same_result.cpp`：結果一致性測試

## 核心流程
1. `LinkingLoader::linking_load(...)` 呼叫 Pass 1 建 `ESTAB` 並計算記憶體空間。
2. 以 nibble 形式配置內部記憶體（預設填 `.`）。
3. Pass 2 載入 `T` record 並用 `ESTAB` 套用 `M` record 重定位。
4. 最後輸出 `ESTAB` 與 memory dump 到報告檔。
