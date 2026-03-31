## ADDED Requirements

### Requirement: Command History Recording
Every successful `lpr_exec` call SHALL automatically record the input string in the `input_history` SQLite table. Failed executions (where `lpr_result.ok` is 0) SHALL NOT be recorded. The history table SHALL use an auto-incrementing sequence for ordering.

#### Scenario: Successful command recorded
- **WHEN** `lpr_exec(ctx, "3 4 +")` succeeds
- **THEN** `lpr_history_count(ctx)` returns at least 1 and `lpr_history_entry(ctx, 0)` returns `"3 4 +"`

#### Scenario: Failed command not recorded
- **WHEN** `lpr_exec(ctx, "+")` fails on an empty stack
- **THEN** `lpr_history_count(ctx)` is unchanged from before the call

### Requirement: lpr_history_count Function
`lpr_history_count(ctx)` SHALL return the total number of entries in the input history table. It SHALL return 0 on a fresh context.

#### Scenario: Fresh context
- **WHEN** `lpr_history_count(ctx)` is called on a newly opened context
- **THEN** it returns 0

#### Scenario: After commands
- **WHEN** three successful `lpr_exec` calls have been made
- **THEN** `lpr_history_count(ctx)` returns 3

### Requirement: lpr_history_entry Function
`lpr_history_entry(ctx, index)` SHALL return the input string at the given index, where 0 is the most recent entry. The caller MUST call `lpr_free()` on non-NULL return values. It SHALL return NULL if the index is out of range.

#### Scenario: Most recent entry
- **WHEN** `lpr_exec(ctx, "DUP")` is the last successful call
- **THEN** `lpr_history_entry(ctx, 0)` returns `"DUP"`

#### Scenario: Older entry
- **WHEN** `lpr_exec(ctx, "3")` then `lpr_exec(ctx, "4")` then `lpr_exec(ctx, "+")` have been called
- **THEN** `lpr_history_entry(ctx, 2)` returns `"3"`

#### Scenario: Out of range
- **WHEN** `lpr_history_entry(ctx, 999)` is called with fewer than 1000 history entries
- **THEN** it returns NULL
