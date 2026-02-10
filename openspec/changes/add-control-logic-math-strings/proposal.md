# Change: Add structured control flow, logic/bitwise ops, transcendental math, and string manipulation

## Why
The runtime cannot express real RPL programs. There are no loops, no boolean operators, no trig/log functions, and no string operations. These four feature groups are the minimum needed to write non-trivial programs and match the HP 50g's core command set.

## What Changes
- **Structured control flow** (Roadmap #11): `IF...THEN...ELSE...END`, `CASE...THEN...END`, `FOR...NEXT/STEP`, `START...NEXT/STEP`, `WHILE...REPEAT...END`, `DO...UNTIL...END`. All are runstream-consuming constructs parsed inside `execute_tokens`, following the same pattern as `->`.
- **Logic & bitwise operations** (Roadmap #12): AND, OR, NOT, XOR (Boolean on integers), BAND, BOR, BXOR, BNOT, SL, SR, ASR (bitwise on integers), SAME (deep structural equality).
- **Transcendental & scientific functions** (Roadmap #13): SIN, COS, TAN, ASIN, ACOS, ATAN, ATAN2, EXP, LN, LOG, ALOG, SQRT, SQ. Constants PI, E. Rounding: FLOOR, CEIL, IP, FP. Utility: MIN, MAX, SIGN. Combinatorics: COMB, PERM, ! (factorial). Percentage: %, %T, %CH. Angle conversion: D->R, R->D. Angle mode commands: DEG, RAD, GRAD (stored in SQLite `meta` table). Trig functions convert input/output according to current angle mode. All operate on the full numeric tower.
- **String manipulation** (Roadmap #14): SIZE, SUB, POS, REPL, HEAD, TAIL, NUM, CHR. Overload `+` for string concatenation.

- **C API: `lpr_get_setting`** -- New generic settings getter function. Reads any key from the SQLite `meta` table, returning a caller-freed string. Covers angle mode now and all future settings (display mode, coordinate mode, flags) without further API changes.

## Impact
- Affected specs: program-execution, context-engine, arithmetic-commands, c-api (new: control-flow, logic-bitwise, transcendental-commands, string-commands)
- Affected code: `src/core/context.cpp` (control flow in execute_tokens loop), `src/core/commands.cpp` (new command registrations), `src/core/commands.hpp` (new register methods), `src/core/store.hpp/.cpp` (generic meta getter), `include/lpr/lpr.h` (new API function), `src/lpr_api.cpp` (implement lpr_get_setting)
- New test files: `tests/test_control_flow.cpp`, `tests/test_logic_bitwise.cpp`, `tests/test_transcendental.cpp`, `tests/test_strings.cpp`
- Build: `CMakeLists.txt` updated for new test files
