## 1. Build system: SymEngine dependency
- [x] 1.1 Add SymEngine FetchContent block to `CMakeLists.txt`: pin `v0.14.0`, set `INTEGER_CLASS=boostmp`, disable all optional deps (FLINT, ARB, MPFR, MPC, LLVM, PTHREAD, OPENMP, BFD, ECM, PRIMESIEVE, PIRANHA, TCMALLOC), `BUILD_TESTS=OFF`, `BUILD_BENCHMARKS=OFF`. Ensure Boost is fetched before SymEngine so `find_package(Boost)` resolves correctly.
- [x] 1.2 Link SymEngine target to `liblpr` (PUBLIC, so tests and CLI inherit it)
- [x] 1.3 Verify clean configure and build on macOS: `cmake -B build && cmake --build build`
- [x] 1.4 **Checkpoint — no regressions**: run `lpr-tests` and confirm all 452+ existing tests pass with SymEngine linked but unused

## 2. CAS Bridge interface
- [x] 2.1 Create `src/cas/bridge.hpp`: abstract `CASBridge` class with pure virtual methods `differentiate`, `integrate`, `solve`, `simplify`, `expand`, `factor` — each taking `const Object&` (and `const std::string& var` where applicable) and returning `Object`
- [x] 2.2 Create `src/cas/symengine_bridge.hpp`: `SymEngineBridge` class declaration inheriting `CASBridge`
- [x] 2.3 Create `src/cas/symengine_bridge.cpp`: implement private helpers `to_symengine(const std::string& expr)` → `SymEngine::RCP<const SymEngine::Basic>` and `from_symengine(const RCP<const Basic>&)` → `std::string`, including the function name mapping table (SIN↔sin, COS↔cos, LN↔log, SQ→pow, etc.)
- [x] 2.4 Tests (in `test_cas.cpp`): string→SymEngine→string round-trip for basic expressions (`'X^2+1'`, `'SIN(X)'`, `'X^2+2*X+1'`, `'3*X'`), verifying that conversion preserves mathematical equivalence

## 3. SymEngineBridge operations
- [x] 3.1 Implement `differentiate()`: extract string from Symbol, call `to_symengine`, call `SymEngine::diff()` with parsed variable symbol, `from_symengine` the result, return new Symbol
- [x] 3.2 Implement `integrate()`: same pattern with `SymEngine::integrate()`
- [x] 3.3 Implement `solve()`: parse expression (implicit `=0`), call `SymEngine::solve()`, convert each solution to appropriate Object type (Integer for integer solutions, Rational for rational, Symbol for symbolic), return as `List`
- [x] 3.4 Implement `simplify()`: use `SymEngine::simplify()` or equivalent canonicalization
- [x] 3.5 Implement `expand()`: call `SymEngine::expand()`
- [x] 3.6 Implement `factor()`: call `SymEngine::factor()` (polynomial factoring over integers)
- [x] 3.7 Tests: unit tests for each bridge method with known input/output pairs — e.g., `differentiate('X^2+3*X', 'X')` → `'2*X+3'`, `expand('(X+1)^2')` → `'X^2+2*X+1'`, `solve('X^2-4', 'X')` → `{-2, 2}`, etc.
- [x] 3.8 Tests: error cases — differentiate a non-symbolic, integrate an expression SymEngine can't handle, solve with no solutions

## 4. Context integration
- [x] 4.1 Add `std::unique_ptr<CASBridge> cas_bridge_` member to `Context`, initialized to `std::make_unique<SymEngineBridge>()` in constructor
- [x] 4.2 Add `CASBridge& cas()` accessor to `Context`
- [x] 4.3 Forward-declare `CASBridge` in `context.hpp`, include `bridge.hpp` only in `context.cpp`
- [x] 4.4 **Checkpoint — no regressions**: all existing tests still pass, bridge is wired but no commands registered yet

## 5. CAS commands
- [x] 5.1 Register DIFF command in `commands.cpp`: pop variable (level 1, Name — extract string), pop expression (level 2, Symbol), call `ctx.cas().differentiate()`, push result
- [x] 5.2 Register INTEGRATE command: same arg pattern as DIFF, call `ctx.cas().integrate()`, push result Symbol
- [x] 5.3 Register SOLVE command: same arg pattern, call `ctx.cas().solve()`, push result List
- [x] 5.4 Register SIMPLIFY command: pop expression (level 1, Symbol), call `ctx.cas().simplify()`, push result
- [x] 5.5 Register EXPAND command: pop expression, call `ctx.cas().expand()`, push result
- [x] 5.6 Register FACTOR command: pop expression, call `ctx.cas().factor()`, push result
- [x] 5.7 Tests: end-to-end via `lpr_exec` for each command — e.g., `lpr_exec(ctx, "'X^2+3*X' 'X' DIFF")` then verify stack top is `'2*X+3'`
- [x] 5.8 Tests: type error cases (DIFF on Integer, SIMPLIFY on String, SOLVE with non-Name variable)
- [x] 5.9 Tests: stack underflow cases (each command with insufficient args)

## 6. Integration tests
- [x] 6.1 Multi-step CAS workflow: `'(X+1)^3' EXPAND 'X' DIFF` → derivative of expanded cubic
- [x] 6.2 CAS + symbolic manipulation: `'X^2+Y' 'X' DIFF` → `'2*X'`, then SUBST Y with a value
- [x] 6.3 CAS + numeric evaluation: `'X^2+3*X' 'X' DIFF` then `5 'X' STO EVAL` → `13`
- [x] 6.4 SOLVE + List operations: `'X^2-9' 'X' SOLVE` → `{ -3 3 }`, then `1 GET` → `-3`
- [x] 6.5 Round-trip: `'X^2+2*X+1' FACTOR EXPAND` returns the original (or equivalent) expression
- [x] 6.6 Undo/redo: perform DIFF, undo, verify original expression is restored

## 7. Documentation
- [x] 7.1 CMD_SET_REFERENCE.md: Add a **Computer Algebra (CAS)** section with DIFF, INTEGRATE, SOLVE, SIMPLIFY, EXPAND, FACTOR — stack effects, descriptions, examples, and known limitations
- [x] 7.2 CMD_SET_REFERENCE.md: Add the 6 new commands to the Command Summary table
- [x] 7.3 ARCHITECTURE.md: Flesh out the CAS Bridge section with implementation details — bridge pattern, SymEngine conversion strategy, function name mapping
- [x] 7.4 ARCHITECTURE.md: Update the External Dependencies table to include SymEngine with version and acquisition method
- [x] 7.5 ARCHITECTURE.md: Update the Directory Layout to show `src/cas/` with its files
