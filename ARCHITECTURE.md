# LPR Runtime Architecture

**LPR** (Lenny Polish Reverse) is a portable C++ runtime for an RPL virtual
machine inspired by the HP 50g. It exposes a minimal C-compatible API designed
around a single paradigm: *everything is a command*.

## Design Principles

1. **Command-centric API.** The host application interacts with the runtime
   almost exclusively through `lpr_exec(ctx, "...")`. Pushing a number, running
   an arithmetic operator, storing a variable, and evaluating a program are all
   the same call with different input strings. This mirrors how the real HP 50g
   works: you type, you press ENTER.

2. **SQLite is the backbone.** The stack, filesystem (directory tree of named
   objects), and undo history all live in a single SQLite database. This gives
   us ACID transactions, trivial serialization, and persistence for free.

3. **Every execution is a transaction.** Each `lpr_exec` call runs inside a
   SQLite transaction. Before mutating state, the runtime snapshots the stack.
   Undo/redo is just restoring a previous snapshot. This is simple and correct.

4. **RPL semantics, not binary compatibility.** We replicate the *behavior* of
   the HP 50g's RPL engine, not its binary object formats. Where the 50g's
   design is awkward or dated, we diverge.

5. **Numeric tower built on Boost.Multiprecision.** Arbitrary-precision
   integers, reals, and rationals via `cpp_int`, `cpp_dec_float`, and
   `cpp_rational`. No GMP dependency, pure C++ headers, trivial iOS
   cross-compilation.

6. **CAS via SymEngine (initially).** Giac is the aspirational target for full
   CAS parity, but SymEngine ships first due to simpler integration and BSD
   licensing. The CAS is accessed through an internal bridge interface so
   backends can be swapped.

---

## C API Surface

```c
typedef struct lpr_ctx lpr_ctx;

typedef struct {
    int ok;            // 1 = success, 0 = error (error object pushed to stack)
    int stack_depth;   // stack depth after execution
} lpr_result;

// Lifecycle
lpr_ctx*    lpr_open(const char* db_path);   // NULL for in-memory
void        lpr_close(lpr_ctx* ctx);

// Execute RPL input — literals, commands, programs, anything
lpr_result  lpr_exec(lpr_ctx* ctx, const char* input);

// Stack inspection
int         lpr_depth(lpr_ctx* ctx);
char*       lpr_repr(lpr_ctx* ctx, int level); // caller must lpr_free()

// History
int         lpr_undo(lpr_ctx* ctx);            // returns 1 on success
int         lpr_redo(lpr_ctx* ctx);            // returns 1 on success

// Memory
void        lpr_free(void* ptr);
```

Seven functions plus one deallocator. That's the entire public API.

### Usage example (from C)

```c
lpr_ctx* ctx = lpr_open(NULL);       // in-memory database

lpr_exec(ctx, "355");                // push integer
lpr_exec(ctx, "113");                // push integer
lpr_exec(ctx, "/");                  // divide → rational 355/113
lpr_exec(ctx, "'X^2 + 1'");         // push symbolic expression
lpr_exec(ctx, "DIFF");              // differentiate → 2*X

char* top = lpr_repr(ctx, 1);       // "2*X"
printf("top: %s\n", top);
lpr_free(top);

lpr_exec(ctx, "'approx_pi' STO");   // store 355/113 into filesystem
lpr_undo(ctx);                       // undo the STO

lpr_close(ctx);
```

---

## Object Type System

RPL is dynamically typed. Every value on the stack is an `Object`. Internally
this is a `std::variant` over the following types:

| Type        | C++ Backing                                | RPL Syntax Example      |
|-------------|-------------------------------------------|-------------------------|
| `Integer`   | `boost::multiprecision::cpp_int`          | `42`, `-7`              |
| `Real`      | `boost::multiprecision::cpp_dec_float_50` | `3.14159`, `1.5E-10`    |
| `Rational`  | `boost::multiprecision::cpp_rational`     | (result of `/` on ints) |
| `Complex`   | `std::pair<Real, Real>`                   | `(3.0, 4.0)`           |
| `String`    | `std::string`                             | `"hello"`               |
| `Symbol`    | CAS expression tree                       | `'X^2 + 1'`            |
| `Program`   | `std::vector<Token>`                      | `« DUP * »`            |
| `Name`      | `std::string`                             | `'myvar'`               |
| `Error`     | `int code` + `std::string message`        | (pushed on failure)     |

### Type Promotion Rules

Arithmetic operations promote operands upward through the numeric tower:

```
Integer → Rational → Real → Complex
```

- `Integer / Integer` → `Rational` (exact)
- `Rational + Real` → `Real`
- `Real + Complex` → `Complex`
- Explicit commands (`→NUM`, `→Q`) convert between representations.

### Symbolic vs Numeric

Expressions wrapped in single quotes (`'...'`) are symbolic — they are *not*
evaluated on push. They live as expression trees and are manipulated by CAS
commands. `EVAL` on a symbolic expression with no free variables collapses it
to a numeric value.

---

## SQLite Schema

A single `.lpr` database file (or `:memory:`) holds all runtime state.

```sql
-- Serialized RPL objects (content-addressed by hash)
CREATE TABLE objects (
    id    INTEGER PRIMARY KEY,
    type  INTEGER NOT NULL,          -- ObjectType enum
    data  BLOB NOT NULL              -- type-specific serialization
);

-- The RPL stack (level 1 = top of stack)
CREATE TABLE stack (
    level     INTEGER PRIMARY KEY,
    object_id INTEGER NOT NULL REFERENCES objects(id)
);

-- Directory tree (HP 50g-style HOME/subdirs)
CREATE TABLE directories (
    id        INTEGER PRIMARY KEY,
    parent_id INTEGER REFERENCES directories(id),
    name      TEXT NOT NULL,
    UNIQUE(parent_id, name)
);

-- Named variables within directories
CREATE TABLE variables (
    id        INTEGER PRIMARY KEY,
    dir_id    INTEGER NOT NULL REFERENCES directories(id),
    name      TEXT NOT NULL,
    object_id INTEGER NOT NULL REFERENCES objects(id),
    UNIQUE(dir_id, name)
);

-- Undo/redo history (stack snapshots)
CREATE TABLE history (
    seq       INTEGER PRIMARY KEY AUTOINCREMENT,
    stack_json TEXT NOT NULL          -- JSON array of {level, type, data}
);

-- Runtime metadata
CREATE TABLE meta (
    key   TEXT PRIMARY KEY,
    value TEXT
);

-- Seed: root HOME directory
INSERT INTO directories (id, parent_id, name) VALUES (1, NULL, 'HOME');
INSERT INTO meta (key, value) VALUES ('cwd', '1');          -- current directory
INSERT INTO meta (key, value) VALUES ('history_pos', '0');  -- undo cursor
```

### Serialization

Each object type defines `serialize() → blob` and `deserialize(blob) → Object`.
Numbers store their string representation (Boost.Multiprecision roundtrips
cleanly through strings). Symbolic expressions serialize as S-expression strings.
Programs serialize as their token sequence.

---

## Internal Architecture

```
┌──────────────────────────────────────────────────────┐
│                     C API (lpr.h)                    │
├──────────────────────────────────────────────────────┤
│                      Context                         │
│  Owns the SQLite connection, parser, command table   │
├────────────┬────────────┬────────────┬───────────────┤
│   Parser   │   Stack    │ Filesystem │    History    │
│            │ (SQLite)   │  (SQLite)  │   (SQLite)   │
├────────────┴────────────┴────────────┴───────────────┤
│                  Command Dispatch                     │
│  Registry of name → function. Each command is a      │
│  C++ callable: void(Stack&, Context&)                │
├──────────────────────────────────────────────────────┤
│                   Object System                       │
│  std::variant<Integer, Real, Rational, Complex,      │
│               String, Symbol, Program, Name, Error>  │
├──────────────────────────────────────────────────────┤
│              Numeric Tower          CAS Bridge        │
│        (Boost.Multiprecision)    (SymEngine / Giac)  │
└──────────────────────────────────────────────────────┘
```

### Parser

The parser tokenizes RPL input into a flat sequence:

- **Number literals** → push as Integer, Real, or Complex
- **Quoted names** (`'X'`, `'myvar'`) → push as Name or Symbol
- **Strings** (`"hello"`) → push as String
- **Program delimiters** (`«` ... `»`) → collect tokens into Program object, push
- **Bare words** (`DUP`, `+`, `STO`) → look up in command registry, execute

The parser is single-pass and non-recursive (programs are token lists, not ASTs).

### Command Dispatch

Commands are registered in a `std::unordered_map<std::string, CommandFn>`.
Each command is a plain function:

```cpp
using CommandFn = std::function<void(Stack&, Context&)>;
```

Commands pop their arguments from the stack, do work, and push results.
On error, they push an `Error` object and throw an internal exception that
the `lpr_exec` transaction handler catches (rolling back the failed operation
and leaving the error on stack).

### Execution Flow of `lpr_exec(ctx, input)`

1. Begin SQLite transaction
2. Snapshot current stack → `history` table
3. Tokenize `input`
4. For each token:
   - If literal: push onto stack
   - If command name: look up in registry, invoke
   - If `«`...`»`: collect into `Program`, push
5. Commit transaction
6. On any error: rollback to pre-snapshot state, push `Error`, commit

---

## CAS Bridge

```cpp
class CASBridge {
public:
    virtual ~CASBridge() = default;

    virtual Object differentiate(const Object& expr, const std::string& var) = 0;
    virtual Object integrate(const Object& expr, const std::string& var) = 0;
    virtual Object solve(const Object& expr, const std::string& var) = 0;
    virtual Object simplify(const Object& expr) = 0;
    virtual Object expand(const Object& expr) = 0;
    virtual Object factor(const Object& expr) = 0;
};
```

`SymEngineBridge` implements this first. `GiacBridge` can slot in later.
The bridge converts between our `Symbol` representation and the CAS library's
native expression type.

---

## Directory Layout

```
lpr-runtime/
├── CMakeLists.txt              # Top-level build
├── ARCHITECTURE.md
├── spec/
│   └── 001-bootstrap.md        # Bootstrap build & MVP spec
├── include/
│   └── lpr/
│       └── lpr.h               # Public C API
├── src/
│   ├── core/
│   │   ├── context.hpp/.cpp    # lpr_ctx implementation
│   │   ├── object.hpp/.cpp     # Object variant + serialization
│   │   ├── stack.hpp/.cpp      # SQLite-backed stack
│   │   ├── parser.hpp/.cpp     # RPL tokenizer
│   │   ├── commands.hpp/.cpp   # Command registry + dispatch
│   │   ├── store.hpp/.cpp      # SQLite persistence layer
│   │   └── history.hpp/.cpp    # Undo/redo management
│   ├── types/
│   │   ├── numeric.hpp         # Integer, Real, Rational, Complex
│   │   ├── symbolic.hpp        # Symbol type + expression builder
│   │   ├── program.hpp         # Program type (token list)
│   │   └── error.hpp           # Error type
│   ├── commands/
│   │   ├── arithmetic.cpp      # + - * / NEG INV ABS
│   │   ├── stack_ops.cpp       # DUP DROP SWAP OVER ROT DEPTH CLEAR
│   │   ├── comparison.cpp      # == != < > <= >=
│   │   ├── transcendental.cpp  # SIN COS TAN EXP LN SQRT
│   │   ├── type_conv.cpp       # →NUM →Q →STR TYPE
│   │   ├── program_ctrl.cpp    # EVAL IFT IFTE
│   │   └── filesystem.cpp      # STO RCL PURGE CRDIR PATH HOME VARS
│   └── cas/
│       ├── bridge.hpp          # Abstract CASBridge interface
│       └── symengine_backend.cpp
├── cli/
│   └── main.cpp                # Interactive REPL for development
└── tests/
    ├── CMakeLists.txt
    ├── test_stack.cpp
    ├── test_arithmetic.cpp
    ├── test_parser.cpp
    ├── test_types.cpp
    ├── test_undo.cpp
    └── test_filesystem.cpp
```

---

## Build & Dependencies

| Dependency            | Role                       | Acquisition       |
|-----------------------|----------------------------|--------------------|
| Boost.Multiprecision  | Arbitrary-precision numbers | FetchContent / system |
| SQLite3               | Persistence & state        | FetchContent / system |
| SymEngine             | Computer algebra (phase 1) | FetchContent       |
| Catch2                | Unit testing               | FetchContent       |

Build system: **CMake 3.20+**, targeting **C++17**.

Cross-compilation targets: macOS CLI (dev), iOS framework, Linux, eventually
WASM and embedded ARM.
