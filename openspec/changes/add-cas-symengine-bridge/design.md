## Context

LPR's Symbol type is a string-backed symbolic expression. Existing infrastructure (SUBST, EXPLODE, ASSEMBLE, the shunting-yard evaluator) operates on the string representation via tokenization. This change adds a computer algebra backend (SymEngine) without disrupting that infrastructure, accessed through an abstract bridge so Giac can replace or supplement it later.

Key stakeholders: the CLI user who expects HP 50g-style CAS commands, and future iOS/WASM hosts that will call these commands through `lpr_exec`.

Constraints:
- C++17 (SymEngine requires C++11, compatible)
- No GMP dependency (SymEngine supports `boostmp` integer class)
- All dependencies via FetchContent
- Symbol's string representation must remain the serialization format (SQLite roundtrip)

## Goals / Non-Goals

Goals:
- Six working CAS commands: DIFF, INTEGRATE, SOLVE, SIMPLIFY, EXPAND, FACTOR
- Abstract bridge interface enabling future Giac backend
- SymEngine built from source via FetchContent with Boost.Multiprecision integers
- Bidirectional conversion between RPL expression strings and SymEngine trees
- Comprehensive tests for each command and the conversion layer

Non-Goals:
- Changing Symbol's internal representation (stays `std::string`)
- Reimplementing SUBST/EXPLODE/ASSEMBLE via SymEngine (string-based approach is fine)
- Definite integration (only indefinite)
- Multi-variable solve or systems of equations (single variable, single equation)
- Numeric solvers (NSOLVE) — that's a separate future command
- Giac backend implementation (deferred to future change)

## Decisions

### D1: String-based bridge pattern (no Symbol struct change)

CAS operations follow a parse → operate → serialize cycle:
1. `SymEngineBridge` receives the Symbol's string value
2. Parses it into a SymEngine `RCP<const Basic>` expression tree
3. Performs the algebraic operation
4. Converts the result back to a string
5. Returns a new `Symbol{result_string}`

**Why:** All existing code (SUBST, EXPLODE, ASSEMBLE, serialization, repr) works on `Symbol.value` strings. Changing Symbol to hold a SymEngine expression tree would require rewriting all of that code for no functional benefit. The parse/unparse overhead is negligible for interactive calculator use.

**Alternatives considered:**
- *Dual representation* (string + cached `RCP<const Basic>`): Adds complexity (cache invalidation after SUBST, serialization of SymEngine objects) with minimal benefit. Rejected.
- *SymEngine as primary representation*: Maximum disruption to existing code, couples the core Object system to SymEngine, makes Giac swap harder. Rejected.

### D2: SymEngine v0.14.0 with `INTEGER_CLASS=boostmp`

Pin SymEngine **v0.14.0** (released 2025-02-17). This is the latest stable release and includes the most boostmp fixes: division-by-zero handling (PR #1942), Boost 1.76+ compatibility (#1803), rational number fixes (#1788), and restored -O3 optimization for Clang+boostmp builds (#1876).

Configure SymEngine with Boost.Multiprecision as its integer backend and all optional native dependencies disabled:

```cmake
FetchContent_Declare(
    symengine
    GIT_REPOSITORY https://github.com/symengine/symengine.git
    GIT_TAG        v0.14.0
    GIT_SHALLOW    TRUE
)

# SymEngine options — BEFORE FetchContent_MakeAvailable
set(INTEGER_CLASS "boostmp" CACHE STRING "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)
set(WITH_SYMENGINE_RCP ON CACHE BOOL "" FORCE)
set(WITH_MPFR OFF CACHE BOOL "" FORCE)
set(WITH_MPC OFF CACHE BOOL "" FORCE)
set(WITH_FLINT OFF CACHE BOOL "" FORCE)
set(WITH_ARB OFF CACHE BOOL "" FORCE)
set(WITH_LLVM OFF CACHE BOOL "" FORCE)
set(WITH_PTHREAD OFF CACHE BOOL "" FORCE)
set(WITH_OPENMP OFF CACHE BOOL "" FORCE)
set(WITH_BFD OFF CACHE BOOL "" FORCE)
set(WITH_ECM OFF CACHE BOOL "" FORCE)
set(WITH_PRIMESIEVE OFF CACHE BOOL "" FORCE)
set(WITH_PIRANHA OFF CACHE BOOL "" FORCE)
set(WITH_TCMALLOC OFF CACHE BOOL "" FORCE)
```

**Why:** We already depend on Boost.Multiprecision for our numeric tower. Using `boostmp` avoids pulling in GMP (which complicates iOS cross-compilation) and means SymEngine's integers are the same type as ours — no conversion needed when extracting numeric solutions from SOLVE. Disabling all optional deps (FLINT, ARB, MPFR, etc.) ensures zero external native dependencies, critical for iOS and embedded ARM targets.

**Alternatives considered:**
- *GMP integer class*: Better SymEngine performance, but adds a native dependency that breaks our "no GMP" constraint and complicates cross-compilation. Rejected.
- *SymEngine's built-in `piranha`*: Experimental, not well-tested. Rejected.
- *Older SymEngine release (v0.12.0, v0.13.0)*: Fewer boostmp fixes, missing the -O3 Clang restoration. No benefit. Rejected.

**iOS / embedded cross-compilation notes:**
- iOS toolchain files must set `CMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY` — SymEngine's configure-time `try_compile()` checks fail during cross-compilation without this because they try to link executables for the target platform.
- boostmp mode is CI-tested on Linux (GCC, Clang) and Windows (MSVC) but not on Apple Clang or ARM. The -O3 segfault issue with Apple Clang was reportedly fixed in v0.14.0 (PR #1876), but if iOS builds segfault at -O3, fall back to `-O2 -DNDEBUG`.
- Boost must be fetched *before* SymEngine so that SymEngine's `find_package(Boost)` resolves to our FetchContent copy, not a system installation.
- For CMake 3.20–3.24, use the manual `FetchContent_Populate` + `add_subdirectory(... EXCLUDE_FROM_ALL)` pattern to avoid SymEngine's install targets polluting the host project. CMake 3.25+ handles this automatically.

### D3: Function name mapping in the conversion layer

Our expressions use HP 50g-style uppercase function names (`SIN`, `COS`, `LN`, `EXP`, `SQRT`, `SQ`). SymEngine uses lowercase mathematical names (`sin`, `cos`, `log`, `exp`, `sqrt`, `pow`).

The conversion layer maintains a bidirectional map:

| RPL name | SymEngine equivalent |
|----------|---------------------|
| SIN      | sin                 |
| COS      | cos                 |
| TAN      | tan                 |
| ASIN     | asin                |
| ACOS     | acos                |
| ATAN     | atan                |
| EXP      | exp                 |
| LN       | log                 |
| LOG      | `log(x)/log(10)`    |
| SQRT     | sqrt                |
| SQ       | `pow(x, 2)`        |
| ABS      | `Abs`               |
| ATAN2    | atan2               |

`to_symengine()` tokenizes our expression string, maps function names, and feeds the transformed string to `SymEngine::parse()`. `from_symengine()` calls `SymEngine::str()` on the result and maps names back to uppercase RPL conventions.

**Why:** Direct string-to-string translation before parsing is simpler than building SymEngine expressions programmatically from our token stream. SymEngine's parser handles operator precedence, parentheses, and implicit multiplication.

**Edge cases:**
- `LN` maps to SymEngine's `log` (natural logarithm). `LOG` (base-10) maps to `log(x)/log(10)` — both are fully supported through the conversion layer.
- `SQ(X)` is rewritten to `(X)**2` before parsing.
- `ATAN2(Y, X)` maps directly to SymEngine's `atan2(y, x)`.
- Functions SymEngine doesn't recognize (e.g., `IFTE`) cause a descriptive error — CAS operations are only meaningful on mathematical expressions.

### D4: SOLVE returns a List

`SOLVE` pushes a `List` of solutions, consistent with HP 50g semantics:
- `'X^2-4' 'X' SOLVE` → `{ -2 2 }`
- `'X-3' 'X' SOLVE` → `{ 3 }`
- No solutions → empty list `{ }`

Solutions that are exact integers or rationals are returned as `Integer` or `Rational` objects, not Symbols. This matches the HP 50g behavior where numeric solutions are numeric.

**Why:** Returning a List is the natural RPL representation for a set of solutions. The HP 50g does exactly this. Individual solutions can be extracted with `GET` or iterated with `DOLIST`.

### D5: Bridge ownership in Context

`Context` owns a `std::unique_ptr<CASBridge>`, initialized to `std::make_unique<SymEngineBridge>()` at construction. Commands access it via `ctx.cas()`.

```cpp
class Context {
    std::unique_ptr<CASBridge> cas_bridge_;
public:
    CASBridge& cas() { return *cas_bridge_; }
};
```

**Why:** Single ownership, no lifetime ambiguity. The bridge is stateless (no cached expressions), so construction is cheap. Future enhancement: a `set_cas_backend()` method or constructor parameter for swapping to Giac.

### D6: Numeric formatting within CAS results

Numeric values appearing inside symbolic expressions use an **auto numerical form**: integers as `"2"`, reals with a trailing dot or decimal places as `"2."` or `"2.34"` — no FIX/SCI/ENG formatting applied inside expression strings. This keeps CAS output clean and mathematically standard.

Standalone numeric results (e.g., Integer or Rational objects returned by SOLVE) use the **system display settings** (FIX/SCI/ENG as set by the user), consistent with how all other commands format numeric output.

**Why:** Applying FIX 4 inside a symbolic derivative would produce unreadable expressions like `'2.0000*X+3.0000'`. The auto form is what every CAS uses. But when SOLVE extracts a concrete number onto the stack, the user expects it formatted like every other number.

### D7: Error handling

CAS errors follow the existing runtime pattern — push an `Error` object and throw internally so the transaction rolls back:

- Wrong argument types (e.g., DIFF on an Integer): `"Error: DIFF requires a symbolic expression"`
- Parse failure (malformed expression): `"Error: Cannot parse expression for CAS operation"`
- Operation not supported (e.g., SymEngine can't integrate a given expression): `"Error: INTEGRATE: unable to find antiderivative"`
- Unknown function in expression: `"Error: Unknown function 'IFTE' in CAS expression"`

**Why:** Consistent with how every other command handles errors. The user sees the error on the stack; undo restores the pre-error state.

## Risks / Trade-offs

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|--------|------------|
| SymEngine FetchContent build is slow (5-10 min first configure) | High | Dev friction | Pin release tag, disable tests/benchmarks/optional deps. Document expected build time. |
| SymEngine's `parse()` doesn't handle our expression format | Medium | Blocks conversion | Pre-transform our string (function name mapping, `^` handling) before calling `parse()`. Fallback: build SymEngine expressions programmatically from our token stream. |
| SymEngine can't integrate non-polynomial expressions | High (known) | Incomplete CAS | Document as known limitation. INTEGRATE returns an error for unsupported integrands. Giac backend will fill this gap later. |
| SymEngine output normalization differs from input | Medium | Cosmetic | Accept SymEngine's canonical form. E.g., `2*X+3` may come back as `3 + 2*x` — the conversion layer normalizes capitalization but doesn't reorder terms. |
| SymEngine + Boost.Multiprecision version conflicts | Low | Build failure | Pin compatible versions. SymEngine v0.14.0 boostmp is tested against Boost 1.84+. |
| Apple Clang -O3 segfaults with boostmp | Low (fixed in v0.14.0, but no Apple Clang CI) | iOS crash | Test -O3 on iOS targets; fall back to -O2 if needed. |
| SymEngine `try_compile` fails during iOS cross-compilation | Medium | Build failure | Set `CMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY` in iOS toolchain file. |

## Migration Plan

No migration needed — this is purely additive. No existing behavior changes:
- Symbol's string representation is unchanged
- All existing commands work identically
- SUBST, EXPLODE, ASSEMBLE are unaffected
- New commands (DIFF, INTEGRATE, etc.) are new registry entries

Rollback: remove the FetchContent block and `src/cas/` directory. No other code is modified except the 6 command registrations in `commands.cpp`.

## Open Questions

None — all resolved during proposal review.
