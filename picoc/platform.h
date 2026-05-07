/* all platform-specific includes and defines go in this file */

/* 平台配置头文件 — 定义主机类型、内存大小、功能开关和平台相关的头文件包含 */

#ifndef PLATFORM_H
#define PLATFORM_H

/* configurable options */
/* select your host type (or do it in the Makefile):
 * #define  UNIX_HOST
 * #define  FLYINGFOX_HOST
 * #define  SURVEYOR_HOST
 * #define  SRV1_UNIX_HOST
 * #define  UMON_HOST
 * #define  WIN32  (predefined on MSVC)
 */

/* 该平台 int 能容纳的最大 10 的幂 */
#define LARGE_INT_POWER_OF_TEN 1000000000   /* the largest power of ten which fits in an int on this architecture */
#if defined(__hppa__) || defined(__sparc__)
#define ALIGN_TYPE double                   /* the default data type to use for alignment */
#else
#define ALIGN_TYPE void *                   /* the default data type to use for alignment */
#endif

/* 全局变量哈希表大小 */
#define GLOBAL_TABLE_SIZE 97                /* global variable table */
/* 共享字符串表大小 */
#define STRING_TABLE_SIZE 97                /* shared string table size */
/* 字符串字面量表大小 */
#define STRING_LITERAL_TABLE_SIZE 97        /* string literal table size */
/* 保留字哈希表大小 */
#define RESERVED_WORD_TABLE_SIZE 97         /* reserved word table size */
/* 函数参数最大数量 */
#define PARAMETER_MAX 16                    /* maximum number of parameters to a function */
/* 单行最大字符数 */
#define LINEBUFFER_MAX 256                  /* maximum number of characters on a line */
/* 局部变量表初始大小 */
#define LOCAL_TABLE_SIZE 11                 /* size of local variable table (can expand) */
/* 结构体/联合体成员表初始大小 */
#define STRUCT_TABLE_SIZE 11                /* size of struct/union member table (can expand) */

/* 交互模式提示符 */
#define INTERACTIVE_PROMPT_START "starting picoc " PICOC_VERSION "\n"
#define INTERACTIVE_PROMPT_STATEMENT "picoc> "
#define INTERACTIVE_PROMPT_LINE "     > "

/* host platform includes */
/* STM32H750 主机配置 — 嵌入式 ARM 平台，使用内置迷你标准库 */
#ifdef STM32H750_HOST
# define BUILTIN_MINI_STDLIB
# define HEAP_SIZE (64*1024)
# define PICOC_MATH_LIBRARY
# define FEATURE_AUTO_DECLARE_VARIABLES
# define FANCY_ERROR_MESSAGES
# include <stdio.h>
# include <stdlib.h>
# include <ctype.h>
# include <string.h>
# include <assert.h>
# include <stdarg.h>
# include <stdint.h>
# include <setjmp.h>
# include <math.h>
# undef BIG_ENDIAN
#else
/* UNIX/Linux 主机配置 — 使用 malloc 堆栈，完整 C 库支持 */
#ifdef UNIX_HOST
# define USE_MALLOC_STACK                   /* stack is allocated using malloc() */
# define USE_MALLOC_HEAP                    /* heap is allocated using malloc() */
# include <stdio.h>
# include <stdlib.h>
# include <ctype.h>
# include <string.h>
# include <assert.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <unistd.h>
# include <stdarg.h>
# include <setjmp.h>
# ifndef NO_FP
#  include <math.h>
#  define PICOC_MATH_LIBRARY
#  define USE_READLINE
#  undef BIG_ENDIAN
#  if defined(__powerpc__) || defined(__hppa__) || defined(__sparc__)
#   define BIG_ENDIAN
#  endif
# endif

extern jmp_buf ExitBuf;

#else
/* Windows 主机配置 — 使用 malloc 堆栈，完整 C 库支持 */
# ifdef WIN32
#  define USE_MALLOC_STACK                   /* stack is allocated using malloc() */
#  define USE_MALLOC_HEAP                    /* heap is allocated using malloc() */
#  include <stdio.h>
#  include <stdlib.h>
#  include <ctype.h>
#  include <string.h>
#  include <assert.h>
#  include <sys/types.h>
#  include <sys/stat.h>
#  include <stdarg.h>
#  include <setjmp.h>
#  include <math.h>
#  define PICOC_MATH_LIBRARY
#  undef BIG_ENDIAN

extern jmp_buf ExitBuf;

# else
/* FlyingFox 嵌入式主机配置 — 16KB 堆，简化标准库 */
#  ifdef FLYINGFOX_HOST
#   define HEAP_SIZE (16*1024)               /* space for the heap and the stack */
#   define NO_HASH_INCLUDE
#   include <stdlib.h>
#   include <ctype.h>
#   include <string.h>
#   include <sys/types.h>
#   include <stdarg.h>
#   include <setjmp.h>
#   include <math.h>
#   define assert(x)
#   define BUILTIN_MINI_STDLIB
#   undef BIG_ENDIAN

#  else
/* Surveyor 机器人主机配置 — 无浮点、无 ctype，使用自定义硬件库 */
#   ifdef SURVEYOR_HOST
#    define HEAP_SIZE C_HEAPSIZE
#    define NO_FP
#    define NO_CTYPE
#    define NO_HASH_INCLUDE
#    define NO_MODULUS
#    include <cdefBF537.h>
#    include "../string.h"
#    include "../print.h"
#    include "../srv.h"
#    include "../setjmp.h"
#    include "../stdarg.h"
#    include "../colors.h"
#    include "../neural.h"
#    include "../gps.h"
#    include "../i2c.h"
#    include "../jpeg.h"
#    include "../malloc.h"
#    include "../xmodem.h"
#    define assert(x)
#    undef BIG_ENDIAN
#    define NO_CALLOC
#    define NO_REALLOC
#    define BROKEN_FLOAT_CASTS
#    define BUILTIN_MINI_STDLIB
#   else
/* uMon 嵌入式主机配置 — 128KB 堆，无浮点，使用 monlib 内存函数 */
#    ifdef UMON_HOST
#     define HEAP_SIZE (128*1024)               /* space for the heap and the stack */
#     define NO_FP
#     define BUILTIN_MINI_STDLIB
#     include <stdlib.h>
#     include <string.h>
#     include <ctype.h>
#     include <sys/types.h>
#     include <stdarg.h>
#     include <math.h>
#     include "monlib.h"
#     define assert(x)
#     define malloc mon_malloc
#     define calloc(a,b) mon_malloc(a*b)
#     define realloc mon_realloc
#     define free mon_free
#    endif
#   endif
#  endif

extern int ExitBuf[];

# endif
#endif
#endif

#endif /* PLATFORM_H */
