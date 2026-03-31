## ADDED Requirements

### Requirement: LIST-> Command
The `LIST->` command SHALL pop a List from level 1, push each element onto the stack in order, then push an Integer count of the elements.

#### Scenario: Explode a list
- **WHEN** List({1, 2, 3}) is on the stack and `LIST->` is executed
- **THEN** the stack contains Integer(1), Integer(2), Integer(3), Integer(3) (count on top)

#### Scenario: Empty list
- **WHEN** List({}) is on the stack and `LIST->` is executed
- **THEN** Integer(0) is on the stack

### Requirement: ->LIST Command
The `->LIST` command SHALL pop an Integer N from level 1, then pop N objects from the stack and collect them into a List (bottom-most popped element first).

#### Scenario: Collect into list
- **WHEN** Integer(10), Integer(20), Integer(30), Integer(3) are on the stack and `->LIST` is executed
- **THEN** List({10, 20, 30}) is on the stack

### Requirement: GET Command on Lists
The `GET` command, when given a List at level 2 and an Integer index at level 1, SHALL return the element at that 1-based index. Index out of range SHALL produce an error.

#### Scenario: Get element by index
- **WHEN** List({10, 20, 30}) is at level 2, Integer(2) is at level 1, and `GET` is executed
- **THEN** Integer(20) is on the stack

#### Scenario: Index out of range
- **WHEN** List({10, 20}) is at level 2, Integer(5) is at level 1, and `GET` is executed
- **THEN** an error is produced

### Requirement: PUT Command on Lists
The `PUT` command, when given a List at level 3, an Integer index at level 2, and a replacement object at level 1, SHALL return a new List with the element at that 1-based index replaced.

#### Scenario: Put element by index
- **WHEN** List({10, 20, 30}) is at level 3, Integer(2) is at level 2, Integer(99) is at level 1, and `PUT` is executed
- **THEN** List({10, 99, 30}) is on the stack

### Requirement: GETI Command
The `GETI` command SHALL pop a List from level 2 and an Integer index from level 1, push the List, the incremented index, and the element at the original index.

#### Scenario: Get with auto-increment
- **WHEN** List({10, 20, 30}) is at level 2, Integer(1) is at level 1, and `GETI` is executed
- **THEN** the stack contains List({10, 20, 30}), Integer(2), Integer(10) (top)

#### Scenario: Index wraps at end
- **WHEN** List({10, 20, 30}) is at level 2, Integer(3) is at level 1, and `GETI` is executed
- **THEN** the incremented index wraps to Integer(1)

### Requirement: PUTI Command
The `PUTI` command SHALL pop a List from level 3, an Integer index from level 2, and a replacement from level 1. It pushes the modified List and the incremented index.

#### Scenario: Put with auto-increment
- **WHEN** List({10, 20, 30}) is at level 3, Integer(2) is at level 2, Integer(99) is at level 1, and `PUTI` is executed
- **THEN** the stack contains List({10, 99, 30}), Integer(3)

### Requirement: HEAD Command on Lists
The `HEAD` command, when applied to a List, SHALL return the first element. An empty list SHALL produce an error.

#### Scenario: Head of list
- **WHEN** List({10, 20, 30}) is on the stack and `HEAD` is executed
- **THEN** Integer(10) is on the stack

### Requirement: TAIL Command on Lists
The `TAIL` command, when applied to a List, SHALL return a new List containing all elements except the first. An empty list SHALL produce an error.

#### Scenario: Tail of list
- **WHEN** List({10, 20, 30}) is on the stack and `TAIL` is executed
- **THEN** List({20, 30}) is on the stack

### Requirement: SIZE Command on Lists
The `SIZE` command, when applied to a List, SHALL push the number of elements as an Integer.

#### Scenario: Size of list
- **WHEN** List({10, 20, 30}) is on the stack and `SIZE` is executed
- **THEN** Integer(3) is on the stack

### Requirement: POS Command on Lists
The `POS` command SHALL pop a List from level 2 and a search object from level 1, then push the 1-based index of the first occurrence, or Integer(0) if not found.

#### Scenario: Element found
- **WHEN** List({10, 20, 30}) is at level 2, Integer(20) is at level 1, and `POS` is executed
- **THEN** Integer(2) is on the stack

#### Scenario: Element not found
- **WHEN** List({10, 20, 30}) is at level 2, Integer(99) is at level 1, and `POS` is executed
- **THEN** Integer(0) is on the stack

### Requirement: SUB Command on Lists
The `SUB` command, when given a List at level 3, start index at level 2, and end index at level 1 (both 1-based, inclusive), SHALL return the sub-list.

#### Scenario: Sub-list extraction
- **WHEN** List({10, 20, 30, 40, 50}) is at level 3, Integer(2) at level 2, Integer(4) at level 1, and `SUB` is executed
- **THEN** List({20, 30, 40}) is on the stack

### Requirement: REVLIST Command
The `REVLIST` command SHALL pop a List from level 1 and push a new List with elements in reverse order.

#### Scenario: Reverse a list
- **WHEN** List({1, 2, 3}) is on the stack and `REVLIST` is executed
- **THEN** List({3, 2, 1}) is on the stack

### Requirement: SORT Command
The `SORT` command SHALL pop a List of homogeneous comparable elements (all numeric or all strings) and push a sorted List in ascending order. Mixed types SHALL produce an error.

#### Scenario: Sort numeric list
- **WHEN** List({3, 1, 2}) is on the stack and `SORT` is executed
- **THEN** List({1, 2, 3}) is on the stack

#### Scenario: Sort string list
- **WHEN** List({"c", "a", "b"}) is on the stack and `SORT` is executed
- **THEN** List({"a", "b", "c"}) is on the stack

### Requirement: ADD Command
The `ADD` command SHALL pop an element from level 1 and a List from level 2, and push a new List with the element appended. If level 2 is an element and level 1 is a List, the element is prepended.

#### Scenario: Append to list
- **WHEN** List({1, 2}) is at level 2, Integer(3) is at level 1, and `ADD` is executed
- **THEN** List({1, 2, 3}) is on the stack

#### Scenario: Prepend to list
- **WHEN** Integer(0) is at level 2, List({1, 2}) is at level 1, and `ADD` is executed
- **THEN** List({0, 1, 2}) is on the stack

### Requirement: DOLIST Command
The `DOLIST` command SHALL pop an Integer N (number of lists) from level 1, a Program from level 2, and N Lists from levels 3..N+2. It SHALL apply the program to corresponding elements of the lists and collect results into a new List. All lists MUST have the same length.

#### Scenario: DOLIST with one list
- **WHEN** List({1, 2, 3}), Program(« SQ »), Integer(1) are on the stack and `DOLIST` is executed
- **THEN** List({1, 4, 9}) is on the stack

#### Scenario: DOLIST with two lists
- **WHEN** List({1, 2, 3}), List({10, 20, 30}), Program(« + »), Integer(2) are on the stack and `DOLIST` is executed
- **THEN** List({11, 22, 33}) is on the stack

### Requirement: MAP Command
The `MAP` command SHALL pop a Program from level 1 and a List from level 2, apply the program to each element, and collect results into a new List.

#### Scenario: Map a program over a list
- **WHEN** List({1, 2, 3}), Program(« 2 * ») are on the stack and `MAP` is executed
- **THEN** List({2, 4, 6}) is on the stack

### Requirement: STREAM Command
The `STREAM` command SHALL pop a Program from level 1 and a List from level 2. It pushes the first element, then for each subsequent element pushes it and executes the program (binary reduction). The final result is left on the stack.

#### Scenario: Sum reduction
- **WHEN** List({1, 2, 3, 4}), Program(« + ») are on the stack and `STREAM` is executed
- **THEN** Integer(10) is on the stack

### Requirement: SEQ Command
The `SEQ` command SHALL pop a Program, a step value, a count, and a start value from the stack. It generates a List by evaluating the program at each step.

#### Scenario: Generate sequence
- **WHEN** Integer(1), Integer(1), Integer(5), Program(« »)  are arranged on the stack for SEQ
- **THEN** List({1, 2, 3, 4, 5}) is produced

### Requirement: FILTER Command
The `FILTER` command SHALL pop a Program from level 1 and a List from level 2. It SHALL apply the program to each element; if the program leaves a truthy value (nonzero numeric) on the stack, the element is included in the result List. Elements for which the program leaves a falsy value (zero or empty) are excluded.

#### Scenario: Filter elements greater than 3
- **WHEN** List({1, 2, 3, 4, 5}), Program(« 3 > ») are on the stack and `FILTER` is executed
- **THEN** List({4, 5}) is on the stack

#### Scenario: Filter even numbers
- **WHEN** List({1, 2, 3, 4, 5, 6}), Program(« 2 MOD 0 == ») are on the stack and `FILTER` is executed
- **THEN** List({2, 4, 6}) is on the stack

#### Scenario: Filter returns empty list
- **WHEN** List({1, 2, 3}), Program(« 10 > ») are on the stack and `FILTER` is executed
- **THEN** List({}) is on the stack

### Requirement: DOSUBS Command
The `DOSUBS` command SHALL pop an Integer N (window size) from level 1, a Program from level 2, and a List from level 3. It SHALL apply the program to each consecutive subsequence of N elements and collect the results into a new List. For a list of length L with window size N, the result has L-N+1 elements.

#### Scenario: Pairwise sums
- **WHEN** List({1, 2, 3, 4}), Program(« + »), Integer(2) are on the stack and `DOSUBS` is executed
- **THEN** List({3, 5, 7}) is on the stack

#### Scenario: Running scan with window 1
- **WHEN** List({10, 20, 30}), Program(« 2 * »), Integer(1) are on the stack and `DOSUBS` is executed
- **THEN** List({20, 40, 60}) is on the stack

### Requirement: ZIP Command
The `ZIP` command SHALL pop an Integer N (number of lists) from level 1 and N Lists of equal length from the stack. It SHALL return a List of Lists, where each inner List contains the corresponding elements from the input lists.

#### Scenario: Zip two lists
- **WHEN** List({1, 2, 3}), List({"a", "b", "c"}), Integer(2) are on the stack and `ZIP` is executed
- **THEN** List({ {1, "a"}, {2, "b"}, {3, "c"} }) is on the stack

#### Scenario: Zip three lists
- **WHEN** List({1, 2}), List({10, 20}), List({100, 200}), Integer(3) are on the stack and `ZIP` is executed
- **THEN** List({ {1, 10, 100}, {2, 20, 200} }) is on the stack

#### Scenario: Mismatched lengths
- **WHEN** List({1, 2}), List({10, 20, 30}), Integer(2) are on the stack and `ZIP` is executed
- **THEN** an error is produced

### Requirement: UNION Command
The `UNION` command SHALL pop two Lists and push a List containing all unique elements from both (set union, preserving first-occurrence order).

#### Scenario: Set union
- **WHEN** List({1, 2, 3}) is at level 2, List({2, 3, 4}) is at level 1, and `UNION` is executed
- **THEN** List({1, 2, 3, 4}) is on the stack

### Requirement: INTERSECT Command
The `INTERSECT` command SHALL pop two Lists and push a List containing only elements present in both (set intersection, preserving order from the first list).

#### Scenario: Set intersection
- **WHEN** List({1, 2, 3}) is at level 2, List({2, 3, 4}) is at level 1, and `INTERSECT` is executed
- **THEN** List({2, 3}) is on the stack

### Requirement: DIFFERENCE Command
The `DIFFERENCE` command SHALL pop two Lists and push a List containing elements in the first list that are not in the second (set difference).

#### Scenario: Set difference
- **WHEN** List({1, 2, 3, 4}) is at level 2, List({2, 4}) is at level 1, and `DIFFERENCE` is executed
- **THEN** List({1, 3}) is on the stack
