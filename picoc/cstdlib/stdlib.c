/* ============================================================================
 * stdlib.c - PicoC 标准库 (stdlib.h)
 *
 * 为 PicoC 解释器提供 C 标准 <stdlib.h> 头文件中定义的通用工具函数，
 * 包含字符串转换、动态内存管理、伪随机数、程序控制和数学工具函数。
 *
 * 功能分组:
 *   - 字符串转换: atoi / atol / atof / strtol / strtoul / strtod
 *   - 内存管理:   malloc / calloc / realloc / free (直接调用系统 libc)
 *   - 随机数:     rand / srand
 *   - 程序控制:   abort / exit / system / getenv (部分为桩实现)
 *   - 数学工具:   abs / labs
 *
 * 注意: getenv() 始终返回 NULL, system() 始终返回 -1,
 *       因为 PicoC 不支持环境变量和外部命令执行。
 *
 * 小嵌入式系统使用 clibrary.c 中的精简版本代替本文件。
 * ============================================================================ */

#include "../interpreter.h"

static int Stdlib_ZeroValue = 0;

/* =========================================================================
 * 字符串转换函数
 * ========================================================================= */

#ifndef NO_FP
/* atof: 将字符串转换为双精度浮点数 */
void StdlibAtof(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->FP = atof(Param[0]->Val->Pointer);
}
#endif

/* atoi: 将字符串转换为整数 (int) */
void StdlibAtoi(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = atoi(Param[0]->Val->Pointer);
}

/* atol: 将字符串转换为长整数 (long int) */
void StdlibAtol(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = atol(Param[0]->Val->Pointer);
}

#ifndef NO_FP
/* strtod: 将字符串转换为双精度浮点数，返回首个未转换字符的指针 */
void StdlibStrtod(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->FP = strtod(Param[0]->Val->Pointer, Param[1]->Val->Pointer);
}
#endif

/* strtol: 将字符串按指定基数 (0/2-36) 转换为长整数 */
void StdlibStrtol(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = strtol(Param[0]->Val->Pointer, Param[1]->Val->Pointer, Param[2]->Val->Integer);
}

/* strtoul: 将字符串按指定基数转换为无符号长整数 */
void StdlibStrtoul(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = strtoul(Param[0]->Val->Pointer, Param[1]->Val->Pointer, Param[2]->Val->Integer);
}

/* =========================================================================
 * 动态内存管理函数 (直接映射到系统 malloc/free)
 * ========================================================================= */

/* malloc: 分配指定字节数的内存块，返回 void* 指针 */
void StdlibMalloc(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Pointer = malloc(Param[0]->Val->Integer);
}

/* calloc: 分配 n 个元素、每个大小为 size 的数组内存，并清零 */
void StdlibCalloc(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Pointer = calloc(Param[0]->Val->Integer, Param[1]->Val->Integer);
}

/* realloc: 重新调整已分配内存块的大小 */
void StdlibRealloc(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Pointer = realloc(Param[0]->Val->Pointer, Param[1]->Val->Integer);
}

/* free: 释放通过 malloc/calloc/realloc 分配的内存 */
void StdlibFree(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    free(Param[0]->Val->Pointer);
}

/* =========================================================================
 * 伪随机数函数
 * ========================================================================= */

/* rand: 生成 [0, RAND_MAX] 范围内的伪随机整数 */
void StdlibRand(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = rand();
}

/* srand: 设置伪随机数生成器的种子值 */
void StdlibSrand(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    srand(Param[0]->Val->Integer);
}

/* =========================================================================
 * 程序控制函数
 * ========================================================================= */

/* abort: 异常终止 PicoC 程序执行 */
void StdlibAbort(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ProgramFail(Parser, "abort");
}

/* exit: 调用平台相关的退出函数终止程序 */
void StdlibExit(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    PlatformExit(Parser->pc, Param[0]->Val->Integer);
}

/* getenv: 获取环境变量 (PicoC 不支持，始终返回 NULL) */
void StdlibGetenv(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Pointer = NULL;
}

/* system: 执行系统命令 (PicoC 不支持，始终返回 -1) */
void StdlibSystem(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = -1;
}

#if 0
void StdlibBsearch(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Pointer = bsearch(Param[0]->Val->Pointer, Param[1]->Val->Pointer, Param[2]->Val->Integer, Param[3]->Val->Integer, (int (*)())Param[4]->Val->Pointer);
}
#endif

/* =========================================================================
 * 数学工具函数
 * ========================================================================= */

/* abs: 计算整数的绝对值 */
void StdlibAbs(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = abs(Param[0]->Val->Integer);
}

/* labs: 计算长整数的绝对值 */
void StdlibLabs(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = labs(Param[0]->Val->Integer);
}

#if 0
void StdlibDiv(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = div(Param[0]->Val->Integer, Param[1]->Val->Integer);
}

void StdlibLdiv(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = ldiv(Param[0]->Val->Integer, Param[1]->Val->Integer);
}
#endif

#if 0
/* 结构体定义: div_t (商和余数) */
const char StdlibDefs[] = "\
typedef struct { \
    int quot, rem; \
} div_t; \
\
typedef struct { \
    int quot, rem; \
} ldiv_t; \
";
#endif

/* stdlib 函数注册表: 将函数指针映射到 C 原型声明 */
struct LibraryFunction StdlibFunctions[] =
{
#ifndef NO_FP
    { StdlibAtof,           "float atof(char *);" },
    { StdlibStrtod,         "float strtod(char *,char **);" },
#endif
    { StdlibAtoi,           "int atoi(char *);" },
    { StdlibAtol,           "int atol(char *);" },
    { StdlibStrtol,         "int strtol(char *,char **,int);" },
    { StdlibStrtoul,        "int strtoul(char *,char **,int);" },
    { StdlibMalloc,         "void *malloc(int);" },
    { StdlibCalloc,         "void *calloc(int,int);" },
    { StdlibRealloc,        "void *realloc(void *,int);" },
    { StdlibFree,           "void free(void *);" },
    { StdlibRand,           "int rand();" },
    { StdlibSrand,          "void srand(int);" },
    { StdlibAbort,          "void abort();" },
    { StdlibExit,           "void exit(int);" },
    { StdlibGetenv,         "char *getenv(char *);" },
    { StdlibSystem,         "int system(char *);" },
/*    { StdlibBsearch,        "void *bsearch(void *,void *,int,int,int (*)());" }, */
/*    { StdlibQsort,          "void *qsort(void *,int,int,int (*)());" }, */
    { StdlibAbs,            "int abs(int);" },
    { StdlibLabs,           "int labs(int);" },
#if 0
    { StdlibDiv,            "div_t div(int);" },
    { StdlibLdiv,           "ldiv_t ldiv(int);" },
#endif
    { NULL,                 NULL }
};

/* StdlibSetupFunc: 初始化 stdlib 库，定义 NULL 常量 */
void StdlibSetupFunc(Picoc *pc)
{
    /* 定义 NULL (如果尚未由其他模块定义) */
    if (!VariableDefined(pc, TableStrRegister(pc, "NULL")))
        VariableDefinePlatformVar(pc, NULL, "NULL", &pc->IntType, (union AnyValue *)&Stdlib_ZeroValue, FALSE);
}
