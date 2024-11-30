#ifndef __SETJMP
#define __SETJMP


#define setjmp _setjmp

typedef int jmp_buf[16];
int setjmp(jmp_buf);
void longjmp(jmp_buf, int);

#endif /* __SETJMP */
