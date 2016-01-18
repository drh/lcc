#ifndef __LIMITS
#define __LIMITS

#define CHAR_BIT	8
#define MB_LEN_MAX	1

#define UCHAR_MAX	0xff
#define USHRT_MAX	0xffff
#define UINT_MAX	(~0U)
#define ULONG_MAX	(~0UL)

#define SCHAR_MAX	0x7f
#define SHRT_MAX	0x7fff
#define INT_MAX		0x7fffffff
#define LONG_MAX	0x7fffffffL

#define SCHAR_MIN	(-SCHAR_MAX-1)
#define SHRT_MIN	(-SHRT_MAX-1)
#define INT_MIN		(-INT_MAX-1)
#define LONG_MIN	(-LONG_MAX-1)

#ifdef __CHAR_UNSIGNED__
#define CHAR_MAX	UCHAR_MAX
#define CHAR_MIN	0
#else
#define CHAR_MAX	SCHAR_MAX
#define CHAR_MIN	SCHAR_MIN
#endif

#endif /* __LIMITS */
