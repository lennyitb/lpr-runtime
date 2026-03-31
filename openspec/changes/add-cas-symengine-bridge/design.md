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

### D2: SymEngine with `INTEGER_CLASS=boostmp`

Configure SymEngine to use Boost.Multiprecision as its integer backend:

```cmake
set(INTEGER_CLASS boostmp CACHE STRING "")
set(BUILD_TESTS OFF CACHE BOOL "")
set(BUILD_BENCHMARKS OFF CACHE BOOL "")
set(WITH_SYMENGINE_THREAD_SAFE OFF CACHE BOOL "")
```

**Why:** We already depend on Boost.Multiprecision for our numeric tower. Using `boostmp` avoids pulling in GMP (which complicates iOS cross-compilation) and means SymEngine's integers are the same type as ours — no conversion needed when extracting numeric solutions from SOLVE.

**Alternatives considered:**
- *GMP integer class*: Better SymEngine performance, but adds a native dependency that breaks our "no GMP" constraint and complicates cross-compilation. Rejected.
- *SymEngine's built-in `piranha`*: Experimental, not well-tested. Rejected.

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
| SQRT     | sqrt                |
| SQ       | `pow(x, 2)`        |
| ABS      | `Abs`               |

`to_symengine()` tokenizes our expression string, maps function names, and feeds the transformed string to `SymEngine::parse()`. `from_symengine()` calls `SymEngine::str()` on the result and maps names back to uppercase RPL conventions.

**Why:** Direct string-to-string translation before parsing is simpler than building SymEngine expressions programmatically from our token stream. SymEngine's parser handles operator precedence, parentheses, and implicit multiplication.

**Edge cases:**
- `LN` maps to SymEngine's `log` (natural logarithm). Our `LOG` (base-10) maps to `log(x)/log(10)` or is rejected with an error if SymEngine doesn't support it natively.
- `SQ(X)` is rewritten to `(X)**2` before parsing.
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

### D6: Error handling

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
| SymEngine + Boost.Multiprecision version conflicts | Low | Build failure | Pin compatible versions. SymEngine's boostmp support is tested against Boost 1.84+. |

## Migration Plan

No migration needed — this is purely additive. No existing behavior changes:
- Symbol's string representation is unchanged
- All existing commands work identically
- SUBST, EXPLODE, ASSEMBLE are unaffected
- New commands (DIFF, INTEGRATE, etc.) are new registry entries

Rollback: remove the FetchContent block and `src/cas/` directory. No other code is modified except the 6 command registrations in `commands.cpp`.

## Open Questions

1. **SymEngine version pin**: Which release tag? Latest stable as of writing is v0.13.0. Need to verify `boostmp` support and Boost 1.84 compatibility.
2. **`LOG` vs `LN` semantics**: SymEngine's `log` is natural log. Should our CAS `LOG` command map to `log10` in SymEngine, or should we only support `LN` for CAS operations and error on `LOG`?
3. **Output formatting**: Should CAS results respect the current display mode (FIX/SCI/ENG) for numeric coefficients within symbolic expressions, or always use exact representation?
4. **ATAN2 in CAS context**: SymEngine supports `atan2`. Should we wire it through, or defer to Giac?
