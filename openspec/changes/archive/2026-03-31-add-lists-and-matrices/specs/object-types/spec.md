## ADDED Requirements

### Requirement: List Type
The List type SHALL store a `std::vector<Object>` representing a heterogeneous ordered collection of RPL objects. Lists MAY contain any Object type including nested Lists.

#### Scenario: List construction
- **WHEN** a List is constructed from objects Integer(1), String("hello"), Integer(3)
- **THEN** the List holds three elements in order

#### Scenario: Nested list construction
- **WHEN** a List is constructed containing another List as an element
- **THEN** the nested List is preserved as a single element

### Requirement: List Display Representation
The List type SHALL implement `repr()` returning the format `{ elem1 elem2 ... }` with elements separated by spaces.

#### Scenario: List repr
- **WHEN** `repr()` is called on a List containing Integer(1), Integer(2), Integer(3)
- **THEN** the result is `"{ 1 2 3 }"`

#### Scenario: Nested list repr
- **WHEN** `repr()` is called on a List containing Integer(1) and a nested List {2, 3}
- **THEN** the result is `"{ 1 { 2 3 } }"`

### Requirement: List Serialization
The List type SHALL implement `serialize()` and `deserialize()` for lossless SQLite roundtrip. Serialization SHALL recursively serialize each element.

#### Scenario: List serialize/deserialize roundtrip
- **WHEN** a List containing Integer(1), Real(2.5), String("hi") is serialized and deserialized
- **THEN** the reconstructed List is element-wise equal to the original

### Requirement: Matrix Type
The Matrix type SHALL store a `std::vector<std::vector<Object>>` in row-major order. Elements MUST be numeric (Integer, Real, Rational, Complex) or symbolic (Symbol). String, Program, Name, Error, List, and Matrix elements SHALL be rejected. A single-row matrix represents a vector.

#### Scenario: Matrix construction
- **WHEN** a Matrix is constructed with rows [[1, 2], [3, 4]]
- **THEN** the Matrix holds 2 rows and 2 columns

#### Scenario: Vector as single-row matrix
- **WHEN** a Matrix is constructed with a single row [1, 2, 3]
- **THEN** the Matrix has 1 row and 3 columns, functioning as a vector

#### Scenario: Symbolic matrix construction
- **WHEN** a Matrix is constructed with rows [['a', 'b'], ['c', 'd']] where entries are Symbols
- **THEN** the Matrix holds 2 rows and 2 columns with symbolic entries

#### Scenario: String element rejected
- **WHEN** a Matrix is constructed with a String element
- **THEN** an error is produced

#### Scenario: List element rejected
- **WHEN** a Matrix is constructed with a List element
- **THEN** an error is produced

### Requirement: Matrix Display Representation
The Matrix type SHALL implement `repr()` returning the format `[[ r1c1 r1c2 ][ r2c1 r2c2 ]]` with `][` separating rows.

#### Scenario: Matrix repr
- **WHEN** `repr()` is called on a 2x2 Matrix [[1, 2], [3, 4]]
- **THEN** the result is `"[[ 1 2 ][ 3 4 ]]"`

#### Scenario: Vector repr
- **WHEN** `repr()` is called on a 1-row Matrix [1, 2, 3]
- **THEN** the result is `"[[ 1 2 3 ]]"`

### Requirement: Matrix Serialization
The Matrix type SHALL implement `serialize()` and `deserialize()` for lossless SQLite roundtrip. Serialization SHALL recursively serialize each element with row structure preserved.

#### Scenario: Matrix serialize/deserialize roundtrip
- **WHEN** a 2x2 Matrix of Integers is serialized and deserialized
- **THEN** the reconstructed Matrix is element-wise equal to the original

## MODIFIED Requirements

### Requirement: Object Variant Type
The runtime SHALL implement an `Object` type as a `std::variant` over eleven value types: Integer, Real, Rational, Complex, String, Program, Name, Error, Symbol, List, and Matrix.

#### Scenario: All eleven types are constructible
- **WHEN** an Object is constructed for each of the eleven types
- **THEN** each Object holds the correct type and value

### Requirement: Object Serialization
Each Object type SHALL implement `serialize()` returning a lossless string representation, and a corresponding `deserialize(type_tag, data)` static method that reconstructs the original Object.

#### Scenario: Serialize/deserialize round-trip for Integer
- **WHEN** an Integer is serialized and then deserialized
- **THEN** the reconstructed Object is equal to the original

#### Scenario: Serialize/deserialize round-trip for Real
- **WHEN** a Real is serialized and then deserialized
- **THEN** the reconstructed Object is equal to the original with full precision preserved

#### Scenario: Serialize/deserialize round-trip for all types
- **WHEN** any Object is serialized and then deserialized
- **THEN** the reconstructed Object is equal to the original

#### Scenario: Serialize/deserialize round-trip for List
- **WHEN** a List is serialized and then deserialized
- **THEN** the reconstructed List is element-wise equal to the original

#### Scenario: Serialize/deserialize round-trip for Matrix
- **WHEN** a Matrix is serialized and then deserialized
- **THEN** the reconstructed Matrix is element-wise equal to the original
