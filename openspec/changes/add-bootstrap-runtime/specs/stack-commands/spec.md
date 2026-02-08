## ADDED Requirements

### Requirement: DUP Command
The `DUP` command SHALL duplicate the object at stack level 1.

#### Scenario: Duplicate top of stack
- **WHEN** the stack contains `[A]` and `DUP` is executed
- **THEN** the stack contains `[A, A]`

### Requirement: DROP Command
The `DROP` command SHALL remove the object at stack level 1.

#### Scenario: Drop top of stack
- **WHEN** the stack contains `[A, B]` and `DROP` is executed
- **THEN** the stack contains `[A]`

### Requirement: SWAP Command
The `SWAP` command SHALL exchange the objects at stack levels 1 and 2.

#### Scenario: Swap top two items
- **WHEN** the stack contains `[A, B]` (B on top) and `SWAP` is executed
- **THEN** the stack contains `[B, A]` (A on top)

### Requirement: OVER Command
The `OVER` command SHALL copy the object at stack level 2 to level 1 (top of stack).

#### Scenario: Copy second item to top
- **WHEN** the stack contains `[A, B]` (B on top) and `OVER` is executed
- **THEN** the stack contains `[A, B, A]` (A on top)

### Requirement: ROT Command
The `ROT` command SHALL rotate the objects at stack levels 1, 2, and 3.

#### Scenario: Rotate top three items
- **WHEN** the stack contains `[A, B, C]` (C on top) and `ROT` is executed
- **THEN** the stack contains `[B, C, A]` (A on top)

### Requirement: DEPTH Command
The `DEPTH` command SHALL push the current stack depth as an Integer.

#### Scenario: Push stack depth
- **WHEN** the stack contains 3 items and `DEPTH` is executed
- **THEN** the stack contains 4 items with Integer(3) on top

### Requirement: CLEAR Command
The `CLEAR` command SHALL remove all objects from the stack.

#### Scenario: Clear entire stack
- **WHEN** the stack contains items and `CLEAR` is executed
- **THEN** the stack is empty (depth 0)
