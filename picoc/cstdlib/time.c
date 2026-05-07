/* ============================================================================
 * time.c - PicoC 时间库 (time.h)
 *
 * 为 PicoC 解释器提供 C 标准 <time.h> 头文件中定义的时间和日期
 * 操作函数。仅在未定义 BUILTIN_MINI_STDLIB 时编译。
 *
 * 功能分组:
 *   - 时间获取:       time / clock
 *   - 时间转换:       ctime / asctime / gmtime / localtime / mktime
 *   - 时间格式化:     strftime / strptime (strptime 非 WIN32)
 *   - 时间差计算:     difftime
 *   - 线程安全版本:   gmtime_r (非 WIN32)
 *
 * 注意: struct tm 通过 TypeCreateOpaqueStruct 创建为不透明结构体，
 *       与系统原生 struct tm 大小相同。time_t 和 clock_t 定义为 int。
 * ============================================================================ */

#include <time.h>
#include "../interpreter.h"

#ifndef BUILTIN_MINI_STDLIB

static int CLOCKS_PER_SECValue = CLOCKS_PER_SEC;

#ifdef CLK_PER_SEC
static int CLK_PER_SECValue = CLK_PER_SEC;
#endif

#ifdef CLK_TCK
static int CLK_TCKValue = CLK_TCK;
#endif

/* =========================================================================
 * 时间转换函数
 * ========================================================================= */

/* asctime: 将 struct tm 结构转换为字符串 "Www Mmm dd hh:mm:ss yyyy\n" */
void StdAsctime(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Pointer = asctime(Param[0]->Val->Pointer);
}

/* ctime: 将 time_t 时间戳转换为字符串 */
void StdCtime(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Pointer = ctime(Param[0]->Val->Pointer);
}

/* gmtime: 将 time_t 时间戳转换为 UTC 时间的 struct tm */
void StdGmtime(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Pointer = gmtime(Param[0]->Val->Pointer);
}

/* localtime: 将 time_t 时间戳转换为本地时间的 struct tm */
void StdLocaltime(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Pointer = localtime(Param[0]->Val->Pointer);
}

/* mktime: 将 struct tm 结构转换为 time_t 时间戳 */
void StdMktime(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = (int)mktime(Param[0]->Val->Pointer);
}

/* =========================================================================
 * 时间格式化函数
 * ========================================================================= */

/* strftime: 将 struct tm 按指定格式转换为字符串 */
void StdStrftime(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = strftime(Param[0]->Val->Pointer, Param[1]->Val->Integer, Param[2]->Val->Pointer, Param[3]->Val->Pointer);
}

#ifndef WIN32
/* strptime: 将字符串按指定格式解析为 struct tm (strftime 的逆操作) */
void StdStrptime(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
	  extern char *strptime(const char *s, const char *format, struct tm *tm);

    ReturnValue->Val->Pointer = strptime(Param[0]->Val->Pointer, Param[1]->Val->Pointer, Param[2]->Val->Pointer);
}

/* gmtime_r: gmtime 的可重入版本 */
void StdGmtime_r(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Pointer = gmtime_r(Param[0]->Val->Pointer, Param[1]->Val->Pointer);
}

/* timegm: 将 UTC 时间的 struct tm 转换为 time_t (mktime 的 UTC 版本) */
void StdTimegm(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = timegm(Param[0]->Val->Pointer);
}
#endif

/* =========================================================================
 * 时间获取和计算函数
 * ========================================================================= */

/* clock: 返回程序使用的处理器时间 (单位: CLOCKS_PER_SEC) */
void StdClock(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = clock();
}

#ifndef NO_FP
/* difftime: 计算两个 time_t 时间戳之间的差值 (秒)，返回 double */
void StdDifftime(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->FP = difftime((time_t)Param[0]->Val->Integer, Param[1]->Val->Integer);
}
#endif

/* time: 获取当前系统时间 (1970-01-01 00:00:00 UTC 以来的秒数) */
void StdTime(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = (int)time(Param[0]->Val->Pointer);
}

/* 类型定义: time_t 和 clock_t (均为 int) */
const char StdTimeDefs[] = "\
typedef int time_t; \
typedef int clock_t;\
";

/* time 函数注册表 */
struct LibraryFunction StdTimeFunctions[] =
{
    { StdAsctime,       "char *asctime(struct tm *);" },
    { StdClock,         "time_t clock();" },
    { StdCtime,         "char *ctime(int *);" },
#ifndef NO_FP
    { StdDifftime,      "double difftime(int, int);" },
#endif
    { StdGmtime,        "struct tm *gmtime(int *);" },
    { StdLocaltime,     "struct tm *localtime(int *);" },
    { StdMktime,        "int mktime(struct tm *ptm);" },
    { StdTime,          "int time(int *);" },
    { StdStrftime,      "int strftime(char *, int, char *, struct tm *);" },
#ifndef WIN32
    { StdStrptime,      "char *strptime(char *, char *, struct tm *);" },
	{ StdGmtime_r,      "struct tm *gmtime_r(int *, struct tm *);" },
    { StdTimegm,        "int timegm(struct tm *);" },
#endif
    { NULL,             NULL }
};


/* StdTimeSetupFunc: 初始化 time 库
 * - 创建 struct tm 不透明结构体 (与系统原生 tm 大小相同)
 * - 注册 CLOCKS_PER_SEC, CLK_PER_SEC, CLK_TCK 等时间常量 */
void StdTimeSetupFunc(Picoc *pc)
{
    /* 创建与原生 tm 结构相同大小的 struct tm */
    TypeCreateOpaqueStruct(pc, NULL, TableStrRegister(pc, "tm"), sizeof(struct tm));

    /* 注册时间相关宏常量 */
    VariableDefinePlatformVar(pc, NULL, "CLOCKS_PER_SEC", &pc->IntType, (union AnyValue *)&CLOCKS_PER_SECValue, FALSE);
#ifdef CLK_PER_SEC
    VariableDefinePlatformVar(pc, NULL, "CLK_PER_SEC", &pc->IntType, (union AnyValue *)&CLK_PER_SECValue, FALSE);
#endif
#ifdef CLK_TCK
    VariableDefinePlatformVar(pc, NULL, "CLK_TCK", &pc->IntType, (union AnyValue *)&CLK_TCKValue, FALSE);
#endif
}

#endif /* !BUILTIN_MINI_STDLIB */
