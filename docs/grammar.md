# Supported grammar

This file is the authoritative scope boundary. rvcc accepts exactly the grammar
below and nothing else; anything outside it is a deliberate non-goal (see
`limitations.md`).

## Current (M0 - M5)

```ebnf
program        = { function } ;
function       = "int" identifier "(" [ params ] ")" block ;
params         = param { "," param } ;
param          = "int" [ "*" ] identifier ;

statement      = "return" expression ";"
               | declaration
               | "if" "(" expression ")" statement [ "else" statement ]
               | "while" "(" expression ")" statement
               | "for" "(" [ for-init ] ";" [ expression ] ";" [ expression ] ")" statement
               | block | expression ";" | ";" ;
declaration    = "int" [ "*" ] identifier [ "[" integer-literal "]" ] [ "=" expression ] ";" ;
for-init       = declaration-without-semi | expression ;
block          = "{" { statement } "}" ;

expression     = assignment ;
assignment     = lvalue "=" assignment | logical-or ;             (* right-assoc *)
logical-or     = logical-and { "||" logical-and } ;               (* short-circuit *)
logical-and    = equality { "&&" equality } ;                     (* short-circuit *)
equality       = relational { ("==" | "!=") relational } ;
relational     = additive { ("<" | "<=" | ">" | ">=") additive } ;
additive       = multiplicative { ("+" | "-") multiplicative } ;
multiplicative = unary { ("*" | "/" | "%") unary } ;
unary          = ("-" | "~" | "!" | "&" | "*") unary | postfix ;  (* & address-of, * deref *)
postfix        = primary { "[" expression "]" } ;                 (* a[i] == *(a + i) *)
primary        = integer-literal | identifier | call | "(" expression ")" ;
call           = identifier "(" [ expression { "," expression } ] ")" ;
lvalue         = identifier | "*" unary ;
```

Notes:
- Multiple functions; calls may forward-reference (signatures are collected
  first), enabling mutual recursion. Return type is always `int`.
- Types: `int`, pointer-to-int (`int *p`), and array-of-int (`int a[N]`, local).
  Arrays decay to a pointer to their first element when used as a value.
  Pointer arithmetic scales by the element size (4): `p + i`, `p - i`, `p[i]`.
- Up to 8 parameters/arguments (register set a0-a7); more is a clear error.
- `&` takes the address of an lvalue; `*` dereferences a pointer (both as an
  rvalue and as an assignment target: `*p = x`, `a[i] = x`).
- `/` and `%` are signed and truncate toward zero (matches gcc).

## Deliberately out of scope (documented non-goals)

`char` and other integer widths; global variables and a `.data`/`.bss` section;
multi-dimensional arrays; pointer-to-pointer and pointer-to-array; array
initialisers; pointer difference (`p - q`); `break`/`continue`, `switch`,
ternary `?:`; bitwise `& | ^`/shifts; compound assignment and `++`/`--`;
the preprocessor, standard library, structs/unions/enums, and casts. Each is a
clean future extension; none is silently mis-handled — inputs outside the grammar
are rejected.
