## ADDED Requirements

### Requirement: Object Variant Type
The runtime SHALL implement an `Object` type as a `std::variant` over nine value types: Integer, Real, Rational, Complex, String, Program, Name, Error, and Symbol (stub).

#### Scenario: All nine types are constructible
- **WHEN** an Object is constructed for each of the nine types
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
