#include "c.h"
extern Interface alphaIR;
extern Interface mipsebIR, mipselIR;
extern Interface sparcIR, solarisIR;
extern Interface x86IR;
extern Interface symbolicIR, symbolic64IR;
extern Interface nullIR;
Binding bindings[] = {
        "alpha/osf",     &alphaIR,
        "mips/irix",     &mipsebIR,
        "mips/ultrix",   &mipselIR,
        "sparc/sun",     &sparcIR,
        "sparc/solaris", &solarisIR,
        "x86/win32",     &x86IR,
        "symbolic/osf",  &symbolic64IR,
        "symbolic/irix", &symbolicIR,
        "symbolic",      &symbolicIR,
        "null",          &nullIR,
        NULL,            NULL
};
