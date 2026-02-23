# Change: Bootstrap LPR Runtime — Build System & Minimal Vertical Slice

## Why
The LPR runtime has no implementation yet. This change establishes the foundation: a buildable CMake project with an object type system, SQLite-backed stack, parser, command registry, undo/redo, variable filesystem, and a CLI REPL. Every subsequent feature builds on this vertical slice.

## What Changes
- Add CMake build system with FetchContent for Boost (headers), SQLite3 (amalgamation), and Catch2
- Add public C API header (`include/lpr/lpr.h`) with 8 functions
- Add `Object` variant type system with 9 types (Integer, Real, Rational, Complex, String, Program, Name, Error, Symbol stub)
- Add SQLite persistence layer (store) with stack, directory, variable, and history tables
- Add single-pass RPL parser/tokenizer
- Add command registry with built-in commands: stack ops, arithmetic, comparison, type conversion, program execution, filesystem
- Add `Context` class implementing the `lpr_exec` transactional flow with undo/redo
- Add CLI REPL for interactive development
- Add Catch2 test suite across 7 test files

## Impact
- Affected specs: build-system, object-types, sqlite-store, parser, stack-commands, arithmetic-commands, comparison-commands, type-conversion-commands, program-execution, filesystem-commands, context-engine, cli-repl (all new)
- Affected code: entire project — creates `include/`, `src/`, `cli/`, `tests/`, `CMakeLists.txt`
