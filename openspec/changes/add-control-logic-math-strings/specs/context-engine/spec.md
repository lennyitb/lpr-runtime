## MODIFIED Requirements

### Requirement: Token Execution
The `execute_tokens` method SHALL iterate through a token stream, pushing literals, dispatching commands, and handling runstream-consuming constructs. Runstream-consuming constructs include `->` (local variable binding) and the structured control flow keywords: `IF`, `CASE`, `FOR`, `START`, `WHILE`, `DO`. When a control flow keyword is encountered, `execute_tokens` SHALL consume the corresponding tokens from the runstream according to the construct's grammar, execute conditional/loop bodies via recursive calls to `execute_tokens`, and advance the token index past the closing `END`/`NEXT`/`STEP`.

#### Scenario: Execute a simple program
- **WHEN** `execute_tokens` is called with `[Literal(3), Literal(4), Command("+")]`
- **THEN** Integer(7) is on the stack

#### Scenario: Execute arrow binding
- **WHEN** Integer(5) is on the stack and `execute_tokens` is called with `[Command("->"), Command("X"), Literal(Symbol("X*X"))]`
- **THEN** Integer(25) is on the stack

#### Scenario: Execute IF construct
- **WHEN** `execute_tokens` is called with tokens for `IF 1 THEN 42 END`
- **THEN** Integer(42) is on the stack

#### Scenario: Execute FOR loop
- **WHEN** Integer(1) and Integer(3) are on the stack and `execute_tokens` processes `FOR I I NEXT`
- **THEN** the stack contains 1, 2, 3
