# SGas

> A small, embeddable, dynamically-typed programming language inspired by **Go** and **Lua** — written in C.

[![Language](https://img.shields.io/badge/language-C11-blue.svg)]()
[![Version](https://img.shields.io/badge/version-0.1.0-green.svg)]()
[![License](https://img.shields.io/badge/license-MIT-yellow.svg)]()

SGas takes the clarity and simplicity of Go (`func`, block statements, `let`) and pairs it with the lightweight stack-based VM design of Lua. It's designed from the ground up to be **small**, **embeddable**, and **hackable** — no external dependencies, just the C standard library.

---

## ✨ Features

| Feature | Status |
|---------|:------:|
| Lexer & recursive-descent parser | ✅ |
| AST-based compiler → bytecode | ✅ |
| Stack-based virtual machine | ✅ |
| Hash-table globals with FNV-1a (O(1) variable access) | ✅ |
| Global and local variables | ✅ |
| `func` declarations with parameters and recursion | ✅ |
| `if` / `else if` / `else`, `while` loops | ✅ |
| String concatenation & numeric arithmetic | ✅ |
| Native stdlib (`print`, `str`, `num`, `len`, `type`, `clock`, `assert`) | ✅ |
| Built-in web playground (embedded HTTP server + browser UI) | ✅ |
| Build scripts for Bash, PowerShell, and Ion (Redox OS) | ✅ |
| Unit tests for lexer, parser, and VM | ✅ |
| Closures / upvalues | 🔜 v0.2 |
| Tables / arrays | 🔜 v0.2 |
| Garbage collector | 🔜 v0.3 |
| Modules & imports | 🔜 v0.3 |

---

## 📦 Project Layout
