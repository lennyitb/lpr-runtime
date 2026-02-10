## Context
The runtime has a working token execution loop (`execute_tokens`) that already handles runstream consumption for the `->` command. Control flow structures follow the same pattern: they consume tokens from the runstream rather than operating on pre-parsed AST nodes. This is faithful to how the HP 50g works -- control flow keywords are consumed at execution time from the token stream.

The runtime has 45 commands across 6 categories. This change adds ~50 more, organized into 4 new categories.

## Goals / Non-Goals
- Goals:
  - All six HP 50g structured control flow constructs
  - Boolean and bitwise integer operations
  - Full scientific calculator math library (trig, log, exp, rounding, combinatorics)
  - String operations sufficient for metaprogramming
  - All commands operate on the existing numeric tower with proper type promotion
  - Comprehensive test coverage for each command group

- Non-Goals:
  - HMS time conversion (->HMS, HMS->) -- deferred to Roadmap #19
  - List-aware overloads (applying commands element-wise to lists) -- deferred to Roadmap #15
  - Complex-valued trig beyond what `std::complex` gives us naturally
  - Full system flag infrastructure (SF, CF, FS?, FC?) -- deferred to Roadmap #19

## Decisions

### Control flow: runstream consumption in execute_tokens
Control flow structures (IF, FOR, WHILE, etc.) are implemented as special cases in the `execute_tokens` loop, not as registered commands. This matches how `->` works today. The loop recognizes keywords like `IF`, `THEN`, `ELSE`, `END` etc. and consumes tokens accordingly.

Rationale: These constructs span multiple tokens and need to conditionally skip or repeat sections of the runstream. They cannot be simple stack-popping commands.

Alternative considered: Pre-parsing control structures into nested Program objects during parse(). Rejected because RPL semantics require runtime token consumption, and pre-parsing would change the execution model.

### Nesting support
Control flow structures nest naturally because the token consumption is recursive. An IF inside a FOR body works because the FOR body is collected as a span of tokens and re-executed via `execute_tokens`, which will then handle the inner IF.

### Angle mode via meta table
`DEG`, `RAD`, `GRAD` commands write to `meta` key `angle_mode` (values: `"DEG"`, `"RAD"`, `"GRAD"`). Default is `"RAD"`. Trig functions (SIN, COS, TAN, ASIN, ACOS, ATAN, ATAN2) read the mode at execution time and convert input/output accordingly:
- RAD: no conversion (native)
- DEG: input *= pi/180 before trig, output *= 180/pi after inverse trig
- GRAD: input *= pi/200 before trig, output *= 200/pi after inverse trig

This uses the existing `meta` table and Store infrastructure -- no new schema.

### Generic settings getter (`lpr_get_setting`)
Rather than adding fields to `lpr_state` for each new setting, we add one new C API function: `char* lpr_get_setting(lpr_ctx* ctx, const char* key)`. It reads from the `meta` table and returns a caller-freed string (NULL if key doesn't exist). This covers angle mode now and all future settings (display mode, coordinate mode, system flags) without ABI changes. `lpr_state` stays reserved for operational state (undo/redo levels).

### String concatenation overloads +
The existing `+` command gains a string case: if both operands are String, concatenate. If one operand is a String and the other is numeric, error (no implicit coercion). This matches HP 50g behavior.

### SAME vs ==
`==` does numeric comparison with type promotion. `SAME` does deep structural equality -- same type AND same value. `SAME("hello", "hello")` -> 1, `SAME(Integer(1), Real(1.0))` -> 0.

## Risks / Trade-offs
- `execute_tokens` grows more complex with control flow. Mitigated by keeping each construct as a self-contained block with clear token-consumption boundaries.
- `commands.cpp` is already 815 lines. New commands will push it past 1500. Acceptable for now; Roadmap notes this as a future performance/splitting task.

## Open Questions
- None blocking. All design decisions are settled.
