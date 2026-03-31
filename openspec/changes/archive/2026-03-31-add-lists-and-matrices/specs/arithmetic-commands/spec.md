## MODIFIED Requirements

### Requirement: Addition Command
The `+` command SHALL pop two values from the stack and push their sum. For numeric types, it follows the numeric tower promotion rules. For String operands, if both are Strings, it SHALL concatenate them. If one operand is a String and the other is not, an error is raised. For symbolic operands, it builds a symbolic expression. For List operands, if both are Lists of equal length, it SHALL perform element-wise addition. For a List and a scalar, it SHALL broadcast the scalar to each element. For Matrix operands, if both are Matrices of equal dimensions, it SHALL perform element-wise addition. For a Matrix and a scalar, it SHALL broadcast the scalar to each element.

#### Scenario: Integer addition
- **WHEN** Integer(2) and Integer(3) are on the stack and `+` is executed
- **THEN** Integer(5) is on the stack

#### Scenario: String concatenation
- **WHEN** String("hello ") and String("world") are on the stack and `+` is executed
- **THEN** String("hello world") is on the stack

#### Scenario: Mixed string and number error
- **WHEN** String("hello") and Integer(1) are on the stack and `+` is executed
- **THEN** an error is raised

#### Scenario: List element-wise addition
- **WHEN** List({1, 2, 3}) and List({10, 20, 30}) are on the stack and `+` is executed
- **THEN** List({11, 22, 33}) is on the stack

#### Scenario: Scalar broadcast to list
- **WHEN** List({1, 2, 3}) and Integer(10) are on the stack and `+` is executed
- **THEN** List({11, 12, 13}) is on the stack

#### Scenario: Mismatched list lengths
- **WHEN** List({1, 2}) and List({1, 2, 3}) are on the stack and `+` is executed
- **THEN** an error is raised

#### Scenario: Matrix element-wise addition
- **WHEN** Matrix [[1,2],[3,4]] and Matrix [[10,20],[30,40]] are on the stack and `+` is executed
- **THEN** Matrix [[11,22],[33,44]] is on the stack

#### Scenario: Symbolic matrix addition
- **WHEN** Matrix [['a','b'],['c','d']] and Matrix [['e','f'],['g','h']] are on the stack and `+` is executed
- **THEN** Matrix of Symbol expressions [['a+e','b+f'],['c+g','d+h']] is on the stack

### Requirement: Subtraction Command
The `-` command SHALL pop levels 1 and 2, subtract level 1 from level 2, and push the result. For Lists, subtraction is element-wise (matching lengths required, scalar broadcast supported). For Matrices, subtraction is element-wise (matching dimensions required, scalar broadcast supported).

#### Scenario: Integer subtraction
- **WHEN** Integer(10) is at level 2, Integer(3) is at level 1, and `-` is executed
- **THEN** Integer(7) is on the stack

#### Scenario: List element-wise subtraction
- **WHEN** List({10, 20, 30}) is at level 2, List({1, 2, 3}) is at level 1, and `-` is executed
- **THEN** List({9, 18, 27}) is on the stack

### Requirement: Multiplication Command
The `*` command SHALL pop levels 1 and 2, multiply them, and push the result. For Lists, multiplication is element-wise (matching lengths required, scalar broadcast supported). For Matrices, `*` performs matrix multiplication when both are Matrices of compatible dimensions. For a Matrix and a scalar, it broadcasts. For a Matrix and a compatible vector, it performs matrix-vector multiplication.

#### Scenario: Integer multiplication
- **WHEN** Integer(3) and Integer(4) are on the stack and `*` is executed
- **THEN** Integer(12) is on the stack

#### Scenario: List element-wise multiplication
- **WHEN** List({2, 3, 4}) and List({10, 20, 30}) are on the stack and `*` is executed
- **THEN** List({20, 60, 120}) is on the stack

#### Scenario: Matrix multiplication
- **WHEN** Matrix [[1,2],[3,4]] and Matrix [[5,6],[7,8]] are on the stack and `*` is executed
- **THEN** Matrix [[19,22],[43,50]] is on the stack

#### Scenario: Scalar times matrix
- **WHEN** Integer(2) and Matrix [[1,2],[3,4]] are on the stack and `*` is executed
- **THEN** Matrix [[2,4],[6,8]] is on the stack

#### Scenario: Symbolic matrix multiplication
- **WHEN** Matrix [['a','b'],['c','d']] and Matrix [['e','f'],['g','h']] are on the stack and `*` is executed
- **THEN** Matrix of Symbol expressions representing the matrix product is on the stack (e.g., element [1,1] is `'a*e+b*g'`)

### Requirement: Division Command
The `/` command SHALL pop levels 1 and 2, divide level 2 by level 1, and push the result. For Lists, division is element-wise (matching lengths required, scalar broadcast supported). For Matrices, division by a scalar divides each element. Matrix / Matrix is not supported (use INV and *).

#### Scenario: Integer division producing rational
- **WHEN** Integer(1) is at level 2, Integer(3) is at level 1, and `/` is executed
- **THEN** Rational(1/3) is on the stack

#### Scenario: List element-wise division
- **WHEN** List({10, 20, 30}) is at level 2, List({2, 5, 10}) is at level 1, and `/` is executed
- **THEN** List({5, 4, 3}) is on the stack

#### Scenario: Matrix divided by scalar
- **WHEN** Matrix [[2,4],[6,8]] is at level 2, Integer(2) is at level 1, and `/` is executed
- **THEN** Matrix [[1,2],[3,4]] is on the stack

### Requirement: NEG Command
The `NEG` command SHALL negate the value at level 1. For Lists, it SHALL negate each element. For Matrices, it SHALL negate each element.

#### Scenario: Negate an integer
- **WHEN** Integer(5) is on the stack and `NEG` is executed
- **THEN** Integer(-5) is on the stack

#### Scenario: Negate a list
- **WHEN** List({1, -2, 3}) is on the stack and `NEG` is executed
- **THEN** List({-1, 2, -3}) is on the stack

#### Scenario: Negate a matrix
- **WHEN** Matrix [[1, -2], [3, -4]] is on the stack and `NEG` is executed
- **THEN** Matrix [[-1, 2], [-3, 4]] is on the stack
