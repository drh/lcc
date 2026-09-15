# LCC 3.6 on Ubuntu 24.04 — NASM/ELF port (v5)

This kit bootstraps the historical LCC 3.6 source on Ubuntu 24.04 using the
historical NASM x86 backend.

The old Linux ELF driver hard-coded 1990s startup-object paths such as
`/usr/lib/crt1.o` and `/usr/lib/crtbegin.o`. On Ubuntu 24.04 those objects
are supplied through the multilib GCC toolchain instead. This version changes
the final link to `/usr/bin/gcc -m32`, allowing GCC to select the correct
multiarch startup objects and libraries.

It also patches the LCC 3.6 lburg grammar for modern Bison and uses the NASM
executable found by `command -v nasm`.

## VS Code workspace

Open this directory itself as the VS Code workspace:

```bash
cd ~/lcc-3_6-ubuntu24
code .
```

The `.vscode/launch.json` is at the workspace root, so F5 exposes the LCC debug configurations automatically.
