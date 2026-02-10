## ADDED Requirements

### Requirement: DEG Command
`DEG` SHALL set the angle mode to degrees by writing `"DEG"` to the `meta` table key `angle_mode`.

#### Scenario: Set degree mode
- **WHEN** `DEG` is executed
- **THEN** the angle mode is set to degrees and subsequent trig functions interpret arguments as degrees

### Requirement: RAD Command
`RAD` SHALL set the angle mode to radians by writing `"RAD"` to the `meta` table key `angle_mode`.

#### Scenario: Set radian mode
- **WHEN** `RAD` is executed
- **THEN** the angle mode is set to radians (the default)

### Requirement: GRAD Command
`GRAD` SHALL set the angle mode to gradians by writing `"GRAD"` to the `meta` table key `angle_mode`.

#### Scenario: Set gradian mode
- **WHEN** `GRAD` is executed
- **THEN** the angle mode is set to gradians and subsequent trig functions interpret arguments as gradians

### Requirement: SIN Command
`SIN` SHALL pop a numeric value and push its sine. The input is interpreted according to the current angle mode (RAD, DEG, or GRAD) and converted to radians before computation. Integers and Rationals are promoted to Real first.

#### Scenario: Sin of zero in radians
- **WHEN** angle mode is RAD, Real(0.0) is on the stack, and `SIN` is executed
- **THEN** Real(0.0) is on the stack

#### Scenario: Sin of pi/2 in radians
- **WHEN** angle mode is RAD, a Real approximating pi/2 is on the stack, and `SIN` is executed
- **THEN** Real(1.0) (approximately) is on the stack

#### Scenario: Sin of 90 in degrees
- **WHEN** angle mode is DEG, Integer(90) is on the stack, and `SIN` is executed
- **THEN** Real(1.0) (approximately) is on the stack

#### Scenario: Sin of 100 in gradians
- **WHEN** angle mode is GRAD, Integer(100) is on the stack, and `SIN` is executed
- **THEN** Real(1.0) (approximately) is on the stack

### Requirement: COS Command
`COS` SHALL pop a numeric value and push its cosine. Input is interpreted according to the current angle mode.

#### Scenario: Cos of zero
- **WHEN** Real(0.0) is on the stack and `COS` is executed
- **THEN** Real(1.0) is on the stack

#### Scenario: Cos of 60 degrees
- **WHEN** angle mode is DEG, Integer(60) is on the stack, and `COS` is executed
- **THEN** Real(0.5) (approximately) is on the stack

### Requirement: TAN Command
`TAN` SHALL pop a numeric value and push its tangent. Input is interpreted according to the current angle mode.

#### Scenario: Tan of zero
- **WHEN** Real(0.0) is on the stack and `TAN` is executed
- **THEN** Real(0.0) is on the stack

### Requirement: ASIN Command
`ASIN` SHALL pop a numeric value and push its arcsine. The result is expressed in the current angle mode.

#### Scenario: Asin of zero
- **WHEN** Real(0.0) is on the stack and `ASIN` is executed
- **THEN** Real(0.0) is on the stack

#### Scenario: Asin of 1 in degree mode
- **WHEN** angle mode is DEG, Real(1.0) is on the stack, and `ASIN` is executed
- **THEN** Real(90.0) (approximately) is on the stack

### Requirement: ACOS Command
`ACOS` SHALL pop a numeric value and push its arccosine. The result is expressed in the current angle mode.

#### Scenario: Acos of one
- **WHEN** Real(1.0) is on the stack and `ACOS` is executed
- **THEN** Real(0.0) is on the stack

### Requirement: ATAN Command
`ATAN` SHALL pop a numeric value and push its arctangent. The result is expressed in the current angle mode.

#### Scenario: Atan of zero
- **WHEN** Real(0.0) is on the stack and `ATAN` is executed
- **THEN** Real(0.0) is on the stack

### Requirement: ATAN2 Command
`ATAN2` SHALL pop y (level 2) and x (level 1) and push atan2(y, x). The result is expressed in the current angle mode.

#### Scenario: Atan2 of (1, 1)
- **WHEN** Real(1.0) and Real(1.0) are on the stack and `ATAN2` is executed
- **THEN** a Real approximating pi/4 is on the stack

### Requirement: EXP Command
`EXP` SHALL pop a numeric value and push e raised to that power.

#### Scenario: Exp of zero
- **WHEN** Real(0.0) is on the stack and `EXP` is executed
- **THEN** Real(1.0) is on the stack

#### Scenario: Exp of one
- **WHEN** Integer(1) is on the stack and `EXP` is executed
- **THEN** a Real approximating 2.71828 is on the stack

### Requirement: LN Command
`LN` SHALL pop a numeric value and push its natural logarithm.

#### Scenario: Ln of one
- **WHEN** Real(1.0) is on the stack and `LN` is executed
- **THEN** Real(0.0) is on the stack

### Requirement: LOG Command
`LOG` SHALL pop a numeric value and push its base-10 logarithm.

#### Scenario: Log of 100
- **WHEN** Integer(100) is on the stack and `LOG` is executed
- **THEN** Real(2.0) (approximately) is on the stack

### Requirement: ALOG Command
`ALOG` SHALL pop a numeric value and push 10 raised to that power.

#### Scenario: Alog of 2
- **WHEN** Integer(2) is on the stack and `ALOG` is executed
- **THEN** Real(100.0) (approximately) is on the stack

### Requirement: SQRT Command
`SQRT` SHALL pop a numeric value and push its square root.

#### Scenario: Sqrt of 4
- **WHEN** Integer(4) is on the stack and `SQRT` is executed
- **THEN** Real(2.0) is on the stack

#### Scenario: Sqrt of 2
- **WHEN** Integer(2) is on the stack and `SQRT` is executed
- **THEN** a Real approximating 1.41421 is on the stack

### Requirement: SQ Command
`SQ` SHALL pop a numeric value and push its square. Preserves type (Integer in, Integer out).

#### Scenario: Sq of integer
- **WHEN** Integer(5) is on the stack and `SQ` is executed
- **THEN** Integer(25) is on the stack

#### Scenario: Sq of real
- **WHEN** Real(2.5) is on the stack and `SQ` is executed
- **THEN** Real(6.25) is on the stack

### Requirement: PI Constant
`PI` SHALL push the mathematical constant pi as a Real.

#### Scenario: Push PI
- **WHEN** `PI` is executed
- **THEN** a Real approximating 3.14159265358979 is on the stack

### Requirement: E Constant
`E` SHALL push Euler's number as a Real.

#### Scenario: Push E
- **WHEN** `E` is executed
- **THEN** a Real approximating 2.71828182845905 is on the stack

### Requirement: FLOOR Command
`FLOOR` SHALL pop a numeric value and push the largest integer not greater than it.

#### Scenario: Floor of positive real
- **WHEN** Real(3.7) is on the stack and `FLOOR` is executed
- **THEN** Integer(3) is on the stack

#### Scenario: Floor of negative real
- **WHEN** Real(-2.3) is on the stack and `FLOOR` is executed
- **THEN** Integer(-3) is on the stack

### Requirement: CEIL Command
`CEIL` SHALL pop a numeric value and push the smallest integer not less than it.

#### Scenario: Ceil of positive real
- **WHEN** Real(3.2) is on the stack and `CEIL` is executed
- **THEN** Integer(4) is on the stack

### Requirement: IP Command
`IP` (Integer Part) SHALL pop a numeric value and push its integer part (truncation toward zero).

#### Scenario: IP of positive real
- **WHEN** Real(3.7) is on the stack and `IP` is executed
- **THEN** Integer(3) is on the stack

#### Scenario: IP of negative real
- **WHEN** Real(-3.7) is on the stack and `IP` is executed
- **THEN** Integer(-3) is on the stack

### Requirement: FP Command
`FP` (Fractional Part) SHALL pop a numeric value and push its fractional part (value - IP(value)).

#### Scenario: FP of positive real
- **WHEN** Real(3.7) is on the stack and `FP` is executed
- **THEN** Real(0.7) (approximately) is on the stack

### Requirement: MIN Command
`MIN` SHALL pop two numeric values and push the smaller one.

#### Scenario: Min of two integers
- **WHEN** Integer(5) and Integer(3) are on the stack and `MIN` is executed
- **THEN** Integer(3) is on the stack

### Requirement: MAX Command
`MAX` SHALL pop two numeric values and push the larger one.

#### Scenario: Max of two integers
- **WHEN** Integer(5) and Integer(3) are on the stack and `MAX` is executed
- **THEN** Integer(5) is on the stack

### Requirement: SIGN Command
`SIGN` SHALL pop a numeric value and push -1, 0, or 1 (as Integer) based on its sign.

#### Scenario: Sign of positive
- **WHEN** Integer(42) is on the stack and `SIGN` is executed
- **THEN** Integer(1) is on the stack

#### Scenario: Sign of zero
- **WHEN** Integer(0) is on the stack and `SIGN` is executed
- **THEN** Integer(0) is on the stack

#### Scenario: Sign of negative
- **WHEN** Real(-3.14) is on the stack and `SIGN` is executed
- **THEN** Integer(-1) is on the stack

### Requirement: COMB Command
`COMB` SHALL pop k (level 1) and n (level 2), both non-negative integers, and push the binomial coefficient C(n, k).

#### Scenario: C(5, 2)
- **WHEN** Integer(5) and Integer(2) are on the stack and `COMB` is executed
- **THEN** Integer(10) is on the stack

### Requirement: PERM Command
`PERM` SHALL pop k (level 1) and n (level 2), both non-negative integers, and push the permutation count P(n, k) = n! / (n-k)!.

#### Scenario: P(5, 2)
- **WHEN** Integer(5) and Integer(2) are on the stack and `PERM` is executed
- **THEN** Integer(20) is on the stack

### Requirement: Factorial Command
`!` SHALL pop a non-negative integer and push its factorial.

#### Scenario: 5 factorial
- **WHEN** Integer(5) is on the stack and `!` is executed
- **THEN** Integer(120) is on the stack

#### Scenario: 0 factorial
- **WHEN** Integer(0) is on the stack and `!` is executed
- **THEN** Integer(1) is on the stack

### Requirement: Percent Command
`%` SHALL pop a percentage (level 1) and a base (level 2) and push base * percentage / 100. Both arguments remain numeric.

#### Scenario: 25% of 200
- **WHEN** Integer(200) and Integer(25) are on the stack and `%` is executed
- **THEN** Integer(50) is on the stack

### Requirement: %T Command
`%T` (Percent Total) SHALL pop a part (level 1) and a total (level 2) and push 100 * part / total.

#### Scenario: What percent is 50 of 200
- **WHEN** Integer(200) and Integer(50) are on the stack and `%T` is executed
- **THEN** Integer(25) is on the stack

### Requirement: %CH Command
`%CH` (Percent Change) SHALL pop a new value (level 1) and an old value (level 2) and push 100 * (new - old) / old.

#### Scenario: Percent change from 200 to 250
- **WHEN** Integer(200) and Integer(250) are on the stack and `%CH` is executed
- **THEN** Integer(25) is on the stack

### Requirement: D->R Command
`D->R` SHALL pop a value in degrees and push the equivalent in radians (value * pi / 180).

#### Scenario: 180 degrees to radians
- **WHEN** Integer(180) is on the stack and `D->R` is executed
- **THEN** a Real approximating pi is on the stack

### Requirement: R->D Command
`R->D` SHALL pop a value in radians and push the equivalent in degrees (value * 180 / pi).

#### Scenario: Pi radians to degrees
- **WHEN** a Real approximating pi is on the stack and `R->D` is executed
- **THEN** Real(180.0) (approximately) is on the stack
