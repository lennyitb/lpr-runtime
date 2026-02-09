## 1. Local Variable Scope Stack
- [ ] 1.1 Add scope stack (`std::vector<std::unordered_map<std::string, Object>>`) to Context
- [ ] 1.2 Add `push_locals` / `pop_locals` / `resolve_local` methods to Context
- [ ] 1.3 Write unit tests for scope push/pop/resolve and nested shadowing

## 2. Arrow Command Parsing
- [ ] 2.1 Parser recognizes `->` and `→` (UTF-8 0xE2 0x86 0x92) as command tokens inside programs
- [ ] 2.2 Write parser tests for `<< -> X Y 'X*Y' >>` and `<< → X Y << X Y * >> >>`

## 3. Arrow Command Execution
- [ ] 3.1 Implement `->` / `→` command: pop N values, bind to names, execute body, pop scope
- [ ] 3.2 Modify `execute_tokens` to support arrow-block interpretation (collect names + body after `->`)
- [ ] 3.3 Write execution tests: basic binding, nested scopes, body as Symbol, body as Program

## 4. Expression Evaluator (Shunting-Yard)
- [ ] 4.1 Implement expression tokenizer: split Symbol string into numbers, names, operators, parens
- [ ] 4.2 Implement shunting-yard infix-to-RPN conversion with standard precedence
- [ ] 4.3 Implement RPN evaluator using the numeric tower (Integer/Real promotion)
- [ ] 4.4 Wire variable resolution: local scope stack first, then global Store variables
- [ ] 4.5 Write unit tests for expression evaluation: arithmetic, precedence, parentheses, variables

## 5. EVAL for Symbols
- [ ] 5.1 Extend EVAL command to handle Symbol objects by calling the expression evaluator
- [ ] 5.2 Write integration tests: `'2+3' EVAL` -> 5, store X then `'X^2' EVAL`, local vars via `->` then EVAL

## 6. Name Resolution in Execution
- [ ] 6.1 Modify bare Name handling: when a Name token is executed and it matches a local variable, push the local value
- [ ] 6.2 Write tests: local name resolution takes precedence over global, unresolved names push as Name objects
