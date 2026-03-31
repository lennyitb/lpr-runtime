## Context

The runtime's `meta` table already stores `angle_mode`, `current_dir`, and
`undo_seq`. This change adds ~5 more keys for display modes/flags, teaches
`repr()` to read them, and adds SQLite-backed command history as a core
runtime feature available to every frontend. The risk is API bloat — adding
too many C API functions, new tables, or per-setting commands that duplicate
the meta infrastructure. The design choices below all optimize for minimalism.

## Goals / Non-Goals

- Goals:
  - Number formatting control (FIX/SCI/ENG/STD) affecting all numeric display
  - Extensible typed flag/setting registry — no fixed flag count
  - SQLite-backed command history as a core runtime feature, accessible from
    every frontend (CLI, iOS, WASM)
  - Minimal C API additions (only what's needed for history access)
- Non-Goals:
  - LASTARG (last-argument recall) — separate concern, depends on per-command
    argument tracking not just input strings
  - Custom display hooks for host frontends — they can read `meta` via
    `lpr_get_setting` and format themselves
  - HP 50g flag numbering or bitfield layout — we diverge intentionally

## Decisions

### 1. DisplaySettings struct passed to repr()

**Decision:** Add a `DisplaySettings` struct with format mode, digits, and
coordinate mode. Provide a `repr(obj, settings)` overload; the existing
`repr(obj)` calls it with STD defaults for backward compatibility.

**Why:** `repr()` is currently a free function with no access to Context or
Store. Threading Context through every repr call would be invasive. A small
POD struct is cheap to construct and keeps repr a pure function.

**Alternatives considered:**
- Global/thread-local settings — breaks when multiple contexts exist
- Making repr a Context method — couples display to execution engine

### 2. Meta keys for display state

**Decision:** Three new meta keys for display modes:
| Key               | Values                        | Default |
|-------------------|-------------------------------|---------|
| `number_format`   | `STD`, `FIX`, `SCI`, `ENG`   | `STD`   |
| `format_digits`   | `0`–`11` (string)            | `0`     |
| `coordinate_mode` | `RECT`, `POLAR`, `SPHERICAL` | `RECT`  |

**Why:** Flat key-value strings in the existing table. No schema migration.
`lpr_get_setting` already exposes them to hosts.

### 3. Typed flags table (registry-style)

**Decision:** Replace the HP 50g's fixed 256-bit flag space with an
unbounded, typed registry. A new `flags` table stores each flag as its own
row with a name, type tag, and value:

```sql
CREATE TABLE flags (
    name     TEXT PRIMARY KEY,
    type_tag INTEGER NOT NULL,  -- 0=bool, 1=integer, 2=real, 3=string
    value    TEXT NOT NULL
);
```

Examples:
| name             | type_tag | value   |
|------------------|----------|---------|
| `exact_mode`     | 0        | `1`     |
| `verbose_errors` | 0        | `0`     |
| `max_iterations` | 1        | `1000`  |
| `tolerance`      | 2        | `1E-10` |
| `user.greeting`  | 3        | `hello` |

**Why:** The HP 50g's 128 system + 128 user flags are a hardware-era
constraint. A typed registry is more expressive (flags can hold integers,
reals, strings — not just booleans), infinitely extensible (no renumbering
when you run out), and self-documenting (named keys vs opaque flag -105).
It's the same pattern as Windows Registry, macOS defaults, or dconf — proven
at scale.

The `meta` table is structurally similar but serves a different purpose:
`meta` holds internal runtime bookkeeping (`current_dir`, `undo_seq`) that
users never touch directly. `flags` holds user-visible and program-accessible
settings. Keeping them separate avoids programs accidentally clobbering
internal state.

**RPL command mapping:**
- **SF / CF** — set/clear boolean flags by name: `'exact_mode' SF`
- **FS? / FC?** — test boolean flags: `'exact_mode' FS?` → 1 or 0
- **SFLAG** (new) — store a typed flag: `1000 'max_iterations' SFLAG`
- **RFLAG** (new) — recall a typed flag: `'max_iterations' RFLAG` → 1000
- **STOF / RCLF** — bulk save/restore as a List of `{ name value }` pairs

**Alternatives considered:**
- Hex-encoded bitfield in `meta` — fixed size (256), boolean-only, opaque
  numbering, stone age
- Individual `meta` keys with a naming convention — works but mixes user
  flags with internal state (`current_dir`, `undo_seq`); programs could
  corrupt runtime internals
- JSON blob — heavier parsing, no SQLite indexing on individual flags

### 4. SQLite-backed command history

**Decision:** Add an `input_history` table to the SQLite database. Every
successful `lpr_exec` call automatically records the input string. Two new
C API functions provide access:

```c
int   lpr_history_count(lpr_ctx* ctx);            // total entries
char* lpr_history_entry(lpr_ctx* ctx, int index);  // 0 = most recent; caller frees
```

**Why:** Command history is a core feature, not a CLI convenience. An iOS
app needs "recent calculations" just like the CLI needs arrow-key recall.
Storing history in the same SQLite database means it persists with the rest
of the runtime state, participates in the same backup/restore lifecycle, and
is available to every frontend through a uniform API. The HP 50g itself
stores command history in the calculator's memory, not in the keypad driver.

**Schema:**
```sql
CREATE TABLE input_history (
    seq   INTEGER PRIMARY KEY AUTOINCREMENT,
    input TEXT NOT NULL
);
```

Minimal — just a sequence number and the input string. No timestamps (the
sequence is the ordering). No deduplication (consecutive duplicates are
recorded; frontends can filter if they want).

**Alternatives considered:**
- File-based history (`~/.lpr_history`) — CLI-only, invisible to other
  frontends, doesn't persist with the database
- RPL command only (HIST pushes a List) — forces stack manipulation just to
  read history; awkward for frontends that want to populate a scrollback UI
- Combined file + SQLite — two sources of truth, sync headaches

### 5. CLI line editing via linenoise

**Decision:** Use linenoise in the CLI for arrow-key navigation, in-line
editing, and Ctrl-R search. On startup, populate linenoise's in-memory
history from the SQLite `input_history` table. After each input, the runtime
records it (via `lpr_exec`), so no separate CLI-side persistence is needed.

**Why:** linenoise is ~1200 lines of C, BSD-licensed, zero dependencies,
battle-tested (Redis, etc.). It handles terminal UX; the runtime handles
persistence. Clean separation.

**Alternatives considered:**
- replxx (C++) — larger, more features than needed
- readline (GPL) — license incompatible, system dependency
- No library (manual terminal handling) — reinventing the wheel

### 6. Minimal C API additions

**Decision:** Two new functions for history access (see Decision #4). Settings
are written via RPL commands (`FIX 4`, `SCI`, `RECT`) executed through
`lpr_exec` and read via the existing `lpr_get_setting`. No `lpr_set_setting`
needed.

**Why:** The command-centric API principle: everything is a command. A host
wanting `FIX 4` just calls `lpr_exec(ctx, "FIX 4")`. History is the
exception — it's a read-heavy, index-based access pattern that doesn't map
well to stack operations. Two functions (count + entry-by-index) is the
minimum a frontend needs to render a history list or populate a scrollback.

## Risks / Trade-offs

- **repr performance:** Constructing `DisplaySettings` on every `lpr_repr`
  call adds two meta reads. These are simple primary-key lookups in SQLite
  (~microseconds). Acceptable until benchmarks say otherwise.
- **History growth:** `input_history` grows unbounded. For now this is fine
  (text strings are small). A future enhancement could add a CLHIST command
  or a meta key for max history size with automatic pruning.
- **HP 50g flag compatibility:** Programs written for the HP 50g use numeric
  flags (e.g. `-3` for fraction mode). We intentionally break from this
  convention. A compatibility shim mapping legacy numbers to named flags is
  possible later but not in scope.
- **linenoise platform support:** linenoise works on POSIX terminals. Windows
  support requires linenoise-ng or a shim. Since the primary dev target is
  macOS and the CLI is a dev tool, this is acceptable. The SQLite history
  is platform-agnostic — only the CLI's line-editing UX is POSIX-specific.

## Open Questions

- Should FIX/SCI/ENG digits default to 0 (show all significant digits) or
  match the HP 50g default of no-argument meaning "current precision"?
- Which named system flags should be defined in the first pass? Likely
  candidates: `exact_mode`, `numeric_mode`, `verbose_errors`.
- Should ->Q use continued-fraction approximation with a configurable
  tolerance, or match the HP 50g's fixed algorithm?
