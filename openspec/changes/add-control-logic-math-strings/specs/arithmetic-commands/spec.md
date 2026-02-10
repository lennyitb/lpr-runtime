## MODIFIED Requirements

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
