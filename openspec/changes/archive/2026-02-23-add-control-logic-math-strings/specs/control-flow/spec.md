## ADDED Requirements

### Requirement: IF THEN END
`IF` SHALL consume tokens from the runstream up to `THEN`, execute them (the condition), pop the result, and if truthy, execute tokens from `THEN` up to `END`. Supports optional `ELSE` clause.

#### Scenario: IF true without ELSE
- **WHEN** `IF 1 THEN 42 END` is executed
- **THEN** Integer(42) is on the stack

#### Scenario: IF false without ELSE
- **WHEN** `IF 0 THEN 42 END` is executed
- **THEN** the stack is empty

#### Scenario: IF true with ELSE
- **WHEN** `IF 1 THEN 42 ELSE 99 END` is executed
- **THEN** Integer(42) is on the stack

#### Scenario: IF false with ELSE
- **WHEN** `IF 0 THEN 42 ELSE 99 END` is executed
- **THEN** Integer(99) is on the stack

#### Scenario: Nested IF
- **WHEN** `IF 1 THEN IF 0 THEN 1 ELSE 2 END END` is executed
- **THEN** Integer(2) is on the stack

### Requirement: CASE THEN END
`CASE` SHALL consume a sequence of `val THEN body END` pairs from the runstream. It evaluates each val; when one is truthy, it executes the corresponding body and skips to the final `END`. An optional default body after the last `END` and before the outer `END` executes if no case matched.

#### Scenario: First case matches
- **WHEN** `CASE 1 THEN 10 END 0 THEN 20 END END` is executed
- **THEN** Integer(10) is on the stack

#### Scenario: Second case matches
- **WHEN** `CASE 0 THEN 10 END 1 THEN 20 END END` is executed
- **THEN** Integer(20) is on the stack

#### Scenario: Default case
- **WHEN** `CASE 0 THEN 10 END 0 THEN 20 END 99 END` is executed
- **THEN** Integer(99) is on the stack

### Requirement: FOR NEXT Loop
`FOR` SHALL pop a start and end value from the stack, consume a variable name and body tokens from the runstream up to `NEXT`. It binds the loop variable and executes the body for each integer from start to end inclusive, incrementing by 1.

#### Scenario: Simple counting loop
- **WHEN** Integer(1) and Integer(5) are on the stack and `FOR I I NEXT` is executed
- **THEN** the stack contains 1, 2, 3, 4, 5 (bottom to top)

#### Scenario: Single iteration
- **WHEN** Integer(3) and Integer(3) are on the stack and `FOR I I I * NEXT` is executed
- **THEN** Integer(9) is on the stack

#### Scenario: Zero iterations (start > end)
- **WHEN** Integer(5) and Integer(1) are on the stack and `FOR I I NEXT` is executed
- **THEN** the stack is empty

### Requirement: FOR STEP Loop
`FOR ... STEP` SHALL work like `FOR ... NEXT` but the step increment is determined by the object at level 1 when `STEP` is reached. If that object is a Program, it is executed (and should leave a new counter value). If numeric, it is the increment.

#### Scenario: Counting by twos
- **WHEN** Integer(1) and Integer(10) are on the stack and `FOR I I 2 STEP` is executed
- **THEN** the stack contains 1, 3, 5, 7, 9 (bottom to top)

#### Scenario: Counting backwards
- **WHEN** Integer(5) and Integer(1) are on the stack and `FOR I I -1 STEP` is executed
- **THEN** the stack contains 5, 4, 3, 2, 1 (bottom to top)

### Requirement: START NEXT Loop
`START` SHALL pop a start and end value from the stack and execute body tokens up to `NEXT` for (end - start + 1) iterations. No loop variable is bound.

#### Scenario: Repeat N times
- **WHEN** Integer(1) and Integer(3) are on the stack, Integer(0) is also pushed, and `START 1 + NEXT` is executed (with start=1, end=3, and 0 on stack before START)
- **THEN** Integer(3) is on the stack (0 + 1 + 1 + 1)

### Requirement: START STEP Loop
`START ... STEP` SHALL work like `START ... NEXT` but uses a custom increment popped from the stack at each `STEP`.

#### Scenario: Start step with increment 2
- **WHEN** Integer(1) and Integer(6) are on the stack and `START 2 STEP` is executed (3 iterations: counter 1, 3, 5)
- **THEN** the stack is empty (body pushes nothing)

### Requirement: WHILE REPEAT END Loop
`WHILE` SHALL repeatedly: execute the condition tokens (up to `REPEAT`), pop the result, and if truthy execute the body tokens (up to `END`), then loop. If the condition is falsy, exit.

#### Scenario: Count down
- **WHEN** Integer(3) is on the stack and `WHILE DUP 0 > REPEAT 1 - END` is executed
- **THEN** Integer(0) is on the stack

#### Scenario: Condition false initially
- **WHEN** Integer(0) is on the stack and `WHILE DUP 0 > REPEAT 1 - END` is executed
- **THEN** Integer(0) is on the stack (body never runs)

### Requirement: DO UNTIL END Loop
`DO` SHALL execute the body tokens (up to `UNTIL`), then execute the condition tokens (up to `END`), pop the result, and if falsy, loop. The body always executes at least once.

#### Scenario: Do until countdown
- **WHEN** Integer(3) is on the stack and `DO 1 - DUP UNTIL 0 == END` is executed
- **THEN** Integer(0) is on the stack

#### Scenario: Body executes at least once
- **WHEN** Integer(0) is on the stack and `DO 1 + UNTIL 1 END` is executed
- **THEN** Integer(1) is on the stack (body ran once)
