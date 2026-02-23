## ADDED Requirements

### Requirement: STO Command
The `STO` command SHALL pop level 1 (a Name) and level 2 (any Object), and store the Object as a variable with the given name in the current directory.

#### Scenario: Store a variable
- **WHEN** Integer(42) is at level 2, Name("x") is at level 1, and `STO` is executed
- **THEN** variable `x` is stored in the current directory with value Integer(42), and the stack is empty

### Requirement: RCL Command
The `RCL` command SHALL pop level 1 (a Name) and push the Object stored under that name in the current directory.

#### Scenario: Recall a variable
- **WHEN** variable `x` stores Integer(42), Name("x") is on the stack, and `RCL` is executed
- **THEN** Integer(42) is on the stack

#### Scenario: Recall nonexistent variable
- **WHEN** Name("unknown") is on the stack and `RCL` is executed
- **THEN** an Error is produced

### Requirement: PURGE Command
The `PURGE` command SHALL pop level 1 (a Name) and delete the variable with that name from the current directory.

#### Scenario: Purge a variable
- **WHEN** variable `x` exists, Name("x") is on the stack, and `PURGE` is executed
- **THEN** variable `x` no longer exists in the current directory

### Requirement: HOME Command
The `HOME` command SHALL set the current directory to the root HOME directory.

#### Scenario: Return to home
- **WHEN** the current directory is a subdirectory and `HOME` is executed
- **THEN** the current directory is HOME

### Requirement: PATH Command
The `PATH` command SHALL push the current directory path as a String.

#### Scenario: Path at home
- **WHEN** the current directory is HOME and `PATH` is executed
- **THEN** String("HOME") or equivalent path representation is on the stack

### Requirement: CRDIR Command
The `CRDIR` command SHALL pop level 1 (a Name) and create a subdirectory with that name in the current directory.

#### Scenario: Create subdirectory
- **WHEN** Name("MYDIR") is on the stack and `CRDIR` is executed
- **THEN** a subdirectory `MYDIR` is created under the current directory

### Requirement: VARS Command
The `VARS` command SHALL push a list of variable names in the current directory.

#### Scenario: List variables
- **WHEN** variables `a` and `b` exist in the current directory and `VARS` is executed
- **THEN** a representation of `{ a b }` is pushed onto the stack
