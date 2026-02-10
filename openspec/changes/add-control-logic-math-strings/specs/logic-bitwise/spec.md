## ADDED Requirements

### Requirement: AND Command
`AND` SHALL pop two integers and push 1 if both are non-zero, else 0.

#### Scenario: Both true
- **WHEN** Integer(1) and Integer(1) are on the stack and `AND` is executed
- **THEN** Integer(1) is on the stack

#### Scenario: One false
- **WHEN** Integer(1) and Integer(0) are on the stack and `AND` is executed
- **THEN** Integer(0) is on the stack

### Requirement: OR Command
`OR` SHALL pop two integers and push 1 if either is non-zero, else 0.

#### Scenario: Both true
- **WHEN** Integer(1) and Integer(1) are on the stack and `OR` is executed
- **THEN** Integer(1) is on the stack

#### Scenario: Both false
- **WHEN** Integer(0) and Integer(0) are on the stack and `OR` is executed
- **THEN** Integer(0) is on the stack

### Requirement: NOT Command
`NOT` SHALL pop one integer and push 1 if it is zero, else 0.

#### Scenario: True to false
- **WHEN** Integer(1) is on the stack and `NOT` is executed
- **THEN** Integer(0) is on the stack

#### Scenario: False to true
- **WHEN** Integer(0) is on the stack and `NOT` is executed
- **THEN** Integer(1) is on the stack

### Requirement: XOR Command
`XOR` SHALL pop two integers and push 1 if exactly one is non-zero, else 0.

#### Scenario: Different values
- **WHEN** Integer(1) and Integer(0) are on the stack and `XOR` is executed
- **THEN** Integer(1) is on the stack

#### Scenario: Same values
- **WHEN** Integer(1) and Integer(1) are on the stack and `XOR` is executed
- **THEN** Integer(0) is on the stack

### Requirement: BAND Command
`BAND` SHALL pop two integers and push their bitwise AND.

#### Scenario: Bitwise AND
- **WHEN** Integer(12) and Integer(10) are on the stack and `BAND` is executed
- **THEN** Integer(8) is on the stack (1100 & 1010 = 1000)

### Requirement: BOR Command
`BOR` SHALL pop two integers and push their bitwise OR.

#### Scenario: Bitwise OR
- **WHEN** Integer(12) and Integer(10) are on the stack and `BOR` is executed
- **THEN** Integer(14) is on the stack (1100 | 1010 = 1110)

### Requirement: BXOR Command
`BXOR` SHALL pop two integers and push their bitwise XOR.

#### Scenario: Bitwise XOR
- **WHEN** Integer(12) and Integer(10) are on the stack and `BXOR` is executed
- **THEN** Integer(6) is on the stack (1100 ^ 1010 = 0110)

### Requirement: BNOT Command
`BNOT` SHALL pop one integer and push its bitwise NOT (complement).

#### Scenario: Bitwise NOT
- **WHEN** Integer(0) is on the stack and `BNOT` is executed
- **THEN** Integer(-1) is on the stack (two's complement)

### Requirement: SL Command
`SL` (Shift Left) SHALL pop a shift count (level 1) and a value (level 2), both integers, and push the value left-shifted by the count.

#### Scenario: Shift left
- **WHEN** Integer(1) and Integer(3) are on the stack and `SL` is executed
- **THEN** Integer(8) is on the stack (1 << 3)

### Requirement: SR Command
`SR` (Shift Right) SHALL pop a shift count (level 1) and a value (level 2), both integers, and push the value right-shifted by the count (logical shift, zero-fill).

#### Scenario: Shift right
- **WHEN** Integer(8) and Integer(2) are on the stack and `SR` is executed
- **THEN** Integer(2) is on the stack (8 >> 2)

### Requirement: ASR Command
`ASR` (Arithmetic Shift Right) SHALL pop a shift count (level 1) and a value (level 2), both integers, and push the value arithmetically right-shifted (sign-preserving).

#### Scenario: Arithmetic shift right positive
- **WHEN** Integer(8) and Integer(2) are on the stack and `ASR` is executed
- **THEN** Integer(2) is on the stack

#### Scenario: Arithmetic shift right negative
- **WHEN** Integer(-8) and Integer(1) are on the stack and `ASR` is executed
- **THEN** Integer(-4) is on the stack

### Requirement: SAME Command
`SAME` SHALL pop two objects and push Integer(1) if they are structurally identical (same type and same value), else Integer(0). Unlike `==`, no type promotion occurs.

#### Scenario: Same integers
- **WHEN** Integer(42) and Integer(42) are on the stack and `SAME` is executed
- **THEN** Integer(1) is on the stack

#### Scenario: Different types same value
- **WHEN** Integer(1) and Real(1.0) are on the stack and `SAME` is executed
- **THEN** Integer(0) is on the stack

#### Scenario: Same strings
- **WHEN** String("hello") and String("hello") are on the stack and `SAME` is executed
- **THEN** Integer(1) is on the stack
