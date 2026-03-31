# Change: Add List and Matrix Types

## Why
Lists are the primary compound data type in RPL — prerequisite for matrices, higher-order operations, and most real-world programs. Vectors and matrices unlock the HP 50g's linear-algebra capabilities. Together these cover roadmap milestones 15 and 16.

## What Changes
- Add `List` variant member (`std::vector<Object>`) to the Object type system
- Add `Matrix` variant member (row-major `std::vector<std::vector<Object>>`) to the Object type system, accepting numeric and symbolic entries
- Parser support for `{ 1 2 3 }` list literals (nesting allowed)
- Parser support for `[[ 1 2 ][ 3 4 ]]` matrix literals
- ~24 list commands: core access, higher-order (MAP, STREAM, DOLIST, SEQ, FILTER, DOSUBS, ZIP), and set operations
- ~15 matrix/vector commands: construction, linear algebra, arithmetic overloads (symbolic matrices supported where algebraically meaningful)
- Arithmetic (`+`, `-`, `*`, `/`) overloaded for element-wise list ops and matrix arithmetic
- Serialization/deserialization for both new types

## Impact
- Affected specs: `object-types`, `parser`, `arithmetic-commands`
- New specs: `list-commands`, `matrix-commands`
- Affected code: `object.hpp/.cpp`, `parser.cpp`, `commands.cpp`, `stack.cpp` (serialization), `store.cpp`
- Type tag enum extended: `List = 9`, `Matrix = 10`
- Variant grows from 9 to 11 alternatives
