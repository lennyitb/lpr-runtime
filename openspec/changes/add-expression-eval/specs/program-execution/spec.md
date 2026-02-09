## ADDED Requirements

### Requirement: Arrow Command
`→` is a runstream-consuming command. It SHALL read N variable names and a body (Symbol or Program) from the runstream, pop N values from the stack, bind them to the names, execute the body, then remove the local variable frame.

#### Scenario: Two variables with Symbol body
- **WHEN** Integer(3) and Integer(4) are on the stack and `-> X Y 'X*Y'` is executed
- **THEN** Integer(12) is on the stack

#### Scenario: Two variables with Program body
- **WHEN** Integer(3) and Integer(4) are on the stack and `-> X Y << X Y * >>` is executed
- **THEN** Integer(12) is on the stack

#### Scenario: One variable
- **WHEN** Integer(5) is on the stack and `-> N 'N*N'` is executed
- **THEN** Integer(25) is on the stack

#### Scenario: Nested arrow blocks
- **WHEN** Integer(2) and Integer(3) are on the stack and `-> A << A 10 -> B << A B * >> >>` is executed
- **THEN** Integer(20) is on the stack (A=2, inner B=10, A*B=20; 3 remains below)

### Requirement: Symbol Expression Evaluation
`EVAL` on a Symbol SHALL parse the expression as infix arithmetic, substitute variable values (local scope first, then global), and compute a numeric result. Supported operators: `+`, `-` (binary and unary), `*`, `/`, `^` with standard precedence. Parentheses for grouping.

#### Scenario: Simple arithmetic
- **WHEN** Symbol("2+3") is on the stack and `EVAL` is executed
- **THEN** Integer(5) is on the stack

#### Scenario: Operator precedence
- **WHEN** Symbol("2+3*4") is on the stack and `EVAL` is executed
- **THEN** Integer(14) is on the stack

#### Scenario: Parentheses
- **WHEN** Symbol("(2+3)*4") is on the stack and `EVAL` is executed
- **THEN** Integer(20) is on the stack

#### Scenario: Global variables
- **WHEN** variable X stores Integer(5), Symbol("X^2") is on the stack, and `EVAL` is executed
- **THEN** Integer(25) is on the stack

#### Scenario: Local variables
- **WHEN** Integer(3) and Integer(4) are on the stack and `-> X Y 'X+Y'` is executed
- **THEN** Integer(7) is on the stack

#### Scenario: Unary negation
- **WHEN** Symbol("-3+5") is on the stack and `EVAL` is executed
- **THEN** Integer(2) is on the stack

#### Scenario: Undefined variable
- **WHEN** Symbol("X+1") is on the stack, X is not defined, and `EVAL` is executed
- **THEN** an error is raised ("Undefined variable: X")

## MODIFIED Requirements

### Requirement: EVAL Command
The `EVAL` command SHALL execute the object at level 1. If it is a Program, execute its tokens. If it is a Name, recall the associated variable and execute it. If it is a Symbol, evaluate the expression numerically by parsing the infix expression, substituting variables, and computing the result.

#### Scenario: Evaluate program
- **WHEN** a Program `<< 2 3 + >>` is on the stack and `EVAL` is executed
- **THEN** Integer(5) is on the stack

#### Scenario: Evaluate name
- **WHEN** a variable `x` stores Integer(42), Name("x") is on the stack, and `EVAL` is executed
- **THEN** Integer(42) is on the stack

#### Scenario: Evaluate symbol expression
- **WHEN** Symbol("2+3*4") is on the stack and `EVAL` is executed
- **THEN** Integer(14) is on the stack
