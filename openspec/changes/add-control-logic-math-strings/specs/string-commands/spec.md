## ADDED Requirements

### Requirement: SIZE Command (Strings)
`SIZE` SHALL pop a String and push its length as an Integer.

#### Scenario: Length of string
- **WHEN** String("hello") is on the stack and `SIZE` is executed
- **THEN** Integer(5) is on the stack

#### Scenario: Empty string
- **WHEN** String("") is on the stack and `SIZE` is executed
- **THEN** Integer(0) is on the stack

### Requirement: SUB Command (Strings)
`SUB` SHALL pop an end position (level 1), a start position (level 2), and a String (level 3). Positions are 1-based. Push the substring from start to end inclusive.

#### Scenario: Substring extraction
- **WHEN** String("hello world"), Integer(1), and Integer(5) are on the stack and `SUB` is executed
- **THEN** String("hello") is on the stack

#### Scenario: Single character
- **WHEN** String("hello"), Integer(1), and Integer(1) are on the stack and `SUB` is executed
- **THEN** String("h") is on the stack

### Requirement: POS Command (Strings)
`POS` SHALL pop a search string (level 1) and a target string (level 2). Push the 1-based position of the first occurrence, or Integer(0) if not found.

#### Scenario: Found
- **WHEN** String("hello world") and String("world") are on the stack and `POS` is executed
- **THEN** Integer(7) is on the stack

#### Scenario: Not found
- **WHEN** String("hello") and String("xyz") are on the stack and `POS` is executed
- **THEN** Integer(0) is on the stack

### Requirement: REPL Command (Strings)
`REPL` SHALL pop a replacement string (level 1), a search string (level 2), and a target string (level 3). Replace the first occurrence of search in target with replacement and push the result.

#### Scenario: Replace substring
- **WHEN** String("hello world"), String("world"), and String("there") are on the stack and `REPL` is executed
- **THEN** String("hello there") is on the stack

#### Scenario: No match
- **WHEN** String("hello"), String("xyz"), and String("abc") are on the stack and `REPL` is executed
- **THEN** String("hello") is on the stack

### Requirement: HEAD Command (Strings)
`HEAD` SHALL pop a String and push its first character as a String. Error if the string is empty.

#### Scenario: Head of string
- **WHEN** String("hello") is on the stack and `HEAD` is executed
- **THEN** String("h") is on the stack

### Requirement: TAIL Command (Strings)
`TAIL` SHALL pop a String and push everything after the first character. Error if the string is empty.

#### Scenario: Tail of string
- **WHEN** String("hello") is on the stack and `TAIL` is executed
- **THEN** String("ello") is on the stack

### Requirement: NUM Command (Strings)
`NUM` SHALL pop a String and push the Unicode codepoint of its first character as an Integer.

#### Scenario: Codepoint of A
- **WHEN** String("A") is on the stack and `NUM` is executed
- **THEN** Integer(65) is on the stack

### Requirement: CHR Command (Strings)
`CHR` SHALL pop an Integer codepoint and push the corresponding single character as a String.

#### Scenario: Character from codepoint
- **WHEN** Integer(65) is on the stack and `CHR` is executed
- **THEN** String("A") is on the stack
