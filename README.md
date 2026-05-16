# SIC/XE Linking Loader (C++17)

[English](README.md) | [繁體中文](.github/README.zh.md)

A standalone SIC/XE linking loader implemented in C++17.  
The project reads one or more object files, performs loading + relocation, and writes a structured execution report.

## Build

Run these commands in the project root (`/Users/jeff/Desktop/Projects/linking-loader`):

```bash
cmake -S . -B build
cmake --build build
```

### Build Output Location

- CMake build directory: `build/`
- Executable path: `build/linking_loader`

## Execute

### Basic Run

```bash
./build/linking_loader prog1.obj
```

### Full CLI Form

```bash
./build/linking_loader <obj1> [obj2 ...] [--load-address 0x1000] [--dump-start 0x1000] [--dump-length 64] [--report output.txt]
```

### Arguments

- `obj1 obj2 ...`: one or more object files to load in sequence
- `--load-address`: base load address (default: `0x1000`)
- `--dump-start`: memory dump start address in report (default: `0x1000`)
- `--dump-length`: dump length in bytes (default: `64`)
- `--report`: output report file path (default: `output.txt`)

### What You Will See After Running

On success, terminal output is a single summary line:

```text
Linking completed. Entry=0x1000. Report written to: output.txt
```

Detailed data is written to the report file (`output.txt` by default), including:

- link summary
- symbol table
- memory window dump

## Project Files

- `CMakeLists.txt`: build configuration
- `main.cpp`: CLI parsing, high-level run flow, report generation
- `linking_loader.hpp`: class contract and module outline
- `linking_loader.cpp`: full linking loader implementation
- `prog1.obj`: sample object input

## `linking_loader.hpp` as the Module Outline

`linking_loader.hpp` is the top-level map of the loader design.

### 1) Core Data Models

- `TextRecord`: one text record (`T`) with address + hex payload
- `ModificationRecord`: one relocation record (`M`) with address, length, operator, symbol
- `ObjectFileData`: parsed result of one object file (`H/T/M/E` aggregate)

### 2) Internal State

- `memory_`: simulated memory image (`std::vector<std::uint8_t>`)
- `symtab_`: symbol-to-address mapping
- `symbol_order_`: insertion order for stable symbol output
- `reloc_info_`: collected relocation actions after address normalization
- `entry_point_`: resolved final execution entry address

### 3) Public API

- `reset()`: clear all loader state
- `linking_load(...)`: run full multi-file linking + relocation pipeline
- `display_symbol_table(std::ostream&)`: serialize symbol table to any output stream
- `display_memory(std::ostream&, ...)`: serialize memory window dump to any output stream

### 4) Private Helpers

The header separates parsing, loading, relocation, and numeric conversion helpers. This keeps call boundaries explicit and predictable.

## Deep Dive: `linking_loader.cpp`

## Parsing Layer (`H/T/M/E`)

- `parse_header`, `parse_text`, `parse_modification`, `parse_end` decode fixed-width record fields.
- `slice_field(...)` safely slices by offset/length.
- `parse_hex_u32` / `parse_hex_byte` enforce valid hex input and bounds.

Key behavior:

- Header defines section name/start/length.
- Text records provide raw bytes to copy into memory.
- Modification records define relocation operations (`+` / `-`) on target fields.
- End record may define section entry point.

## File Aggregation Layer

- `read_object_file(...)` reads a file line-by-line.
- It dispatches by leading record type (`H`, `T`, `M`, `E`).
- Result is normalized into one `ObjectFileData` object.

This step isolates I/O + decoding from memory mutation.

## Load Layer

- `load_program(...)` computes each text record's effective address:
  - `actual = load_base + record_addr - section_start`
- It converts hex pairs into bytes and writes into `memory_`.

This preserves object-relative layout while supporting relocated base addresses.

## Relocation Layer

- `linking_load(...)` first collects symbols and relocation requests across all input files.
- Each relocation address is normalized into final memory coordinates before execution.
- `apply_relocations(...)` resolves symbol addresses and updates target fields.

### Numeric Relocation Details

- `read_signed_be(...)` reads N-byte signed big-endian values (`1..8 bytes`).
- relocation applies `+symbol` or `-symbol`.
- `write_signed_be(...)` writes the updated value back in big-endian form.

This is the core linker behavior: symbolic references become concrete runtime addresses.

## End-to-End Loader Flow

`linking_load(...)` runs this sequence:

1. reset state
2. for each object file:
   - parse records
   - register section symbol at current load address
   - normalize and store modification records
   - copy text bytes into memory
   - resolve first valid entry point if present
   - advance current load address by section length
3. apply relocation list
4. fallback entry point to base load address if none defined
5. return final entry point

## Output Strategy

The loader module writes formatted content to `std::ostream` instead of printing directly to `stdout`.  
`main.cpp` chooses where to send output (currently the report file), which keeps the loader reusable for other front-ends.
