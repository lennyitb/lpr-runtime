## ADDED Requirements

### Requirement: Number Format Mode
The runtime SHALL support four number format modes — STD, FIX, SCI, ENG — controlled by corresponding commands. FIX, SCI, and ENG SHALL pop an Integer (0–11) specifying the number of displayed digits. STD SHALL take no argument and restore default full-precision display. The active mode and digit count SHALL be stored in the `meta` table as `number_format` and `format_digits`.

#### Scenario: FIX mode
- **WHEN** `FIX 4` is executed and `3.14159265` is on the stack
- **THEN** `lpr_repr` displays `3.1416`

#### Scenario: SCI mode
- **WHEN** `SCI 3` is executed and `12345.6789` is on the stack
- **THEN** `lpr_repr` displays `1.235E4`

#### Scenario: ENG mode
- **WHEN** `ENG 3` is executed and `12345.6789` is on the stack
- **THEN** `lpr_repr` displays `12.346E3` (exponent is a multiple of 3)

#### Scenario: STD mode restores full precision
- **WHEN** `STD` is executed after a previous `FIX 2`
- **THEN** `lpr_repr` displays numbers at full available precision

### Requirement: Formatted Display
The `repr()` function SHALL accept an optional `DisplaySettings` parameter. When provided, numeric types (Real, Rational, Integer, Complex) SHALL be formatted according to the active number format mode and coordinate mode. The existing no-argument `repr(obj)` SHALL continue to produce STD output for backward compatibility.

#### Scenario: lpr_repr reads active settings
- **WHEN** `FIX 2` has been executed and the stack contains `3.14159`
- **THEN** `lpr_repr(ctx, 1)` returns `"3.14"`

#### Scenario: Default repr unchanged
- **WHEN** no format command has been executed
- **THEN** `repr(obj)` produces the same output as before this change

### Requirement: Coordinate Modes
The runtime SHALL support three coordinate display modes — RECT, POLAR, SPHERICAL — controlled by corresponding commands. The active mode SHALL be stored in the `meta` table as `coordinate_mode`. RECT is the default.

#### Scenario: POLAR display
- **WHEN** `POLAR` is executed and Complex `(3.0, 4.0)` is on the stack
- **THEN** `lpr_repr` displays the value in polar form (magnitude and angle in the current angle mode)

#### Scenario: RECT display
- **WHEN** `RECT` is executed (or is default) and Complex `(3.0, 4.0)` is on the stack
- **THEN** `lpr_repr` displays `(3., 4.)`

### Requirement: Flag Registry Table
The runtime SHALL maintain a `flags` table in SQLite with columns `name` (TEXT PRIMARY KEY), `type_tag` (INTEGER), and `value` (TEXT). Type tags SHALL be: 0 = bool, 1 = integer, 2 = real, 3 = string. The flag space SHALL be unbounded — any string name is a valid flag key.

#### Scenario: Flag table exists
- **WHEN** a fresh context is opened
- **THEN** the `flags` table exists and is empty

#### Scenario: Independent from meta
- **WHEN** a flag is stored via `SFLAG`
- **THEN** it does not appear in the `meta` table, and vice versa

### Requirement: Boolean Flags (SF, CF, FS?, FC?)
SF SHALL pop a String name and set the named flag to boolean true (type_tag=0, value="1"), creating it if it does not exist. CF SHALL pop a String name and set the named flag to boolean false (type_tag=0, value="0"). FS? SHALL pop a String name and push Integer 1 if the flag exists and is truthy, 0 otherwise. FC? SHALL pop a String name and push Integer 1 if the flag is absent or falsy, 0 otherwise.

#### Scenario: Set and test a boolean flag
- **WHEN** `'exact_mode' SF` then `'exact_mode' FS?` is executed
- **THEN** the stack contains Integer `1`

#### Scenario: Clear and test a boolean flag
- **WHEN** `'exact_mode' SF` then `'exact_mode' CF` then `'exact_mode' FS?` is executed
- **THEN** the stack contains Integer `0`

#### Scenario: Test nonexistent flag
- **WHEN** `'nonexistent' FC?` is executed on a fresh context
- **THEN** the stack contains Integer `1`

### Requirement: Typed Flag Access (SFLAG, RFLAG)
SFLAG SHALL pop a String name and a value from the stack, then store the value in the `flags` table with the appropriate type tag inferred from the object type (Integer→1, Real→2, String→3). RFLAG SHALL pop a String name and push the stored value with its original type. RFLAG on a nonexistent flag SHALL push an Error.

#### Scenario: Store and recall an integer flag
- **WHEN** `1000 'max_iterations' SFLAG` then `'max_iterations' RFLAG` is executed
- **THEN** the stack contains Integer `1000`

#### Scenario: Store and recall a string flag
- **WHEN** `"hello" 'greeting' SFLAG` then `'greeting' RFLAG` is executed
- **THEN** the stack contains String `"hello"`

#### Scenario: Recall nonexistent flag
- **WHEN** `'nonexistent' RFLAG` is executed
- **THEN** an Error is pushed onto the stack

### Requirement: Bulk Flag Access (STOF, RCLF)
STOF SHALL push a List of all flags onto the stack, where each flag is represented as a List `{ name value }`. RCLF SHALL pop a List in the same format and replace all flags with its contents (clearing any flags not in the list).

#### Scenario: Round-trip bulk flags
- **WHEN** `'exact_mode' SF` then `1000 'max_iterations' SFLAG` then `STOF` is executed
- **THEN** the stack contains a List with two entries representing the stored flags

#### Scenario: Restore flags from list
- **WHEN** a saved flag list is on the stack and `RCLF` is executed
- **THEN** the flags table matches the list contents exactly

### Requirement: Rational Approximation (->Q)
->Q SHALL pop a Real from the stack and push the nearest Rational approximation using a continued-fraction algorithm.

#### Scenario: Approximate pi
- **WHEN** `3.14159265358979` then `->Q` is executed
- **THEN** the stack contains a Rational close to pi (e.g. `355/113` or similar)

#### Scenario: Exact value
- **WHEN** `0.5` then `->Q` is executed
- **THEN** the stack contains Rational `1/2`

### Requirement: HMS Conversion
HMS-> SHALL pop a Real in H.MMSSss format and push the equivalent decimal hours. ->HMS SHALL pop a decimal hours Real and push the H.MMSSss representation.

#### Scenario: HMS to decimal
- **WHEN** `2.3000` then `HMS->` is executed
- **THEN** the stack contains Real `2.5` (2 hours 30 minutes = 2.5 hours)

#### Scenario: Decimal to HMS
- **WHEN** `2.5` then `->HMS` is executed
- **THEN** the stack contains Real `2.3` (2 hours 30 minutes 0 seconds)
