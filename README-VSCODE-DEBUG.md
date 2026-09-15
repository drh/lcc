# LCC 3.6 Ubuntu 24.04 + VS Code Debug Lab

This kit extends the working Ubuntu 24.04 LCC 3.6 port with a VS Code/GDB study environment.

## 1. Build

```bash
chmod +x BUILD-UBUNTU24.sh
./BUILD-UBUNTU24.sh
```

The build installs `gdb` in addition to the compiler dependencies. LCC, RCC, and CPP are built with `-O0 -g`.

## 2. Open in VS Code

Open the kit root:

```bash
code .
```

Install the Microsoft C/C++ extension if it is not already installed.

## 3. Debug configurations

Open Run and Debug (`Ctrl+Shift+D`).

### LCC: Debug Driver (lcc)

Starts the LCC driver on `hello.c`. This is useful for studying `etc/lcc.c`, especially how the driver assembles the CPP/RCC/assembler/linker command lines.

### LCC: Debug RCC (preprocessed C -> assembly)

Run the task **LCC: Preprocess hello.c** first (or use the task from Command Palette), then start this configuration.

This is the main compiler-study configuration. It launches:

```text
rcc -target=x86/nasm -g hello.i hello.s
```

Set breakpoints in `src/` and step through parsing, semantic analysis, tree/IR construction, and code generation.

### LCC: Debug CPP

Starts the historical preprocessor on `cpp-study.c`. Set breakpoints in `cpp/` to study tokenization, macro expansion, include processing, and preprocessing.

## 4. Recommended first breakpoints

For RCC, start with:

- `src/main.c` — `main`
- `src/lex.c` — tokenization
- `src/decl.c` — declarations/types
- `src/expr.c` — expressions
- `src/stmt.c` — statements
- `src/tree.c` — tree construction
- `src/gen.c` — code generation
- `src/dag.c` — DAG processing

For CPP:

- `cpp/cpp.c`
- `cpp/lex.c`
- `cpp/macro.c`
- `cpp/include.c`

For the driver:

- `etc/lcc.c`

## 5. Important architecture

The normal end-to-end invocation is:

```text
hello.c
  |
  v
lcc (etc/lcc.c)
  |
  +--> cpp
  |
  +--> rcc
  |      |
  |      +--> x86/nasm backend
  |              |
  |              v
  |            NASM
  |
  +--> gcc -m32 (final link)
  |
  v
executable
```

For source-level compiler study, debugging `rcc` directly on `hello.i` is intentionally recommended. It gives stable breakpoints inside the compiler without requiring GDB to follow every fork/exec performed by the driver.

## 6. First exercise

1. Start `LCC: Debug RCC (preprocessed C -> assembly)`.
2. Break at `src/main.c: main`.
3. Step into `program()`.
4. Inspect `gettok()` and the first tokens.
5. Continue into declaration/statement/expression parsing.
6. Compare what you see with *A Retargetable C Compiler: Design and Implementation*.

The purpose of this kit is not merely to compile LCC. It is to make LCC 3.6 a source-level compiler laboratory in VS Code.
