# SIC/XE Linking Loader (C++17)

[English](README.md) | [繁體中文](.github/README.zh.md)

Two-pass SIC/XE linking loader in C++:
- Pass 1 builds `ESTAB`
- Pass 2 loads `T` records and applies `M` relocation

## Build
Run from project root:

```bash
cmake -S . -B build
cmake --build build
```

Output executable:
- `build/linking_loader`

## Run

### Basic (default report in root)
```bash
./build/linking_loader 4000 PROGA.obj PROGB.obj PROGC.obj
```
This writes `output.txt` in project root.

### Custom report filename/path
```bash
./build/linking_loader 4000 PROGA.obj PROGB.obj PROGC.obj --report reports/run1.txt
```

### Dump range examples
```bash
./build/linking_loader 4000 PROGA.obj PROGB.obj PROGC.obj --dump-start 0x4040 --dump-length 128
```

### CLI format
```bash
./build/linking_loader <PROGADDR_HEX> <obj1> [obj2 ...] [--dump-start <addr>] [--dump-length <bytes>] [--report <path>]
```

Arguments:
- `PROGADDR_HEX`: program load base in hex (for example `4000`)
- `obj1 obj2 ...`: object files for one linking run
- `--dump-start`: report memory dump start (default: `PROGADDR`)
- `--dump-length`: dump length in bytes (default: total linked memory span)
- `--report`: report file path (default: `output.txt`)

## Test
This project includes a C++ test that compares Python reference output and C++ output.

```bash
ctest --test-dir build --output-on-failure
```

## Project layout
- `main.cpp`: CLI + report writing
- `linking_loader.hpp/.cpp`: orchestrates pass1 + pass2
- `passers.hpp/.cpp`: pass implementations
  - `Passer1::build_estab(...)`
  - `Passer2::load_and_relocate(...)`
- `util.hpp/.cpp`: shared parsing / hex helpers
- `test/test_same_result.cpp`: output equivalence test

## Core flow
1. `LinkingLoader::linking_load(...)` calls Pass 1 to build `ESTAB` and memory span.
2. It allocates internal memory as hex nibbles (`.` default fill).
3. Pass 2 loads object text and applies relocation with ESTAB symbols.
4. Loader writes ESTAB + memory dump into report.
