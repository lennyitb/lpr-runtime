## Context
Adding structural expression manipulation to the RPL runtime. The core challenge is decomposing/reconstructing expressions that are stored as infix strings (Symbol type), not ASTs.

## Goals / Non-Goals
- Goals: SUBST, EXPLODE, STASH/STASHN/UNSTASH, ASSEMBLE commands; symbolic function support (unary and multi-arg); comma-separated argument syntax in expressions
- Non-Goals: full CAS simplification, pattern matching

## Decisions

### EXPLODE reuses existing tokenizer
- Decision: Reuse `tokenize_expression()` from expression.cpp and add a top-level operator finder (~30 lines) that scans tokens at paren depth 0 for the lowest-precedence operator. Reuse existing `precedence()` function.
- Alternatives: Build a full AST (overkill for peeling one layer), regex-based splitting (fragile with nested parens).

### Operator pushed as single-token Program
- Decision: EXPLODE pushes operators as `Program{[Token::Command("+")]}` so EVAL naturally executes them for reassembly.
- Alternatives: Push as Name (EVAL would try variable recall, not command execution), push as String (would need special handling in ASSEMBLE).

### Stash stored in SQLite
- Decision: New `stash` table with `(group_id INTEGER, pos INTEGER, object_id INTEGER)`. Groups are numbered sequentially. This keeps the stash inside the undo/redo transaction boundary automatically.
- Alternatives: In-memory vector on Context (would not survive undo/redo, inconsistent with architecture).

### SUBST operates at token level
- Decision: Tokenize expression, replace matching Name tokens, reconstruct string. Use `needs_parens()` to parenthesize compound replacements in higher-precedence contexts.
- Alternatives: Raw string find/replace (would match substrings: "X" inside "AX"), regex with word boundaries (close but misses edge cases).

### No count pushed by EXPLODE
- Decision: EXPLODE pushes only operands + operator, no count. The arity is evident from context (binary ops push 3 items, unary functions push 2). This keeps the common `EXPLODE ... STASHN` workflow clean.
- Alternatives: Push count like HP 50g's OBJ→ (adds ceremony: `1 - STASHN` every time to stash everything but the target operand).

### Multi-argument function syntax
- Decision: Add Comma token to `tokenize_expression()`. Expressions support `FUNC(arg1, arg2, ..., argN)` syntax. EXPLODE decomposes into N args + function Program. A `symbolic_func()` helper builds `Symbol{"FUNC(a, b, c)"}` for any arity.
- Alternatives: Only support unary functions in expressions (artificially limits the syntax when the implementation cost of commas is trivial).

## Risks / Trade-offs
- Symbolic support touches ~13+ existing commands — risk of regression. Mitigated by: each function gets a simple `if (is_symbolic(a))` guard at the top, existing numeric paths unchanged.
- ASSEMBLE assumes each unstash+EVAL cycle produces exactly one stack item. This holds for all standard operations but would break for commands with non-standard arity.

## Open Questions
- Should EXPLODE handle unary negation (e.g., `'-X'` → `'X'`, `« NEG »`)? Leaning yes for completeness.
