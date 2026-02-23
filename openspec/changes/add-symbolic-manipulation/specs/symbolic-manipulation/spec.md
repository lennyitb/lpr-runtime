## ADDED Requirements

### Requirement: SUBST Command
The system SHALL provide a SUBST command that substitutes all occurrences of a variable name within a symbolic expression with a replacement value.

Arguments (stack bottom to top):
- Level 3: Symbol (expression to modify)
- Level 2: Name or Symbol (variable name to find)
- Level 1: Any object (replacement value)

The command SHALL respect token boundaries (replacing whole variable names, not substrings). Compound replacement expressions SHALL be parenthesized when inserted into higher-precedence contexts to preserve semantics.

#### Scenario: Simple numeric substitution
- **WHEN** the stack contains `'X+1'` at level 3, `'X'` at level 2, `5` at level 1
- **AND** SUBST is executed
- **THEN** the stack SHALL contain `'5+1'`

#### Scenario: Expression substitution with parenthesization
- **WHEN** the stack contains `'X*3'` at level 3, `'X'` at level 2, `'A+1'` at level 1
- **AND** SUBST is executed
- **THEN** the stack SHALL contain `'(A+1)*3'`

#### Scenario: No match leaves expression unchanged
- **WHEN** the stack contains `'X+1'` at level 3, `'Y'` at level 2, `5` at level 1
- **AND** SUBST is executed
- **THEN** the stack SHALL contain `'X+1'`

#### Scenario: Multiple occurrences
- **WHEN** the stack contains `'X+X*2'` at level 3, `'X'` at level 2, `3` at level 1
- **AND** SUBST is executed
- **THEN** the stack SHALL contain `'3+3*2'`

### Requirement: EXPLODE Command
The system SHALL provide an EXPLODE command that decomposes a symbolic expression's top-level operation into its operands and the operator.

The operator SHALL be pushed as a single-token Program (e.g., `« + »` or `« SQ »`) suitable for EVAL-based reassembly.

For binary operations (`+`, `-`, `*`, `/`, `^`): the left operand is pushed first, then the right operand, then the operator Program. For function application (e.g., `SQ(X)`): arguments are pushed first, then the function Program.

Numeric literals within the expression SHALL be pushed as their native numeric type (Integer, Real). Variable names SHALL be pushed as Names. Sub-expressions SHALL be pushed as Symbols.

#### Scenario: Binary operation
- **WHEN** the stack contains `'A+3'` and EXPLODE is executed
- **THEN** the stack SHALL contain level 3: Name `A`, level 2: Integer `3`, level 1: Program `« + »`

#### Scenario: Unary function
- **WHEN** the stack contains `'SQ(X)'` and EXPLODE is executed
- **THEN** the stack SHALL contain level 2: Name `X`, level 1: Program `« SQ »`

#### Scenario: Nested expression preserves sub-expressions
- **WHEN** the stack contains `'SQ(X)+3'` and EXPLODE is executed
- **THEN** the stack SHALL contain level 3: Symbol `'SQ(X)'`, level 2: Integer `3`, level 1: Program `« + »`

#### Scenario: Atomic expression raises error
- **WHEN** the stack contains a Symbol with a single variable name (e.g., `'X'`) and EXPLODE is executed
- **THEN** an error SHALL be raised

### Requirement: STASH Command
The system SHALL provide a STASH command that removes the top item from the visible stack and stores it as a single-item group on a hidden stash stack (LIFO).

The stash SHALL be persisted in SQLite and included in undo/redo snapshots.

#### Scenario: Stash one item
- **WHEN** the stack contains level 2: Name `A`, level 1: Program `« + »`
- **AND** STASH is executed
- **THEN** the visible stack SHALL contain only Name `A`
- **AND** the stash SHALL hold one group containing `« + »`

#### Scenario: Stash from empty stack raises error
- **WHEN** the stack is empty and STASH is executed
- **THEN** an error SHALL be raised

### Requirement: STASHN Command
The system SHALL provide a STASHN command that takes a count N from stack level 1, then removes the next N items from the top of the visible stack and stores them as a single group on the stash.

Items SHALL be restored in their original stack order when later unstashed.

#### Scenario: Stash multiple items after EXPLODE
- **WHEN** the stack contains level 4: Symbol `'SQ(X)'`, level 3: Integer `3`, level 2: Program `« + »`, level 1: Integer `2`
- **AND** STASHN is executed
- **THEN** the visible stack SHALL contain only Symbol `'SQ(X)'`
- **AND** the stash SHALL hold one group containing (`3`, `« + »`) in that order

#### Scenario: Insufficient stack depth raises error
- **WHEN** the stack contains `1` at level 1 but no other items
- **AND** STASHN is executed
- **THEN** an error SHALL be raised

### Requirement: UNSTASH Command
The system SHALL provide an UNSTASH command that pops the most recent group from the stash and pushes all its items back onto the visible stack in their original order.

#### Scenario: Unstash restores a group
- **GIVEN** the stash holds one group (`3`, `« + »`) and the stack contains Symbol `'SQ(Y+1)'`
- **WHEN** UNSTASH is executed
- **THEN** the stack SHALL contain level 3: `'SQ(Y+1)'`, level 2: `3`, level 1: `« + »`

#### Scenario: Unstash from empty stash raises error
- **WHEN** the stash is empty and UNSTASH is executed
- **THEN** an error SHALL be raised

### Requirement: ASSEMBLE Command
The system SHALL provide an ASSEMBLE command that repeatedly unstashes the most recent group and EVALs the item at stack level 1, until the stash is empty.

Each cycle performs: UNSTASH, then EVAL. The EVAL executes the operator Program, which consumes its operands from the stack and pushes the reconstructed expression.

#### Scenario: Full round-trip reconstruction
- **GIVEN** the stack contains Name `Y+1`
- **AND** the stash holds (oldest to newest): group 1 = (`3`, `« + »`), group 2 = (`« SQ »`)
- **WHEN** ASSEMBLE is executed
- **THEN** cycle 1: unstash `« SQ »`, EVAL produces `'SQ(Y+1)'`
- **AND** cycle 2: unstash `3` and `« + »`, EVAL produces `'SQ(Y+1)+3'`
- **AND** the stash SHALL be empty
- **AND** the stack SHALL contain `'SQ(Y+1)+3'`

#### Scenario: Empty stash is a no-op
- **WHEN** the stash is empty and ASSEMBLE is executed
- **THEN** the stack SHALL remain unchanged

### Requirement: Symbolic Unary Function Handling
Existing unary function commands SHALL produce symbolic results when given a symbolic input (Name or Symbol), instead of raising a type error.

The symbolic result SHALL be a Symbol of the form `FUNC(expr)` where FUNC is the uppercase command name and expr is the expression string of the input.

Commands that SHALL support symbolic inputs: SQ, SQRT, SIN, COS, TAN, ASIN, ACOS, ATAN, EXP, LN, ABS, NEG, INV.

#### Scenario: SQ with symbolic input
- **WHEN** the stack contains Name `X` and SQ is executed
- **THEN** the stack SHALL contain Symbol `'SQ(X)'`

#### Scenario: SIN with compound symbolic input
- **WHEN** the stack contains Symbol `'X+1'` and SIN is executed
- **THEN** the stack SHALL contain Symbol `'SIN(X+1)'`

#### Scenario: Numeric inputs still produce numeric results
- **WHEN** the stack contains Integer `5` and SQ is executed
- **THEN** the stack SHALL contain Integer `25` (unchanged behavior)
