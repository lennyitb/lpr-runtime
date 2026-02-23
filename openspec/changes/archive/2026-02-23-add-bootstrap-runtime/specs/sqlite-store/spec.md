## ADDED Requirements

### Requirement: SQLite Database Lifecycle
The Store SHALL open or create a SQLite database (file path or `:memory:`) and run schema migrations to establish the required tables: `objects`, `stack`, `directories`, `variables`, `history`, and `meta`.

#### Scenario: In-memory database creation
- **WHEN** a Store is opened with a NULL path
- **THEN** an in-memory SQLite database is created with all required tables and a root HOME directory

#### Scenario: File-based database creation
- **WHEN** a Store is opened with a file path
- **THEN** the database file is created (or opened if existing) with all required tables

### Requirement: Stack Operations
The Store SHALL provide `push(Object)`, `pop() → Object`, `peek(level) → Object`, `depth() → int`, and `clear_stack()` operations backed by the SQLite `stack` table.

#### Scenario: Push and pop
- **WHEN** an Object is pushed and then popped
- **THEN** the popped Object is equal to the pushed Object and the stack depth returns to the original value

#### Scenario: Peek at arbitrary level
- **WHEN** multiple Objects are pushed and `peek(2)` is called
- **THEN** the Object at stack level 2 is returned without modifying the stack

#### Scenario: Stack depth
- **WHEN** three Objects are pushed onto an empty stack
- **THEN** `depth()` returns 3

#### Scenario: Clear stack
- **WHEN** `clear_stack()` is called
- **THEN** `depth()` returns 0 and `pop()` fails

### Requirement: Stack History Snapshots
The Store SHALL provide `snapshot_stack() → int` to save the current stack state and `restore_stack(seq)` to restore a previous snapshot, enabling undo/redo.

#### Scenario: Snapshot and restore
- **WHEN** a snapshot is taken, the stack is modified, and the snapshot is restored
- **THEN** the stack matches its state at the time of the snapshot

### Requirement: Variable Storage
The Store SHALL provide `store_variable(dir_id, name, Object)`, `recall_variable(dir_id, name) → Object`, and `purge_variable(dir_id, name)` for persistent named variables within directories.

#### Scenario: Store and recall variable
- **WHEN** a variable `x` is stored with value Integer(42) and then recalled
- **THEN** the recalled Object is Integer(42)

#### Scenario: Purge variable
- **WHEN** a variable `x` is purged
- **THEN** recalling `x` fails or returns an error

### Requirement: Directory Operations
The Store SHALL manage a directory tree rooted at HOME, supporting creation of subdirectories and enumeration of variables within a directory.

#### Scenario: Create subdirectory
- **WHEN** a subdirectory `MYDIR` is created under HOME
- **THEN** variables can be stored and recalled within `MYDIR`

#### Scenario: List variables
- **WHEN** variables are stored in a directory and the variable list is requested
- **THEN** all variable names in that directory are returned

### Requirement: Transaction Support
All public Store methods SHALL operate within the caller's transaction, allowing the Context to group operations atomically.

#### Scenario: Rollback on error
- **WHEN** an error occurs during a sequence of store operations within a transaction
- **THEN** all operations in the transaction are rolled back
