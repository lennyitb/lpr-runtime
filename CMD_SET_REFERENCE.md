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
| `^`     | `( a b -- a^b )` | Power / exponentiation |

**Division behavior:** Integer division produces a Rational result (e.g. `355 113 /` yields `355/113`), preserving exactness. Division by zero throws an error and restores the stack.

**MOD** only operates on Integers.

**Power (`^`)** preserves exact types when the exponent is an integer: `Integer ^ positive-int` yields Integer, `Integer ^ negative-int` yields Rational, `Rational ^ int` yields Rational. Non-integer exponents promote to Real. Exponents are capped at 1,000,000 to prevent resource exhaustion.

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
2 10 ^         => 1024
2 -3 ^         => 1/8
2 3 / 3 ^      => 8/27         (Rational base, integer exponent)
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
| 9 | List |
| 10 | Matrix |

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
| `PATH`  | `( -- string )` | Push the full current directory path |
| `CRDIR` | `( 'name' -- )` | Create a subdirectory |
| `CD`    | `( 'name' -- )` | Change into a named subdirectory |
| `UPDIR` | `( -- )` | Move up one directory level (no-op at HOME) |
| `PGDIR` | `( 'name' -- )` | Recursively delete a subdirectory and its contents |
| `VARS`  | `( -- list )` | List directory contents as Names (dirs suffixed with `/`) |

Variable names are specified using quoted Names (e.g. `'x'`). STO expects the value at level 2 and the name at level 1.

**Examples:**

```
42 'x' STO           (stores 42 as x)
'x' RCL         => 42
'x' PURGE            (deletes x)
'MYDIR' CRDIR        (creates subdirectory MYDIR)
'MYDIR' CD           (enters MYDIR)
PATH             => "HOME/MYDIR"
UPDIR                (back to HOME)
VARS             => { 'x' 'MYDIR/' }
'MYDIR' PGDIR        (deletes MYDIR and all contents)
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
- **Name**: recalls the variable; if it contains a Program, executes it; if undefined, pushes the Name back unchanged
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

## List Operations

Lists are heterogeneous ordered collections using `{ }` delimiters. Elements can be any type including nested lists. Indexing is 1-based.

### Core Access

| Command | Stack Effect | Description |
|---------|-------------|-------------|
| `LIST->` | `( { a b c } -- a b c 3 )` | Explode list onto stack with count |
| `->LIST` | `( a b c 3 -- { a b c } )` | Collect N items from stack into list |
| `GET` | `( list idx -- elem )` | 1-based element access |
| `PUT` | `( list idx val -- list' )` | 1-based element replacement |
| `GETI` | `( list idx -- list idx' elem )` | GET with auto-incremented index |
| `PUTI` | `( list idx val -- list' idx' )` | PUT with auto-incremented index |
| `HEAD` | `( list -- first )` | First element (also works on strings) |
| `TAIL` | `( list -- rest )` | All but first element (also works on strings) |
| `SIZE` | `( list -- n )` | Element count (also works on strings and matrices) |
| `POS` | `( list elem -- idx )` | Find element, return 1-based index or 0 (also works on strings) |
| `SUB` | `( list start end -- sublist )` | Sub-list by 1-based indices (also works on strings) |
| `ADD` | `( list elem -- list' )` | Append element to list |
| `REVLIST` | `( list -- list' )` | Reverse list |
| `SORT` | `( list -- list' )` | Sort homogeneous numeric or string list |

### Higher-Order Operations

| Command | Stack Effect | Description |
|---------|-------------|-------------|
| `MAP` | `( list prog -- list' )` | Apply program to each element, collect results |
| `FILTER` | `( list prog -- list' )` | Keep elements where program returns truthy |
| `STREAM` | `( list prog -- result )` | Reduce list with binary program |
| `DOLIST` | `( list1 ... listN N prog -- list' )` | Apply program to corresponding elements of N lists |
| `SEQ` | `( start step count prog -- list )` | Generate list by applying program to sequence values |
| `DOSUBS` | `( list N prog -- list' )` | Apply program to sliding windows of N elements |
| `ZIP` | `( list1 ... listN N -- list' )` | Transpose N lists into list of lists |

### Set Operations

| Command | Stack Effect | Description |
|---------|-------------|-------------|
| `UNION` | `( list1 list2 -- list )` | Set union (preserve order, no duplicates) |
| `INTERSECT` | `( list1 list2 -- list )` | Set intersection |
| `DIFFERENCE` | `( list1 list2 -- list )` | Set difference (elements in list1 not in list2) |

### Arithmetic Overloads

- `{ a b } + { c d }` — element-wise addition (matching lengths)
- `scalar + { a b }` / `{ a b } + scalar` — broadcast scalar
- Same for `-`, `*`, `/`
- `{ a b } NEG` — negate each element

**Examples:**

```
{ 1 2 3 } { 4 5 6 } +           => { 5 7 9 }
{ 1 2 3 } 10 *                   => { 10 20 30 }
{ 5 3 1 4 } SORT                 => { 1 3 4 5 }
{ 1 2 3 } << 2 * >> MAP          => { 2 4 6 }
{ 1 2 3 4 } << + >> STREAM       => 10
{ 1 2 3 } { 2 3 4 } UNION        => { 1 2 3 4 }
```

---

## Matrix/Vector Operations

Matrices use `[[ ]]` delimiters with `][` row separators. A 1-row matrix acts as a vector. Elements must be numeric (Integer, Real, Rational, Complex) or symbolic (Name, Symbol). Indexing via `{ row col }` lists.

### Construction

| Command | Stack Effect | Description |
|---------|-------------|-------------|
| `->V2` | `( x y -- [[ x y ]] )` | Construct 2D vector |
| `->V3` | `( x y z -- [[ x y z ]] )` | Construct 3D vector |
| `V->` | `( [[ x y z ]] -- x y z )` | Explode vector onto stack |
| `CON` | `( { dims } val -- matrix )` | Constant matrix/vector |
| `IDN` | `( n -- matrix )` | n×n identity matrix |
| `RDM` | `( matrix { dims } -- matrix' )` | Redimension matrix |

### Access & Properties

| Command | Stack Effect | Description |
|---------|-------------|-------------|
| `GET` | `( matrix { r c } -- elem )` | Element access by row/col |
| `PUT` | `( matrix { r c } val -- matrix' )` | Element replacement |
| `SIZE` | `( matrix -- { rows cols } )` | Matrix dimensions |
| `TRN` | `( matrix -- matrix' )` | Transpose |

### Linear Algebra

| Command | Stack Effect | Description |
|---------|-------------|-------------|
| `DET` | `( matrix -- value )` | Determinant (numeric or symbolic) |
| `INV` | `( matrix -- matrix' )` | Matrix inverse (numeric only) |
| `CROSS` | `( vec3 vec3 -- vec3 )` | Cross product (3D vectors) |
| `DOT` | `( vec vec -- scalar )` | Dot product |
| `ABS` | `( vec -- norm )` | Euclidean norm (numeric only) |

### Arithmetic Overloads

- `matrix + matrix` — element-wise addition (matching dimensions)
- `matrix * matrix` — matrix multiplication
- `scalar * matrix` / `matrix * scalar` — scalar multiplication
- `matrix * vector` — matrix-vector product
- `matrix NEG` — negate all elements

Symbolic matrices support arithmetic (`+`, `-`, `*`), `DET`, `CROSS`, `DOT`, and `TRN`. Results are unsimplified expression strings.

**Examples:**

```
[[ 1 2 ][ 3 4 ]] [[ 5 6 ][ 7 8 ]] *    => [[ 19 22 ][ 43 50 ]]
[[ 1 2 ][ 3 4 ]] DET                    => -2
3 IDN                                    => [[ 1 0 0 ][ 0 1 0 ][ 0 0 1 ]]
[[ 1 0 0 ]] [[ 0 1 0 ]] CROSS           => [[ 0 0 1 ]]
[[ 1 2 3 ]] [[ 4 5 6 ]] DOT             => 32
[[ 'a' 'b' ][ 'c' 'd' ]] DET           => 'a*d-b*c'
```

---

## Symbolic Manipulation

Commands for inspecting, decomposing, and reconstructing symbolic expressions. These commands operate on the expression tree stored as an infix string inside a Symbol, using the expression tokenizer to work at the structural level.

### Symbolic Pass-Through

When a symbolic operand (Name or Symbol) is passed to a math command, the command produces a Symbol wrapping the operation instead of evaluating numerically. This allows building symbolic expressions compositionally.

**Commands with symbolic pass-through:** `+`, `-`, `*`, `/`, `^`, `SQ`, `SQRT`, `SIN`, `COS`, `TAN`, `ASIN`, `ACOS`, `ATAN`, `EXP`, `LN`, `ABS`, `NEG`, `INV`, `IFT`, `IFTE`

```
'X' SQ           => 'X^2'
'X+1' SQ         => '(X+1)^2'
'X' 2 ^          => 'X^2'
'A+B' SQRT       => 'SQRT(A+B)'
'X' 'Y' +        => 'X+Y'
'X' 'Y' IFT      => 'IFT(X, Y)'
'A' 'B' 'C' IFTE => 'IFTE(A, B, C)'
5 SQ              => 25               (numeric input unchanged)
```

### Expression Syntax

Expressions inside Symbols support standard infix notation with operator precedence (`+` `-` at precedence 1, `*` `/` at precedence 2, `^` at precedence 3), parentheses for grouping, function calls (`FUNC(arg)`), and comma-separated multi-argument function calls (`FUNC(a, b, c)`).

### Substitution

| Command | Stack Effect | Description |
|---------|-------------|-------------|
| `SUBST` | `( 'expr' 'var' 'replacement' -- 'result' )` | Replace all occurrences of variable `var` in `expr` with `replacement` |

SUBST operates at the token level: it tokenizes the expression, replaces matching Name tokens, and reconstructs the string. Compound replacements are automatically parenthesized when inserted into higher-precedence contexts.

```
'X+1' 'X' 'Y' SUBST          => 'Y+1'
'X*2' 'X' 'A+B' SUBST        => '(A+B)*2'       (parens added)
'X+X' 'X' 'Y' SUBST          => 'Y+Y'           (all occurrences)
'X+1' 'Y' 'Z' SUBST          => 'X+1'           (no match)
'IFTE(X, Y, Z)' 'X' 'A' SUBST => 'IFTE(A, Y, Z)'
```

### Auxiliary Stash Stack

The stash is a hidden LIFO auxiliary stack stored in SQLite alongside the main stack. Items are stored and retrieved in groups, enabling structured decomposition/reconstruction workflows. The stash participates in undo/redo — all stash mutations are transactional.

| Command | Stack Effect | Description |
|---------|-------------|-------------|
| `STASH` | `( item -- )` | Pop 1 item from stack, store as single-item group on stash |
| `STASHN` | `( item₁...itemₙ n -- )` | Pop count from level 1, pop N items, store as group on stash |
| `UNSTASH` | `( -- item₁...itemₙ )` | Pop most recent group from stash, push items to stack in original order |

```
42 STASH                       (stack empty; stash: [42])
UNSTASH                   => 42 (stash empty)
1 2 3 3 STASHN                (stack empty; stash: [1, 2, 3])
UNSTASH                   => 1 2 3
```

### Decomposition and Reconstruction

| Command | Stack Effect | Description |
|---------|-------------|-------------|
| `EXPLODE` | `( 'expr' -- operands... « op » )` | Decompose top-level operation into operands and operator program |
| `ASSEMBLE` | `( operands... -- result )` | Loop: while stash non-empty, UNSTASH then EVAL level 1 |

**EXPLODE** peels one layer off a symbolic expression:

- **Binary operators:** finds the lowest-precedence operator at paren depth 0 and splits into left operand, right operand, and operator program.
- **Function calls:** `FUNC(args...)` becomes the arguments pushed individually, followed by a `« FUNC »` program.
- Operands are pushed as their natural type: numeric literals as Integer/Real, simple names as Name, sub-expressions as Symbol.
- The operator is always pushed as a single-token Program so that EVAL naturally executes it.

```
'A+B' EXPLODE              => 'A' 'B' « + »
'X*Y-3' EXPLODE            => 'X*Y' 3 « - »
'SIN(X)' EXPLODE           => 'X' « SIN »
'IFTE(A, B, C)' EXPLODE    => 'A' 'B' 'C' « IFTE »
'SQ(X)+3' EXPLODE          => 'SQ(X)' 3 « + »
```

**ASSEMBLE** is the inverse of a multi-level EXPLODE+STASH sequence. It repeatedly unstashes a group and EVALs the top item (the operator program), rebuilding the expression from the leaves up.

```
'SQ(X)+3' EXPLODE          => 'SQ(X)' 3 « + »
2 STASHN                       stash: [3, « + »]
EXPLODE                     => 'X' « SQ »
STASH                          stash: [« SQ »], [3, « + »]
DROP 'Y+1'                  => 'Y+1'
ASSEMBLE                    => 'SQ(Y+1)+3'
```

---

## Display Settings

Commands for controlling how numbers are displayed. Settings are stored in the `meta` table and persist across sessions.

### Number Format Modes

| Command | Stack Effect | Description |
|---------|-------------|-------------|
| `STD`   | `( -- )` | Restore full-precision display (default) |
| `FIX`   | `( n -- )` | Fixed-point display with n decimal places (0-11) |
| `SCI`   | `( n -- )` | Scientific notation with n significant digits (0-11) |
| `ENG`   | `( n -- )` | Engineering notation (exponent multiple of 3) with n digits (0-11) |

**Examples:**

```
3.14159265 4 FIX            => 3.1416
12345.6789 3 SCI            => 1.235E4
12345.6789 3 ENG            => 12.346E3
STD 3.14159265              => 3.14159265
```

### Coordinate Display Modes

| Command | Stack Effect | Description |
|---------|-------------|-------------|
| `RECT`  | `( -- )` | Rectangular Complex display `(re, im)` (default) |
| `POLAR` | `( -- )` | Polar Complex display `(r, ∠θ)` |
| `SPHERICAL` | `( -- )` | Spherical coordinate mode |

**Examples:**

```
(3., 4.) POLAR 4 FIX        => (5.0000, ∠0.9273)
RECT (3., 4.)               => (3., 4.)
```

Format settings persist across `lpr_exec` calls and are stored in `meta` keys `number_format`, `format_digits`, and `coordinate_mode`.

---

## Flag Registry

An extensible, typed flag registry backed by a dedicated `flags` table. Flags are named (string keys) and can hold booleans, integers, reals, or strings — unlike the HP 50g's fixed 128-flag bitfield.

### Boolean Flags

| Command | Stack Effect | Description |
|---------|-------------|-------------|
| `SF`    | `( 'name' -- )` | Set named flag to true |
| `CF`    | `( 'name' -- )` | Clear named flag (set to false) |
| `FS?`   | `( 'name' -- n )` | Push 1 if flag exists and is true, 0 otherwise |
| `FC?`   | `( 'name' -- n )` | Push 1 if flag is absent or false, 0 otherwise |

### Typed Flags

| Command | Stack Effect | Description |
|---------|-------------|-------------|
| `SFLAG` | `( value 'name' -- )` | Store typed flag (Integer, Real, or String) |
| `RFLAG` | `( 'name' -- value )` | Recall typed flag with original type; error if undefined |

### Bulk Flag Access

| Command | Stack Effect | Description |
|---------|-------------|-------------|
| `STOF`  | `( -- list )` | Push List of all flags as `{ name value }` pairs |
| `RCLF`  | `( list -- )` | Replace all flags from a saved list |

**Examples:**

```
'exact_mode' SF              set boolean flag
'exact_mode' FS?             => 1
'exact_mode' CF
'exact_mode' FS?             => 0
1000 'max_iterations' SFLAG  store typed flag
'max_iterations' RFLAG       => 1000
STOF                         => { { "exact_mode" 0 } { "max_iterations" 1000 } }
```

---

## Conversions

| Command | Stack Effect | Description |
|---------|-------------|-------------|
| `->Q`   | `( x -- rational )` | Approximate Real as nearest Rational (continued fraction) |
| `HMS->` | `( h.mmss -- hours )` | Convert H.MMSSss format to decimal hours |
| `->HMS` | `( hours -- h.mmss )` | Convert decimal hours to H.MMSSss format |

**Examples:**

```
0.5 ->Q                      => 1/2
3.14159265358979 ->Q          => 355/113 (or similar)
2.3000 HMS->                  => 2.5
2.5 ->HMS                     => 2.3
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
| **List** | `{ 1 2 3 }` | Heterogeneous ordered collection; nesting allowed |
| **Matrix** | `[[ 1 2 ][ 3 4 ]]` | Row-major numeric/symbolic matrix; 1-row = vector |

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
| 41 | `CD` | Filesystem | 1 | Change into subdirectory |
| 42 | `UPDIR` | Filesystem | 0 | Move up one directory level |
| 43 | `PGDIR` | Filesystem | 1 | Delete subdirectory tree |
| 44 | `VARS` | Filesystem | 0 | List directory contents |
| 45 | `EVAL` | Program | 1 | Evaluate object (program, name, or symbol expression) |
| 46 | `IFT` | Program | 2 | Conditional (if-then) |
| 47 | `IFTE` | Program | 3 | Conditional (if-then-else) |
| 48 | `->` / `→` | Local Binding | *runstream* | Bind stack values to local variables, execute body |
| 49 | `IF...THEN...END` | Control Flow | *runstream* | Conditional execution |
| 50 | `CASE...END` | Control Flow | *runstream* | Multi-way branch |
| 51 | `FOR...NEXT` | Control Flow | 2 + *runstream* | Counted loop with variable |
| 52 | `FOR...STEP` | Control Flow | 2 + *runstream* | Counted loop with variable and custom step |
| 53 | `START...NEXT` | Control Flow | 2 + *runstream* | Counted loop |
| 54 | `START...STEP` | Control Flow | 2 + *runstream* | Counted loop with custom step |
| 55 | `WHILE...REPEAT...END` | Control Flow | *runstream* | Pre-test loop |
| 56 | `DO...UNTIL...END` | Control Flow | *runstream* | Post-test loop |
| 57 | `AND` | Logic | 2 | Boolean AND |
| 58 | `OR` | Logic | 2 | Boolean OR |
| 59 | `NOT` | Logic | 1 | Boolean NOT |
| 60 | `XOR` | Logic | 2 | Boolean XOR |
| 61 | `BAND` | Bitwise | 2 | Bitwise AND |
| 62 | `BOR` | Bitwise | 2 | Bitwise OR |
| 63 | `BXOR` | Bitwise | 2 | Bitwise XOR |
| 64 | `BNOT` | Bitwise | 1 | Bitwise NOT |
| 65 | `SL` | Bitwise | 2 | Shift left |
| 66 | `SR` | Bitwise | 2 | Shift right |
| 67 | `ASR` | Bitwise | 2 | Arithmetic shift right |
| 68 | `SAME` | Logic | 2 | Deep structural equality |
| 69 | `DEG` | Angle Mode | 0 | Set degrees mode |
| 70 | `RAD` | Angle Mode | 0 | Set radians mode |
| 71 | `GRAD` | Angle Mode | 0 | Set gradians mode |
| 72 | `SIN` | Transcendental | 1 | Sine |
| 73 | `COS` | Transcendental | 1 | Cosine |
| 74 | `TAN` | Transcendental | 1 | Tangent |
| 75 | `ASIN` | Transcendental | 1 | Inverse sine |
| 76 | `ACOS` | Transcendental | 1 | Inverse cosine |
| 77 | `ATAN` | Transcendental | 1 | Inverse tangent |
| 78 | `ATAN2` | Transcendental | 2 | Two-argument inverse tangent |
| 79 | `EXP` | Transcendental | 1 | Natural exponential |
| 80 | `LN` | Transcendental | 1 | Natural logarithm |
| 81 | `LOG` | Transcendental | 1 | Common logarithm |
| 82 | `ALOG` | Transcendental | 1 | Common antilogarithm |
| 83 | `SQRT` | Transcendental | 1 | Square root |
| 84 | `SQ` | Transcendental | 1 | Square |
| 85 | `PI` | Constant | 0 | Push π |
| 86 | `E` | Constant | 0 | Push e |
| 87 | `FLOOR` | Rounding | 1 | Floor |
| 88 | `CEIL` | Rounding | 1 | Ceiling |
| 89 | `IP` | Rounding | 1 | Integer part |
| 90 | `FP` | Rounding | 1 | Fractional part |
| 91 | `MIN` | Utility | 2 | Minimum |
| 92 | `MAX` | Utility | 2 | Maximum |
| 93 | `SIGN` | Utility | 1 | Sign |
| 94 | `!` | Combinatorics | 1 | Factorial |
| 95 | `COMB` | Combinatorics | 2 | Combinations |
| 96 | `PERM` | Combinatorics | 2 | Permutations |
| 97 | `%` | Percentage | 2 | Percentage |
| 98 | `%T` | Percentage | 2 | Percent of total |
| 99 | `%CH` | Percentage | 2 | Percent change |
| 100 | `D->R` | Angle Conv | 1 | Degrees to radians |
| 101 | `R->D` | Angle Conv | 1 | Radians to degrees |
| 102 | `SIZE` | String/List/Matrix | 1 | Length, element count, or dimensions |
| 103 | `HEAD` | String/List | 1 | First character or element |
| 104 | `TAIL` | String/List | 1 | All but first |
| 105 | `SUB` | String/List | 3 | Substring or sub-list |
| 106 | `POS` | String/List | 2 | Find position |
| 107 | `REPL` | String | 3 | Replace first occurrence |
| 108 | `NUM` | String | 1 | Char to codepoint |
| 109 | `CHR` | String | 1 | Codepoint to char |
| 110 | `SUBST` | Symbolic | 3 | Substitute variable in expression |
| 111 | `STASH` | Symbolic | 1 | Stash one item |
| 112 | `STASHN` | Symbolic | n+1 | Stash N items as a group |
| 113 | `UNSTASH` | Symbolic | 0 | Restore most recent stash group |
| 114 | `EXPLODE` | Symbolic | 1 | Decompose top-level operation |
| 115 | `ASSEMBLE` | Symbolic | varies | Reassemble from stash groups |
| 116 | `LIST->` | List | 1 | Explode list with count |
| 117 | `->LIST` | List | n+1 | Collect N items into list |
| 118 | `GET` | List/Matrix | 2 | Element access (1-based) |
| 119 | `PUT` | List/Matrix | 3 | Element replacement (1-based) |
| 120 | `GETI` | List | 2 | GET with auto-increment |
| 121 | `PUTI` | List | 3 | PUT with auto-increment |
| 122 | `ADD` | List | 2 | Append element |
| 123 | `REVLIST` | List | 1 | Reverse list |
| 124 | `SORT` | List | 1 | Sort homogeneous list |
| 125 | `MAP` | List | 2 | Apply program to each element |
| 126 | `FILTER` | List | 2 | Keep elements passing test |
| 127 | `STREAM` | List | 2 | Reduce with binary program |
| 128 | `DOLIST` | List | n+2 | Apply program to N lists |
| 129 | `SEQ` | List | 4 | Generate list from sequence |
| 130 | `DOSUBS` | List | 3 | Sliding window application |
| 131 | `ZIP` | List | n+1 | Transpose N lists |
| 132 | `UNION` | List | 2 | Set union |
| 133 | `INTERSECT` | List | 2 | Set intersection |
| 134 | `DIFFERENCE` | List | 2 | Set difference |
| 135 | `->V2` | Matrix | 2 | Construct 2D vector |
| 136 | `->V3` | Matrix | 3 | Construct 3D vector |
| 137 | `V->` | Matrix | 1 | Explode vector |
| 138 | `CON` | Matrix | 2 | Constant matrix |
| 139 | `IDN` | Matrix | 1 | Identity matrix |
| 140 | `RDM` | Matrix | 2 | Redimension matrix |
| 141 | `TRN` | Matrix | 1 | Transpose |
| 142 | `DET` | Matrix | 1 | Determinant |
| 143 | `CROSS` | Matrix | 2 | Cross product |
| 144 | `DOT` | Matrix | 2 | Dot product |
| 145 | `STD` | Display | 0 | Standard full-precision display |
| 146 | `FIX` | Display | 1 | Fixed-point display |
| 147 | `SCI` | Display | 1 | Scientific notation |
| 148 | `ENG` | Display | 1 | Engineering notation |
| 149 | `RECT` | Display | 0 | Rectangular coordinate mode |
| 150 | `POLAR` | Display | 0 | Polar coordinate mode |
| 151 | `SPHERICAL` | Display | 0 | Spherical coordinate mode |
| 152 | `SF` | Flags | 1 | Set boolean flag |
| 153 | `CF` | Flags | 1 | Clear boolean flag |
| 154 | `FS?` | Flags | 1 | Test flag set |
| 155 | `FC?` | Flags | 1 | Test flag clear |
| 156 | `SFLAG` | Flags | 2 | Store typed flag |
| 157 | `RFLAG` | Flags | 1 | Recall typed flag |
| 158 | `STOF` | Flags | 0 | Save all flags to list |
| 159 | `RCLF` | Flags | 1 | Restore flags from list |
| 160 | `->Q` | Conversion | 1 | Real to Rational |
| 161 | `HMS->` | Conversion | 1 | H.MMSSss to decimal hours |
| 162 | `->HMS` | Conversion | 1 | Decimal hours to H.MMSSss |
