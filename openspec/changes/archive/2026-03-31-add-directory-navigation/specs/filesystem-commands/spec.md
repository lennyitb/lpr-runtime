## MODIFIED Requirements

### Requirement: VARS Command
The `VARS` command SHALL push a List of Name objects representing all contents of the current directory.  Variable names SHALL appear as plain Names.  Subdirectory names SHALL be suffixed with `/` (e.g. Name("MYDIR/")) so that consumers can distinguish directories from variables.  The list SHALL contain variables first, then subdirectories.

#### Scenario: List variables and subdirectories
- **WHEN** variables `a` and `b` and subdirectory `MYDIR` exist in the current directory and `VARS` is executed
- **THEN** a List `{ a b MYDIR/ }` is on the stack, where `a` and `b` are plain Names and `MYDIR/` is a Name with trailing slash

#### Scenario: Empty directory
- **WHEN** the current directory contains no variables or subdirectories and `VARS` is executed
- **THEN** an empty List `{ }` is on the stack

### Requirement: PATH Command
The `PATH` command SHALL push the full directory path from HOME to the current directory as a String, with directory names separated by `/`.

#### Scenario: Path at home
- **WHEN** the current directory is HOME and `PATH` is executed
- **THEN** String("HOME") is on the stack

#### Scenario: Path in subdirectory
- **WHEN** the current directory is `MYDIR` under HOME and `PATH` is executed
- **THEN** String("HOME/MYDIR") is on the stack

#### Scenario: Path in nested subdirectory
- **WHEN** the current directory is `SUB` under `MYDIR` under HOME and `PATH` is executed
- **THEN** String("HOME/MYDIR/SUB") is on the stack

## ADDED Requirements

### Requirement: CD Command
The `CD` command SHALL pop level 1 (a Name) and change the current directory to the subdirectory with that name under the current directory.  If no such subdirectory exists, the stack SHALL be restored and an error produced.

#### Scenario: Change into existing subdirectory
- **WHEN** subdirectory `MYDIR` exists under the current directory, Name("MYDIR") is on the stack, and `CD` is executed
- **THEN** the current directory is `MYDIR` and the stack is empty

#### Scenario: Change into nonexistent subdirectory
- **WHEN** no subdirectory `NODIR` exists under the current directory, Name("NODIR") is on the stack, and `CD` is executed
- **THEN** an error is produced and Name("NODIR") remains on the stack

### Requirement: UPDIR Command
The `UPDIR` command SHALL change the current directory to its parent directory.  If the current directory is already HOME, UPDIR SHALL be a no-op.

#### Scenario: Move up from subdirectory
- **WHEN** the current directory is `MYDIR` under HOME and `UPDIR` is executed
- **THEN** the current directory is HOME

#### Scenario: UPDIR at HOME is no-op
- **WHEN** the current directory is HOME and `UPDIR` is executed
- **THEN** the current directory remains HOME and no error is produced

### Requirement: PGDIR Command
The `PGDIR` command SHALL pop level 1 (a Name) and recursively delete the named subdirectory of the current directory, including all variables and nested subdirectories within it.  If no such subdirectory exists, the stack SHALL be restored and an error produced.

#### Scenario: Purge a subdirectory
- **WHEN** subdirectory `MYDIR` exists under the current directory with variables and sub-subdirectories, Name("MYDIR") is on the stack, and `PGDIR` is executed
- **THEN** `MYDIR` and all its contents are deleted and the stack is empty

#### Scenario: Purge nonexistent subdirectory
- **WHEN** no subdirectory `NODIR` exists under the current directory, Name("NODIR") is on the stack, and `PGDIR` is executed
- **THEN** an error is produced and Name("NODIR") remains on the stack
