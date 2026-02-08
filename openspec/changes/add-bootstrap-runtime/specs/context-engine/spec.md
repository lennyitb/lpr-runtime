## ADDED Requirements

### Requirement: C API Lifecycle
The runtime SHALL expose `lpr_open(db_path)` to create a context (NULL for in-memory) and `lpr_close(ctx)` to destroy it. All functions SHALL use C linkage (`extern "C"`) with an opaque `lpr_ctx` pointer.

#### Scenario: Open and close in-memory context
- **WHEN** `lpr_open(NULL)` is called
- **THEN** a valid `lpr_ctx*` is returned with an initialized in-memory database

#### Scenario: Open and close file-based context
- **WHEN** `lpr_open("test.lpr")` is called
- **THEN** a valid `lpr_ctx*` is returned with a persistent database file

### Requirement: Transactional Execution
The `lpr_exec(ctx, input)` function SHALL execute RPL input within a SQLite transaction. Before mutating state, it SHALL snapshot the stack. On success it commits; on error it rolls back, pushes an Error object, and commits.

#### Scenario: Successful execution
- **WHEN** `lpr_exec(ctx, "3 4 +")` is called
- **THEN** `lpr_result.ok` is 1 and Integer(7) is on the stack

#### Scenario: Error during execution
- **WHEN** `lpr_exec(ctx, "+")` is called on an empty stack
- **THEN** `lpr_result.ok` is 0 and an Error object is on the stack

### Requirement: Stack Inspection
The runtime SHALL expose `lpr_depth(ctx)` returning the current stack depth and `lpr_repr(ctx, level)` returning the display representation of the object at the given level. The caller must free the repr string with `lpr_free()`.

#### Scenario: Inspect stack after push
- **WHEN** `lpr_exec(ctx, "42")` is called
- **THEN** `lpr_depth(ctx)` returns 1 and `lpr_repr(ctx, 1)` returns `"42"`

### Requirement: Undo/Redo
The runtime SHALL expose `lpr_undo(ctx)` and `lpr_redo(ctx)` which restore the stack to previous/next snapshots in the history. Returns 1 on success, 0 if no undo/redo is available.

#### Scenario: Undo restores previous state
- **WHEN** `lpr_exec(ctx, "42")` then `lpr_exec(ctx, "DROP")` then `lpr_undo(ctx)`
- **THEN** Integer(42) is back on the stack

#### Scenario: Redo re-applies undone operation
- **WHEN** an undo is performed and then `lpr_redo(ctx)` is called
- **THEN** the state matches the post-operation state before undo

#### Scenario: Undo at beginning of history
- **WHEN** `lpr_undo(ctx)` is called with no history
- **THEN** it returns 0 and the stack is unchanged

### Requirement: Memory Management
The runtime SHALL expose `lpr_free(ptr)` to deallocate memory returned by `lpr_repr` and any future API functions that return allocated strings.

#### Scenario: Free repr string
- **WHEN** `lpr_repr(ctx, 1)` returns a string and `lpr_free()` is called on it
- **THEN** the memory is deallocated without error
