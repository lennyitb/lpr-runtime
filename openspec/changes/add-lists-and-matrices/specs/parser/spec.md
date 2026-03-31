## ADDED Requirements

### Requirement: List Literal Parsing
The parser SHALL recognize list literals delimited by `{` and `}`, collect the enclosed elements (which may be any valid token including nested lists), and produce a List Object. List delimiters SHALL nest.

#### Scenario: Simple list
- **WHEN** the input `"{ 1 2 3 }"` is tokenized
- **THEN** a List token containing [Integer(1), Integer(2), Integer(3)] is produced

#### Scenario: Heterogeneous list
- **WHEN** the input `"{ 1 \"hello\" 3.14 }"` is tokenized
- **THEN** a List token containing [Integer(1), String("hello"), Real(3.14)] is produced

#### Scenario: Nested list
- **WHEN** the input `"{ 1 { 2 3 } 4 }"` is tokenized
- **THEN** a List token is produced with the inner list preserved as a nested List element

#### Scenario: Empty list
- **WHEN** the input `"{ }"` is tokenized
- **THEN** an empty List token is produced

### Requirement: Matrix Literal Parsing
The parser SHALL recognize matrix literals delimited by `[[` and `]]`, with `][` separating rows. Elements within rows are whitespace-delimited. Elements MUST be numeric literals or symbolic expressions (single-quoted). String, Program, Name, and other non-numeric/non-symbolic elements SHALL be rejected.

#### Scenario: 2x2 matrix
- **WHEN** the input `"[[ 1 2 ][ 3 4 ]]"` is tokenized
- **THEN** a Matrix token with rows [[1, 2], [3, 4]] is produced

#### Scenario: Row vector
- **WHEN** the input `"[[ 1 2 3 ]]"` is tokenized
- **THEN** a Matrix token with a single row [1, 2, 3] is produced

#### Scenario: Symbolic matrix
- **WHEN** the input `"[[ 'a' 'b' ][ 'c' 'd' ]]"` is tokenized
- **THEN** a Matrix token with symbolic entries is produced

#### Scenario: Mixed numeric and symbolic matrix
- **WHEN** the input `"[[ 1 'x' ][ 'y' 2 ]]"` is tokenized
- **THEN** a Matrix token with mixed numeric and symbolic entries is produced

#### Scenario: Non-uniform row lengths rejected
- **WHEN** the input `"[[ 1 2 ][ 3 ]]"` is tokenized
- **THEN** a parse error is produced indicating non-uniform row lengths

#### Scenario: String element rejected
- **WHEN** the input `"[[ 1 \"hello\" ]]"` is tokenized
- **THEN** a parse error is produced indicating invalid matrix element type
