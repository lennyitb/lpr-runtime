## Context
RPL's `→` command binds stack values to local variables. The syntax is:

```
→ var1 var2 ... varN body
```

`→` is a runstream-consuming command: it reads the variable names and body from the token stream following its invocation, rather than from the stack. The body is a single Symbol or Program. Stack values are popped and bound to the names; within the body those names resolve to the bound values.

Separately, `EVAL` on a Symbol needs to parse the infix expression, substitute variables (local then global), and compute a numeric result.

## Goals / Non-Goals
- Goals:
  - `→` command with local variable binding and nested scoping
  - Numeric evaluation of Symbol expressions via `EVAL`
  - `+`, `-`, `*`, `/`, `^`, unary negation, parentheses in expressions
- Non-Goals:
  - Symbolic/CAS operations (DIFF, INTEGRATE, SIMPLIFY)
  - Expression rewriting or simplification
  - List or matrix expressions

## Decisions

### Local Variable Scope Stack
- **Decision**: `std::vector<std::unordered_map<std::string, Object>>` in Context.
- **Rationale**: Push a frame on `→`, pop on body completion. Locals are transient execution state — no SQLite involvement, not part of undo/redo.
- **Rejected**: Storing locals in the SQLite variables table (too heavyweight, conflicts with STO/RCL semantics).

### Runstream Consumption
- **Decision**: `→` is registered as a command. During `execute_tokens`, when `→` is dispatched, it advances the token iterator to consume Name tokens (parameter names) and the next Symbol or Program (body).
- **Rationale**: This is how RPL runstream-consuming commands work. The token stream is the runstream; `→` reads ahead in it.
- **Rejected**: Pre-processing `→` blocks at parse time into a dedicated AST node (unnecessary complexity).

### Expression Evaluation (Shunting-Yard)
- **Decision**: Shunting-yard algorithm to convert Symbol strings from infix to RPN, then evaluate numerically.
- **Operators**: `+`, `-` (binary/unary), `*`, `/`, `^` with standard precedence. Parentheses for grouping.
- **Variable resolution**: Local scope stack (innermost first), then global variables via Store.

## Risks / Trade-offs
- Expression parser is a second parser alongside the RPL tokenizer. Kept minimal — only infix numeric expressions.
- Scope stack lives in Context, not Store. Correct: locals are transient.
