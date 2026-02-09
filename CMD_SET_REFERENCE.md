# LPR Command Set Reference

Complete reference for all commands in the LPR (Lenny Polish Reverse) runtime. LPR is an RPL virtual machine inspired by the HP 50g calculator, using postfix (reverse Polish) notation where operands are pushed onto a stack before commands consume them.

All commands are **case-insensitive** (internally uppercased). Stack levels are numbered from the top: level 1 is the topmost item, level 2 is below it, etc.

---

## Stack Operations

| Command | Stack Effect | Description |
|---------|-------------|-------------|
| `DUP`   | `( a -- a a )` | Duplicate the top item |
| `DROP`  | `( a -- )` | Remove the top item |
| `SWAP`  | `( a b -- b a )` | Swap the top two items |
| `OVER`  | `( a b -- a b a )` | Copy the second item to the top |
| `ROT`   | `( a b c -- b c a )` | Rotate the third item to the top |
| `DEPTH` | `( -- n )` | Push the current stack depth as an Integer |
| `CLEAR` | `( ... -- )` | Remove all items from the stack |

**Examples:**

```
5 DUP          => 5 5
3 7 SWAP       => 7 3
1 2 3 ROT      => 2 3 1
1 2 3 DEPTH    => 1 2 3 3
```

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
| `EVAL`  | `( obj -- ... )` | Evaluate a program or recall a variable |
| `IFT`   | `( then cond -- ... )` | If-then: execute `then` when `cond` is truthy |
| `IFTE`  | `( else then cond -- ... )` | If-then-else: execute `then` or `else` based on `cond` |

### EVAL

- **Program**: executes the program's tokens
- **Name**: recalls the variable; if it contains a Program, executes it
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
| **Symbol** | `'X^2 + 1'` | Algebraic expression (single-quoted, contains operators) |
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
| 2 | `DROP` | Stack | 1 | Remove top |
| 3 | `SWAP` | Stack | 2 | Swap top two |
| 4 | `OVER` | Stack | 2 | Copy second to top |
| 5 | `ROT` | Stack | 3 | Rotate third to top |
| 6 | `DEPTH` | Stack | 0 | Push stack depth |
| 7 | `CLEAR` | Stack | 0 | Clear entire stack |
| 8 | `+` | Arithmetic | 2 | Addition |
| 9 | `-` | Arithmetic | 2 | Subtraction |
| 10 | `*` | Arithmetic | 2 | Multiplication |
| 11 | `/` | Arithmetic | 2 | Division |
| 12 | `NEG` | Arithmetic | 1 | Negation |
| 13 | `INV` | Arithmetic | 1 | Reciprocal |
| 14 | `ABS` | Arithmetic | 1 | Absolute value |
| 15 | `MOD` | Arithmetic | 2 | Modulo (Integer only) |
| 16 | `==` | Comparison | 2 | Equal |
| 17 | `!=` | Comparison | 2 | Not equal |
| 18 | `<` | Comparison | 2 | Less than |
| 19 | `>` | Comparison | 2 | Greater than |
| 20 | `<=` | Comparison | 2 | Less or equal |
| 21 | `>=` | Comparison | 2 | Greater or equal |
| 22 | `TYPE` | Type | 1 | Type tag |
| 23 | `->NUM` | Type | 1 | Convert to Real |
| 24 | `->STR` | Type | 1 | Convert to String |
| 25 | `STR->` | Type | 1 | Evaluate String as code |
| 26 | `STO` | Filesystem | 2 | Store variable |
| 27 | `RCL` | Filesystem | 1 | Recall variable |
| 28 | `PURGE` | Filesystem | 1 | Delete variable |
| 29 | `HOME` | Filesystem | 0 | Go to home directory |
| 30 | `PATH` | Filesystem | 0 | Current directory path |
| 31 | `CRDIR` | Filesystem | 1 | Create subdirectory |
| 32 | `VARS` | Filesystem | 0 | List variables |
| 33 | `EVAL` | Program | 1 | Evaluate object |
| 34 | `IFT` | Program | 2 | Conditional (if-then) |
| 35 | `IFTE` | Program | 3 | Conditional (if-then-else) |
