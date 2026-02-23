## 1. Build System
- [x] 1.1 Create top-level `CMakeLists.txt` requiring CMake 3.20+, C++17
- [x] 1.2 Add FetchContent for Boost (headers only, 1.84+), SQLite3 amalgamation (3.46+), Catch2 (v3.5+)
- [x] 1.3 Define `liblpr` static library target from `src/`
- [x] 1.4 Define `lpr-cli` executable target from `cli/`
- [x] 1.5 Define `lpr-tests` executable target from `tests/`
- [x] 1.6 Define install target exporting `include/lpr/lpr.h` and `liblpr.a`
- [x] 1.7 Verify empty project configures and builds

## 2. Object Type System
- [x] 2.1 Create `include/lpr/lpr.h` with complete C API (8 functions)
- [x] 2.2 Implement `Object` variant in `src/core/object.hpp/.cpp` with 9 types
- [x] 2.3 Implement `repr()` for all types
- [x] 2.4 Implement `serialize()` / `deserialize()` for all types
- [x] 2.5 Write `tests/test_types.cpp` — construction, repr, serialize/deserialize roundtrip
- [x] 2.6 Verify tests pass

## 3. SQLite Store
- [x] 3.1 Implement `Store` class in `src/core/store.hpp/.cpp`
- [x] 3.2 Implement schema creation and migration
- [x] 3.3 Implement stack operations: `push`, `pop`, `peek`, `depth`, `clear_stack`
- [x] 3.4 Implement history: `snapshot_stack`, `restore_stack`
- [x] 3.5 Implement variables: `store_variable`, `recall_variable`, `purge_variable`
- [x] 3.6 Implement directory operations
- [x] 3.7 Write `tests/test_stack.cpp` — push/pop/peek/depth/clear through SQLite
- [x] 3.8 Verify tests pass

## 4. Parser
- [x] 4.1 Implement parser/tokenizer in `src/core/parser.hpp/.cpp`
- [x] 4.2 Handle integer literals, real literals, complex literals
- [x] 4.3 Handle strings, quoted names, quoted expressions (Symbol stub)
- [x] 4.4 Handle program literals (`« ... »`) with nesting
- [x] 4.5 Handle bare words (command lookup)
- [x] 4.6 Write `tests/test_parser.cpp` — tokenization of all literal types and commands
- [x] 4.7 Verify tests pass

## 5. Command Registry & Arithmetic
- [x] 5.1 Implement command registry in `src/core/commands.hpp/.cpp`
- [x] 5.2 Implement arithmetic commands: `+`, `-`, `*`, `/`, `NEG`, `INV`, `ABS`, `MOD`
- [x] 5.3 Implement type promotion across numeric tower (Integer → Rational → Real → Complex)
- [x] 5.4 Write `tests/test_arithmetic.cpp` — basic ops and type promotion
- [x] 5.5 Verify tests pass

## 6. Context & Execution Engine
- [x] 6.1 Implement `Context` class in `src/core/context.hpp/.cpp`
- [x] 6.2 Implement `lpr_exec` transactional flow (begin, snapshot, tokenize, execute, commit/rollback)
- [x] 6.3 Implement `lpr_open`, `lpr_close`, `lpr_depth`, `lpr_repr`, `lpr_free`
- [x] 6.4 Implement `lpr_undo`, `lpr_redo`
- [x] 6.5 Write `tests/test_undo.cpp` — snapshot, undo, redo sequences
- [x] 6.6 Verify tests pass

## 7. Stack Operation Commands
- [x] 7.1 Implement `DUP`, `DROP`, `SWAP`, `OVER`, `ROT`, `DEPTH`, `CLEAR`
- [x] 7.2 Add stack operation tests to existing test suite
- [x] 7.3 Verify tests pass

## 8. Filesystem Commands
- [x] 8.1 Implement `STO`, `RCL`, `PURGE`
- [x] 8.2 Implement `HOME`, `PATH`, `CRDIR`, `VARS`
- [x] 8.3 Write `tests/test_filesystem.cpp` — STO, RCL, PURGE, directory operations
- [x] 8.4 Verify tests pass

## 9. Program Execution Commands
- [x] 9.1 Implement `EVAL` (execute Program, recall Name)
- [x] 9.2 Implement `IFT`, `IFTE`
- [x] 9.3 Write `tests/test_programs.cpp` — Program push, EVAL, IFT, IFTE
- [x] 9.4 Verify tests pass

## 10. Type Conversion & Comparison Commands
- [x] 10.1 Implement `TYPE`, `→NUM`, `→STR`, `STR→`
- [x] 10.2 Implement `==`, `!=`, `<`, `>`, `<=`, `>=`
- [x] 10.3 Add tests for type conversion and comparison
- [x] 10.4 Verify tests pass

## 11. CLI REPL
- [x] 11.1 Implement `cli/main.cpp` with interactive loop
- [x] 11.2 Support optional database path argument
- [x] 11.3 Display stack bottom-up after each command
- [x] 11.4 Support `q`/`quit` to exit
- [x] 11.5 Display errors in distinct format
- [x] 11.6 Manual smoke test of the full vertical slice
