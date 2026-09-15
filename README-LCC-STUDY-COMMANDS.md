# LCC 3.6 study[Korean text removed; English documentation] [Korean text removed; English documentation] [Korean text removed; English documentation]

project [Korean text removed; English documentation]:

```bash
cd ~/lcc-3_6-ubuntu24
```

## 0. project [Korean text removed; English documentation] check

Ubuntu[Korean text removed; English documentation] [Korean text removed; English documentation] installation[Korean text removed; English documentation] root[Korean text removed; English documentation] [Korean text removed; English documentation] [Korean text removed; English documentation] project[Korean text removed; English documentation] root [Korean text removed; English documentation] [Korean text removed; English documentation] [Korean text removed; English documentation] [Korean text removed; English documentation]. VS Code[Korean text removed; English documentation] [Korean text removed; English documentation] user[Korean text removed; English documentation] [Korean text removed; English documentation]:

```bash
sudo chown -R $USER:$USER ~/lcc-3_6-ubuntu24
```

check:

```bash
whoami
ls -ld ~/lcc-3_6-ubuntu24
```

`peter_cho peter_cho`[Korean text removed; English documentation] current user [Korean text removed; English documentation] check[Korean text removed; English documentation].

build [Korean text removed; English documentation] `777`[Korean text removed; English documentation] [Korean text removed; English documentation] [Korean text removed; English documentation] [Korean text removed; English documentation]. run [Korean text removed; English documentation] [Korean text removed; English documentation] [Korean text removed; English documentation]:

```bash
chmod +x BUILD-UBUNTU24.sh
```

## 1. LCC 3.6 build

```bash
./BUILD-UBUNTU24.sh
```

[Korean text removed; English documentation] user[Korean text removed; English documentation] run[Korean text removed; English documentation]. `sudo ./BUILD-UBUNTU24.sh`[Korean text removed; English documentation] [Korean text removed; English documentation] [Korean text removed; English documentation] [Korean text removed; English documentation] [Korean text removed; English documentation].

build [Korean text removed; English documentation]:

```text
build-ubuntu24/lcc-build/lcc
build-ubuntu24/lcc-build/rcc
build-ubuntu24/lcc-build/cpp
build-ubuntu24/lcc-build/lburg
```

## 2. C -> [Korean text removed; English documentation] [Korean text removed; English documentation](.i)

LCC[Korean text removed; English documentation] `cpp`[Korean text removed; English documentation] [Korean text removed; English documentation] run[Korean text removed; English documentation] [Korean text removed; English documentation] LCC[Korean text removed; English documentation] [Korean text removed; English documentation] include [Korean text removed; English documentation] [Korean text removed; English documentation].

```bash
./build-ubuntu24/lcc-build/cpp \
    -I./build-ubuntu24/lcc-build/include \
    hello.c \
    hello.i
```

`-I`[Korean text removed; English documentation] [Korean text removed; English documentation]:

> [Korean text removed; English documentation] [Korean text removed; English documentation](`.h`)[Korean text removed; English documentation] [Korean text removed; English documentation] [Korean text removed; English documentation] [Korean text removed; English documentation] [Korean text removed; English documentation] [Korean text removed; English documentation].

[Korean text removed; English documentation] [Korean text removed; English documentation] `hello.c`[Korean text removed; English documentation]:

```c
#include <stdio.h>
```

[Korean text removed; English documentation] [Korean text removed; English documentation] [Korean text removed; English documentation] path[Korean text removed; English documentation] [Korean text removed; English documentation] [Korean text removed; English documentation] [Korean text removed; English documentation] [Korean text removed; English documentation]:

```text
build-ubuntu24/lcc-build/include/stdio.h
```

check:

```bash
ls -l hello.i
less hello.i
```

[Korean text removed; English documentation]:

```text
hello.c
   |
   | cpp
   | -I ./build-ubuntu24/lcc-build/include
   v
hello.i
```

## 3. [Korean text removed; English documentation] C(.i) -> assembly(.s)

[Korean text removed; English documentation] C compiler[Korean text removed; English documentation] `rcc`[Korean text removed; English documentation] [Korean text removed; English documentation] run[Korean text removed; English documentation].

```bash
./build-ubuntu24/lcc-build/rcc \
    -target=x86/nasm \
    -g \
    hello.i \
    hello.s
```

check:

```bash
ls -l hello.s
less hello.s
```

study[Korean text removed; English documentation] [Korean text removed; English documentation] RCC[Korean text removed; English documentation] [Korean text removed; English documentation] [Korean text removed; English documentation] source:

```text
src/main.c
src/lex.c
src/decl.c
src/expr.c
src/stmt.c
src/tree.c
src/dag.c
src/gen.c
```

[Korean text removed; English documentation] [Korean text removed; English documentation]:

```text
hello.i
   |
   v
lex.c       [Korean text removed; English documentation]
   |
   v
decl.c / expr.c / stmt.c    [Korean text removed; English documentation]/[Korean text removed; English documentation] [Korean text removed; English documentation]
   |
   v
tree.c      [Korean text removed; English documentation] [Korean text removed; English documentation](IR) [Korean text removed; English documentation]
   |
   v
dag.c      DAG [Korean text removed; English documentation]/[Korean text removed; English documentation]
   |
   v
gen.c       [Korean text removed; English documentation] [Korean text removed; English documentation] [Korean text removed; English documentation] [Korean text removed; English documentation]
   |
   v
hello.s
```

## 4. [Korean text removed; English documentation] [Korean text removed; English documentation] LCC [Korean text removed; English documentation] run

`lcc`[Korean text removed; English documentation] CPP, RCC, assembler, linker [Korean text removed; English documentation] [Korean text removed; English documentation] [Korean text removed; English documentation].

```bash
./build-ubuntu24/lcc-build/lcc hello.c
```

[Korean text removed; English documentation]:

```text
hello.c
   |
   v
  cpp
   |
   v
hello.i
   |
   v
  rcc
   |
   v
hello.s
   |
   v
assembler
   |
   v
object
   |
   v
linker
   |
   v
run [Korean text removed; English documentation]
```

## 5. study[Korean text removed; English documentation] [Korean text removed; English documentation] [Korean text removed; English documentation] [Korean text removed; English documentation] [Korean text removed; English documentation]

```bash
cd ~/lcc-3_6-ubuntu24

# C -> .i
./build-ubuntu24/lcc-build/cpp \
    -I./build-ubuntu24/lcc-build/include \
    hello.c hello.i

# .i -> .s
./build-ubuntu24/lcc-build/rcc \
    -target=x86/nasm \
    -g \
    hello.i hello.s

# [Korean text removed; English documentation] check
less hello.i
less hello.s
```

## 6. VS Code + GDB study [Korean text removed; English documentation]

[Korean text removed; English documentation] [Korean text removed; English documentation]:

1. `LCC: Debug CPP`
2. `LCC: Debug RCC (preprocessed C -> assembly)`
3. `LCC: Debug Driver (lcc)`

CPP [Korean text removed; English documentation] [Korean text removed; English documentation]:

```text
cpp/cpp.c
cpp/lex.c
cpp/macro.c
cpp/include.c
```

RCC [Korean text removed; English documentation] [Korean text removed; English documentation]:

```text
src/main.c
src/lex.c
src/decl.c
src/expr.c
src/stmt.c
src/tree.c
src/dag.c
src/gen.c
```

Driver:

```text
etc/lcc.c
```

## 7. `stdio.h`[Korean text removed; English documentation] [Korean text removed; English documentation] [Korean text removed; English documentation] [Korean text removed; English documentation]

[Korean text removed; English documentation] [Korean text removed; English documentation] [Korean text removed; English documentation]:

```text
cpp: hello.c:1 Could not find include file <stdio.h>
```

[Korean text removed; English documentation]:

```bash
find . -name stdio.h
```

[Korean text removed; English documentation] [Korean text removed; English documentation] [Korean text removed; English documentation] [Korean text removed; English documentation] [Korean text removed; English documentation] [Korean text removed; English documentation]:

```text
./build-ubuntu24/lcc-build/include/stdio.h
```

[Korean text removed; English documentation] [Korean text removed; English documentation] `cpp`[Korean text removed; English documentation] run[Korean text removed; English documentation] [Korean text removed; English documentation] [Korean text removed; English documentation] `-I`[Korean text removed; English documentation] [Korean text removed; English documentation]:

```bash
./build-ubuntu24/lcc-build/cpp \
    -I./build-ubuntu24/lcc-build/include \
    hello.c hello.i
```

## 8. `can't write hello.s` [Korean text removed; English documentation] `mkdir ... Permission denied`[Korean text removed; English documentation] [Korean text removed; English documentation] [Korean text removed; English documentation]

current user[Korean text removed; English documentation] check:

```bash
whoami
```

project [Korean text removed; English documentation] check:

```bash
ls -ld ~/lcc-3_6-ubuntu24
```

`root root`[Korean text removed; English documentation]:

```bash
sudo chown -R $USER:$USER ~/lcc-3_6-ubuntu24
```

[Korean text removed; English documentation] [Korean text removed; English documentation] [Korean text removed; English documentation] check:

```bash
ls -ld ~/lcc-3_6-ubuntu24
```

project[Korean text removed; English documentation] current user [Korean text removed; English documentation] [Korean text removed; English documentation].

---

## [Korean text removed; English documentation] [Korean text removed; English documentation] [Korean text removed; English documentation]

```text
hello.c --(cpp + -I include)--> hello.i --(rcc)--> hello.s
```

[Korean text removed; English documentation] `hello.c -> hello.i`[Korean text removed; English documentation] [Korean text removed; English documentation], `hello.i -> hello.s`[Korean text removed; English documentation] RCC[Korean text removed; English documentation] C[Korean text removed; English documentation] [Korean text removed; English documentation] [Korean text removed; English documentation] assembly[Korean text removed; English documentation] [Korean text removed; English documentation] [Korean text removed; English documentation] [Korean text removed; English documentation] [Korean text removed; English documentation].
