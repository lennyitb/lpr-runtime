# LPR Runtime Roadmap

Living document tracking the arc from bootstrap to full RPL runtime parity.
Each milestone is roughly one spec-level unit of work.

---

## Completed

### 1. Core Architecture & C API
SQLite-backed runtime with transactional execution. Every `lpr_exec` call is
an atomic SQLite transaction with automatic snapshot/rollback. The public API
is 11 functions + 1 deallocator -- minimal surface for any host (iOS, WASM,
CLI). Includes `lpr_get_setting` for generic meta table access,
`lpr_history_count`/`lpr_history_entry` for command history.

### 2. Object Type System
Eleven-type `std::variant`: Integer, Real, Rational, Complex, String, Program,
Name, Symbol, Error, List, Matrix. Full serialize/deserialize roundtrip through
SQLite. Arbitrary-precision numerics via Boost.Multiprecision (no GMP
dependency).

### 3. Parser
Single-pass tokenizer handles all literal forms: integers, reals, scientific
notation, complex `(re, im)`, strings with escapes, quoted names, symbolic
expressions, nested program delimiters (`<< >>` / guillemets), lists `{ }`,
and matrices `[[ ]]`. Case-insensitive bare words emit as commands.

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
Full undo/redo via SQLite stack snapshots. Both stack and stash participate
in snapshots for crash safety and undo consistency.

### 11. Structured Control Flow (8 constructs)
All six HP 50g structured control flow constructs implemented as runstream-
consuming handlers in `execute_tokens`: `IF...THEN...END`,
`IF...THEN...ELSE...END`, `CASE...THEN...END`, `FOR...NEXT`, `FOR...STEP`,
`START...NEXT`, `START...STEP`, `WHILE...REPEAT...END`, `DO...UNTIL...END`.
Smart nesting tracker distinguishes FOR/START (closed by NEXT/STEP) from
IF/CASE/WHILE/DO (closed by END).

### 12. Logic & Bitwise Operations (12 commands)
Boolean: AND, OR, NOT, XOR (on integers; nonzero = true). Bitwise: BAND,
BOR, BXOR, BNOT, SL, SR, ASR (on integers). SAME for deep structural
equality (same type AND value, unlike `==` which promotes).

### 13. Transcendental & Scientific Functions (~30 commands)
Angle mode: DEG, RAD, GRAD (stored in SQLite `meta` table, default RAD).
Trig: SIN, COS, TAN, ASIN, ACOS, ATAN, ATAN2 (angle-mode-aware). Exp/log:
EXP, LN, LOG, ALOG, SQRT, SQ. Constants: PI, E (50-digit precision).
Rounding: FLOOR, CEIL, IP, FP. Utility: MIN, MAX, SIGN. Combinatorics:
COMB, PERM, ! (factorial). Percentage: %, %T, %CH. Angle conversion: D->R,
R->D. Generic `Store::get_meta`/`set_meta` helpers for the meta table.
C API: `lpr_get_setting()` for reading any meta key.

### 14. String Manipulation (8 commands + overload)
SIZE, HEAD, TAIL, SUB (1-based substring), POS (find), REPL (replace first
occurrence), NUM (char to codepoint), CHR (codepoint to char). `+` overloaded
for String concatenation (mixed String/numeric is an error).

### 15. List Type & Operations (~20 commands)
`List` variant member holding `std::vector<Object>` with `{ }` literal syntax.
Core: LIST->, ->LIST, GET, PUT, GETI, PUTI, HEAD, TAIL, SIZE, POS, SUB, ADD,
REVLIST, SORT. Higher-order: MAP, FILTER, STREAM, SEQ, DOLIST, DOSUBS.
Set operations: UNION, INTERSECT, DIFFERENCE. ZIP for transposing lists.
Lists are heterogeneous and nestable.

### 16. Vector & Matrix Types (~15 commands)
`Matrix` variant member with `[[ ]]` literal syntax. Construction: ->V2, ->V3,
V-> (explode). Operations: CON (constant matrix), IDN (identity), RDM
(redimension), TRN (transpose), DET (determinant), CROSS, DOT. Arithmetic
`+`, `-`, `*` overloaded for element-wise and matrix operations. ABS on
vectors computes Euclidean norm.

### 18. Full Directory Navigation & Filesystem (6 commands)
CD (change directory), UPDIR (go up), PGDIR (purge directory tree). PATH
returns full directory path. VARS lists directory contents (variables and
subdirectories). Complete tree traversal backed by SQLite `directories` table.

### 19. Display Modes, Flags & History (18 commands + 2 C API functions)
Number formatting: STD, FIX, SCI, ENG with `DisplaySettings` struct passed
to `repr()`. Coordinate modes: RECT, POLAR, SPHERICAL. Typed flag registry:
SF, CF, FS?, FC? (boolean), SFLAG, RFLAG (typed), STOF, RCLF (bulk). Backed
by a `flags` SQLite table with name, type_tag, value columns. Conversions:
->Q (continued-fraction rational approximation), HMS->, ->HMS. SQLite-backed
command history (`input_history` table) with `lpr_history_count` /
`lpr_history_entry` C API. CLI upgraded from `std::getline` to linenoise
for arrow-key navigation, in-line editing, and Ctrl-R search.

Test suite: **452 test cases, 1178 assertions** across 17 files.
Runtime command count: **~177 commands** (170 registered + 7 control flow
constructs).

---

## In Progress / Next Up

### 17. CAS Integration (SymEngine Bridge)
Implement the `CASBridge` abstract interface defined in ARCHITECTURE.md.
Integrate SymEngine via FetchContent. Convert between `Symbol` objects and
SymEngine's native expression trees. Commands: DIFF, INTEGRATE, SOLVE,
SIMPLIFY, EXPAND, FACTOR. This transforms Symbol from a string-with-eval
into a proper symbolic algebra type. Giac backend can slot in later via the
same bridge interface.

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
  renderable plot data for host UIs to consume. I don't really care too much about this.
- **Unit system** -- physical unit types with automatic conversion this would be cool indeed
  (HP 50g's unit library is extensive)
- **I/O & system commands** -- WAIT, BEEP, DATE, TIME, TICKS, MEM, BYTES,
  VERSION (host-dependent, exposed through the C API)
- **User-defined command registration** -- host applications registering
  custom commands via the C API. better yet:
- **System RPL Exposure** -- how awesome would it be if you could punch assembler into your iOS app.
- **Performance** -- command file splitting (commands.cpp is now 3000+ lines),
  object pooling, prepared statement caching, benchmarking harness
- **OpenSpec spec promotion** -- archive completed changes and promote
  specs to `openspec/specs/` as canonical reference
