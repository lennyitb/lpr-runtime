# arithmetic-commands Specification

## Purpose
TBD - created by archiving change add-bootstrap-runtime. Update Purpose after archive.
## Requirements
### Requirement: Addition Command
The `+` command SHALL pop two values from the stack and push their sum. For numeric types, it follows the numeric tower promotion rules. For String operands, if both are Strings, it SHALL concatenate them. If one operand is a String and the other is not, an error is raised. For symbolic operands, it builds a symbolic expression.

#### Scenario: Integer addition
- **WHEN** Integer(2) and Integer(3) are on the stack and `+` is executed
- **THEN** Integer(5) is on the stack

#### Scenario: String concatenation
- **WHEN** String("hello ") and String("world") are on the stack and `+` is executed
- **THEN** String("hello world") is on the stack

#### Scenario: Mixed string and number error
- **WHEN** String("hello") and Integer(1) are on the stack and `+` is executed
- **THEN** an error is raised

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

