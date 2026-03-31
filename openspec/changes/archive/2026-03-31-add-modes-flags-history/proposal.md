# Change: Add display modes, system flags, and command history

## Why
Roadmap item 19. The runtime renders all numbers at full precision with no
formatting control — there's no FIX, SCI, ENG, or STD. Complex numbers always
display in rectangular form regardless of context. The HP 50g's 128 system
flags (used to control dozens of behaviors) have no equivalent. Meanwhile the
CLI has bare `std::getline` input with no command history or line editing,
making interactive sessions painful. All of these are meta-level settings
stored in (or naturally adjacent to) the SQLite `meta` table. Designing them
together lets us extend the settings infrastructure once and keep the API
minimal.

## What Changes

### Runtime — new commands (~22)
- **Number format:** STD, FIX, SCI, ENG (pop precision arg where applicable,
  store mode + digits in `meta`)
- **Coordinate modes:** RECT, POLAR, SPHERICAL (store in `meta`, affect
  Complex repr)
- **Flag registry:** SF, CF, FS?, FC? (boolean flags by name), SFLAG, RFLAG
  (typed flag access), STOF, RCLF (bulk save/restore). Backed by a new
  `flags` table — arbitrary named flags with type tags, not a fixed bitfield
- **Conversions:** ->Q (Real to nearest Rational), HMS->, ->HMS

### Runtime — formatted display
- Extend `repr()` to accept a `DisplaySettings` struct so numeric output
  respects the active format/coordinate mode
- `lpr_repr` reads settings from `meta` and passes them through — zero C API
  signature changes

### Runtime — command history (core, all platforms)
- Add `input_history` SQLite table — every successful `lpr_exec` records the
  input string automatically
- Two new C API functions: `lpr_history_count` and `lpr_history_entry` for
  index-based access (frontends need this for scrollback UIs, recent
  calculations, arrow-key recall)
- History lives in the same `.lpr` database — persists, backs up, and
  restores with all other runtime state

### CLI — line editing
- Integrate **linenoise** (BSD, ~1200 LOC, zero deps) for arrow-key
  navigation, in-line editing, and Ctrl-R search
- On startup, populate linenoise from `input_history` table — no separate
  history file needed

## Impact
- Affected specs: `cli-repl` (line editing), `c-api` (history functions),
  new capability `display-settings`
- Affected code:
  - `src/core/commands.cpp` — register ~20 new commands
  - `src/core/object.hpp` / `object.cpp` — `repr()` gains `DisplaySettings` overload
  - `src/core/store.cpp` / `store.hpp` — `input_history` table, history helpers
  - `src/lpr_api.cpp` — `lpr_history_count`, `lpr_history_entry`
  - `include/lpr/lpr.h` — declare new history API functions
  - `cli/main.cpp` — replace `std::getline` with linenoise, seed from SQLite
  - `CMakeLists.txt` — fetch linenoise
  - `tests/` — new test file(s) for display/flag/history
