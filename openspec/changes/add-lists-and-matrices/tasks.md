## 1. Object Type System
- [x] 1.1 Add `List` struct (`std::vector<Object>`) to object.hpp
- [x] 1.2 Add `Matrix` struct (`std::vector<std::vector<Object>>`) to object.hpp
- [x] 1.3 Extend `TypeTag` enum: `List = 9`, `Matrix = 10`
- [x] 1.4 Add List and Matrix to the `Object` variant
- [x] 1.5 Implement `repr()` for List (`{ 1 2 3 }`) and Matrix (`[[ 1 2 ][ 3 4 ]]`)
- [x] 1.6 Implement `serialize()` / `deserialize()` for List and Matrix
- [x] 1.7 Add `type_tag()` cases for List and Matrix
- [x] 1.8 Write test cases for List/Matrix construction, repr, and serialization roundtrip

## 2. Parser
- [x] 2.1 Add `{` / `}` list literal parsing with nesting support
- [x] 2.2 Add `[[` / `][` / `]]` matrix literal parsing
- [x] 2.3 Validate matrix rows have uniform length at parse time
- [x] 2.4 Validate matrix elements are numeric or symbolic at parse time (reject String, Program, etc.)
- [x] 2.5 Write parser test cases for list and matrix literals

## 3. List Commands — Core Access
- [x] 3.1 LIST-> (explode list onto stack with count)
- [x] 3.2 ->LIST (collect N items from stack into list)
- [x] 3.3 GET (1-based element access)
- [x] 3.4 PUT (1-based element replacement)
- [x] 3.5 GETI (GET with auto-incremented index)
- [x] 3.6 PUTI (PUT with auto-incremented index)
- [x] 3.7 HEAD (first element)
- [x] 3.8 TAIL (all but first element)
- [x] 3.9 SIZE (element count)
- [x] 3.10 POS (find element, return 1-based index or 0)
- [x] 3.11 SUB (sub-list by 1-based start/end indices)
- [x] 3.12 REVLIST (reverse list)
- [x] 3.13 SORT (sort list of homogeneous numeric/string elements)
- [x] 3.14 Write test cases for core list commands

## 4. List Commands — Higher-Order & Set Operations
- [x] 4.1 DOLIST (apply program to corresponding elements of N lists)
- [x] 4.2 MAP (apply program to each element, collect results into list)
- [x] 4.3 STREAM (reduce list with binary program)
- [x] 4.4 SEQ (generate list from start, step, count using program)
- [x] 4.5 FILTER (keep elements where program returns truthy)
- [x] 4.6 DOSUBS (apply program to sliding windows of N elements)
- [x] 4.7 ZIP (transpose N lists into list of lists)
- [x] 4.8 ADD (append element to list)
- [x] 4.9 UNION (set union of two lists)
- [x] 4.10 INTERSECT (set intersection of two lists)
- [x] 4.11 DIFFERENCE (set difference of two lists)
- [x] 4.12 Write test cases for higher-order and set operations

## 5. Matrix/Vector Commands
- [x] 5.1 ->V2 / ->V3 (construct 2D/3D vectors from numeric or symbolic values)
- [x] 5.2 V-> (explode vector onto stack)
- [x] 5.3 CON (constant matrix/vector of given dimensions)
- [x] 5.4 IDN (identity matrix of given size)
- [x] 5.5 RDM (redimension matrix)
- [x] 5.6 TRN (transpose — works for numeric and symbolic)
- [x] 5.7 DET (determinant — numeric result or symbolic expression)
- [x] 5.8 INV (matrix inverse — numeric only, error on symbolic)
- [x] 5.9 CROSS (cross product — numeric result or symbolic expression)
- [x] 5.10 DOT (dot product — numeric result or symbolic expression)
- [x] 5.11 ABS on vectors (Euclidean norm — numeric only, error on symbolic)
- [x] 5.12 GET / PUT for matrix elements (row, col indexing via list `{ r c }`, PUT accepts numeric or symbolic)
- [x] 5.13 SIZE for matrices (returns `{ rows cols }` list)
- [x] 5.14 Write test cases for numeric matrix/vector commands
- [x] 5.15 Write test cases for symbolic matrix operations (DET, CROSS, DOT, TRN, arithmetic)

## 6. Arithmetic Overloads
- [x] 6.1 List + List (element-wise, matching lengths)
- [x] 6.2 Scalar + List / List + Scalar (broadcast)
- [x] 6.3 List - List, Scalar - List, List * List, List / List (element-wise)
- [x] 6.4 Matrix + Matrix (element-wise, matching dimensions)
- [x] 6.5 Matrix * Matrix (matrix multiplication)
- [x] 6.6 Scalar * Matrix / Matrix * Scalar (scalar multiplication)
- [x] 6.7 Matrix * Vector (matrix-vector product)
- [x] 6.8 NEG on lists and matrices (negate all elements)
- [x] 6.9 Write test cases for arithmetic overloads

## 7. Integration
- [x] 7.1 Ensure STO/RCL work with List and Matrix objects
- [x] 7.2 Ensure undo/redo works with List and Matrix on stack
- [x] 7.3 Ensure DUP, DROP, SWAP etc. work with List and Matrix
- [x] 7.4 TYPE command returns correct type numbers for List and Matrix
- [x] 7.5 Update CMD_SET_REFERENCE.md with all new commands
- [x] 7.6 Update ARCHITECTURE.md with List and Matrix types
