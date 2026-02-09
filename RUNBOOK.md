# Dev Runbook

## Prerequisites

- CMake 3.20+
- C++17 compiler (Clang 14+, GCC 11+, MSVC 2019+)
- Git
- Internet connection (first build fetches dependencies)

## Quick Start

```sh
cmake -B build
cmake --build build -j$(nproc)    # Linux
cmake --build build -j$(sysctl -n hw.ncpu)  # macOS
```

First configure takes a few minutes — Boost, SQLite, and Catch2 are fetched via FetchContent. Subsequent builds use the CMake cache.

## Build Targets

| Target | Output | Description |
|--------|--------|-------------|
| `liblpr` | `build/liblpr.a` | Static library — all core runtime code |
| `lpr-cli` | `build/lpr-cli` | Interactive REPL |
| `lpr-tests` | `build/lpr-tests` | Catch2 test suite |

Build a single target:

```sh
cmake --build build --target lpr-tests
```

## Running Tests

```sh
# All tests
cd build && ctest --output-on-failure

# Or run directly with Catch2 options
./build/lpr-tests

# Specific test tag
./build/lpr-tests "[arithmetic]"
./build/lpr-tests "[undo]"
./build/lpr-tests "[parser]"

# List all tags
./build/lpr-tests --list-tags
```

Test tags: `[types]`, `[stack]`, `[parser]`, `[arithmetic]`, `[undo]`, `[filesystem]`, `[programs]`

## Running the CLI

```sh
# In-memory (ephemeral)
./build/lpr-cli

# Persistent database
./build/lpr-cli mydb.lpr
```

Type RPL expressions at the `>` prompt. Stack is displayed bottom-up after each command. Type `q` or `quit` to exit.

Example session:

```
> 355 113 /
1: 355/113
> DUP ->NUM
2: 355/113
1: 3.14159292035398230088495575221238938053
> 3 4 +
3: 355/113
2: 3.14159292035398230088495575221238938053
1: 7
```

## Project Layout

```
include/lpr/lpr.h       Public C API (8 functions)
src/
  core/
    object.hpp/.cpp      Object variant (9 types), repr, serialize
    store.hpp/.cpp       SQLite persistence layer
    parser.hpp/.cpp      Single-pass RPL tokenizer
    commands.hpp/.cpp    Command registry + all built-in commands
    context.hpp/.cpp     Transactional execution engine
  lpr_api.cpp            C API implementation (bridges lpr.h → Context)
cli/main.cpp             Interactive REPL
tests/
  test_types.cpp         Object construction, repr, roundtrip
  test_stack.cpp         SQLite stack operations
  test_parser.cpp        Tokenizer coverage
  test_arithmetic.cpp    Arithmetic + type promotion
  test_undo.cpp          Snapshot, undo, redo
  test_filesystem.cpp    STO, RCL, PURGE, directories
  test_programs.cpp      Program EVAL, IFT, IFTE
```

## Dependencies

All fetched automatically at configure time — no manual install needed.

| Dependency | Version | Purpose |
|------------|---------|---------|
| Boost.Multiprecision | 1.84 | Arbitrary-precision `cpp_int`, `cpp_dec_float_50`, `cpp_rational` |
| SQLite3 | 3.46 | Stack, filesystem, undo history persistence |
| Catch2 | 3.5.2 | Testing framework |

## Clean Rebuild

```sh
rm -rf build
cmake -B build
cmake --build build -j$(sysctl -n hw.ncpu)
```

## Install

```sh
cmake --install build --prefix /usr/local
```

Installs `include/lpr/lpr.h` and `liblpr.a`.

## C API Reference

```c
lpr_ctx*   lpr_open(const char* db_path);   // NULL for in-memory
void       lpr_close(lpr_ctx* ctx);
lpr_result lpr_exec(lpr_ctx* ctx, const char* input);  // .ok = 1 on success
int        lpr_depth(lpr_ctx* ctx);
char*      lpr_repr(lpr_ctx* ctx, int level);  // caller must lpr_free()
int        lpr_undo(lpr_ctx* ctx);   // 1 on success, 0 if nothing to undo
int        lpr_redo(lpr_ctx* ctx);   // 1 on success, 0 if nothing to redo
void       lpr_free(void* ptr);
```

## Available Commands

**Stack:** `DUP`, `DROP`, `SWAP`, `OVER`, `ROT`, `DEPTH`, `CLEAR`

**Arithmetic:** `+`, `-`, `*`, `/`, `NEG`, `INV`, `ABS`, `MOD`

**Comparison:** `==`, `!=`, `<`, `>`, `<=`, `>=`

**Type conversion:** `TYPE`, `->NUM`, `->STR`, `STR->`

**Filesystem:** `STO`, `RCL`, `PURGE`, `HOME`, `PATH`, `CRDIR`, `VARS`

**Program execution:** `EVAL`, `IFT`, `IFTE`

## Troubleshooting

**First configure is slow** — Boost git clone takes 1-2 minutes. The CMake cache persists across builds, so subsequent configures are fast.

**`FetchContent_Populate` deprecation warning** — Harmless CMake 4.x warning for the SQLite fetch. Does not affect the build.

**Tests fail after schema changes** — Tests use in-memory databases, so no stale state. If you change the schema in `store.cpp`, just rebuild.

**Command not found at runtime** — Commands are case-insensitive in input (`dup` works) but registered as uppercase. Check `commands.cpp` for the registry.
