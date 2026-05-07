/* ============================================================================
 * ctype.c - PicoC 字符分类库 (ctype.h)
 *
 * 为 PicoC 解释器提供 C 标准 <ctype.h> 头文件中定义的字符类型判断
 * 和字符大小写转换函数。所有函数以 int 为参数和返回值。
 *
 * 功能分组:
 *   - 字符分类: isalnum / isalpha / isblank / iscntrl / isdigit / isgraph /
 *               islower / isprint / ispunct / isspace / isupper / isxdigit
 *   - 字符转换: tolower / toupper
 *   - 扩展函数: isascii / toascii (非 C 标准，但常用)
 *
 * 参数限制: 所有函数参数应为 unsigned char 值或 EOF(-1)。
 * isblank 直接判断空格和制表符，不依赖系统 ctype.h。
 * ============================================================================ */

#include <ctype.h>
#include "../interpreter.h"

/* =========================================================================
 * 字符分类函数 - 返回非零 (true) 或零 (false)
 * ========================================================================= */

/* isalnum: 判断是否为字母或数字 (A-Z, a-z, 0-9) */
void StdIsalnum(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = isalnum(Param[0]->Val->Integer);
}

/* isalpha: 判断是否为字母 (A-Z, a-z) */
void StdIsalpha(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = isalpha(Param[0]->Val->Integer);
}

/* isblank: 判断是否为空白字符 (空格 ' ' 或制表符 '\t') */
void StdIsblank(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    int ch = Param[0]->Val->Integer;
    ReturnValue->Val->Integer = (ch == ' ') | (ch == '\t');
}

/* iscntrl: 判断是否为控制字符 (0x00-0x1F 或 0x7F DEL) */
void StdIscntrl(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = iscntrl(Param[0]->Val->Integer);
}

/* isdigit: 判断是否为十进制数字 (0-9) */
void StdIsdigit(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = isdigit(Param[0]->Val->Integer);
}

/* isgraph: 判断是否为可打印字符 (空格除外，即 isprint 且非空格) */
void StdIsgraph(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = isgraph(Param[0]->Val->Integer);
}

/* islower: 判断是否为小写字母 (a-z) */
void StdIslower(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = islower(Param[0]->Val->Integer);
}

/* isprint: 判断是否为可打印字符 (包括空格，0x20-0x7E) */
void StdIsprint(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = isprint(Param[0]->Val->Integer);
}

/* ispunct: 判断是否为标点符号 (可打印但非字母数字非空格) */
void StdIspunct(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = ispunct(Param[0]->Val->Integer);
}

/* isspace: 判断是否为空白字符 (空格、制表符、换行、回车等) */
void StdIsspace(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = isspace(Param[0]->Val->Integer);
}

/* isupper: 判断是否为大写字母 (A-Z) */
void StdIsupper(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = isupper(Param[0]->Val->Integer);
}

/* isxdigit: 判断是否为十六进制数字 (0-9, A-F, a-f) */
void StdIsxdigit(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = isxdigit(Param[0]->Val->Integer);
}

/* =========================================================================
 * 字符转换函数
 * ========================================================================= */

/* tolower: 将大写字母转换为小写，非大写字母保持原样 */
void StdTolower(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = tolower(Param[0]->Val->Integer);
}

/* toupper: 将小写字母转换为大写，非小写字母保持原样 */
void StdToupper(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = toupper(Param[0]->Val->Integer);
}

/* =========================================================================
 * 扩展函数 (非 C 标准，来源于 UNIX/BSD 传统)
 * ========================================================================= */

/* isascii: 判断是否为 ASCII 字符 (值在 0-127 范围内) */
void StdIsascii(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    int c = Param[0]->Val->Integer;
    ReturnValue->Val->Integer = (c >= 0 && c <= 127);
}

/* toascii: 将字符截断为低 7 位 (与 0x7F 按位与) */
void StdToascii(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = Param[0]->Val->Integer & 0x7F;
}

/* ctype 函数注册表: 将函数指针映射到 C 原型声明 */
struct LibraryFunction StdCtypeFunctions[] =
{
    { StdIsalnum,      "int isalnum(int);" },
    { StdIsalpha,      "int isalpha(int);" },
    { StdIsblank,      "int isblank(int);" },
    { StdIscntrl,      "int iscntrl(int);" },
    { StdIsdigit,      "int isdigit(int);" },
    { StdIsgraph,      "int isgraph(int);" },
    { StdIslower,      "int islower(int);" },
    { StdIsprint,      "int isprint(int);" },
    { StdIspunct,      "int ispunct(int);" },
    { StdIsspace,      "int isspace(int);" },
    { StdIsupper,      "int isupper(int);" },
    { StdIsxdigit,     "int isxdigit(int);" },
    { StdTolower,      "int tolower(int);" },
    { StdToupper,      "int toupper(int);" },
    { StdIsascii,      "int isascii(int);" },
    { StdToascii,      "int toascii(int);" },
    { NULL,             NULL }
};
