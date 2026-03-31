## MODIFIED Requirements

### Requirement: Symbol Stub Type
The Symbol type SHALL store the raw expression text as a `std::string` and serve as the operand type for CAS (Computer Algebra System) commands. CAS operations (DIFF, INTEGRATE, SOLVE, SIMPLIFY, EXPAND, FACTOR) SHALL accept Symbol objects, converting between the string representation and the CAS backend's native expression trees on demand. The string representation remains the canonical form for serialization, display, and non-CAS operations (SUBST, EXPLODE, ASSEMBLE).

#### Scenario: Symbol construction
- **WHEN** a Symbol is constructed from `"X^2 + 1"`
- **THEN** the repr displays `'X^2 + 1'`

#### Scenario: Symbol participates in CAS operations
- **WHEN** a Symbol containing `'X^2+1'` is passed to DIFF with variable `'X'`
- **THEN** a new Symbol containing the derivative is returned

#### Scenario: Symbol serialization unchanged
- **WHEN** a Symbol is serialized to SQLite and deserialized
- **THEN** the string value roundtrips exactly, regardless of whether CAS operations have been applied

#### Scenario: Non-CAS operations unaffected
- **WHEN** SUBST, EXPLODE, or ASSEMBLE operate on a Symbol
- **THEN** they continue to use the string representation directly, with no CAS backend involvement
