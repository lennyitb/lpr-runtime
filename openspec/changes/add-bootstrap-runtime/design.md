## Context
LPR is a portable C++ RPL runtime inspired by the HP 50g. It must support multiple host platforms (iOS, CLI, WASM) through a minimal C API. All state lives in SQLite for persistence, transactional safety, and trivial undo/redo.

## Goals / Non-Goals
- Goals:
  - Buildable project from empty directory with all dependencies fetched automatically
  - Complete numeric tower (Integer, Rational, Real, Complex) with type promotion
  - Persistent stack, filesystem, and undo history via SQLite
  - Enough commands to be useful for basic RPN calculations
  - Symbol type as a stub (stores raw string, no CAS operations)
- Non-Goals:
  - SymEngine/Giac CAS integration (deferred to spec 002)
  - Transcendental functions — SIN, COS, EXP, LN (deferred to spec 002)
  - Lists, vectors, matrices (deferred to spec 003)
  - Unit conversions (deferred)
  - FOR/NEXT/START loops, local variables (deferred to spec 003)
  - iOS framework target and Xcode integration (deferred to spec 004)
  - WASM build target (future)

## Decisions
- **SQLite for the stack**: Seems heavy but gives us persistence, ACID transactions, trivial undo via snapshots, and eliminates serialization bugs. Stack depth is typically <100 items so performance is not a concern. An in-memory cache can be added later if profiling shows a bottleneck.
- **String serialization for numbers**: Boost.Multiprecision roundtrips perfectly through decimal strings. Binary would be faster but adds complexity and endianness concerns. String is debuggable and correct by default.
- **Errors as stack objects**: Matches HP 50g model where errors are inspectable values. The C API reports success/failure via `lpr_result.ok` but error detail is always on the stack.
- **Symbol stub**: Stores raw expression text as a string. No CAS parsing or operations. This keeps Boost as the only heavy dependency in the bootstrap phase.
- **Integer division produces Rational**: Exact arithmetic by default, matching RPL semantics. Users convert explicitly with `->NUM`.
- **FetchContent for all deps**: No manual submodule management. Boost headers-only, SQLite amalgamation, Catch2 v3 all fetched at configure time.

## Risks / Trade-offs
- SQLite overhead for hot-path stack ops → Mitigation: defer optimization; profile first
- Boost headers increase compile time → Mitigation: precompiled headers if needed
- FetchContent downloads at configure time → Mitigation: CMake cache persists across builds

## Open Questions
- None for bootstrap phase; design decisions are straightforward and reversible.
