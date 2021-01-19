#ifndef __STDDEF
#define __STDDEF

/* $Id: stddef.h#1 2002/08/30 14:23:17 REDMOND\\drh $ */

#ifndef NULL
#define NULL ((void*)0)
#endif
#define offsetof(ty,mem) ((size_t)((char*)&((ty*)0)->mem - (char*)0))

typedef long ptrdiff_t;

#if !defined(_SIZE_T) && !defined(_SIZE_T_) && !defined(_SIZE_T_DEFINED)
#define _SIZE_T
#define _SIZE_T_
#define _SIZE_T_DEFINED
typedef unsigned long size_t;
#endif

#if !defined(_WCHAR_T) && !defined(_WCHAR_T_) && !defined(_WCHAR_T_DEFINED)
#define _WCHAR_T
#define _WCHAR_T_
#define _WCHAR_T_DEFINED
#if   (_WCHAR_T_SIZE + 0) == 1
typedef unsigned char wchar_t;
#elif (_WCHAR_T_SIZE + 0) == 2
typedef unsigned short wchar_t;
#elif (_WCHAR_T_SIZE + 0) == 4
typedef unsigned int wchar_t;
#else
typedef unsigned short wchar_t;
#endif
#endif

#endif /* __STDDEF */
