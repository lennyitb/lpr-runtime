# LPR Runtime Roadmap

Living document tracking the arc from bootstrap to full RPL runtime parity.
Each milestone is roughly one spec-level unit of work.

---

## Completed

### 1. Core Architecture & C API
SQLite-backed runtime with transactional execution. Every `lpr_exec` call is
an atomic SQLite transaction with automatic snapshot/rollback. The public API
is 9 functions + 1 deallocator -- minimal surface for any host (iOS, WASM,
CLI). Includes `lpr_get_setting` for generic meta table access.

### 2. Object Type System
Nine-type `std::variant`: Integer, Real, Rational, Complex, String, Program,
Name, Symbol, Error. Full serialize/deserialize roundtrip through SQLite.
Arbitrary-precision numerics via Boost.Multiprecision (no GMP dependency).

### 3. Parser
Single-pass tokenizer handles all literal forms: integers, reals, scientific
notation, complex `(re, im)`, strings with escapes, quoted names, symbolic
expressions, and nested program delimiters (`<< >>` / guillemets). Case-
insensitive bare words emit as commands.

### 4. Stack Manipulation (16 commands)
Complete HP 50g stack command set: DUP, DUP2, DUPN, DROP, DROP2, DROPN,
SWAP, OVER, ROT, UNROT, PICK, ROLL, ROLLD, UNPICK, DEPTH, CLEAR.

### 5. Arithmetic with Numeric Tower (8 commands)
`+`, `-`, `*`, `/`, NEG, INV, ABS, MOD with automatic promotion through
`Integer -> Rational -> Real -> Complex`. Integer division produces exact
Rationals. Arithmetic on symbolic operands builds new expression strings
with precedence-aware parenthesization.

### 6. Comparisons, Type Conversion & Filesystem (17 commands)
Six relational operators (`==`, `!=`, `<`, `>`, `<=`, `>=`). Four type
commands (TYPE, ->NUM, ->STR, STR->). Seven filesystem commands (STO, RCL,
PURGE, HOME, PATH, CRDIR, VARS) backed by the SQLite directory tree.

### 7. Program Execution & Conditionals
EVAL dispatches on type: executes Programs, recalls Names (auto-executing
stored Programs), and numerically evaluates Symbols. IFT and IFTE provide
conditional branching with truthiness semantics matching the HP 50g.

### 8. Local Variable Binding (`->`)
Runstream-consuming `->` command with lexically scoped local frames. Supports
multiple bindings, nested shadowing, and both Program and Symbol bodies.
Resolution order: innermost local scope first, then global filesystem.

### 9. Expression Evaluator
Shunting-yard infix evaluator for Symbol strings. Handles `+`, `-`, `*`, `/`,
`^`, unary negation, parentheses, and variable substitution (locals then
globals). Enables `'X^2 + 1' EVAL` when X is bound.

### 10. Undo/Redo & Test Suite
Full undo/redo via SQLite stack snapshots. ~133 Catch2 test cases across 10
files covering types, stack ops, parsing, arithmetic, undo, filesystem,
programs, locals, arrow binding, and expression evaluation.

### 11. Structured Control Flow (8 constructs)
All six HP 50g structured control flow constructs implemented as runstream-
consuming handlers in `execute_tokens`: `IF...THEN...END`,
`IF...THEN...ELSE...END`, `CASE...THEN...END`, `FOR...NEXT`, `FOR...STEP`,
`START...NEXT`, `START...STEP`, `WHILE...REPEAT...END`, `DO...UNTIL...END`.
Smart nesting tracker distinguishes FOR/START (closed by NEXT/STEP) from
IF/CASE/WHILE/DO (closed by END). 24 test cases.

### 12. Logic & Bitwise Operations (12 commands)
Boolean: AND, OR, NOT, XOR (on integers; nonzero = true). Bitwise: BAND,
BOR, BXOR, BNOT, SL, SR, ASR (on integers). SAME for deep structural
equality (same type AND value, unlike `==` which promotes). 20 test cases.

### 13. Transcendental & Scientific Functions (~30 commands)
Angle mode: DEG, RAD, GRAD (stored in SQLite `meta` table, default RAD).
Trig: SIN, COS, TAN, ASIN, ACOS, ATAN, ATAN2 (angle-mode-aware). Exp/log:
EXP, LN, LOG, ALOG, SQRT, SQ. Constants: PI, E (50-digit precision).
Rounding: FLOOR, CEIL, IP, FP. Utility: MIN, MAX, SIGN. Combinatorics:
COMB, PERM, ! (factorial). Percentage: %, %T, %CH. Angle conversion: D->R,
R->D. Generic `Store::get_meta`/`set_meta` helpers for the meta table.
C API: `lpr_get_setting()` for reading any meta key. 50 test cases.

### 14. String Manipulation (8 commands + overload)
SIZE, HEAD, TAIL, SUB (1-based substring), POS (find), REPL (replace first
occurrence), NUM (char to codepoint), CHR (codepoint to char). `+` overloaded
for String concatenation (mixed String/numeric is an error). 20 test cases.

Test suite now at **281 test cases, 721 assertions** across 14 files.
Runtime command count: **~106 commands** (up from 45).

---

## In Progress / Next Up

### 15. List Type & Operations
New `List` variant member holding `std::vector<Object>`. Parser support for
`{ 1 2 3 }` literal syntax. Core: LIST->, ->LIST, GET, PUT, GETI, PUTI,
HEAD, TAIL, SIZE, POS, SUB, REVLIST, SORT. Higher-order: DOLIST, MAP,
STREAM, SEQ. Set operations: ADD, UNION, INTERSECT, DIFFERENCE. Lists are
the primary compound data type in RPL and a prerequisite for matrix support.

### 16. Vector & Matrix Types
Vectors as typed lists, matrices as `[[ ]]` nested syntax. Arithmetic
overloads for element-wise and matrix operations. Linear algebra: DET, INV,
TRN (transpose), CROSS, DOT, CON (constant matrix), IDN (identity), RDM
(redimension). Stretch: EIGENVALUES, EIGENVECTORS. Depends on List
infrastructure.

### 17. CAS Integration (SymEngine Bridge)
Implement the `CASBridge` abstract interface defined in ARCHITECTURE.md.
Integrate SymEngine via FetchContent. Convert between `Symbol` objects and
SymEngine's native expression trees. Commands: DIFF, INTEGRATE, SOLVE,
SIMPLIFY, EXPAND, FACTOR. This transforms Symbol from a string-with-eval
into a proper symbolic algebra type. Giac backend can slot in later via the
same bridge interface.

### 18. Full Directory Navigation & Filesystem
Fix PATH (currently a stub returning "HOME"). Implement CD (change
directory), UPDIR (go up), PGDIR (purge directory tree), ORDER (reorder
variables), TVARS (typed variable listing). The SQLite directory tree schema
already supports hierarchy -- this is wiring up the traversal commands.

### 19. Display Modes, Flags & Settings
Number formatting: FIX, SCI, ENG, STD. Coordinate modes: RECT, POLAR,
SPHERICAL. System flags: SF, CF, FS?, FC?, STOF, RCLF. ->Q (convert to
rational approximation). HMS time conversion: HMS->, ->HMS. These affect
repr output and command behavior. Stored in the SQLite `meta` table.
(Angle modes DEG/RAD/GRAD already implemented in milestone 13.)

### 20. Cross-Platform Targets & Packaging
iOS framework build target (CMake + Xcode). WASM via Emscripten for
browser-based calculators. Linux shared library. Embedded ARM support.
The C API is already designed for FFI -- this milestone is build system and
CI, not runtime code.

---

## Future / Aspirational

- **Giac CAS backend** -- full CAS parity with the HP 50g, swappable via
  the bridge interface once SymEngine is proven out
- **Graphing & plotting data** -- DRAW, PLOT, FUNCTION types that produce
  renderable plot data for host UIs to consume
- **Unit system** -- physical unit types with automatic conversion
  (HP 50g's unit library is extensive)
- **I/O & system commands** -- WAIT, BEEP, DATE, TIME, TICKS, MEM, BYTES,
  VERSION (host-dependent, exposed through the C API)
- **User-defined command registration** -- host applications registering
  custom commands via the C API
- **Performance** -- command file splitting (commands.cpp is now 1500+ lines),
  object pooling, prepared statement caching, benchmarking harness
- **OpenSpec spec promotion** -- archive completed changes and promote
  specs to `openspec/specs/` as canonical reference
