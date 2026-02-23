## ADDED Requirements

### Requirement: Arrow Command Parsing
The parser SHALL recognize `->` (ASCII) and `→` (Unicode U+2192, UTF-8 bytes 0xE2 0x86 0x92) as command tokens. Both forms SHALL produce the command token `"→"`.

#### Scenario: ASCII arrow
- **WHEN** the input `"-> X Y 'X*Y'"` is tokenized
- **THEN** tokens are produced: command `"→"`, Name("X"), Name("Y"), Symbol("X*Y")

#### Scenario: Unicode arrow
- **WHEN** the input `"→ X Y 'X*Y'"` is tokenized
- **THEN** tokens are produced: command `"→"`, Name("X"), Name("Y"), Symbol("X*Y")
