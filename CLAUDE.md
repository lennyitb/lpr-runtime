
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
