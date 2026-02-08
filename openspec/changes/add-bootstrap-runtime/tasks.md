## 1. Build System
- [ ] 1.1 Create top-level `CMakeLists.txt` requiring CMake 3.20+, C++17
- [ ] 1.2 Add FetchContent for Boost (headers only, 1.84+), SQLite3 amalgamation (3.44+), Catch2 (v3.5+)
- [ ] 1.3 Define `liblpr` static library target from `src/`
- [ ] 1.4 Define `lpr-cli` executable target from `cli/`
- [ ] 1.5 Define `lpr-tests` executable target from `tests/`
- [ ] 1.6 Define install target exporting `include/lpr/lpr.h` and `liblpr.a`
- [ ] 1.7 Verify empty project configures and builds

## 2. Object Type System
- [ ] 2.1 Create `include/lpr/lpr.h` with complete C API (8 functions)
- [ ] 2.2 Implement `Object` variant in `src/core/object.hpp/.cpp` with 9 types
- [ ] 2.3 Implement `repr()` for all types
- [ ] 2.4 Implement `serialize()` / `deserialize()` for all types
- [ ] 2.5 Write `tests/test_types.cpp` — construction, repr, serialize/deserialize roundtrip
- [ ] 2.6 Verify tests pass

## 3. SQLite Store
- [ ] 3.1 Implement `Store` class in `src/core/store.hpp/.cpp`
- [ ] 3.2 Implement schema creation and migration
- [ ] 3.3 Implement stack operations: `push`, `pop`, `peek`, `depth`, `clear_stack`
- [ ] 3.4 Implement history: `snapshot_stack`, `restore_stack`
- [ ] 3.5 Implement variables: `store_variable`, `recall_variable`, `purge_variable`
- [ ] 3.6 Implement directory operations
- [ ] 3.7 Write `tests/test_stack.cpp` — push/pop/peek/depth/clear through SQLite
- [ ] 3.8 Verify tests pass

## 4. Parser
- [ ] 4.1 Implement parser/tokenizer in `src/core/parser.hpp/.cpp`
- [ ] 4.2 Handle integer literals, real literals, complex literals
- [ ] 4.3 Handle strings, quoted names, quoted expressions (Symbol stub)
- [ ] 4.4 Handle program literals (`« ... »`) with nesting
- [ ] 4.5 Handle bare words (command lookup)
- [ ] 4.6 Write `tests/test_parser.cpp` — tokenization of all literal types and commands
- [ ] 4.7 Verify tests pass

## 5. Command Registry & Arithmetic
- [ ] 5.1 Implement command registry in `src/core/commands.hpp/.cpp`
- [ ] 5.2 Implement arithmetic commands: `+`, `-`, `*`, `/`, `NEG`, `INV`, `ABS`, `MOD`
- [ ] 5.3 Implement type promotion across numeric tower (Integer → Rational → Real → Complex)
- [ ] 5.4 Write `tests/test_arithmetic.cpp` — basic ops and type promotion
- [ ] 5.5 Verify tests pass

## 6. Context & Execution Engine
- [ ] 6.1 Implement `Context` class in `src/core/context.hpp/.cpp`
- [ ] 6.2 Implement `lpr_exec` transactional flow (begin, snapshot, tokenize, execute, commit/rollback)
- [ ] 6.3 Implement `lpr_open`, `lpr_close`, `lpr_depth`, `lpr_repr`, `lpr_free`
- [ ] 6.4 Implement `lpr_undo`, `lpr_redo`
- [ ] 6.5 Write `tests/test_undo.cpp` — snapshot, undo, redo sequences
- [ ] 6.6 Verify tests pass

## 7. Stack Operation Commands
- [ ] 7.1 Implement `DUP`, `DROP`, `SWAP`, `OVER`, `ROT`, `DEPTH`, `CLEAR`
- [ ] 7.2 Add stack operation tests to existing test suite
- [ ] 7.3 Verify tests pass

## 8. Filesystem Commands
- [ ] 8.1 Implement `STO`, `RCL`, `PURGE`
- [ ] 8.2 Implement `HOME`, `PATH`, `CRDIR`, `VARS`
- [ ] 8.3 Write `tests/test_filesystem.cpp` — STO, RCL, PURGE, directory operations
- [ ] 8.4 Verify tests pass

## 9. Program Execution Commands
- [ ] 9.1 Implement `EVAL` (execute Program, recall Name)
- [ ] 9.2 Implement `IFT`, `IFTE`
- [ ] 9.3 Write `tests/test_programs.cpp` — Program push, EVAL, IFT, IFTE
- [ ] 9.4 Verify tests pass

## 10. Type Conversion & Comparison Commands
- [ ] 10.1 Implement `TYPE`, `→NUM`, `→STR`, `STR→`
- [ ] 10.2 Implement `==`, `!=`, `<`, `>`, `<=`, `>=`
- [ ] 10.3 Add tests for type conversion and comparison
- [ ] 10.4 Verify tests pass

## 11. CLI REPL
- [ ] 11.1 Implement `cli/main.cpp` with interactive loop
- [ ] 11.2 Support optional database path argument
- [ ] 11.3 Display stack bottom-up after each command
- [ ] 11.4 Support `q`/`quit` to exit
- [ ] 11.5 Display errors in distinct format
- [ ] 11.6 Manual smoke test of the full vertical slice
