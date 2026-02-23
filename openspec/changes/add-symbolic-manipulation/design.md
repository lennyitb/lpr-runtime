## Context
Adding structural expression manipulation to the RPL runtime. The core challenge is decomposing/reconstructing expressions that are stored as infix strings (Symbol type), not ASTs.

## Goals / Non-Goals
- Goals: SUBST, EXPLODE, STASH/STASHN/UNSTASH, ASSEMBLE commands; symbolic unary function support
- Non-Goals: full CAS simplification, pattern matching, multi-argument function decomposition (e.g., IFTE)

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

## Risks / Trade-offs
- Symbolic unary support touches ~13 existing commands — risk of regression. Mitigated by: each function gets a simple `if (is_symbolic(a))` guard at the top, existing numeric paths unchanged.
- ASSEMBLE assumes each unstash+EVAL cycle produces exactly one stack item. This holds for all standard unary/binary operations but would break for hypothetical commands with non-standard arity.

## Open Questions
- Should EXPLODE handle unary negation (e.g., `'-X'` → `'X'`, `« NEG »`)? Leaning yes for completeness.
