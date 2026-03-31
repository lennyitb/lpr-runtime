# matrix-commands Specification

## Purpose
TBD - created by archiving change add-lists-and-matrices. Update Purpose after archive.
## Requirements
### Requirement: ->V2 Command
The `->V2` command SHALL pop two numeric or symbolic values from the stack and push a 1x2 Matrix (2D vector).

#### Scenario: Construct 2D vector
- **WHEN** Integer(3) is at level 2, Integer(4) is at level 1, and `->V2` is executed
- **THEN** Matrix([[3, 4]]) is on the stack

#### Scenario: Construct symbolic 2D vector
- **WHEN** Symbol("x") is at level 2, Symbol("y") is at level 1, and `->V2` is executed
- **THEN** Matrix([['x', 'y']]) is on the stack

### Requirement: ->V3 Command
The `->V3` command SHALL pop three numeric or symbolic values from the stack and push a 1x3 Matrix (3D vector).

#### Scenario: Construct 3D vector
- **WHEN** Integer(1), Integer(2), Integer(3) are on the stack and `->V3` is executed
- **THEN** Matrix([[1, 2, 3]]) is on the stack

### Requirement: V-> Command
The `V->` command SHALL pop a vector (single-row Matrix) from the stack, push each element, then push the element count.

#### Scenario: Explode vector
- **WHEN** Matrix([[3, 4, 5]]) is on the stack and `V->` is executed
- **THEN** the stack contains Integer(3), Integer(4), Integer(5), Integer(3) (count on top)

### Requirement: CON Command
The `CON` command SHALL pop a numeric value from level 1 and a dimensions List `{ rows cols }` (or `{ n }` for vector) from level 2, and push a Matrix filled with that constant value.

#### Scenario: Constant 2x3 matrix
- **WHEN** List({2, 3}) is at level 2, Integer(0) is at level 1, and `CON` is executed
- **THEN** a 2x3 Matrix of all zeros is on the stack

#### Scenario: Constant vector
- **WHEN** List({4}) is at level 2, Integer(1) is at level 1, and `CON` is executed
- **THEN** Matrix([[1, 1, 1, 1]]) is on the stack

### Requirement: IDN Command
The `IDN` command SHALL pop an Integer N from level 1 and push an NxN identity Matrix.

#### Scenario: 3x3 identity
- **WHEN** Integer(3) is on the stack and `IDN` is executed
- **THEN** a 3x3 identity Matrix [[1,0,0],[0,1,0],[0,0,1]] is on the stack

### Requirement: RDM Command
The `RDM` command SHALL pop a dimensions List `{ rows cols }` from level 1 and a Matrix from level 2, and reshape the matrix to the new dimensions. The total element count MUST match. If target is `{ n }`, the result is a vector.

#### Scenario: Reshape 2x3 to 3x2
- **WHEN** a 2x3 Matrix is at level 2, List({3, 2}) is at level 1, and `RDM` is executed
- **THEN** a 3x2 Matrix with elements in row-major order is on the stack

#### Scenario: Element count mismatch
- **WHEN** a 2x2 Matrix is at level 2, List({3, 3}) is at level 1, and `RDM` is executed
- **THEN** an error is produced

### Requirement: TRN Command
The `TRN` command SHALL pop a Matrix from level 1 and push its transpose.

#### Scenario: Transpose a 2x3 matrix
- **WHEN** a 2x3 Matrix [[1,2,3],[4,5,6]] is on the stack and `TRN` is executed
- **THEN** a 3x2 Matrix [[1,4],[2,5],[3,6]] is on the stack

### Requirement: DET Command
The `DET` command SHALL pop a square Matrix from level 1 and push its determinant. For numeric matrices the result is a numeric value. For matrices containing symbolic entries the result is a Symbol expression built via the existing expression builder. Non-square matrices SHALL produce an error.

#### Scenario: 2x2 determinant
- **WHEN** Matrix [[1,2],[3,4]] is on the stack and `DET` is executed
- **THEN** Integer(-2) is on the stack

#### Scenario: 2x2 symbolic determinant
- **WHEN** Matrix [['a','b'],['c','d']] is on the stack and `DET` is executed
- **THEN** a Symbol representing `a*d-b*c` is on the stack

#### Scenario: Non-square matrix
- **WHEN** a 2x3 Matrix is on the stack and `DET` is executed
- **THEN** an error is produced

### Requirement: INV Command on Matrices
The `INV` command, when applied to a square numeric Matrix, SHALL push its inverse. Singular matrices SHALL produce an error. Symbolic matrices SHALL produce an error (pending CAS integration for symbolic inversion).

#### Scenario: 2x2 inverse
- **WHEN** Matrix [[1,2],[3,4]] is on the stack and `INV` is executed
- **THEN** the inverse Matrix [[-2,1],[1.5,-0.5]] is on the stack

#### Scenario: Singular matrix
- **WHEN** Matrix [[1,2],[2,4]] is on the stack and `INV` is executed
- **THEN** an error is produced

#### Scenario: Symbolic matrix inverse rejected
- **WHEN** a symbolic Matrix is on the stack and `INV` is executed
- **THEN** an error is produced indicating symbolic inversion requires CAS

### Requirement: CROSS Command
The `CROSS` command SHALL pop two 3-element vectors from the stack and push their cross product. For symbolic vectors, the result elements are Symbol expressions built via the expression builder.

#### Scenario: Cross product
- **WHEN** vectors [1,0,0] and [0,1,0] are on the stack and `CROSS` is executed
- **THEN** vector [0,0,1] is on the stack

#### Scenario: Symbolic cross product
- **WHEN** symbolic vectors ['a','b','c'] and ['d','e','f'] are on the stack and `CROSS` is executed
- **THEN** a vector of Symbol expressions representing the cross product is on the stack

#### Scenario: Non-3D vectors
- **WHEN** two 2-element vectors are on the stack and `CROSS` is executed
- **THEN** an error is produced

### Requirement: DOT Command
The `DOT` command SHALL pop two vectors of equal length from the stack and push their dot product (scalar). For symbolic vectors, the result is a Symbol expression.

#### Scenario: Dot product
- **WHEN** vectors [1,2,3] and [4,5,6] are on the stack and `DOT` is executed
- **THEN** Integer(32) is on the stack (1*4 + 2*5 + 3*6)

#### Scenario: Symbolic dot product
- **WHEN** symbolic vectors ['a','b'] and ['c','d'] are on the stack and `DOT` is executed
- **THEN** a Symbol representing `a*c+b*d` is on the stack

#### Scenario: Mismatched lengths
- **WHEN** vectors of lengths 2 and 3 are on the stack and `DOT` is executed
- **THEN** an error is produced

### Requirement: ABS Command on Vectors
The `ABS` command, when applied to a numeric vector (single-row Matrix), SHALL push the Euclidean norm (L2 norm) as a Real. Symbolic vectors SHALL produce an error (norm requires numeric evaluation).

#### Scenario: Vector norm
- **WHEN** vector [3, 4] is on the stack and `ABS` is executed
- **THEN** Real(5.0) is on the stack

#### Scenario: Symbolic vector norm rejected
- **WHEN** a symbolic vector is on the stack and `ABS` is executed
- **THEN** an error is produced

### Requirement: GET Command on Matrices
The `GET` command, when given a Matrix at level 2 and a List `{ row col }` at level 1, SHALL return the element at that 1-based position.

#### Scenario: Get matrix element
- **WHEN** Matrix [[1,2],[3,4]] is at level 2, List({2, 1}) is at level 1, and `GET` is executed
- **THEN** Integer(3) is on the stack

### Requirement: PUT Command on Matrices
The `PUT` command, when given a Matrix at level 3, a List `{ row col }` at level 2, and a numeric or symbolic value at level 1, SHALL return a new Matrix with that element replaced. Non-numeric, non-symbolic values SHALL produce an error.

#### Scenario: Put matrix element
- **WHEN** Matrix [[1,2],[3,4]] is at level 3, List({1, 2}) at level 2, Integer(99) at level 1, and `PUT` is executed
- **THEN** Matrix [[1,99],[3,4]] is on the stack

### Requirement: SIZE Command on Matrices
The `SIZE` command, when applied to a Matrix, SHALL push a List `{ rows cols }`.

#### Scenario: Matrix size
- **WHEN** a 2x3 Matrix is on the stack and `SIZE` is executed
- **THEN** List({2, 3}) is on the stack

#### Scenario: Vector size
- **WHEN** a 1x4 vector Matrix is on the stack and `SIZE` is executed
- **THEN** List({1, 4}) is on the stack

