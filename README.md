# Lua Source Optimizer

Lua Source Optimizer is a source-to-source optimizer for Lua.
It takes Lua source code as input, parses it, applies optimization passes, and outputs an optimized version of the Lua source code.
The goal of this project is to help improve Lua code used in games, mods, tools, and embedded scripting environments, especially when performance is affected by poorly optimized scripts.

## Planned pipeline
The optimizer is intended to work in several stages:

1. **Lexing** – convert Lua source into tokens
2. **Parsing** – build an AST
3. **IR generation** – lower the AST into an intermediate representation
4. **Optimization passes** – perform transformations such as constant folding
5. **Code generation** – recreate optimized Lua source code

## Current state
- Solo project, developed slowly in free time
- Early-stage and very buggy
- Does not support full Lua syntax yet
- Parser/AST/IR base is partially implemented
- Only limited optimization work is implemented so far
- Code quality and internal performance still need improvement

## Currently implemented
- Basic lexer
- Basic parser
- AST construction
- Simple constant folding
- Early IR generation
- Partial support for assignments, expressions, and `if` blocks

## Not supported yet
- Full Lua syntax
- Functions
- Loops
- Tables
- Proper unary operators
- Full expression coverage
- Full source regeneration
- Robust error recovery

## Notes
- Comments in the code may be noisy or outdated
- Some explanations inside the source may be incorrect
- Portability should be reasonably good across C compilers, but it has not been fully tested

This project is experimental and not ready for production use.
