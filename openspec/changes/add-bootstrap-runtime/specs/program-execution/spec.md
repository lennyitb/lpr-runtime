## ADDED Requirements

### Requirement: EVAL Command
The `EVAL` command SHALL execute the object at level 1. If it is a Program, execute its tokens. If it is a Name, recall the associated variable and execute it.

#### Scenario: Evaluate program
- **WHEN** a Program `« 2 3 + »` is on the stack and `EVAL` is executed
- **THEN** Integer(5) is on the stack

#### Scenario: Evaluate name
- **WHEN** a variable `x` stores Integer(42), Name("x") is on the stack, and `EVAL` is executed
- **THEN** Integer(42) is on the stack

### Requirement: IFT Command
The `IFT` command SHALL pop level 1 (condition) and level 2 (then-program). If the condition is truthy (non-zero), execute the then-program.

#### Scenario: Condition true
- **WHEN** a Program `« "yes" »` is at level 2, Integer(1) is at level 1, and `IFT` is executed
- **THEN** String("yes") is on the stack

#### Scenario: Condition false
- **WHEN** a Program `« "yes" »` is at level 2, Integer(0) is at level 1, and `IFT` is executed
- **THEN** the stack is unchanged (both arguments consumed, nothing pushed)

### Requirement: IFTE Command
The `IFTE` command SHALL pop level 1 (condition), level 2 (then-program), and level 3 (else-program). If the condition is truthy, execute the then-program; otherwise execute the else-program.

#### Scenario: IFTE true branch
- **WHEN** a Program `« "no" »` is at level 3, a Program `« "yes" »` is at level 2, Integer(1) is at level 1, and `IFTE` is executed
- **THEN** String("yes") is on the stack

#### Scenario: IFTE false branch
- **WHEN** a Program `« "no" »` is at level 3, a Program `« "yes" »` is at level 2, Integer(0) is at level 1, and `IFTE` is executed
- **THEN** String("no") is on the stack
