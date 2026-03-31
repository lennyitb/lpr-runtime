## ADDED Requirements

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
