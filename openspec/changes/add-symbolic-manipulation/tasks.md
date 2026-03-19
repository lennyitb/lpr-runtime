## 1. Symbolic function support (prerequisite)
- [ ] 1.1 Add `symbolic_func()` helper in commands.cpp that produces `Symbol{"FUNC(a, b, ...)"}` for any arity
- [ ] 1.2 Add `is_symbolic` guard to unary commands: SQ, SQRT, SIN, COS, TAN, ASIN, ACOS, ATAN, EXP, LN, ABS, NEG, INV
- [ ] 1.3 Add symbolic handling to multi-arg commands: IFTE (3 args), IFT (2 args)
- [ ] 1.4 Tests: symbolic input produces correct Symbol output; numeric inputs unchanged; multi-arg symbolic output

## 2. Comma syntax and SUBST command
- [ ] 2.1 Add Comma token type to expression tokenizer
- [ ] 2.2 Expose `tokenize_expression()`, `ExprToken` types, and `needs_parens()` in expression.hpp
- [ ] 2.3 Implement SUBST: tokenize, replace matching Name tokens, parenthesize compound replacements, reconstruct string
- [ ] 2.4 Tests: simple substitution, expression replacement with parens, no-match, multiple occurrences, expressions containing commas

## 3. Stash infrastructure
- [ ] 3.1 Add `stash` table to SQLite schema (`group_id`, `pos`, `object_id`)
- [ ] 3.2 Add Store methods: `stash_push(group)`, `stash_pop() -> group`, `stash_depth()`, `stash_clear()`
- [ ] 3.3 Include stash in undo/redo snapshot/restore cycle
- [ ] 3.4 Tests: stash push/pop ordering, empty stash behavior, undo/redo preserves stash state
- [ ] 3.5 **Checkpoint — transaction atomicity**: verify that stash mutations (push/pop/clear) and stack mutations within the same command execute inside a single SQLite transaction, so a crash or undo never leaves stash and stack out of sync

## 4. STASH, STASHN, UNSTASH commands
- [ ] 4.1 Implement STASH: pop 1 item from stack, store as single-item group on stash
- [ ] 4.2 Implement STASHN: pop count from level 1, pop N items, store as group on stash
- [ ] 4.3 Implement UNSTASH: pop most recent group from stash, push items back to stack in original order
- [ ] 4.4 Tests: stash/unstash round-trip, STASHN with various counts, error cases (empty stack, empty stash)
- [ ] 4.5 **Checkpoint — undo coherence**: run a multi-step sequence (e.g. STASHN → EXPLODE → UNSTASH), undo each step, and confirm the stash and stack are both restored correctly at every intermediate state

## 5. EXPLODE command
- [ ] 5.1 Expose `precedence()` in expression.hpp (or implement top-level operator finder internally)
- [ ] 5.2 Implement EXPLODE: tokenize expression, find lowest-precedence operator at paren depth 0, extract operands and operator; handle function-call pattern `NAME(args)` including comma-separated multi-arg calls
- [ ] 5.3 Push operands as appropriate types (Integer for numeric literals, Name for variables, Symbol for sub-expressions), operator as single-token Program
- [ ] 5.4 Tests: binary ops, unary functions, multi-arg functions, nested expressions, atomic expression error

## 6. ASSEMBLE command
- [ ] 6.1 Implement ASSEMBLE: loop while stash non-empty — UNSTASH then EVAL level 1
- [ ] 6.2 Tests: single-level reassembly, multi-level reassembly, empty stash no-op

## 7. Integration
- [ ] 7.1 End-to-end test: `'SQ(X)+3'` → EXPLODE → STASHN → EXPLODE → STASH → DROP replacement → ASSEMBLE → verify `'SQ(Y+1)+3'`

## 8. Documentation
- [ ] 8.1 CMD_SET_REFERENCE.md: Add a Symbolic Manipulation section covering SUBST, EXPLODE, STASH, STASHN, UNSTASH, and ASSEMBLE with stack effects, descriptions, and examples
- [ ] 8.2 CMD_SET_REFERENCE.md: Document symbolic pass-through behavior for existing commands (SQ, SQRT, SIN, COS, etc. producing Symbol output from Symbol input)
- [ ] 8.3 CMD_SET_REFERENCE.md: Add the new commands to the Command Summary table
- [ ] 8.4 ARCHITECTURE.md: Add `stash` table to the SQLite Schema section
- [ ] 8.5 ARCHITECTURE.md: Document the auxiliary stash stack concept and comma-separated expression syntax in the relevant architecture sections
