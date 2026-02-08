## ADDED Requirements

### Requirement: Addition Command
The `+` command SHALL pop levels 1 and 2, add them, and push the result. Type promotion SHALL follow the numeric tower: Integer → Rational → Real → Complex.

#### Scenario: Integer addition
- **WHEN** Integer(3) and Integer(4) are on the stack and `+` is executed
- **THEN** Integer(7) is on the stack

#### Scenario: Mixed-type addition promotes
- **WHEN** Integer(1) and Real(2.5) are on the stack and `+` is executed
- **THEN** Real(3.5) is on the stack

### Requirement: Subtraction Command
The `-` command SHALL pop levels 1 and 2, subtract level 1 from level 2, and push the result.

#### Scenario: Integer subtraction
- **WHEN** Integer(10) is at level 2, Integer(3) is at level 1, and `-` is executed
- **THEN** Integer(7) is on the stack

### Requirement: Multiplication Command
The `*` command SHALL pop levels 1 and 2, multiply them, and push the result.

#### Scenario: Integer multiplication
- **WHEN** Integer(6) and Integer(7) are on the stack and `*` is executed
- **THEN** Integer(42) is on the stack

### Requirement: Division Command
The `/` command SHALL pop levels 1 and 2, divide level 2 by level 1, and push the result. Division of two Integers SHALL produce a Rational (exact).

#### Scenario: Integer division produces Rational
- **WHEN** Integer(355) is at level 2, Integer(113) is at level 1, and `/` is executed
- **THEN** Rational(355/113) is on the stack

#### Scenario: Division by zero
- **WHEN** any number is at level 2, Integer(0) is at level 1, and `/` is executed
- **THEN** an Error is produced

### Requirement: NEG Command
The `NEG` command SHALL negate the object at level 1.

#### Scenario: Negate integer
- **WHEN** Integer(5) is on the stack and `NEG` is executed
- **THEN** Integer(-5) is on the stack

### Requirement: INV Command
The `INV` command SHALL invert the object at level 1 (compute 1/x).

#### Scenario: Invert integer
- **WHEN** Integer(4) is on the stack and `INV` is executed
- **THEN** Rational(1/4) is on the stack

### Requirement: ABS Command
The `ABS` command SHALL compute the absolute value of the object at level 1.

#### Scenario: Absolute value of negative
- **WHEN** Integer(-7) is on the stack and `ABS` is executed
- **THEN** Integer(7) is on the stack

### Requirement: MOD Command
The `MOD` command SHALL compute level 2 modulo level 1.

#### Scenario: Integer modulo
- **WHEN** Integer(10) is at level 2, Integer(3) is at level 1, and `MOD` is executed
- **THEN** Integer(1) is on the stack

### Requirement: Numeric Tower Type Promotion
Arithmetic operations SHALL promote operands upward through the tower: Integer → Rational → Real → Complex. The result type SHALL be the highest type among the operands.

#### Scenario: Rational plus Real promotes to Real
- **WHEN** Rational(1/2) and Real(0.3) are on the stack and `+` is executed
- **THEN** the result is a Real

#### Scenario: Real plus Complex promotes to Complex
- **WHEN** Real(1.0) and Complex(2.0, 3.0) are on the stack and `+` is executed
- **THEN** the result is a Complex
