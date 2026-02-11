# LPR Command Set Reference

Complete reference for all commands in the LPR (Lenny Polish Reverse) runtime. LPR is an RPL virtual machine inspired by the HP 50g calculator, using postfix (reverse Polish) notation where operands are pushed onto a stack before commands consume them.

All commands are **case-insensitive** (internally uppercased). Stack levels are numbered from the top: level 1 is the topmost item, level 2 is below it, etc.

---

## Stack Operations

| Command | Stack Effect | Description |
|---------|-------------|-------------|
| `DUP`   | `( a -- a a )` | Duplicate the top item |
| `DUP2`  | `( a b -- a b a b )` | Duplicate the top two items |
| `DUPN`  | `( ...items n -- ...items ...items )` | Pop n, duplicate the top n items |
| `DROP`  | `( a -- )` | Remove the top item |
| `DROP2` | `( a b -- )` | Remove the top two items |
| `DROPN` | `( ...items n -- )` | Pop n, remove the top n items |
| `SWAP`  | `( a b -- b a )` | Swap the top two items |
| `OVER`  | `( a b -- a b a )` | Copy the second item to the top |
| `ROT`   | `( a b c -- b c a )` | Rotate the third item to the top |
| `UNROT` | `( a b c -- c a b )` | Reverse rotate — move top to third position |
| `PICK`  | `( ...items n -- ...items nth )` | Pop n, copy the nth item to the top |
| `ROLL`  | `( ...items n -- ...rolled )` | Pop n, roll the nth item to the top |
| `ROLLD` | `( ...items n -- ...rolled )` | Pop n, roll the top item down to the nth position |
| `UNPICK`| `( ...items obj n -- ...modified )` | Pop n and obj, replace the nth item with obj |
| `DEPTH` | `( -- n )` | Push the current stack depth as an Integer |
| `CLEAR` | `( ... -- )` | Remove all items from the stack |

**Examples:**

```
5 DUP          => 5 5
1 2 DUP2       => 1 2 1 2
1 2 3 2 DUPN   => 1 2 3 2 3
3 7 SWAP       => 7 3
1 2 3 ROT      => 2 3 1
1 2 3 UNROT    => 3 1 2
1 2 3 3 PICK   => 1 2 3 1
1 2 3 3 ROLL   => 2 3 1     (same as ROT)
1 2 3 3 ROLLD  => 3 1 2     (same as UNROT)
1 2 3 99 2 UNPICK  => 1 99 3
1 2 3 DROP2    => 1
1 2 3 4 3 DROPN => 1
1 2 3 DEPTH    => 1 2 3 3
```

**Parameterized commands** (`DUPN`, `DROPN`, `PICK`, `ROLL`, `ROLLD`, `UNPICK`) consume an Integer from level 1 to determine their operand count or position. A type error is thrown if level 1 is not an Integer. `PICK` with n=1 is equivalent to `DUP`; `ROLL` with n=3 is equivalent to `ROT`; `ROLLD` with n=3 is equivalent to `UNROT`.

---

## Arithmetic Operations

All arithmetic commands perform **automatic type promotion** through the numeric tower: `Integer -> Rational -> Real -> Complex`. When operands differ in type, the lower-ranked operand is promoted before the operation.

| Command | Stack Effect | Description |
|---------|-------------|-------------|
| `+`     | `( a b -- a+b )` | Add two numbers |
| `-`     | `( a b -- a-b )` | Subtract level 1 from level 2 |
| `*`     | `( a b -- a*b )` | Multiply two numbers |
| `/`     | `( a b -- a/b )` | Divide level 2 by level 1 |
| `NEG`   | `( a -- -a )` | Negate a number |
| `INV`   | `( a -- 1/a )` | Reciprocal |
| `ABS`   | `( a -- |a| )` | Absolute value (magnitude for Complex) |
| `MOD`   | `( a b -- a%b )` | Integer modulo |

**Division behavior:** Integer division produces a Rational result (e.g. `355 113 /` yields `355/113`), preserving exactness. Division by zero throws an error and restores the stack.

**MOD** only operates on Integers. All other arithmetic commands work across the full numeric tower.

**Complex multiplication** uses the standard formula `(a+bi)(c+di) = (ac-bd) + (ad+bc)i`. Complex division uses conjugate multiplication.

**Examples:**

```
3 4 +          => 7
10 3 -         => 7
6 7 *          => 42
355 113 /      => 355/113
5 NEG          => -5
4 INV          => 1/4
-7 ABS         => 7
10 3 MOD       => 1
1 2.5 +        => 3.5          (Integer promoted to Real)
```

---

## Comparison Operations

All comparison commands consume two numeric values and push `1` (true) or `0` (false) as an Integer. Operands are promoted to the same type before comparison.

| Command | Stack Effect | Description |
|---------|-------------|-------------|
| `==`    | `( a b -- flag )` | Equal |
| `!=`    | `( a b -- flag )` | Not equal |
| `<`     | `( a b -- flag )` | Less than |
| `>`     | `( a b -- flag )` | Greater than |
| `<=`    | `( a b -- flag )` | Less than or equal |
| `>=`    | `( a b -- flag )` | Greater than or equal |

For Complex numbers, comparisons operate on the real part only. Only `==` and `!=` are semantically meaningful for complex values.

**Examples:**

```
5 5 ==         => 1
5 3 !=         => 1
3 5 <          => 1
5 3 >          => 1
3 3 <=         => 1
2 3 >=         => 0
```

---

## Type Operations

| Command | Stack Effect | Description |
|---------|-------------|-------------|
| `TYPE`  | `( obj -- tag )` | Push the object's type tag as an Integer |
| `->NUM` | `( obj -- real )` | Convert to Real (decimal approximation) |
| `->STR` | `( obj -- string )` | Convert any object to its String representation |
| `STR->` | `( string -- ... )` | Parse and evaluate a String as LPR code |

UTF-8 arrow variants `->NUM`, `->STR`, `STR->` are also accepted as `->NUM`, `->STR`, `STR->` (ASCII fallback).

### Type Tags

| Tag | Type |
|-----|------|
| 0 | Integer |
| 1 | Real |
| 2 | Rational |
| 3 | Complex |
| 4 | String |
| 5 | Program |
| 6 | Name |
| 7 | Error |
| 8 | Symbol |

**Examples:**

```
42 TYPE              => 0
3.14 TYPE            => 1
42 ->STR             => "42"
"3 4 +" STR->        => 7
355 113 / ->NUM      => 3.14159292035398...
```

---

## Filesystem Operations

LPR provides a SQLite-backed hierarchical variable filesystem. Variables are stored in directories, starting from the HOME directory.

| Command | Stack Effect | Description |
|---------|-------------|-------------|
| `STO`   | `( value 'name' -- )` | Store a value in a variable |
| `RCL`   | `( 'name' -- value )` | Recall a variable's value |
| `PURGE` | `( 'name' -- )` | Delete a variable |
| `HOME`  | `( -- )` | Change to the home directory |
| `PATH`  | `( -- string )` | Push the current directory path |
| `CRDIR` | `( 'name' -- )` | Create a subdirectory |
| `VARS`  | `( -- string )` | List variables in the current directory |

Variable names are specified using quoted Names (e.g. `'x'`). STO expects the value at level 2 and the name at level 1.

**Examples:**

```
42 'x' STO           (stores 42 as x)
'x' RCL         => 42
'x' PURGE            (deletes x)
'MYDIR' CRDIR        (creates subdirectory MYDIR)
VARS             => "{ x y z }"
PATH             => "HOME"
```

---

## Program Control

| Command | Stack Effect | Description |
|---------|-------------|-------------|
| `EVAL`  | `( obj -- ... )` | Evaluate a program, recall a variable, or evaluate a symbol expression |
| `IFT`   | `( then cond -- ... )` | If-then: execute `then` when `cond` is truthy |
| `IFTE`  | `( else then cond -- ... )` | If-then-else: execute `then` or `else` based on `cond` |

### EVAL

- **Program**: executes the program's tokens
- **Name**: recalls the variable; if it contains a Program, executes it
- **Symbol**: parses the infix expression, substitutes variables (locals then globals), and pushes the numeric result
- **Other types**: pushes the object back unchanged

### IFT / IFTE

A value is **truthy** if it is a non-zero numeric value (Integer, Real, Rational, or Complex with any non-zero component). If the selected branch is a Program it is executed; otherwise it is pushed to the stack.

**Examples:**

```
<< 2 3 + >> EVAL                    => 5
<< "yes" >> 1 IFT                   => "yes"
<< "yes" >> 0 IFT                   (nothing pushed)
<< "no" >> << "yes" >> 1 IFTE       => "yes"
<< "no" >> << "yes" >> 0 IFTE       => "no"
'2+3' EVAL                          => 5
'(2+3)*(4-1)' EVAL                  => 15
5 'X' STO  'X^2' EVAL              => 25.
```

---

## Local Binding (`->`)

The `->` command (or its UTF-8 equivalent `→`) is a **runstream-consuming command**: unlike normal commands that take arguments from the stack, `->` reads variable names and a body directly from the token stream following it. This makes it fundamentally different from all other LPR commands.

### Syntax

```
-> var1 var2 ... varN body
```

- **Variable names**: one or more bare names consumed from the runstream (not the stack)
- **Body**: a single Program (`<< ... >>`) or Symbol (`'...'`) consumed from the runstream

### Behavior

1. `->` reads ahead in the token stream to collect variable names until it encounters a Program or Symbol literal (the body).
2. It pops N values from the stack (one per variable name). The first name binds the deepest value, the last name binds level 1.
3. A local scope frame is pushed with the bindings.
4. The body is executed:
   - **Program body**: tokens are executed directly
   - **Symbol body**: the expression is evaluated via `EVAL` (infix expression with variable substitution)
5. The local scope frame is popped when the body completes.

### Local Variable Resolution

Within the body, bound names resolve as local variables. Resolution order:

1. **Local scopes** (innermost first) -- local bindings shadow outer ones
2. **Global variables** (via STO/RCL filesystem)

Local variables are transient execution state and are not stored in the variable filesystem.

### Nesting

`->` blocks can nest. An inner `->` creates a new scope frame that shadows outer bindings of the same name. When the inner body completes, its frame is removed and the outer bindings become visible again.

**Examples:**

```
3 5 << -> X Y << X Y * >> >> EVAL           => 15
10 20 << -> A B << A B + >> >> EVAL         => 30
7 << -> N << N N * >> >> EVAL               => 49
42 << -> N << N >> >> EVAL                  => 42
3 5 << -> X Y 'X*Y' >> EVAL                => 15
```

**Nesting example:**

```
2 << -> X << 5 << -> X << X >> >> EVAL >> >> EVAL    => 5
```

The outer `->` binds X=2. The inner `->` binds X=5 in a new scope. The inner body sees X=5. After the inner body completes, X=2 is visible again.

**Local shadows global:**

```
100 'X' STO
5 << -> X << X >> >> EVAL    => 5    (local X=5 shadows global X=100)
```

### Errors

- `-> missing body` -- no Symbol or Program found after the variable names
- `-> requires at least one variable name` -- no names before the body
- `Too few arguments for ->` -- stack has fewer values than variable names

---

## Structured Control Flow

Control flow structures are **runstream-consuming** — they consume tokens directly from the token stream at execution time, not from the stack. They nest naturally because collected bodies are re-executed via `execute_tokens`.

| Construct | Syntax | Description |
|-----------|--------|-------------|
| `IF...THEN...END` | `IF cond THEN body END` | Execute body if cond is nonzero |
| `IF...THEN...ELSE...END` | `IF cond THEN body1 ELSE body2 END` | Execute body1 if true, body2 if false |
| `CASE...END` | `CASE test1 THEN body1 END ... END` | Multi-way branch; first true test wins |
| `FOR...NEXT` | `start end FOR var body NEXT` | Counted loop with step +1, binds loop variable |
| `FOR...STEP` | `start end FOR var body step STEP` | Counted loop with custom step (popped from stack) |
| `START...NEXT` | `start end START body NEXT` | Counted loop without loop variable |
| `START...STEP` | `start end START body step STEP` | Counted loop without variable, custom step |
| `WHILE...REPEAT...END` | `WHILE cond REPEAT body END` | Pre-test loop |
| `DO...UNTIL...END` | `DO body UNTIL cond END` | Post-test loop (executes at least once) |

**Examples:**

```
IF 1 THEN 42 END                              => 42
IF 0 THEN 10 ELSE 20 END                      => 20
1 5 FOR I I NEXT                               => 1 2 3 4 5
0 1 3 FOR I I + NEXT                           => 6
5 WHILE DUP 0 > REPEAT 1 - END                => 0
5 DO 1 - DUP 0 == UNTIL END                   => 0
```

---

## Logic & Bitwise Operations

| Command | Stack Effect | Description |
|---------|-------------|-------------|
| `AND`   | `( a b -- flag )` | Boolean AND (integers; nonzero = true) |
| `OR`    | `( a b -- flag )` | Boolean OR |
| `NOT`   | `( a -- flag )` | Boolean NOT |
| `XOR`   | `( a b -- flag )` | Boolean exclusive OR |
| `BAND`  | `( a b -- n )` | Bitwise AND |
| `BOR`   | `( a b -- n )` | Bitwise OR |
| `BXOR`  | `( a b -- n )` | Bitwise XOR |
| `BNOT`  | `( a -- n )` | Bitwise NOT (complement) |
| `SL`    | `( a n -- result )` | Shift left by n bits |
| `SR`    | `( a n -- result )` | Shift right by n bits |
| `ASR`   | `( a n -- result )` | Arithmetic shift right (sign-extending) |
| `SAME`  | `( a b -- flag )` | Deep structural equality (same type AND value) |

All logic and bitwise commands require Integer arguments. `SAME` differs from `==`: `SAME` requires identical types (`1 1.0 SAME` → 0), while `==` promotes types (`1 1.0 ==` → 1).

---

## Transcendental & Scientific Functions

### Angle Mode

| Command | Description |
|---------|-------------|
| `DEG`   | Set angle mode to degrees |
| `RAD`   | Set angle mode to radians (default) |
| `GRAD`  | Set angle mode to gradians |

Trig functions convert input/output according to the current angle mode.

### Trigonometry

| Command | Stack Effect | Description |
|---------|-------------|-------------|
| `SIN`   | `( x -- sin(x) )` | Sine |
| `COS`   | `( x -- cos(x) )` | Cosine |
| `TAN`   | `( x -- tan(x) )` | Tangent |
| `ASIN`  | `( x -- asin(x) )` | Inverse sine |
| `ACOS`  | `( x -- acos(x) )` | Inverse cosine |
| `ATAN`  | `( x -- atan(x) )` | Inverse tangent |
| `ATAN2` | `( y x -- atan2(y,x) )` | Two-argument inverse tangent |

### Exponential & Logarithmic

| Command | Stack Effect | Description |
|---------|-------------|-------------|
| `EXP`   | `( x -- e^x )` | Natural exponential |
| `LN`    | `( x -- ln(x) )` | Natural logarithm |
| `LOG`   | `( x -- log10(x) )` | Common (base-10) logarithm |
| `ALOG`  | `( x -- 10^x )` | Common antilogarithm |
| `SQRT`  | `( x -- √x )` | Square root |
| `SQ`    | `( x -- x² )` | Square |

### Constants

| Command | Description |
|---------|-------------|
| `PI`    | Push π (50-digit precision) |
| `E`     | Push e (50-digit precision) |

### Rounding

| Command | Stack Effect | Description |
|---------|-------------|-------------|
| `FLOOR` | `( x -- n )` | Floor (round toward -∞) |
| `CEIL`  | `( x -- n )` | Ceiling (round toward +∞) |
| `IP`    | `( x -- n )` | Integer part (truncate toward zero) |
| `FP`    | `( x -- r )` | Fractional part |

### Utility

| Command | Stack Effect | Description |
|---------|-------------|-------------|
| `MIN`   | `( a b -- min )` | Minimum of two values |
| `MAX`   | `( a b -- max )` | Maximum of two values |
| `SIGN`  | `( x -- s )` | Sign: -1, 0, or 1 |

### Combinatorics

| Command | Stack Effect | Description |
|---------|-------------|-------------|
| `!`     | `( n -- n! )` | Factorial |
| `COMB`  | `( n k -- C(n,k) )` | Combinations |
| `PERM`  | `( n k -- P(n,k) )` | Permutations |

### Percentage

| Command | Stack Effect | Description |
|---------|-------------|-------------|
| `%`     | `( base pct -- result )` | Percentage: base × pct / 100 |
| `%T`    | `( total part -- pct )` | What percent is part of total |
| `%CH`   | `( old new -- pct )` | Percent change from old to new |

### Angle Conversion

| Command | Stack Effect | Description |
|---------|-------------|-------------|
| `D->R`  | `( deg -- rad )` | Degrees to radians |
| `R->D`  | `( rad -- deg )` | Radians to degrees |

---

## String Manipulation

| Command | Stack Effect | Description |
|---------|-------------|-------------|
| `SIZE`  | `( str -- n )` | Length of string |
| `HEAD`  | `( str -- char )` | First character |
| `TAIL`  | `( str -- rest )` | All but first character |
| `SUB`   | `( str start end -- substr )` | 1-based substring |
| `POS`   | `( str search -- pos )` | Find position (0 if not found) |
| `REPL`  | `( str search repl -- result )` | Replace first occurrence |
| `NUM`   | `( str -- codepoint )` | First char to ASCII codepoint |
| `CHR`   | `( codepoint -- str )` | ASCII codepoint to character |

The `+` command is overloaded: when both operands are Strings, it concatenates. Mixed String/numeric operands are an error.

**Examples:**

```
"hello" SIZE                     => 5
"hello" HEAD                     => "h"
"hello" TAIL                     => "ello"
"hello" 2 4 SUB                  => "ell"
"hello world" "world" POS        => 7
"hello world" "world" "there" REPL  => "hello there"
"A" NUM                          => 65
65 CHR                           => "A"
"hello" " world" +               => "hello world"
```

---

## Data Types

### Numeric Types

| Type | Precision | Literal Syntax | Example |
|------|-----------|---------------|---------|
| **Integer** | Arbitrary precision | Digits, optional sign | `42`, `-7`, `99999999999999999999` |
| **Real** | 50 decimal digits | Decimal point or exponent | `3.14159`, `-2.5`, `1.5E-10` |
| **Rational** | Arbitrary precision | Created by integer division | `355/113` |
| **Complex** | Pair of Reals | Parenthesized pair | `(3.0, 4.0)` |

### Numeric Tower

Automatic promotion order: **Integer -> Rational -> Real -> Complex**

When an operation involves two different numeric types, the lower-ranked operand is promoted to match the higher-ranked one before the operation executes.

### Other Types

| Type | Syntax | Description |
|------|--------|-------------|
| **String** | `"hello"` | Text; supports `\n`, `\t`, `\"`, `\\` escapes |
| **Name** | `'x'` | Variable identifier (single-quoted) |
| **Symbol** | `'X^2 + 1'` | Algebraic expression (single-quoted, contains operators); evaluable via `EVAL` |
| **Program** | `<< DUP * >>` | Executable code block (guillemets) |
| **Error** | (auto-generated) | Error with code and message |

---

## Error Handling

Commands throw exceptions on error conditions such as:

- **Stack underflow** -- not enough arguments on the stack
- **Type mismatch** -- wrong argument type for the command
- **Division by zero** -- dividing or inverting zero
- **Undefined name** -- recalling a variable that doesn't exist

On error, the stack is **restored** to its pre-command state (transactional rollback via SQLite), and an Error object is pushed.

---

## Command Summary

| # | Command | Category | Args | Description |
|---|---------|----------|------|-------------|
| 1 | `DUP` | Stack | 1 | Duplicate top |
| 2 | `DUP2` | Stack | 2 | Duplicate top two |
| 3 | `DUPN` | Stack | n+1 | Duplicate top n items |
| 4 | `DROP` | Stack | 1 | Remove top |
| 5 | `DROP2` | Stack | 2 | Remove top two |
| 6 | `DROPN` | Stack | n+1 | Remove top n items |
| 7 | `SWAP` | Stack | 2 | Swap top two |
| 8 | `OVER` | Stack | 2 | Copy second to top |
| 9 | `ROT` | Stack | 3 | Rotate third to top |
| 10 | `UNROT` | Stack | 3 | Reverse rotate top to third |
| 11 | `PICK` | Stack | n+1 | Copy nth item to top |
| 12 | `ROLL` | Stack | n+1 | Roll nth item to top |
| 13 | `ROLLD` | Stack | n+1 | Roll top down to nth position |
| 14 | `UNPICK` | Stack | n+2 | Replace nth item with obj |
| 15 | `DEPTH` | Stack | 0 | Push stack depth |
| 16 | `CLEAR` | Stack | 0 | Clear entire stack |
| 17 | `+` | Arithmetic | 2 | Addition |
| 18 | `-` | Arithmetic | 2 | Subtraction |
| 19 | `*` | Arithmetic | 2 | Multiplication |
| 20 | `/` | Arithmetic | 2 | Division |
| 21 | `NEG` | Arithmetic | 1 | Negation |
| 22 | `INV` | Arithmetic | 1 | Reciprocal |
| 23 | `ABS` | Arithmetic | 1 | Absolute value |
| 24 | `MOD` | Arithmetic | 2 | Modulo (Integer only) |
| 25 | `==` | Comparison | 2 | Equal |
| 26 | `!=` | Comparison | 2 | Not equal |
| 27 | `<` | Comparison | 2 | Less than |
| 28 | `>` | Comparison | 2 | Greater than |
| 29 | `<=` | Comparison | 2 | Less or equal |
| 30 | `>=` | Comparison | 2 | Greater or equal |
| 31 | `TYPE` | Type | 1 | Type tag |
| 32 | `->NUM` | Type | 1 | Convert to Real |
| 33 | `->STR` | Type | 1 | Convert to String |
| 34 | `STR->` | Type | 1 | Evaluate String as code |
| 35 | `STO` | Filesystem | 2 | Store variable |
| 36 | `RCL` | Filesystem | 1 | Recall variable |
| 37 | `PURGE` | Filesystem | 1 | Delete variable |
| 38 | `HOME` | Filesystem | 0 | Go to home directory |
| 39 | `PATH` | Filesystem | 0 | Current directory path |
| 40 | `CRDIR` | Filesystem | 1 | Create subdirectory |
| 41 | `VARS` | Filesystem | 0 | List variables |
| 42 | `EVAL` | Program | 1 | Evaluate object (program, name, or symbol expression) |
| 43 | `IFT` | Program | 2 | Conditional (if-then) |
| 44 | `IFTE` | Program | 3 | Conditional (if-then-else) |
| 45 | `->` / `→` | Local Binding | *runstream* | Bind stack values to local variables, execute body |
| 46 | `IF...THEN...END` | Control Flow | *runstream* | Conditional execution |
| 47 | `CASE...END` | Control Flow | *runstream* | Multi-way branch |
| 48 | `FOR...NEXT` | Control Flow | 2 + *runstream* | Counted loop with variable |
| 49 | `FOR...STEP` | Control Flow | 2 + *runstream* | Counted loop with variable and custom step |
| 50 | `START...NEXT` | Control Flow | 2 + *runstream* | Counted loop |
| 51 | `START...STEP` | Control Flow | 2 + *runstream* | Counted loop with custom step |
| 52 | `WHILE...REPEAT...END` | Control Flow | *runstream* | Pre-test loop |
| 53 | `DO...UNTIL...END` | Control Flow | *runstream* | Post-test loop |
| 54 | `AND` | Logic | 2 | Boolean AND |
| 55 | `OR` | Logic | 2 | Boolean OR |
| 56 | `NOT` | Logic | 1 | Boolean NOT |
| 57 | `XOR` | Logic | 2 | Boolean XOR |
| 58 | `BAND` | Bitwise | 2 | Bitwise AND |
| 59 | `BOR` | Bitwise | 2 | Bitwise OR |
| 60 | `BXOR` | Bitwise | 2 | Bitwise XOR |
| 61 | `BNOT` | Bitwise | 1 | Bitwise NOT |
| 62 | `SL` | Bitwise | 2 | Shift left |
| 63 | `SR` | Bitwise | 2 | Shift right |
| 64 | `ASR` | Bitwise | 2 | Arithmetic shift right |
| 65 | `SAME` | Logic | 2 | Deep structural equality |
| 66 | `DEG` | Angle Mode | 0 | Set degrees mode |
| 67 | `RAD` | Angle Mode | 0 | Set radians mode |
| 68 | `GRAD` | Angle Mode | 0 | Set gradians mode |
| 69 | `SIN` | Transcendental | 1 | Sine |
| 70 | `COS` | Transcendental | 1 | Cosine |
| 71 | `TAN` | Transcendental | 1 | Tangent |
| 72 | `ASIN` | Transcendental | 1 | Inverse sine |
| 73 | `ACOS` | Transcendental | 1 | Inverse cosine |
| 74 | `ATAN` | Transcendental | 1 | Inverse tangent |
| 75 | `ATAN2` | Transcendental | 2 | Two-argument inverse tangent |
| 76 | `EXP` | Transcendental | 1 | Natural exponential |
| 77 | `LN` | Transcendental | 1 | Natural logarithm |
| 78 | `LOG` | Transcendental | 1 | Common logarithm |
| 79 | `ALOG` | Transcendental | 1 | Common antilogarithm |
| 80 | `SQRT` | Transcendental | 1 | Square root |
| 81 | `SQ` | Transcendental | 1 | Square |
| 82 | `PI` | Constant | 0 | Push π |
| 83 | `E` | Constant | 0 | Push e |
| 84 | `FLOOR` | Rounding | 1 | Floor |
| 85 | `CEIL` | Rounding | 1 | Ceiling |
| 86 | `IP` | Rounding | 1 | Integer part |
| 87 | `FP` | Rounding | 1 | Fractional part |
| 88 | `MIN` | Utility | 2 | Minimum |
| 89 | `MAX` | Utility | 2 | Maximum |
| 90 | `SIGN` | Utility | 1 | Sign |
| 91 | `!` | Combinatorics | 1 | Factorial |
| 92 | `COMB` | Combinatorics | 2 | Combinations |
| 93 | `PERM` | Combinatorics | 2 | Permutations |
| 94 | `%` | Percentage | 2 | Percentage |
| 95 | `%T` | Percentage | 2 | Percent of total |
| 96 | `%CH` | Percentage | 2 | Percent change |
| 97 | `D->R` | Angle Conv | 1 | Degrees to radians |
| 98 | `R->D` | Angle Conv | 1 | Radians to degrees |
| 99 | `SIZE` | String | 1 | String length |
| 100 | `HEAD` | String | 1 | First character |
| 101 | `TAIL` | String | 1 | All but first character |
| 102 | `SUB` | String | 3 | Substring |
| 103 | `POS` | String | 2 | Find position |
| 104 | `REPL` | String | 3 | Replace first occurrence |
| 105 | `NUM` | String | 1 | Char to codepoint |
| 106 | `CHR` | String | 1 | Codepoint to char |
