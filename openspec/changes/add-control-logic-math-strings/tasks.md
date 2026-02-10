## 1. Structured Control Flow
- [ ] 1.1 Implement `IF...THEN...END` and `IF...THEN...ELSE...END` in `execute_tokens` (collect condition tokens up to THEN, collect then-body up to ELSE or END, optional else-body up to END)
- [ ] 1.2 Implement `CASE...THEN...END...END` in `execute_tokens` (iterate val/THEN/body/END pairs, optional default before final END)
- [ ] 1.3 Implement `FOR...NEXT` in `execute_tokens` (pop start/end, consume variable name and body up to NEXT, bind loop var, iterate)
- [ ] 1.4 Implement `FOR...STEP` in `execute_tokens` (like FOR...NEXT but pop step increment or execute step program at STEP)
- [ ] 1.5 Implement `START...NEXT` in `execute_tokens` (pop start/end, no loop variable, execute body count times)
- [ ] 1.6 Implement `START...STEP` in `execute_tokens` (like START...NEXT with custom step)
- [ ] 1.7 Implement `WHILE...REPEAT...END` in `execute_tokens` (collect condition tokens up to REPEAT, body up to END, loop while condition truthy)
- [ ] 1.8 Implement `DO...UNTIL...END` in `execute_tokens` (collect body up to UNTIL, condition up to END, loop until condition truthy)
- [ ] 1.9 Write `tests/test_control_flow.cpp` covering all constructs, nesting, edge cases (zero iterations, empty bodies)
- [ ] 1.10 Update CMakeLists.txt to build test_control_flow.cpp

## 2. Logic & Bitwise Operations
- [ ] 2.1 Add `register_logic_commands()` to CommandRegistry: AND, OR, NOT, XOR (Boolean on integers)
- [ ] 2.2 Add bitwise commands: BAND, BOR, BXOR, BNOT, SL, SR, ASR (all on Integer)
- [ ] 2.3 Implement SAME (deep structural equality across all types)
- [ ] 2.4 Write `tests/test_logic_bitwise.cpp` covering Boolean ops, bitwise ops, SAME across types
- [ ] 2.5 Update CMakeLists.txt to build test_logic_bitwise.cpp

## 3. Angle Mode & Transcendental Functions
- [ ] 3.1 Add `get_meta` / `set_meta` generic helpers to Store for reading/writing arbitrary `meta` table keys
- [ ] 3.2 Add DEG, RAD, GRAD commands that write to `meta` key `angle_mode` (default: `"RAD"`)
- [ ] 3.3 Add angle-mode-aware helper that reads `angle_mode` from Store and converts to/from radians
- [ ] 3.4 Add `register_transcendental_commands()` to CommandRegistry: SIN, COS, TAN, ASIN, ACOS, ATAN, ATAN2 (angle-mode-aware, using Boost.Multiprecision trig or cast to double and back)
- [ ] 3.5 Add EXP, LN, LOG, ALOG, SQRT, SQ
- [ ] 3.6 Add constants PI and E as zero-argument commands
- [ ] 3.7 Add rounding commands: FLOOR, CEIL, IP, FP
- [ ] 3.8 Add utility commands: MIN, MAX, SIGN
- [ ] 3.9 Add combinatorics: COMB, PERM, ! (factorial)
- [ ] 3.10 Add percentage commands: %, %T, %CH
- [ ] 3.11 Add angle conversion: D->R, R->D
- [ ] 3.12 Write `tests/test_transcendental.cpp` covering all functions, angle modes, type promotion, edge cases
- [ ] 3.13 Update CMakeLists.txt to build test_transcendental.cpp

## 4. String Manipulation
- [ ] 4.1 Add `register_string_commands()` to CommandRegistry: SIZE, HEAD, TAIL
- [ ] 4.2 Add SUB (1-based substring), POS (find), REPL (replace first occurrence)
- [ ] 4.3 Add NUM (first char -> codepoint), CHR (codepoint -> char)
- [ ] 4.4 Modify `+` command to support String + String concatenation (error on mixed String/non-String)
- [ ] 4.5 Write `tests/test_strings.cpp` covering all string ops, edge cases (empty strings, not found)
- [ ] 4.6 Update CMakeLists.txt to build test_strings.cpp

## 5. C API: lpr_get_setting
- [ ] 5.1 Add `char* lpr_get_setting(lpr_ctx* ctx, const char* key)` to `include/lpr/lpr.h`
- [ ] 5.2 Implement in `src/lpr_api.cpp`: read from Store's generic meta getter, return strdup'd string (NULL if not found)
- [ ] 5.3 Add tests for lpr_get_setting: angle_mode after DEG/RAD/GRAD, nonexistent key returns NULL, cwd default

## 6. Integration & Wiring
- [ ] 6.1 Call `register_logic_commands()`, `register_transcendental_commands()`, `register_string_commands()` from CommandRegistry constructor
- [ ] 6.2 Seed `angle_mode` default to `"RAD"` in Store schema (`create_schema`)
- [ ] 6.3 Run full test suite, ensure no regressions in existing 133 tests
- [ ] 6.4 Update CMD_SET_REFERENCE.md with all new commands
