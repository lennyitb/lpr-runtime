## ADDED Requirements

### Requirement: Local Variable Scope Stack
The Context SHALL maintain a scope stack of local variable frames. Each frame maps variable names to Object values. Frames are pushed by `→` and popped when the body completes.

#### Scenario: Scope lifetime
- **WHEN** `→` binds X=3 and Y=4, and the body completes
- **THEN** the frame is removed and X, Y are no longer resolvable as locals

#### Scenario: Nested scope shadowing
- **WHEN** an outer `→` binds X=10 and an inner `→` binds X=20
- **THEN** inside the inner block X resolves to 20; after it completes X resolves to 10

### Requirement: Local Variable Resolution
When executing a command token that matches a local variable name, the Context SHALL push the variable's value instead of dispatching the command. Innermost scope takes precedence. Registered commands are checked first and always win.

#### Scenario: Local variable lookup
- **WHEN** `→` binds X=42 and the body contains the token `X`
- **THEN** Integer(42) is pushed onto the stack

#### Scenario: Registered commands are not shadowed
- **WHEN** `→` binds DUP=99 and the body contains the token `DUP`
- **THEN** the built-in DUP command executes
