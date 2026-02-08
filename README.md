# LPR Runtime

**LPR** (Lenny Polish Reverse) is a portable C++ runtime for an RPL virtual machine inspired by the HP 50g calculator. It replicates HP 50g RPL semantics with a modern architecture, backed entirely by SQLite for stack state, filesystem, and undo history.

## Overview

The runtime exposes a minimal C API built around a single idea: **everything is a command**. Pushing a number, running an operator, storing a variable, and evaluating a program all go through the same call:

```c
lpr_exec(ctx, "355");          // push integer
lpr_exec(ctx, "113");          // push integer
lpr_exec(ctx, "/");            // divide → exact rational 355/113
lpr_exec(ctx, "'X^2 + 1'");   // push symbolic expression
lpr_exec(ctx, "DIFF");         // differentiate → 2*X
```

The entire public API is 7 functions + 1 deallocator.

## Key Features

- **RPL semantics** — stack-based evaluation, HP 50g command set (DUP, DROP, SWAP, STO, RCL, etc.)
- **Arbitrary-precision numerics** — integers, reals, and rationals via Boost.Multiprecision (no GMP dependency)
- **Numeric tower with automatic promotion** — `Integer → Rational → Real → Complex`
- **SQLite-backed persistence** — stack, directory filesystem, and undo/redo history in a single `.lpr` database
- **Transactional execution** — every `lpr_exec` call is an atomic SQLite transaction with automatic snapshot/rollback
- **Computer algebra** — SymEngine backend (phase 1), with a bridge interface for swapping in Giac later
- **Cross-platform** — C++17, CMake 3.20+, targeting macOS, iOS, Linux, WASM, and embedded ARM

## Building

```sh
cmake -B build
cmake --build build
```

Dependencies (Boost.Multiprecision, SQLite, Catch2) are fetched automatically via CMake FetchContent.

## License

This project is dual-licensed.

### Open Source — AGPL-3.0

The default license for this project is the [GNU Affero General Public License v3.0](LICENSE). You are free to use, modify, and distribute this software under the terms of the AGPL-3.0. This means:

- You may use this runtime in your own projects at no cost.
- If you modify this software and distribute it, or allow users to interact with it over a network, you must release the complete source code of your modified version under the AGPL-3.0.
- Attribution is required.

The full license text is available in the [LICENSE](LICENSE) file.

### Commercial License

If the terms of the AGPL-3.0 don't work for your use case — for example, if you want to embed this runtime in a proprietary application without releasing your source code — a commercial license is available.

For inquiries, contact [lenny@lenny.zone](mailto:lenny@lenny.zone).

### Dependencies

This project's dependencies are licensed as follows:

| Dependency | License |
|---|---|
| SQLite | Public Domain |
| Boost.Multiprecision | Boost Software License 1.0 |
| SymEngine | MIT |

All dependencies use permissive licenses and impose no additional copyleft obligations on adopters.