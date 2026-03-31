## 1. DisplaySettings infrastructure
- [ ] 1.1 Define `DisplaySettings` struct in `object.hpp` (format mode, digits, coordinate mode)
- [ ] 1.2 Add `repr(const Object&, const DisplaySettings&)` overload in `object.cpp`
- [ ] 1.3 Update `lpr_repr` in `lpr_api.cpp` to read meta keys and construct `DisplaySettings`
- [ ] 1.4 Verify existing `repr(obj)` still works unchanged (backward compat)

## 2. Number format commands
- [ ] 2.1 Implement STD — set `number_format` meta to `STD`, clear `format_digits`
- [ ] 2.2 Implement FIX — pop Integer n (0–11), set `number_format` to `FIX`, `format_digits` to n
- [ ] 2.3 Implement SCI — pop Integer n (0–11), set `number_format` to `SCI`, `format_digits` to n
- [ ] 2.4 Implement ENG — pop Integer n (0–11), set `number_format` to `ENG`, `format_digits` to n
- [ ] 2.5 Implement formatted Real/Rational/Complex output for FIX, SCI, ENG modes in repr

## 3. Coordinate mode commands
- [ ] 3.1 Implement RECT — set `coordinate_mode` meta to `RECT`
- [ ] 3.2 Implement POLAR — set `coordinate_mode` meta to `POLAR`
- [ ] 3.3 Implement SPHERICAL — set `coordinate_mode` meta to `SPHERICAL`
- [ ] 3.4 Implement Complex repr in POLAR mode (r∠θ notation, angle-mode-aware)

## 4. Flag registry
- [ ] 4.1 Add `flags` table to SQLite schema in `store.cpp` (`name TEXT PRIMARY KEY, type_tag INTEGER NOT NULL, value TEXT NOT NULL`)
- [ ] 4.2 Add Store helpers: `set_flag(name, type_tag, value)`, `get_flag(name) → optional<tuple>`, `all_flags()`, `clear_all_flags()`
- [ ] 4.3 Implement SF — pop String name, upsert boolean true in `flags` table
- [ ] 4.4 Implement CF — pop String name, upsert boolean false in `flags` table
- [ ] 4.5 Implement FS? — pop String name, push 1 if exists and truthy, 0 otherwise
- [ ] 4.6 Implement FC? — pop String name, push 1 if absent or falsy, 0 otherwise
- [ ] 4.7 Implement SFLAG — pop String name + typed value, infer type_tag, store in `flags`
- [ ] 4.8 Implement RFLAG — pop String name, push stored value with original type; error if missing
- [ ] 4.9 Implement STOF — push List of `{ name value }` pairs for all flags
- [ ] 4.10 Implement RCLF — pop List, replace all flags with list contents

## 5. Conversion commands
- [ ] 5.1 Implement ->Q — pop Real, push nearest Rational approximation (continued fraction)
- [ ] 5.2 Implement HMS-> — pop Real in H.MMSSss format, push decimal hours
- [ ] 5.3 Implement ->HMS — pop decimal hours Real, push H.MMSSss format

## 6. Command history (runtime core)
- [ ] 6.1 Add `input_history` table to SQLite schema in `store.cpp` (`seq INTEGER PRIMARY KEY AUTOINCREMENT, input TEXT NOT NULL`)
- [ ] 6.2 Record input in `input_history` on every successful `lpr_exec` call (in `lpr_api.cpp`, after commit)
- [ ] 6.3 Add `Store::history_count()` and `Store::history_entry(int index)` helpers (0 = most recent)
- [ ] 6.4 Implement `lpr_history_count(ctx)` and `lpr_history_entry(ctx, n)` in C API
- [ ] 6.5 Declare new functions in `include/lpr/lpr.h`

## 7. CLI line editing
- [ ] 7.1 Add linenoise to CMakeLists.txt via FetchContent
- [ ] 7.2 Replace `std::getline` loop in `cli/main.cpp` with `linenoise()` calls
- [ ] 7.3 On startup, populate linenoise history from `input_history` table via `lpr_history_count`/`lpr_history_entry`
- [ ] 7.4 Ensure -e batch mode bypasses linenoise (no terminal interaction)

## 8. Tests
- [ ] 8.1 Test STD/FIX/SCI/ENG formatting output for Real, Rational, Integer, Complex
- [ ] 8.2 Test RECT/POLAR/SPHERICAL Complex display
- [ ] 8.3 Test SF/CF/FS?/FC? on named boolean flags; SFLAG/RFLAG on typed values; STOF/RCLF bulk round-trip
- [ ] 8.4 Test ->Q produces correct rational approximations (pi, e, known fractions)
- [ ] 8.5 Test HMS->/->HMS round-trip conversions
- [ ] 8.6 Test format persistence across lpr_exec calls (settings survive transactions)
- [ ] 8.7 Test `lpr_history_count` / `lpr_history_entry` — entries recorded, 0 = most recent, freed correctly
- [ ] 8.8 Test history records only successful executions (failed commands not recorded)

## 9. Documentation
- [ ] 9.1 Update CMD_SET_REFERENCE.md with all new commands
- [ ] 9.2 Update ARCHITECTURE.md with DisplaySettings pattern, meta key table, and input_history schema
