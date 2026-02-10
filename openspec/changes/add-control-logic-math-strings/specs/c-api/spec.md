## ADDED Requirements

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
