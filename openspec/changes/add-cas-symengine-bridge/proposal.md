# Change: Integrate SymEngine CAS via abstract bridge interface

## Why
The Symbol type is currently a string with an eval-capable shunting-yard evaluator — it can substitute variables and decompose expressions, but it cannot perform any actual algebra. Differentiation, integration, factoring, and equation solving are the core capabilities that separate a programmable calculator from a computer algebra system. The HP 50g provides all of these through its built-in CAS; LPR needs them to reach RPL parity. SymEngine is the right first backend: BSD-licensed, C++, supports Boost.Multiprecision as its integer class (matching our existing numeric tower), and covers the polynomial/elementary-function algebra that represents 90%+ of calculator CAS usage.

## What Changes
- **New external dependency**: SymEngine fetched via CMake FetchContent, configured with `INTEGER_CLASS=boostmp` to share our Boost.Multiprecision stack (no GMP)
- **New `src/cas/` module**: Abstract `CASBridge` interface (`bridge.hpp`) and concrete `SymEngineBridge` implementation (`symengine_bridge.hpp/.cpp`)
- **String ↔ SymEngine conversion layer**: Bidirectional translation between our uppercase RPL-style expression strings (`'SIN(X)'`) and SymEngine's native expression trees (`sin(x)`), handling function name mapping, operator notation, and result normalization
- **Six new CAS commands**: DIFF, INTEGRATE, SOLVE, SIMPLIFY, EXPAND, FACTOR — registered in the command table, each delegating to the bridge
- **Context wiring**: `Context` owns a `std::unique_ptr<CASBridge>`, defaulting to `SymEngineBridge`, accessible to commands via `Context::cas()`
- **Symbol type unchanged**: Symbol keeps its `std::string value` representation. CAS operations parse on-demand, operate, and convert results back to strings. No structural change to Object or serialization.

## Impact
- Affected specs:
  - `cas-commands` — **new capability** (6 commands)
  - `object-types` — **modified** (Symbol graduates from stub to CAS-capable)
  - `build-system` — **modified** (SymEngine added to dependency list)
- Affected code:
  - `CMakeLists.txt` — SymEngine FetchContent block, link to liblpr
  - `src/cas/bridge.hpp` — new file, abstract interface
  - `src/cas/symengine_bridge.hpp/.cpp` — new files, SymEngine implementation
  - `src/core/context.hpp/.cpp` — CASBridge ownership and accessor
  - `src/core/commands.cpp` — 6 new command registrations
  - `tests/test_cas.cpp` — new test file
- Risk areas:
  - SymEngine build time (FetchContent from source) — mitigated by pinning a release and disabling tests/benchmarks
  - SymEngine's integration capabilities are limited (mainly polynomial) — documented as known limitation; Giac backend slots in later
  - Expression string round-trip fidelity — SymEngine may normalize output differently than input (e.g., reordering terms). Conversion layer must handle this gracefully.
