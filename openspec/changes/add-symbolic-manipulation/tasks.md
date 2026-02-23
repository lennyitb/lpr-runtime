## 1. Symbolic unary function support (prerequisite)
- [ ] 1.1 Add `symbolic_unary()` helper in commands.cpp that produces `Symbol{"FUNC(expr)"}` from a symbolic input
- [ ] 1.2 Add `is_symbolic` guard to SQ, SQRT, SIN, COS, TAN, ASIN, ACOS, ATAN, EXP, LN, ABS, NEG, INV
- [ ] 1.3 Tests: symbolic input produces correct Symbol output; numeric inputs unchanged

## 2. SUBST command
- [ ] 2.1 Expose `tokenize_expression()` and `ExprToken` types in expression.hpp
- [ ] 2.2 Implement SUBST: tokenize, replace matching Name tokens, parenthesize compound replacements via `needs_parens()`, reconstruct string
- [ ] 2.3 Tests: simple substitution, expression replacement with parens, no-match, multiple occurrences

## 3. Stash infrastructure
- [ ] 3.1 Add `stash` table to SQLite schema (`group_id`, `pos`, `object_id`)
- [ ] 3.2 Add Store methods: `stash_push(group)`, `stash_pop() -> group`, `stash_depth()`, `stash_clear()`
- [ ] 3.3 Include stash in undo/redo snapshot/restore cycle
- [ ] 3.4 Tests: stash push/pop ordering, empty stash behavior, undo/redo preserves stash state

## 4. STASH, STASHN, UNSTASH commands
- [ ] 4.1 Implement STASH: pop 1 item from stack, store as single-item group on stash
- [ ] 4.2 Implement STASHN: pop count from level 1, pop N items, store as group on stash
- [ ] 4.3 Implement UNSTASH: pop most recent group from stash, push items back to stack in original order
- [ ] 4.4 Tests: stash/unstash round-trip, STASHN with various counts, error cases (empty stack, empty stash)

## 5. EXPLODE command
- [ ] 5.1 Expose `precedence()` in expression.hpp (or implement top-level operator finder internally)
- [ ] 5.2 Implement EXPLODE: tokenize expression, find lowest-precedence operator at paren depth 0, extract operands and operator; handle function-call pattern `NAME(args)`
- [ ] 5.3 Push operands as appropriate types (Integer for numeric literals, Name for variables, Symbol for sub-expressions), operator as single-token Program
- [ ] 5.4 Tests: binary ops, unary functions, nested expressions, atomic expression error

## 6. ASSEMBLE command
- [ ] 6.1 Implement ASSEMBLE: loop while stash non-empty — UNSTASH then EVAL level 1
- [ ] 6.2 Tests: single-level reassembly, multi-level reassembly, empty stash no-op

## 7. Integration
- [ ] 7.1 End-to-end test: `'SQ(X)+3'` → EXPLODE → STASHN → EXPLODE → STASH → DROP replacement → ASSEMBLE → verify `'SQ(Y+1)+3'`
