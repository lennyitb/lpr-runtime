# Change: Add symbolic expression manipulation commands

## Why
RPL programs need a way to inspect and modify symbolic expressions structurally. Currently there's no way to decompose an expression, replace a deeply nested value, and reconstruct it. The HP 50g provides SUBST for simple substitution; we add that plus a novel explode/stash/assemble workflow for structural editing — essentially a zipper over the expression tree expressed as RPL stack idioms.

## What Changes
- Add **SUBST** command for variable substitution in symbolic expressions (HP 50g precedent)
- Add **EXPLODE** command to decompose a Symbol's top-level operation into operands + operator
- Add **STASH** and **STASHN** commands to hide stack items on a hidden auxiliary stack (LIFO)
- Add **UNSTASH** command to restore items from the hidden stack
- Add **ASSEMBLE** command to reconstruct expressions via repeated unstash + EVAL
- Add **symbolic handling to existing commands** (unary: SQ, SQRT, SIN, etc.; multi-arg: IFTE, IFT) so they produce symbolic output from symbolic input — prerequisite for ASSEMBLE
- Add **comma-separated argument syntax** to expression tokenizer for `FUNC(a, b, c)` style calls in Symbols
- Add **stash table** to SQLite schema for persistence and undo/redo consistency

## Impact
- Affected specs: new capability `symbolic-manipulation`
- Affected code:
  - `commands.cpp` — new commands + symbolic handling for ~15 existing commands
  - `store.hpp/cpp` — stash table and methods
  - `expression.hpp/cpp` — expose `tokenize_expression()` and `precedence()` for EXPLODE
