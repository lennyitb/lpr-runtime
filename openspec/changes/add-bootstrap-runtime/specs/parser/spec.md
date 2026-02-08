## ADDED Requirements

### Requirement: Single-Pass Tokenizer
The parser SHALL tokenize RPL input in a single pass, producing a `std::vector<Token>` where each Token is either a literal Object to push or a command name to execute. Whitespace is the delimiter.

#### Scenario: Simple token sequence
- **WHEN** the input `"3 4 +"` is tokenized
- **THEN** three tokens are produced: Integer(3), Integer(4), and command `"+"`

### Requirement: Integer Literal Parsing
The parser SHALL recognize integer literals consisting of an optional `-` followed by digits and produce an Integer Object.

#### Scenario: Positive integer
- **WHEN** the input `"42"` is tokenized
- **THEN** an Integer(42) token is produced

#### Scenario: Negative integer
- **WHEN** the input `"-7"` is tokenized
- **THEN** an Integer(-7) token is produced

### Requirement: Real Literal Parsing
The parser SHALL recognize real literals containing digits with `.` and/or `E` notation and produce a Real Object.

#### Scenario: Decimal real
- **WHEN** the input `"3.14159"` is tokenized
- **THEN** a Real(3.14159) token is produced

#### Scenario: Scientific notation
- **WHEN** the input `"1.5E-10"` is tokenized
- **THEN** a Real(1.5e-10) token is produced

### Requirement: Complex Literal Parsing
The parser SHALL recognize complex literals in the form `(re, im)` and produce a Complex Object.

#### Scenario: Complex number
- **WHEN** the input `"(3.0, 4.0)"` is tokenized
- **THEN** a Complex(3.0, 4.0) token is produced

### Requirement: String Literal Parsing
The parser SHALL recognize string literals enclosed in double quotes and produce a String Object.

#### Scenario: String literal
- **WHEN** the input `"\"hello\""` is tokenized
- **THEN** a String("hello") token is produced

### Requirement: Quoted Name Parsing
The parser SHALL recognize single-quoted identifiers (bare words with no operators) as Name Objects.

#### Scenario: Quoted name
- **WHEN** the input `"'myvar'"` is tokenized
- **THEN** a Name("myvar") token is produced

### Requirement: Quoted Expression Parsing (Symbol Stub)
The parser SHALL recognize single-quoted expressions containing operators as Symbol Objects (storing the raw text in the bootstrap phase).

#### Scenario: Quoted expression
- **WHEN** the input `"'X^2 + 1'"` is tokenized
- **THEN** a Symbol("X^2 + 1") token is produced

### Requirement: Program Literal Parsing
The parser SHALL recognize program literals delimited by `«` and `»`, collect the enclosed tokens, and produce a Program Object. Program delimiters SHALL nest.

#### Scenario: Simple program
- **WHEN** the input `"« DUP * »"` is tokenized
- **THEN** a Program token containing [DUP, *] is produced

#### Scenario: Nested programs
- **WHEN** the input `"« 1 « 2 3 + » EVAL »"` is tokenized
- **THEN** a Program token is produced with the inner program preserved as a nested Program object

### Requirement: Bare Word Command Lookup
The parser SHALL treat any unrecognized bare word as a command name to look up in the command registry.

#### Scenario: Command name
- **WHEN** the input `"DUP"` is tokenized
- **THEN** a command token for `"DUP"` is produced
