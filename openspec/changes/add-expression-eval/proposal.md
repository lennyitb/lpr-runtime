# Change: Add expression evaluation and local variable binding

## Why
Symbols like `'X*Y'` currently sit inert on the stack. RPL needs numeric expression evaluation and the `→` command for local variable binding before useful programs can be written.

## What Changes
- `EVAL` on a Symbol: parse infix expression, substitute variables, compute numeric result
- `→` command (ASCII `->` alias): runstream-consuming command that pops stack values, binds them to names consumed from the runstream, then executes a body token
- Context gains a local-variable scope stack for nested bindings
- Shunting-yard expression evaluator for Symbol strings (numeric only, no CAS)

## Impact
- Affected specs: program-execution, parser, context-engine
- Affected code: `src/core/parser.cpp`, `src/core/commands.cpp`, `src/core/context.hpp`, `src/core/context.cpp`
