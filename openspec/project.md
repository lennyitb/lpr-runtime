# Project Context

## Purpose

LPR (Lenny Polish Reverse) is a portable C++ runtime for an RPL virtual machine inspired by the HP 50g calculator. It replicates the behavior and semantics of the HP 50g's RPL engine with a modern, clean architecture. The runtime exposes a minimal C-compatible API designed around a single paradigm: everything is a command executed via `lpr_exec(ctx, "...")`.

Goals:
- Faithful RPL semantics (not binary compatibility) with the HP 50g
- Cross-platform deployment: macOS CLI (dev), iOS framework, Linux, WASM, embedded ARM
- SQLite-backed persistence for stack, filesystem, and undo/redo history
- Arbitrary-precision numeric tower with eventual computer algebra support

## Tech Stack
- **Language:** C++17
- **Build System:** CMake 3.20+
- **Numerics:** Boost.Multiprecision (headers-only) — `cpp_int`, `cpp_dec_float_50`, `cpp_rational`
- **Persistence:** SQLite3 (amalgamation, 3.44+)
- **Testing:** Catch2 v3.5+
- **Future CAS:** SymEngine (phase 1), Giac (aspirational)
- **Dependency management:** CMake FetchContent for all deps

## Project Conventions

### Code Style
- C++17 standard throughout
- Header-only dependencies preferred (except SQLite)
- Commands use UPPERCASE names matching HP 50g: `DUP`, `DROP`, `STO`, `+`, `-`
- Internal classes/functions use CamelCase
- Command function signature: `using CommandFn = std::function<void(Stack&, Context&)>`
- Numbers serialized as strings (Boost.Multiprecision roundtrips cleanly)
- Errors are RPL objects pushed onto the stack, not exceptions exposed to callers
- Single-pass parsing — no recursive descent ASTs

### Architecture Patterns
- **Variant-based type system:** All 9 RPL types in one `std::variant` (no polymorphic inheritance)
- **SQLite-backed everything:** Stack, filesystem (directory tree), undo history all in a single `.lpr` database
- **Transaction-per-execution:** Each `lpr_exec` call is an atomic SQLite transaction with automatic snapshot/rollback
- **Command registry:** `std::unordered_map<std::string, CommandFn>` — commands pop args, do work, push results
- **Opaque C API:** 7 functions + 1 deallocator; all interaction through `lpr_exec(ctx, string)`
- **CAS bridge interface:** Abstract `CASBridge` class so algebra backends (SymEngine, Giac) can be swapped

### Testing Strategy
- **Framework:** Catch2 v3.5+
- **Test-driven development:** Each implementation step includes passing tests before moving on
- **Test files:**
  - `test_types.cpp` — Object construction, repr, serialize/deserialize roundtrip
  - `test_stack.cpp` — Push/pop/peek/depth/clear through SQLite
  - `test_parser.cpp` — Tokenization of all literal types and commands
  - `test_arithmetic.cpp` — Basic arithmetic ops and type promotion
  - `test_undo.cpp` — Snapshot, undo, redo sequences
  - `test_filesystem.cpp` — STO, RCL, PURGE, directory operations
  - `test_programs.cpp` — Program push, EVAL, IFT, IFTE

### Git Workflow
- OpenSpec-based change management: proposals for features, archived after deployment
- Each implementation step should be a separate commit
- Implementation tracked via tasks checklist in `openspec/changes/*/tasks.md`

## Domain Context

**RPL (Reverse Polish Lisp)** is a stack-based programming language used in HP calculators:
- Stack-based evaluation: commands operate on top items of the stack
- Level 1 = top of stack, Level 2 = second from top, etc.
- Numeric tower with type promotion: `Integer -> Rational -> Real -> Complex`
  - `Integer / Integer` produces exact `Rational`
  - Mixed-type operations promote to the higher type
  - Explicit conversion: `->NUM`, `->Q`, `->STR`
- Variable storage via a directory filesystem rooted at HOME (HP 50g model)
  - `STO` stores, `RCL` recalls, `PURGE` deletes, `CRDIR` creates subdirectories
- Programs delimited by `<< ... >>` (or `<< ... >>`) contain deferred token sequences
  - `EVAL` executes a program; `IFT`/`IFTE` for conditionals
- Symbolic expressions in single quotes (`'X^2 + 1'`) are not evaluated on push
- Every execution creates a stack snapshot for undo/redo support

## Important Constraints
- **C API minimalism:** Only 7 functions + 1 deallocator; all host interaction through string-based `lpr_exec`
- **SQLite is mandatory:** All state (stack, filesystem, history) lives in SQLite — no in-process caching initially
- **Bootstrap phase limits:** Symbol type is a stub (no CAS), no transcendental functions, no lists/matrices, no loop constructs (`FOR`/`NEXT`) — these are deferred to later specs
- **No GMP dependency:** Using Boost.Multiprecision (pure C++ headers) for easier iOS cross-compilation
- **String-based number serialization:** No binary format; numbers roundtrip through string representation
- **Errors are values:** Errors are stack objects, not host-level exceptions; C API signals success/failure via `lpr_result.ok`

## External Dependencies

| Dependency | Version | Acquisition | Purpose |
|---|---|---|---|
| Boost.Multiprecision | 1.84+ | FetchContent (headers-only) | Arbitrary-precision integers, reals, rationals |
| SQLite3 | 3.44+ | FetchContent (amalgamation) | Persistence, transactions, state management |
| Catch2 | v3.5+ | FetchContent | Unit testing framework |
| SymEngine | TBD (phase 2) | FetchContent | Computer algebra backend |
| Giac | TBD (aspirational) | TBD | Full CAS parity with HP 50g |

No runtime dependencies on system libraries beyond the C++ standard library. All dependencies fetched at CMake configure time via FetchContent.
