/* ============================================================================
 * string.c - PicoC 字符串处理库 (string.h)
 *
 * 为 PicoC 解释器提供 C 标准 <string.h> 头文件中定义的字符串和内存
 * 操作函数。所有函数直接委托给系统 libc 实现。
 *
 * 功能分组:
 *   - 字符串复制:  strcpy / strncpy / strdup
 *   - 字符串拼接:  strcat / strncat
 *   - 字符串比较:  strcmp / strncmp / strcoll
 *   - 字符串搜索:  strchr / strrchr / strstr / strspn / strcspn / strpbrk
 *   - 字符串分割:  strtok / strtok_r
 *   - 字符串长度:  strlen
 *   - 字符串变换:  strxfrm
 *   - 错误信息:    strerror
 *   - 内存操作:    memcpy / memmove / memset / memcmp / memchr
 *   - BSD 兼容:    index / rindex (非 WIN32)
 *
 * 小嵌入式系统使用 clibrary.c 中的精简版本代替本文件。
 * ============================================================================ */

#include "../interpreter.h"

static int String_ZeroValue = 0;

/* =========================================================================
 * 字符串复制函数
 * ========================================================================= */

/* strcpy: 将 src 字符串复制到 dst (包括结尾 '\0')，返回 dst */
void StringStrcpy(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Pointer = strcpy(Param[0]->Val->Pointer, Param[1]->Val->Pointer);
}

/* strncpy: 最多复制 n 个字符，若 src 长度不足 n 则用 '\0' 填充 */
void StringStrncpy(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Pointer = strncpy(Param[0]->Val->Pointer, Param[1]->Val->Pointer, Param[2]->Val->Integer);
}

#ifndef WIN32
/* strdup: 复制字符串到新分配的内存 (内部使用 malloc + memcpy) */
void StringStrdup(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    char *s = (char *)Param[0]->Val->Pointer;
    size_t len = strlen(s) + 1;
    char *p = (char *)malloc(len);
    if (p) memcpy(p, s, len);
    ReturnValue->Val->Pointer = p;
}
#endif

/* =========================================================================
 * 字符串拼接函数
 * ========================================================================= */

/* strcat: 将 src 追加到 dst 末尾，返回 dst */
void StringStrcat(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Pointer = strcat(Param[0]->Val->Pointer, Param[1]->Val->Pointer);
}

/* strncat: 将 src 的最多 n 个字符追加到 dst 末尾 */
void StringStrncat(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Pointer = strncat(Param[0]->Val->Pointer, Param[1]->Val->Pointer, Param[2]->Val->Integer);
}

/* =========================================================================
 * 字符串比较函数
 * ========================================================================= */

/* strcmp: 比较两个字符串，返回值 <0 / 0 / >0 */
void StringStrcmp(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = strcmp(Param[0]->Val->Pointer, Param[1]->Val->Pointer);
}

/* strncmp: 比较两个字符串的最多 n 个字符 */
void StringStrncmp(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = strncmp(Param[0]->Val->Pointer, Param[1]->Val->Pointer, Param[2]->Val->Integer);
}

/* strcoll: 按当前区域设置 (locale) 比较两个字符串 */
void StringStrcoll(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = strcoll(Param[0]->Val->Pointer, Param[1]->Val->Pointer);
}

/* =========================================================================
 * 字符串搜索函数
 * ========================================================================= */

#ifndef WIN32
/* index: 查找字符首次出现 (BSD 风格，等同 strchr) */
void StringIndex(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Pointer = strchr(Param[0]->Val->Pointer, Param[1]->Val->Integer);
}

/* rindex: 查找字符最后一次出现 (BSD 风格，等同 strrchr) */
void StringRindex(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Pointer = strrchr(Param[0]->Val->Pointer, Param[1]->Val->Integer);
}
#endif

/* strchr: 查找字符在字符串中首次出现的位置 */
void StringStrchr(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Pointer = strchr(Param[0]->Val->Pointer, Param[1]->Val->Integer);
}

/* strrchr: 查找字符在字符串中最后一次出现的位置 */
void StringStrrchr(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Pointer = strrchr(Param[0]->Val->Pointer, Param[1]->Val->Integer);
}

/* strstr: 查找子串 needle 在 haystack 中首次出现的位置 */
void StringStrstr(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Pointer = strstr(Param[0]->Val->Pointer, Param[1]->Val->Pointer);
}

/* strspn: 计算由 accept 中字符组成的最大前缀长度 */
void StringStrspn(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = strspn(Param[0]->Val->Pointer, Param[1]->Val->Pointer);
}

/* strcspn: 计算不包含 reject 中任何字符的最大前缀长度 */
void StringStrcspn(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = strcspn(Param[0]->Val->Pointer, Param[1]->Val->Pointer);
}

/* strpbrk: 查找字符串中第一个出现在 accept 中的字符 */
void StringStrpbrk(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Pointer = strpbrk(Param[0]->Val->Pointer, Param[1]->Val->Pointer);
}

/* =========================================================================
 * 字符串分割函数
 * ========================================================================= */

/* strtok: 使用分隔符拆分字符串 (不可重入，内部有静态状态) */
void StringStrtok(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Pointer = strtok(Param[0]->Val->Pointer, Param[1]->Val->Pointer);
}

#ifndef WIN32
/* strtok_r: 使用分隔符拆分字符串 (可重入版本) */
void StringStrtok_r(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Pointer = strtok_r(Param[0]->Val->Pointer, Param[1]->Val->Pointer, Param[2]->Val->Pointer);
}
#endif

/* =========================================================================
 * 其他字符串函数
 * ========================================================================= */

/* strlen: 计算字符串长度 (不包括结尾 '\0') */
void StringStrlen(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = strlen(Param[0]->Val->Pointer);
}

/* strerror: 返回 errno 值对应的错误描述字符串 */
void StringStrerror(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Pointer = strerror(Param[0]->Val->Integer);
}

/* strxfrm: 根据当前区域设置转换字符串的前 n 个字符 */
void StringStrxfrm(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = strxfrm(Param[0]->Val->Pointer, Param[1]->Val->Pointer, Param[2]->Val->Integer);
}

/* =========================================================================
 * 内存操作函数
 * ========================================================================= */

/* memset: 用字节值 c 填充内存区域的前 n 个字节 */
void StringMemset(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Pointer = memset(Param[0]->Val->Pointer, Param[1]->Val->Integer, Param[2]->Val->Integer);
}

/* memcpy: 复制 n 个字节 (源和目标不可重叠) */
void StringMemcpy(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Pointer = memcpy(Param[0]->Val->Pointer, Param[1]->Val->Pointer, Param[2]->Val->Integer);
}

/* memcmp: 比较两个内存区域的前 n 个字节 */
void StringMemcmp(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = memcmp(Param[0]->Val->Pointer, Param[1]->Val->Pointer, Param[2]->Val->Integer);
}

/* memmove: 复制 n 个字节 (源和目标可以重叠，更安全的 memcpy) */
void StringMemmove(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Pointer = memmove(Param[0]->Val->Pointer, Param[1]->Val->Pointer, Param[2]->Val->Integer);
}

/* memchr: 在内存区域的前 n 个字节中查找字节值 c */
void StringMemchr(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Pointer = memchr(Param[0]->Val->Pointer, Param[1]->Val->Integer, Param[2]->Val->Integer);
}

/* string 函数注册表: 将函数指针映射到 C 原型声明 */
struct LibraryFunction StringFunctions[] =
{
#ifndef WIN32
	{ StringIndex,         "char *index(char *,int);" },
    { StringRindex,        "char *rindex(char *,int);" },
#endif
    { StringMemcpy,        "void *memcpy(void *,void *,int);" },
    { StringMemmove,       "void *memmove(void *,void *,int);" },
    { StringMemchr,        "void *memchr(char *,int,int);" },
    { StringMemcmp,        "int memcmp(void *,void *,int);" },
    { StringMemset,        "void *memset(void *,int,int);" },
    { StringStrcat,        "char *strcat(char *,char *);" },
    { StringStrncat,       "char *strncat(char *,char *,int);" },
    { StringStrchr,        "char *strchr(char *,int);" },
    { StringStrrchr,       "char *strrchr(char *,int);" },
    { StringStrcmp,        "int strcmp(char *,char *);" },
    { StringStrncmp,       "int strncmp(char *,char *,int);" },
    { StringStrcoll,       "int strcoll(char *,char *);" },
    { StringStrcpy,        "char *strcpy(char *,char *);" },
    { StringStrncpy,       "char *strncpy(char *,char *,int);" },
    { StringStrerror,      "char *strerror(int);" },
    { StringStrlen,        "int strlen(char *);" },
    { StringStrspn,        "int strspn(char *,char *);" },
    { StringStrcspn,       "int strcspn(char *,char *);" },
    { StringStrpbrk,       "char *strpbrk(char *,char *);" },
    { StringStrstr,        "char *strstr(char *,char *);" },
    { StringStrtok,        "char *strtok(char *,char *);" },
    { StringStrxfrm,       "int strxfrm(char *,char *,int);" },
#ifndef WIN32
	{ StringStrdup,        "char *strdup(char *);" },
    { StringStrtok_r,      "char *strtok_r(char *,char *,char **);" },
#endif
    { NULL,             NULL }
};

/* StringSetupFunc: 初始化 string 库，定义 NULL 常量 */
void StringSetupFunc(Picoc *pc)
{
    /* 定义 NULL (如果尚未由其他模块定义) */
    if (!VariableDefined(pc, TableStrRegister(pc, "NULL")))
        VariableDefinePlatformVar(pc, NULL, "NULL", &pc->IntType, (union AnyValue *)&String_ZeroValue, FALSE);
}
