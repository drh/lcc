#ifndef __ERRNO
#define __ERRNO

#define EDOM	33
#define ERANGE	34
extern int *__errno(void);
#define errno (*__errno())

#endif /* __ERRNO */
