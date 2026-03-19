<!-- OPENSPEC:START -->
# OpenSpec Instructions

These instructions are for AI assistants working in this project.

Always open `@/openspec/AGENTS.md` when the request:
- Mentions planning or proposals (words like proposal, spec, change, plan)
- Introduces new capabilities, breaking changes, architecture shifts, or big performance/security work
- Sounds ambiguous and you need the authoritative spec before coding

Use `@/openspec/AGENTS.md` to learn:
- How to create and apply change proposals
- Spec format and conventions
- Project structure and guidelines

Keep this managed block so 'openspec update' can refresh the instructions.

<!-- OPENSPEC:END -->

# Documentation

When implementing new features, proactively update the project's reference docs:

- **CMD_SET_REFERENCE.md** — command set reference
- **ARCHITECTURE.md** — architectural overview

When adding content to these documents, integrate it consistently with the existing structure and style. Place new entries where they logically belong rather than appending afterthoughts, postscripts, or addendums.

# Runtime Interaction

Use `lpr-cli -e` to execute RPL expressions non-interactively:

```sh
./build/lpr-cli -e "3 4 +"         # single expression
./build/lpr-cli -e "3 4" -e "+"    # multiple expressions, same context
./build/lpr-cli db.lpr -e "VARS"   # query a persistent database
```

The `-e` flag executes the expression, prints the resulting stack, and exits. Multiple `-e` flags execute sequentially on the same context. Exit code is 0 on success, 1 on error.