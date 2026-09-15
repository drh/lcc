# How to install lcc 3.6 in Ubuntu 24.04.3 LTS
## First, you need to open the terminal.

cd ~

sudo apt update

sudo apt upgrade

sudo apt install git

git clone https://github.com/IkhyeonJo/lcc

cd lcc

sudo chmod 777 BUILD-UBUNTU24.sh

./BUILD-UBUNTU24.sh

sudo snap install code --classic

code .

# Required Visual Studio code Extensions:
C/C++ Extension Pack

# LCC Components

The main components used for compiler study are:

    lcc   - compiler driver
    cpp   - C preprocessor
    rcc   - C compiler
    lburg - code-generator generator

The basic pipeline is:

    hello.c
       |
       v
      lcc
       |
       +----> cpp
       |
       +----> rcc
       |
       v
    assembly / object / executable


Debugging Modes
==================


A. LCC: Debug CPP (Preprocessor)
-----------------

Source:

    cpp/

Purpose:

Debug the C preprocessor.

Pipeline:

    hello.c
       |
       v
      cpp
       |
       v
    hello.i

Important source files:

    cpp/cpp.c
    cpp/lex.c
    cpp/macro.c
    cpp/include.c

Study topics:

- preprocessing
- lexical processing
- #include
- #define and macros
- conditional compilation
- token handling


B. LCC: Debug RCC (preprocessed C -> assembly)
-----------------------------------------------

Source:

    src/

Purpose:

Debug the actual C compiler.

Pipeline:

    hello.i
       |
       v
      rcc
       |
       v
    hello.s

This is the main debugging mode for compiler-construction study.

Important source files:

    src/main.c
    src/lex.c
    src/decl.c
    src/expr.c
    src/stmt.c
    src/tree.c
    src/dag.c
    src/gen.c

Study topics:

- lexical analysis
- declarations and types
- expressions
- statements
- syntax processing
- tree representation
- DAG construction
- optimization
- instruction selection
- assembly generation


Recommended Debugging Order
==============================

Start with:

    1. LCC: Debug CPP (Preprocessor)
    2. LCC: Debug RCC (preprocessed C -> assembly)

For compiler-construction study, spend most of the time debugging RCC.
