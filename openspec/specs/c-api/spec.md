# c-api Specification

## Purpose
TBD - created by archiving change add-control-logic-math-strings. Update Purpose after archive.
## Requirements
### Requirement: lpr_get_setting Function
`lpr_get_setting(ctx, key)` SHALL read the value for the given key from the SQLite `meta` table and return it as a caller-freed string (`char*`). If the key does not exist, it SHALL return NULL. The caller MUST call `lpr_free()` on non-NULL return values.

#### Scenario: Read angle mode
- **WHEN** `lpr_get_setting(ctx, "angle_mode")` is called after `lpr_exec(ctx, "DEG")`
- **THEN** it returns a string `"DEG"`

#### Scenario: Read default angle mode
- **WHEN** `lpr_get_setting(ctx, "angle_mode")` is called on a fresh context
- **THEN** it returns a string `"RAD"`

#### Scenario: Read nonexistent key
- **WHEN** `lpr_get_setting(ctx, "nonexistent")` is called
- **THEN** it returns NULL

#### Scenario: Read current directory
- **WHEN** `lpr_get_setting(ctx, "cwd")` is called on a fresh context
- **THEN** it returns a string `"1"` (the HOME directory ID)

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

