# SGas Language – Design Document

## Version
0.1.0

## Overview
SGas is a small, embeddable, dynamically-typed programming language
inspired by Go (syntax clarity, `func` keyword, block statements)
and Lua (lightweight VM, `--` comments, end-of-block via `}`).

## Goals
- Simple, readable syntax
- Small stack-based bytecode VM (like Lua)
- Go-like function declarations
- Easy to embed in C/C++ hosts

## Pipeline
Source (.sgas)
   │
   ▼
[Lexer]  →  Tokens
   │
   ▼
[Parser] →  AST
   │
   ▼
[Compiler] → Bytecode (Chunk)
   │
   ▼
[VM]  →  executes bytecode using Runtime + Stdlib

## Values
- nil
- bool
- number (double)
- string
- function
- native function

## Memory
- Objects are tracked on a linked list (GC hook reserved for v0.2)
- Strings are interned by hash (v0.1: no dedup, hook reserved)

## Error Model
- Lexer/Parser/Compiler errors → printed with line number, exit code 65
- Runtime errors → stack trace printed, exit code 70