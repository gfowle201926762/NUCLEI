### C Coursework (Nuclei)

Create a parser and interpreter in C for a made up language (Nuclei), which is similar to LISP, for my CS coursework at Bristol.

## Usage:
Take a look at the Makefile.
```make parser``` builds the parser only.
```make interp``` builds the interpreter.
```make extension``` builds the extention - take a look at extension.txt to see what it does.

Use .ncl files for the parser and interpreter.
Use .ecl files for the extension

## Nuclei Grammar:

```
PROG ::= "(" INSTRCTS

INSTRCTS ::= INSTRCT INSTRCTS | ")"

INSTRCT ::= "(" FUNC ")"

FUNC ::= RETFUNC | IOFUNC | IF | LOOP

RETFUNC ::= LISTFUNC | INTFUNC | BOOLFUNC

LISTFUNC ::= "CAR" LIST | "CDR" LIST | "CONS" LIST LIST

INTFUNC ::= "PLUS" LIST LIST | "LENGTH" LIST

BOOLFUNC ::= "LESS" LIST LIST | "GREATER" LIST LIST | "EQUAL" LIST LIST

IOFUNC ::= SET | PRINT

SET ::= "SET" VAR LIST

PRINT ::= "PRINT" LIST | "PRINT" STRING

IF ::= "IF" "(" BOOLFUNC ")" "(" INSTRCTS "(" INSTRCTS

LOOP ::= "WHILE" "(" BOOLFUNC ")" "(" INSRCTS

LIST ::= VAR | LITERAL | "NIL" | "(" RETFUNC ")"

VAR ::= [A-Z]

STRING ::= Double-quoted string constant e.g. "Hello, World!", or "FAILURE ?"

LITERAL ::= Single-quoted list e.g. ’(1)’, ’(1 2 3 (4 5))’, or ’2’
```
