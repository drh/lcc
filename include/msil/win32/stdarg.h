#ifndef __STDARG
#define __STDARG

#if !defined(_VA_LIST)
#define _VA_LIST
#define va_list __va_list
typedef struct __va_list *__va_list;
#endif

#define va_start(list,last) __va_start(&list)
#define va_arg(list,type) (*(type*)__va_arg(&list,__typecode(type)))
#define va_end(list) ((void)0)

extern void __va_start(va_list*);
extern void* __va_arg(va_list*,int);
#endif
