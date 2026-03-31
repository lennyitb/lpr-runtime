## 1. Store helpers
- [x] 1.1 Add `parent_dir_id(int dir_id)` to Store — returns parent_id or -1 for HOME
- [x] 1.2 Add `dir_name(int dir_id)` to Store — returns directory name string
- [x] 1.3 Add `dir_path(int dir_id)` to Store — walks parent chain, returns `"HOME/A/B"`
- [x] 1.4 Add `purge_directory_tree(int dir_id)` to Store — recursively deletes variables, subdirectories, and the directory itself
- [x] 1.5 Add `list_subdirectories(int dir_id)` to Store — returns child directory names (needed by purge and VARS)

## 2. Commands (no List dependency)
- [x] 2.1 Fix PATH to call `s.dir_path(s.current_dir())` instead of returning stub
- [x] 2.2 Implement CD — pop Name, `find_directory`, `set_current_dir`; error if not found
- [x] 2.3 Implement UPDIR — `parent_dir_id`, `set_current_dir`; no-op at HOME
- [x] 2.4 Implement PGDIR — pop Name, `find_directory`, `purge_directory_tree`; error if not found

## 3. VARS rework (depends on List type from task 15)
- [x] 3.1 Rework VARS to return a List of Names instead of a formatted String
- [x] 3.2 Include subdirectory names suffixed with `/` after variable names
- [x] 3.3 Return empty List when directory has no contents

## 4. Tests
- [x] 4.1 Test PATH returns real path after CD into subdirectory and nested subdirectory
- [x] 4.2 Test CD into existing subdirectory; test CD into nonexistent subdirectory errors
- [x] 4.3 Test UPDIR from subdirectory returns to parent; test UPDIR at HOME is no-op
- [x] 4.4 Test PGDIR removes directory and contents; test PGDIR on nonexistent directory errors
- [x] 4.5 Test VARS returns List with variable Names and directory Names (with `/` suffix)
- [x] 4.6 Test VARS on empty directory returns empty List

## 5. Documentation
- [x] 5.1 Update CMD_SET_REFERENCE.md with CD, UPDIR, PGDIR entries and updated PATH/VARS descriptions
