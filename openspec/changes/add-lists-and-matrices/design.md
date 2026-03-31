## Context

Lists and matrices are the two compound data types in RPL. On the HP 50g, lists are heterogeneous ordered collections; matrices can contain both numeric and symbolic entries. Adding both in a single change avoids churning the variant and serialization code twice. Supporting symbolic entries from the start ensures the type system is correct before CAS integration (milestone 17) — avoiding a painful migration later.

## Goals / Non-Goals

- Goals:
  - Faithful HP 50g list semantics (heterogeneous, 1-based indexing)
  - Matrix type with row-major storage supporting numeric and symbolic entries
  - Standard linear algebra commands (numeric matrices)
  - Symbolic matrix arithmetic via existing expression-building codepath
  - Arithmetic overloads that match HP 50g behavior (element-wise for lists, matrix math for matrices)
  - Full serialization roundtrip through SQLite
  - Comprehensive test coverage

- Non-Goals:
  - Sparse matrix support
  - LAPACK/BLAS integration (pure Boost.Multiprecision arithmetic)
  - Graphics/plotting of matrix data
  - Symbolic simplification of matrix results (deferred to CAS milestone 17)

## Decisions

### List representation: `std::vector<Object>`
Lists are heterogeneous — any Object can be an element, including nested lists. This maps directly to `std::vector<Object>`. The variant becomes self-referential (List contains Objects which can be Lists), which works fine with `std::variant` since `std::vector` stores elements on the heap.

### Matrix representation: `std::vector<std::vector<Object>>`
Row-major vector-of-vectors. Simple, correct, and sufficient for the sizes RPL calculators handle. Elements may be numeric (Integer, Real, Rational, Complex) or symbolic (Symbol). String, Program, Name, Error, List, and Matrix elements are rejected. A 1-row matrix acts as a vector; commands like DOT and CROSS operate on vectors specifically.

### 1-based indexing
HP 50g uses 1-based indexing for GET, PUT, SUB, POS. We follow this convention. Index 0 or out-of-range pushes an error.

### Arithmetic overload strategy
- **List + List**: element-wise, matching lengths required (HP 50g behavior)
- **Scalar + List / List + Scalar**: broadcast scalar to each element
- **Matrix + Matrix**: element-wise, matching dimensions required
- **Matrix * Matrix**: true matrix multiplication (not element-wise)
- **Scalar * Matrix / Matrix * Scalar**: scalar multiplication
- **Matrix * Vector**: matrix-vector product (vector treated as column)

When matrix elements are symbolic, arithmetic dispatches to the existing symbolic expression builder (e.g., `'a' 'b' +` → `'a+b'`). Results are unsimplified expression strings — CAS integration (milestone 17) will add SIMPLIFY/EXPAND/FACTOR to make these useful. The point is that the model is correct now.

### Symbolic matrix constraints
DET, INV, CROSS, DOT all work on symbolic matrices by building expression strings. The results are algebraically correct but unsimplified. Commands that require numeric comparison (SORT on matrix elements, numeric thresholds for singularity) will error on symbolic entries — this is correct behavior pending CAS.

### Parser approach
- `{` opens list collection, `}` closes. Nesting supported (lists within lists).
- `[[` opens matrix, `][` separates rows, `]]` closes. Elements are whitespace-delimited within rows.
- Both integrate into the existing single-pass tokenizer.

### Higher-order list commands
DOLIST, MAP apply programs to list elements. These consume a program from the stack and apply it — consistent with existing EVAL/IFT patterns in the execution engine.

## Risks / Trade-offs

- **Variant size growth**: 9 → 11 alternatives. Minimal runtime cost; `std::visit` dispatch tables grow but this is not a hot path concern at calculator scale.
- **Self-referential variant**: `List` containing `Object` containing `List` works because `std::vector` heap-allocates. No circular reference issue since there are no shared pointers.
- **Matrix element enforcement**: Elements must be numeric or symbolic. Strings, Programs, Names, Errors, Lists, and Matrices are rejected at parse time and in PUT/PUTI.
- **Unsimplified symbolic results**: DET of a 3x3 symbolic matrix produces a large, correct but unreadable expression string. This is by design — CAS will clean it up later. Users building symbolic matrices before CAS lands should expect verbose output.

## Open Questions
- Should EIGENVALUES/EIGENVECTORS be included now or deferred? Roadmap says "stretch" — proposing to defer to a follow-up change.
