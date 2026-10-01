# SGas Grammar (v0.1)

program     → declaration* EOF

declaration → funcDecl | letDecl | statement

funcDecl    → "func" IDENT "(" paramList? ")" block
paramList   → IDENT ("," IDENT)*

letDecl     → "let" IDENT "=" expression ";"

statement   → ifStmt | whileStmt | returnStmt | block | exprStmt

ifStmt      → "if" expression block ("else" (ifStmt | block))?
whileStmt   → "while" expression block
returnStmt  → "return" expression? ";"
block       → "{" declaration* "}"
exprStmt    → expression ";"

expression  → assignment
assignment  → IDENT "=" assignment | logic_or
logic_or    → logic_and ("or" logic_and)*
logic_and   → equality ("and" equality)*
equality    → comparison (("==" | "!=") comparison)*
comparison  → term (("<" | ">" | "<=" | ">=") term)*
term        → factor (("+" | "-") factor)*
factor      → unary (("*" | "/" | "%") unary)*
unary       → ("not" | "-") unary | call
call        → primary ("(" arguments? ")")*
arguments   → expression ("," expression)*
primary     → NUMBER | STRING | "true" | "false" | "nil"
            | IDENT | "(" expression ")"

## Comments
Line comments start with `--` and run to end of line.

## Keywords
let  func  if  else  while  return
true false nil   and  or  not

## Operators
+ - * / %   == != < > <= >=   =   and or not