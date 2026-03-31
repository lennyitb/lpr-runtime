## ADDED Requirements

### Requirement: CAS Bridge Interface
The runtime SHALL define an abstract `CASBridge` class in `src/cas/bridge.hpp` with pure virtual methods: `differentiate`, `integrate`, `solve`, `simplify`, `expand`, and `factor`. A concrete `SymEngineBridge` implementation SHALL convert between the runtime's Symbol string representation and SymEngine's native expression trees, performing algebraic operations via SymEngine's API.

The bridge SHALL maintain a bidirectional function name mapping between RPL conventions (uppercase: SIN, COS, LN, EXP, SQRT, SQ) and SymEngine conventions (lowercase: sin, cos, log, exp, sqrt, pow). Expressions containing functions not recognized by SymEngine (e.g., IFTE) SHALL produce a descriptive error.

`Context` SHALL own a `std::unique_ptr<CASBridge>`, defaulting to `SymEngineBridge`, accessible via `Context::cas()`.

#### Scenario: Bridge converts expression round-trip
- **WHEN** the string `'X^2+3*X+1'` is converted to a SymEngine expression and back
- **THEN** the result SHALL be mathematically equivalent to the original

#### Scenario: Bridge maps function names
- **WHEN** the string `'SIN(X)'` is converted to SymEngine
- **THEN** SymEngine receives the equivalent of `sin(x)`
- **AND** converting the SymEngine result back produces `'SIN(X)'` (or equivalent uppercase form)

#### Scenario: Unknown function produces error
- **WHEN** a CAS operation is attempted on an expression containing `'IFTE(A, B, C)'`
- **THEN** an error SHALL be produced indicating the function is not supported for CAS operations

### Requirement: DIFF Command
The system SHALL provide a DIFF command that symbolically differentiates an expression with respect to a variable.

Arguments (stack bottom to top):
- Level 2: Symbol (expression to differentiate)
- Level 1: Name (variable of differentiation)

The result SHALL be a Symbol containing the symbolic derivative, pushed to level 1. The derivative SHALL be mathematically correct for polynomial, trigonometric, exponential, and logarithmic expressions.

#### Scenario: Polynomial differentiation
- **WHEN** the stack contains `'X^2+3*X+1'` at level 2 and `'X'` at level 1
- **AND** DIFF is executed
- **THEN** the stack SHALL contain a Symbol mathematically equivalent to `'2*X+3'`

#### Scenario: Trigonometric differentiation
- **WHEN** the stack contains `'SIN(X)'` at level 2 and `'X'` at level 1
- **AND** DIFF is executed
- **THEN** the stack SHALL contain a Symbol mathematically equivalent to `'COS(X)'`

#### Scenario: Chain rule
- **WHEN** the stack contains `'SIN(X^2)'` at level 2 and `'X'` at level 1
- **AND** DIFF is executed
- **THEN** the stack SHALL contain a Symbol mathematically equivalent to `'2*X*COS(X^2)'`

#### Scenario: Constant expression
- **WHEN** the stack contains `'5'` at level 2 and `'X'` at level 1
- **AND** DIFF is executed
- **THEN** the stack SHALL contain a Symbol equivalent to `'0'`

#### Scenario: Wrong type at level 2
- **WHEN** the stack contains Integer `42` at level 2 and `'X'` at level 1
- **AND** DIFF is executed
- **THEN** an error SHALL be raised indicating DIFF requires a symbolic expression

### Requirement: INTEGRATE Command
The system SHALL provide an INTEGRATE command that computes the indefinite integral of an expression with respect to a variable.

Arguments (stack bottom to top):
- Level 2: Symbol (integrand)
- Level 1: Name (variable of integration)

The result SHALL be a Symbol containing the antiderivative (without constant of integration), pushed to level 1. If the antiderivative cannot be found, an error SHALL be pushed.

#### Scenario: Polynomial integration
- **WHEN** the stack contains `'2*X+3'` at level 2 and `'X'` at level 1
- **AND** INTEGRATE is executed
- **THEN** the stack SHALL contain a Symbol mathematically equivalent to `'X^2+3*X'`

#### Scenario: Trigonometric integration
- **WHEN** the stack contains `'COS(X)'` at level 2 and `'X'` at level 1
- **AND** INTEGRATE is executed
- **THEN** the stack SHALL contain a Symbol mathematically equivalent to `'SIN(X)'`

#### Scenario: Integration failure
- **WHEN** the stack contains an expression that SymEngine cannot integrate
- **AND** INTEGRATE is executed
- **THEN** an error SHALL be pushed indicating the antiderivative could not be found

### Requirement: SOLVE Command
The system SHALL provide a SOLVE command that solves an expression for a variable, treating the expression as implicitly equal to zero.

Arguments (stack bottom to top):
- Level 2: Symbol (expression, implicitly `= 0`)
- Level 1: Name (variable to solve for)

The result SHALL be a List of solutions pushed to level 1. Solutions that are exact integers SHALL be returned as Integer objects. Solutions that are exact rationals SHALL be returned as Rational objects. Symbolic solutions SHALL be returned as Symbol objects. If no solutions exist, an empty List SHALL be pushed.

#### Scenario: Quadratic with integer solutions
- **WHEN** the stack contains `'X^2-4'` at level 2 and `'X'` at level 1
- **AND** SOLVE is executed
- **THEN** the stack SHALL contain a List equivalent to `{ -2 2 }` (Integer objects)

#### Scenario: Linear equation
- **WHEN** the stack contains `'2*X-6'` at level 2 and `'X'` at level 1
- **AND** SOLVE is executed
- **THEN** the stack SHALL contain a List equivalent to `{ 3 }`

#### Scenario: Rational solution
- **WHEN** the stack contains `'3*X-1'` at level 2 and `'X'` at level 1
- **AND** SOLVE is executed
- **THEN** the stack SHALL contain a List with a single Rational `1/3`

#### Scenario: No real solutions
- **WHEN** the stack contains `'X^2+1'` at level 2 and `'X'` at level 1
- **AND** SOLVE is executed over the reals
- **THEN** the stack SHALL contain an empty List `{ }`

### Requirement: SIMPLIFY Command
The system SHALL provide a SIMPLIFY command that applies algebraic simplification to a symbolic expression.

Arguments:
- Level 1: Symbol (expression to simplify)

The result SHALL be a Symbol containing a simplified form of the expression, pushed to level 1. Simplification SHALL at minimum combine like terms and cancel common factors.

#### Scenario: Combine like terms
- **WHEN** the stack contains `'X+X'`
- **AND** SIMPLIFY is executed
- **THEN** the stack SHALL contain a Symbol mathematically equivalent to `'2*X'`

#### Scenario: Cancel common factors
- **WHEN** the stack contains `'X^2/X'`
- **AND** SIMPLIFY is executed
- **THEN** the stack SHALL contain a Symbol mathematically equivalent to `'X'`

#### Scenario: Trigonometric identity
- **WHEN** the stack contains `'SIN(X)^2+COS(X)^2'`
- **AND** SIMPLIFY is executed
- **THEN** the stack SHALL contain a Symbol equivalent to `'1'`

#### Scenario: Already simple
- **WHEN** the stack contains `'X+1'`
- **AND** SIMPLIFY is executed
- **THEN** the stack SHALL contain `'X+1'` (unchanged or equivalent)

### Requirement: EXPAND Command
The system SHALL provide an EXPAND command that algebraically expands a symbolic expression (distributing products over sums, expanding powers of sums).

Arguments:
- Level 1: Symbol (expression to expand)

The result SHALL be a Symbol in fully expanded form, pushed to level 1.

#### Scenario: Binomial expansion
- **WHEN** the stack contains `'(X+1)^2'`
- **AND** EXPAND is executed
- **THEN** the stack SHALL contain a Symbol mathematically equivalent to `'X^2+2*X+1'`

#### Scenario: Distribution
- **WHEN** the stack contains `'(X+1)*(X-1)'`
- **AND** EXPAND is executed
- **THEN** the stack SHALL contain a Symbol mathematically equivalent to `'X^2-1'`

#### Scenario: Cubic expansion
- **WHEN** the stack contains `'(X+1)^3'`
- **AND** EXPAND is executed
- **THEN** the stack SHALL contain a Symbol mathematically equivalent to `'X^3+3*X^2+3*X+1'`

### Requirement: FACTOR Command
The system SHALL provide a FACTOR command that factors a polynomial expression over the integers.

Arguments:
- Level 1: Symbol (expression to factor)

The result SHALL be a Symbol in factored form, pushed to level 1. If the expression cannot be factored (irreducible over the integers), it SHALL be returned unchanged.

#### Scenario: Difference of squares
- **WHEN** the stack contains `'X^2-4'`
- **AND** FACTOR is executed
- **THEN** the stack SHALL contain a Symbol mathematically equivalent to `'(X-2)*(X+2)'`

#### Scenario: Perfect square
- **WHEN** the stack contains `'X^2+2*X+1'`
- **AND** FACTOR is executed
- **THEN** the stack SHALL contain a Symbol mathematically equivalent to `'(X+1)^2'`

#### Scenario: Irreducible polynomial
- **WHEN** the stack contains `'X^2+1'`
- **AND** FACTOR is executed
- **THEN** the stack SHALL contain `'X^2+1'` (unchanged, irreducible over integers)

#### Scenario: Common factor extraction
- **WHEN** the stack contains `'2*X^2+4*X'`
- **AND** FACTOR is executed
- **THEN** the stack SHALL contain a Symbol mathematically equivalent to `'2*X*(X+2)'`
