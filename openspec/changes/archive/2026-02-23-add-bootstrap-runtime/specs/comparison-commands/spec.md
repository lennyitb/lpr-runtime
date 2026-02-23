## ADDED Requirements

### Requirement: Equality Command
The `==` command SHALL pop levels 1 and 2, push Integer(1) if they are equal, Integer(0) otherwise.

#### Scenario: Equal integers
- **WHEN** Integer(5) and Integer(5) are on the stack and `==` is executed
- **THEN** Integer(1) is on the stack

#### Scenario: Unequal integers
- **WHEN** Integer(5) and Integer(3) are on the stack and `==` is executed
- **THEN** Integer(0) is on the stack

### Requirement: Inequality Command
The `!=` command SHALL pop levels 1 and 2, push Integer(1) if they are not equal, Integer(0) otherwise.

#### Scenario: Not equal
- **WHEN** Integer(5) and Integer(3) are on the stack and `!=` is executed
- **THEN** Integer(1) is on the stack

### Requirement: Less Than Command
The `<` command SHALL pop levels 1 and 2, push Integer(1) if level 2 is less than level 1, Integer(0) otherwise.

#### Scenario: Less than true
- **WHEN** Integer(3) is at level 2, Integer(5) is at level 1, and `<` is executed
- **THEN** Integer(1) is on the stack

### Requirement: Greater Than Command
The `>` command SHALL pop levels 1 and 2, push Integer(1) if level 2 is greater than level 1, Integer(0) otherwise.

#### Scenario: Greater than true
- **WHEN** Integer(5) is at level 2, Integer(3) is at level 1, and `>` is executed
- **THEN** Integer(1) is on the stack

### Requirement: Less Than or Equal Command
The `<=` command SHALL pop levels 1 and 2, push Integer(1) if level 2 is less than or equal to level 1, Integer(0) otherwise.

#### Scenario: Less than or equal when equal
- **WHEN** Integer(5) is at level 2, Integer(5) is at level 1, and `<=` is executed
- **THEN** Integer(1) is on the stack

### Requirement: Greater Than or Equal Command
The `>=` command SHALL pop levels 1 and 2, push Integer(1) if level 2 is greater than or equal to level 1, Integer(0) otherwise.

#### Scenario: Greater than or equal when greater
- **WHEN** Integer(7) is at level 2, Integer(3) is at level 1, and `>=` is executed
- **THEN** Integer(1) is on the stack
