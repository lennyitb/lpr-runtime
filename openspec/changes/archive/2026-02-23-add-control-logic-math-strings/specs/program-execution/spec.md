## MODIFIED Requirements

### Requirement: EVAL Command
The `EVAL` command SHALL execute the object at level 1. If it is a Program, execute its tokens. If it is a Name, recall the associated variable and execute it. If it is a Symbol, evaluate the expression numerically by parsing the infix expression, substituting variables, and computing the result. For all other types, push the object back (identity).

#### Scenario: Evaluate program
- **WHEN** a Program `<< 2 3 + >>` is on the stack and `EVAL` is executed
- **THEN** Integer(5) is on the stack

#### Scenario: Evaluate name
- **WHEN** a variable `x` stores Integer(42), Name("x") is on the stack, and `EVAL` is executed
- **THEN** Integer(42) is on the stack

#### Scenario: Evaluate symbol expression
- **WHEN** Symbol("2+3*4") is on the stack and `EVAL` is executed
- **THEN** Integer(14) is on the stack

#### Scenario: Evaluate program with control flow
- **WHEN** a Program `<< 0 1 5 START OVER + SWAP END DROP >>` and `EVAL` is executed
- **THEN** Integer(5) is on the stack (5th Fibonacci number)
