## ADDED Requirements

### Requirement: Interactive REPL
The `lpr-cli` executable SHALL provide an interactive read-eval-print loop that reads input lines, calls `lpr_exec`, and displays the stack state.

#### Scenario: Basic calculation
- **WHEN** the user types `3` then `4` then `+`
- **THEN** the stack displays Integer(7) at level 1

### Requirement: Optional Database Path
The CLI SHALL accept an optional command-line argument for the database file path. If omitted, it SHALL use an in-memory database.

#### Scenario: Open with file path
- **WHEN** `lpr-cli mydb.lpr` is run
- **THEN** the runtime uses `mydb.lpr` for persistent state

#### Scenario: Open without arguments
- **WHEN** `lpr-cli` is run with no arguments
- **THEN** the runtime uses an in-memory database

### Requirement: Stack Display
The CLI SHALL display the stack bottom-up after each command, with the highest level at the top and level 1 at the bottom, prefixed by level numbers.

#### Scenario: Multi-item stack display
- **WHEN** the stack contains Integer(355) at level 2 and Integer(113) at level 1
- **THEN** the display shows `2: 355` above `1: 113`

### Requirement: Quit Command
The CLI SHALL exit when the user types `q` or `quit`.

#### Scenario: Quit with q
- **WHEN** the user types `q`
- **THEN** the CLI exits cleanly

### Requirement: Error Display
The CLI SHALL display errors in a distinct format, distinguishable from normal stack output.

#### Scenario: Error display
- **WHEN** an error occurs during execution
- **THEN** the error message is displayed in a format distinct from normal stack values
