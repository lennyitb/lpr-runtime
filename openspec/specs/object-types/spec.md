# object-types Specification

## Purpose
TBD - created by archiving change add-bootstrap-runtime. Update Purpose after archive.
## Requirements
### Requirement: Object Variant Type
The runtime SHALL implement an `Object` type as a `std::variant` over eleven value types: Integer, Real, Rational, Complex, String, Program, Name, Error, Symbol, List, and Matrix.

#### Scenario: All eleven types are constructible
- **WHEN** an Object is constructed for each of the eleven types
- **THEN** each Object holds the correct type and value

### Requirement: Integer Type
The Integer type SHALL use `boost::multiprecision::cpp_int` for arbitrary-precision integer arithmetic.

#### Scenario: Large integer construction
- **WHEN** an Integer is constructed from a value exceeding 64-bit range
- **THEN** the value is stored and retrieved without loss of precision

### Requirement: Real Type
The Real type SHALL use `boost::multiprecision::cpp_dec_float_50` for 50-digit decimal precision.

#### Scenario: Real construction with scientific notation
- **WHEN** a Real is constructed from `"1.5E-10"`
- **THEN** the value is stored accurately to at least 50 significant digits

### Requirement: Rational Type
The Rational type SHALL use `boost::multiprecision::cpp_rational` for exact rational arithmetic.

#### Scenario: Rational construction
- **WHEN** a Rational is constructed from numerator 355 and denominator 113
- **THEN** the repr displays `355/113`

### Requirement: Complex Type
The Complex type SHALL be represented as a `std::pair<Real, Real>` storing the real and imaginary parts.

#### Scenario: Complex construction
- **WHEN** a Complex is constructed with real part 3.0 and imaginary part 4.0
- **THEN** the repr displays `(3, 4)` or equivalent

### Requirement: String Type
The String type SHALL store a `std::string` value.

#### Scenario: String with special characters
- **WHEN** a String is constructed from `"hello world"`
- **THEN** the repr displays `"hello world"` with enclosing quotes

### Requirement: Program Type
The Program type SHALL store a `std::vector<Token>` representing a sequence of RPL tokens.

#### Scenario: Program construction
- **WHEN** a Program is constructed from tokens `DUP *`
- **THEN** the repr displays `« DUP * »`

### Requirement: Name Type
The Name type SHALL store a `std::string` identifying a variable name.

#### Scenario: Name construction
- **WHEN** a Name is constructed from `"myvar"`
- **THEN** the repr displays `'myvar'`

### Requirement: Error Type
The Error type SHALL store an integer error code and a `std::string` message.

#### Scenario: Error construction
- **WHEN** an Error is constructed with code 1 and message `"Stack underflow"`
- **THEN** the repr displays the error code and message

### Requirement: Symbol Stub Type
The Symbol type SHALL store the raw expression text as a `std::string` in the bootstrap phase, deferring CAS operations to a later spec.

#### Scenario: Symbol stub construction
- **WHEN** a Symbol is constructed from `"X^2 + 1"`
- **THEN** the repr displays `'X^2 + 1'` and no CAS operations are available

### Requirement: Object Display Representation
Each Object type SHALL implement a `repr()` method that returns a human-readable `std::string` suitable for display to the user.

#### Scenario: repr round-trip readability
- **WHEN** `repr()` is called on any Object
- **THEN** the result is a non-empty string that conveys the object's type and value

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

