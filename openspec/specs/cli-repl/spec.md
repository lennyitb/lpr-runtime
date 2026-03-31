# cli-repl Specification

## Purpose
TBD - created by archiving change add-bootstrap-runtime. Update Purpose after archive.
## Requirements
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

### Requirement: Execute Expression Flag (-e)
The CLI SHALL accept one or more `-e <expression>` flags. When provided, the CLI SHALL execute each expression sequentially, print the resulting stack, and exit without entering the interactive REPL.

#### Scenario: Single expression
- **WHEN** `lpr-cli -e "3 4 +"` is run
- **THEN** the output is `1: 7` and the exit code is 0

#### Scenario: Multiple expressions
- **WHEN** `lpr-cli -e "3 4" -e "+"` is run
- **THEN** the output is `1: 7` and the exit code is 0

#### Scenario: Expression with database
- **WHEN** `lpr-cli mydb.lpr -e "3 4 +"` is run
- **THEN** the expression is executed against mydb.lpr

#### Scenario: Expression error
- **WHEN** `lpr-cli -e "+"` is run with an empty stack
- **THEN** the error is printed to stderr and the exit code is non-zero

### Requirement: Help Flag (-h)
The CLI SHALL accept a `-h` flag that prints usage information and exits.

#### Scenario: Help flag
- **WHEN** `lpr-cli -h` is run
- **THEN** usage information is printed and the exit code is 0

### Requirement: Command History Navigation
The interactive REPL SHALL support navigating command history with the up and down arrow keys. Up SHALL recall the previous input, down SHALL move forward through history. The history SHALL be populated from the SQLite `input_history` table on startup.

#### Scenario: Recall previous command
- **WHEN** the user types `3 4 +` and presses Enter, then presses the up arrow
- **THEN** the input line shows `3 4 +`

#### Scenario: Navigate forward
- **WHEN** the user presses up twice then down once
- **THEN** the input line shows the second-most-recent command

### Requirement: Line Editing
The interactive REPL SHALL support in-line editing: left/right arrow cursor movement, backspace, delete, Home/End, and Ctrl-R reverse search. The `-e` batch mode SHALL NOT use line editing (no terminal interaction).

#### Scenario: Edit within line
- **WHEN** the user types `3 5 +`, moves the cursor left to `5`, and changes it to `4`
- **THEN** the input submitted is `3 4 +`

#### Scenario: Batch mode unaffected
- **WHEN** `lpr-cli -e "3 4 +"` is run
- **THEN** no terminal line editing is attempted and the command executes normally

