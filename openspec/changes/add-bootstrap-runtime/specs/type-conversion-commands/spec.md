## ADDED Requirements

### Requirement: TYPE Command
The `TYPE` command SHALL pop level 1 and push its type tag as an Integer.

#### Scenario: Type of integer
- **WHEN** Integer(42) is on the stack and `TYPE` is executed
- **THEN** the Integer type tag is pushed onto the stack

### Requirement: NUM Conversion Command
The `→NUM` command SHALL convert the object at level 1 to a Real numeric representation.

#### Scenario: Rational to real
- **WHEN** Rational(355/113) is on the stack and `→NUM` is executed
- **THEN** a Real approximation of 355/113 is on the stack

### Requirement: STR Conversion Command
The `→STR` command SHALL convert the object at level 1 to its String representation.

#### Scenario: Integer to string
- **WHEN** Integer(42) is on the stack and `→STR` is executed
- **THEN** String("42") is on the stack

### Requirement: STR Parse Command
The `STR→` command SHALL parse a String at level 1 as RPL input and execute it.

#### Scenario: Parse and execute string
- **WHEN** String("3 4 +") is on the stack and `STR→` is executed
- **THEN** Integer(7) is on the stack
