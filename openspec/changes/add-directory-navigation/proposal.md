# Change: Add directory navigation commands

## Why
The filesystem has CRDIR to create directories but no way to enter, leave, or
delete them.  PATH is a stub returning "HOME" regardless of location.  VARS
returns a flat string with no distinction between variables and subdirectories,
giving frontends no way to render the softmenu-style directory view the HP 50g
provides.  This makes the directory tree unusable beyond the root.

## What Changes
- Fix **PATH** to return the real directory path (e.g. `"HOME/MYDIR/SUB"`)
- Add **CD** — change into a named subdirectory
- Add **UPDIR** — move up one level toward HOME
- Add **PGDIR** — recursively purge a named directory and all its contents
- Rework **VARS** — return a List of Names including both variables and
  subdirectories; subdirectory names are suffixed with `/` so frontends can
  distinguish them visually (e.g. `{ a b MYDIR/ }`)

**Depends on:** List type (task 15) for the VARS rework.  The navigation
commands (CD, UPDIR, PGDIR, PATH) have no List dependency and can land first.

## Impact
- Affected specs: `filesystem-commands`
- Affected code: `src/core/commands.cpp` (command registration),
  `src/core/store.cpp` / `src/core/store.hpp` (new Store helpers for parent
  lookup, path building, recursive directory deletion, and subdirectory listing),
  `tests/test_filesystem.cpp` (new and updated test cases)
