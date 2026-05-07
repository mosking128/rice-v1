/* ============================================================================
 * stdio.c - PicoC 标准输入输出库 (stdio.h)
 *
 * 为 PicoC 解释器提供 C 标准 <stdio.h> 头文件中定义的格式化 I/O 功能，
 * 包含 printf / sprintf / fprintf / scanf / fopen / fclose 等核心函数。
 *
 * 架构说明:
 *   - StdioBasePrintf()  统一格式化输出引擎，负责解析 %d/%f/%s 等转换符，
 *                         支持输出到 FILE* 流或字符串缓冲区
 *   - StdioBaseScanf()   统一格式化输入引擎
 *   - StdOutStream       内部输出流抽象，可同时面向 FILE* 和内存字符串
 *   - StdVararg           PicoC 内部可变参数表示
 *
 * 条件编译:
 *   - BUILTIN_MINI_STDLIB 定义时跳过本文件 (嵌入式系统使用 clibrary.c)
 *   - NO_FP               定义时禁用浮点格式 (%e/%f/%g)
 *   - WIN32               定义时使用 _snprintf/_fileno 等 Windows 变体
 *
 * 小嵌入式系统使用 clibrary.c 中的精简版本代替本文件。
 * ============================================================================ */

#include "../interpreter.h"
#ifndef BUILTIN_MINI_STDLIB

#include <errno.h>

#define MAX_FORMAT 80
#define MAX_SCANF_ARGS 10

static int Stdio_ZeroValue = 0;
static int EOFValue = EOF;
static int SEEK_SETValue = SEEK_SET;
static int SEEK_CURValue = SEEK_CUR;
static int SEEK_ENDValue = SEEK_END;
static int BUFSIZValue = BUFSIZ;
static int FILENAME_MAXValue = FILENAME_MAX;
static int _IOFBFValue = _IOFBF;
static int _IOLBFValue = _IOLBF;
static int _IONBFValue = _IONBF;
static int L_tmpnamValue = L_tmpnam;
static int GETS_MAXValue = 255;     /* arbitrary maximum size of a gets() file */

static FILE *stdinValue;
static FILE *stdoutValue;
static FILE *stderrValue;


/* 内部输出流结构体: 可输出到 FILE* 或字符串缓冲区 */
typedef struct StdOutStreamStruct
{
    FILE *FilePtr;
    char *StrOutPtr;
    int StrOutLen;
    int CharCount;

} StdOutStream;

/* PicoC 内部可变参数表示 (va_list 的替代) */
struct StdVararg
{
    struct Value **Param;
    int NumArgs;
};

/* 初始化 I/O 系统，将 PicoC 输出流绑定到系统 stdout */
void BasicIOInit(Picoc *pc)
{
    pc->CStdOut = stdout;
    stdinValue = stdin;
    stdoutValue = stdout;
    stderrValue = stderr;
}

/* 输出单个字符到 FILE* 或字符串缓冲区 */
void StdioOutPutc(int OutCh, StdOutStream *Stream)
{
    if (Stream->FilePtr != NULL)
    {
        /* 输出到标准 I/O 流 */
        putc(OutCh, Stream->FilePtr);
        Stream->CharCount++;
    }
    else if (Stream->StrOutLen < 0 || Stream->StrOutLen > 1)
    {
        /* 输出到字符串 */
        *Stream->StrOutPtr = OutCh;
        Stream->StrOutPtr++;

        if (Stream->StrOutLen > 1)
            Stream->StrOutLen--;

        Stream->CharCount++;
    }
}

/* 输出字符串到 FILE* 或字符串缓冲区 */
void StdioOutPuts(const char *Str, StdOutStream *Stream)
{
    if (Stream->FilePtr != NULL)
    {
        /* 输出到标准 I/O 流 */
        fputs(Str, Stream->FilePtr);
    }
    else
    {
        /* 输出到字符串 */
        while (*Str != '\0')
        {
            if (Stream->StrOutLen < 0 || Stream->StrOutLen > 1)
            {
                *Stream->StrOutPtr = *Str;
                Str++;
                Stream->StrOutPtr++;

                if (Stream->StrOutLen > 1)
                    Stream->StrOutLen--;

                Stream->CharCount++;
            }
        }
    }
}

/* printf 风格的整数/字长对象格式化输出 */
void StdioFprintfWord(StdOutStream *Stream, const char *Format, unsigned long Value)
{
    if (Stream->FilePtr != NULL)
        Stream->CharCount += fprintf(Stream->FilePtr, Format, Value);

    else if (Stream->StrOutLen >= 0)
    {
#ifndef WIN32
			int CCount = snprintf(Stream->StrOutPtr, Stream->StrOutLen, Format, Value);
#else
			int CCount = _snprintf(Stream->StrOutPtr, Stream->StrOutLen, Format, Value);
#endif
			Stream->StrOutPtr += CCount;
	        Stream->StrOutLen -= CCount;
	        Stream->CharCount += CCount;
    }
    else
    {
        int CCount = sprintf(Stream->StrOutPtr, Format, Value);
        Stream->CharCount += CCount;
        Stream->StrOutPtr += CCount;
    }
}

/* printf 风格的浮点数格式化输出 */
void StdioFprintfFP(StdOutStream *Stream, const char *Format, double Value)
{
    if (Stream->FilePtr != NULL)
        Stream->CharCount += fprintf(Stream->FilePtr, Format, Value);

    else if (Stream->StrOutLen >= 0)
    {
#ifndef WIN32
        int CCount = snprintf(Stream->StrOutPtr, Stream->StrOutLen, Format, Value);
#else
        int CCount = _snprintf(Stream->StrOutPtr, Stream->StrOutLen, Format, Value);
#endif
			Stream->StrOutPtr += CCount;
        Stream->StrOutLen -= CCount;
        Stream->CharCount += CCount;
    }
    else
    {
        int CCount = sprintf(Stream->StrOutPtr, Format, Value);
        Stream->CharCount += CCount;
        Stream->StrOutPtr += CCount;
    }
}

/* printf 风格的指针格式化输出 */
void StdioFprintfPointer(StdOutStream *Stream, const char *Format, void *Value)
{
    if (Stream->FilePtr != NULL)
        Stream->CharCount += fprintf(Stream->FilePtr, Format, Value);

    else if (Stream->StrOutLen >= 0)
    {
#ifndef WIN32
        int CCount = snprintf(Stream->StrOutPtr, Stream->StrOutLen, Format, Value);
#else
			int CCount = _snprintf(Stream->StrOutPtr, Stream->StrOutLen, Format, Value);
#endif
        Stream->StrOutPtr += CCount;
        Stream->StrOutLen -= CCount;
        Stream->CharCount += CCount;
    }
    else
    {
        int CCount = sprintf(Stream->StrOutPtr, Format, Value);
        Stream->CharCount += CCount;
        Stream->StrOutPtr += CCount;
    }
}

/* =========================================================================
 * StdioBasePrintf() - 核心 printf 引擎
 *
 * 处理所有 v/s/n/printf() 变体的格式化输出。遍历格式字符串，识别 % 转换
 * 说明符 (d/i/o/u/x/X/e/E/f/F/g/G/a/A/c/s/p/n/m/%)，从可变参数列表中读取
 * 对应类型的值并格式化输出到 FILE* 流或内存字符串。
 *
 * 参数:
 *   Parser     - 解析器状态
 *   Stream     - 目标 FILE* 流 (可为 NULL 表示输出到字符串)
 *   StrOut     - 目标字符串缓冲区 (Stream 为 NULL 时使用)
 *   StrOutLen  - 缓冲区长度 (0 为只计数不写入, -1 为无限制)
 *   Format     - printf 格式字符串
 *   Args       - 可变参数封装
 *
 * 返回: 写入的字符总数
 * ========================================================================= */
int StdioBasePrintf(struct ParseState *Parser, FILE *Stream, char *StrOut, int StrOutLen, char *Format, struct StdVararg *Args)
{
    struct Value *ThisArg = Args->Param[0];
    int ArgCount = 0;
    char *FPos;
    char OneFormatBuf[MAX_FORMAT+1];
    int OneFormatCount;
    struct ValueType *ShowType;
    StdOutStream SOStream;
    Picoc *pc = Parser->pc;

    if (Format == NULL)
        Format = "[null format]\n";

    FPos = Format;
    SOStream.FilePtr = Stream;
    SOStream.StrOutPtr = StrOut;
    SOStream.StrOutLen = StrOutLen;
    SOStream.CharCount = 0;

    while (*FPos != '\0')
    {
        if (*FPos == '%')
        {
            /* 确定要打印的类型 */
            FPos++;
            ShowType = NULL;
            OneFormatBuf[0] = '%';
            OneFormatCount = 1;

            do
            {
                switch (*FPos)
                {
                    case 'd': case 'i':     ShowType = &pc->IntType; break;     /* 有符号十进制整数 */
                    case 'o': case 'u': case 'x': case 'X': ShowType = &pc->IntType; break; /* 进制转换 */
#ifndef NO_FP
                    case 'e': case 'E':     ShowType = &pc->FPType; break;      /* 双精度浮点，科学计数法 */
                    case 'f': case 'F':     ShowType = &pc->FPType; break;      /* 双精度浮点，定点格式 */
                    case 'g': case 'G':     ShowType = &pc->FPType; break;      /* 双精度浮点，自动选择 */
#endif
                    case 'a': case 'A':     ShowType = &pc->IntType; break;     /* 十六进制，0x 前缀格式 */
                    case 'c':               ShowType = &pc->IntType; break;     /* 单个字符 */
                    case 's':               ShowType = pc->CharPtrType; break;  /* 字符串 */
                    case 'p':               ShowType = pc->VoidPtrType; break;  /* 指针 */
                    case 'n':               ShowType = &pc->VoidType; break;    /* 已写入字符数 */
                    case 'm':               ShowType = &pc->VoidType; break;    /* strerror(errno) */
                    case '%':               ShowType = &pc->VoidType; break;    /* 字面量 '%' */
                    case '\0':              ShowType = &pc->VoidType; break;    /* 格式字符串结束 */
                }

                /* 将格式字符复制到 OneFormatBuf 暂存 */
                OneFormatBuf[OneFormatCount] = *FPos;
                OneFormatCount++;

                /* 根据转换类型执行特殊操作 */
                if (ShowType == &pc->VoidType)
                {
                    switch (*FPos)
                    {
                        case 'm':   StdioOutPuts(strerror(errno), &SOStream); break;
                        case '%':   StdioOutPutc(*FPos, &SOStream); break;
                        case '\0':  OneFormatBuf[OneFormatCount] = '\0'; StdioOutPutc(*FPos, &SOStream); break;
                        case 'n':   /* 将当前已写入字符数存入参数指向的 int */
                            ThisArg = (struct Value *)((char *)ThisArg + MEM_ALIGN(sizeof(struct Value) + TypeStackSizeValue(ThisArg)));
                            if (ThisArg->Typ->Base == TypeArray && ThisArg->Typ->FromType->Base == TypeInt)
                                *(int *)ThisArg->Val->Pointer = SOStream.CharCount;
                            break;
                    }
                }

                FPos++;

            } while (ShowType == NULL && OneFormatCount < MAX_FORMAT);

            if (ShowType != &pc->VoidType)
            {
                if (ArgCount >= Args->NumArgs)
                    StdioOutPuts("XXX", &SOStream);
                else
                {
                    /* 添加字符串结尾符 */
                    OneFormatBuf[OneFormatCount] = '\0';

                    /* 打印当前参数 */
                    ThisArg = (struct Value *)((char *)ThisArg + MEM_ALIGN(sizeof(struct Value) + TypeStackSizeValue(ThisArg)));
                    if (ShowType == &pc->IntType)
                    {
                        /* 显示有符号整数 */
                        if (IS_NUMERIC_COERCIBLE(ThisArg))
                            StdioFprintfWord(&SOStream, OneFormatBuf, ExpressionCoerceUnsignedInteger(ThisArg));
                        else
                            StdioOutPuts("XXX", &SOStream);
                    }
#ifndef NO_FP
                    else if (ShowType == &pc->FPType)
                    {
                        /* 显示浮点数 */
                        if (IS_NUMERIC_COERCIBLE(ThisArg))
                            StdioFprintfFP(&SOStream, OneFormatBuf, ExpressionCoerceFP(ThisArg));
                        else
                            StdioOutPuts("XXX", &SOStream);
                    }
#endif
                    else if (ShowType == pc->CharPtrType)
                    {
                        if (ThisArg->Typ->Base == TypePointer)
                            StdioFprintfPointer(&SOStream, OneFormatBuf, ThisArg->Val->Pointer);

                        else if (ThisArg->Typ->Base == TypeArray && ThisArg->Typ->FromType->Base == TypeChar)
                            StdioFprintfPointer(&SOStream, OneFormatBuf, &ThisArg->Val->ArrayMem[0]);

                        else
                            StdioOutPuts("XXX", &SOStream);
                    }
                    else if (ShowType == pc->VoidPtrType)
                    {
                        if (ThisArg->Typ->Base == TypePointer)
                            StdioFprintfPointer(&SOStream, OneFormatBuf, ThisArg->Val->Pointer);

                        else if (ThisArg->Typ->Base == TypeArray)
                            StdioFprintfPointer(&SOStream, OneFormatBuf, &ThisArg->Val->ArrayMem[0]);

                        else
                            StdioOutPuts("XXX", &SOStream);
                    }

                    ArgCount++;
                }
            }
        }
        else
        {
            /* 输出普通字符 */
            StdioOutPutc(*FPos, &SOStream);
            FPos++;
        }
    }

    /* 添加字符串结尾符 */
    if (SOStream.StrOutPtr != NULL && SOStream.StrOutLen > 0)
        *SOStream.StrOutPtr = '\0';

    return SOStream.CharCount;
}

/* =========================================================================
 * StdioBaseScanf() - 核心 scanf 引擎
 *
 * 处理所有 v/s/f/scanf() 变体的格式化输入。验证所有参数是否为指针类型,
 * 然后调用系统的 fscanf/sscanf 完成实际输入解析。
 * 最多支持 MAX_SCANF_ARGS 个参数。
 * ========================================================================= */
int StdioBaseScanf(struct ParseState *Parser, FILE *Stream, char *StrIn, char *Format, struct StdVararg *Args)
{
    struct Value *ThisArg = Args->Param[0];
    int ArgCount = 0;
    void *ScanfArg[MAX_SCANF_ARGS];

    if (Args->NumArgs > MAX_SCANF_ARGS)
        ProgramFail(Parser, "too many arguments to scanf() - %d max", MAX_SCANF_ARGS);

    for (ArgCount = 0; ArgCount < Args->NumArgs; ArgCount++)
    {
        ThisArg = (struct Value *)((char *)ThisArg + MEM_ALIGN(sizeof(struct Value) + TypeStackSizeValue(ThisArg)));

        if (ThisArg->Typ->Base == TypePointer)
            ScanfArg[ArgCount] = ThisArg->Val->Pointer;

        else if (ThisArg->Typ->Base == TypeArray)
            ScanfArg[ArgCount] = &ThisArg->Val->ArrayMem[0];

        else
            ProgramFail(Parser, "non-pointer argument to scanf() - argument %d after format", ArgCount+1);
    }

    if (Stream != NULL)
        return fscanf(Stream, Format, ScanfArg[0], ScanfArg[1], ScanfArg[2], ScanfArg[3], ScanfArg[4], ScanfArg[5], ScanfArg[6], ScanfArg[7], ScanfArg[8], ScanfArg[9]);
    else
        return sscanf(StrIn, Format, ScanfArg[0], ScanfArg[1], ScanfArg[2], ScanfArg[3], ScanfArg[4], ScanfArg[5], ScanfArg[6], ScanfArg[7], ScanfArg[8], ScanfArg[9]);
}

/* =========================================================================
 * 文件操作函数 (fopen / fclose / fread / fwrite / ...)
 * ========================================================================= */

/* fopen: 打开文件，返回 FILE* 指针 */
void StdioFopen(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Pointer = fopen(Param[0]->Val->Pointer, Param[1]->Val->Pointer);
}

/* freopen: 将现有文件流重定向到新文件 */
void StdioFreopen(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Pointer = freopen(Param[0]->Val->Pointer, Param[1]->Val->Pointer, Param[2]->Val->Pointer);
}

/* fclose: 关闭文件流 */
void StdioFclose(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = fclose(Param[0]->Val->Pointer);
}

/* fread: 从文件流读取数据块 */
void StdioFread(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = fread(Param[0]->Val->Pointer, Param[1]->Val->Integer, Param[2]->Val->Integer, Param[3]->Val->Pointer);
}

/* fwrite: 向文件流写入数据块 */
void StdioFwrite(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = fwrite(Param[0]->Val->Pointer, Param[1]->Val->Integer, Param[2]->Val->Integer, Param[3]->Val->Pointer);
}

/* fgetc / getc: 从文件流读取一个字符 */
void StdioFgetc(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = fgetc(Param[0]->Val->Pointer);
}

/* fgets: 从文件流读取一行字符串 */
void StdioFgets(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Pointer = fgets(Param[0]->Val->Pointer, Param[1]->Val->Integer, Param[2]->Val->Pointer);
}

/* remove: 删除文件 */
void StdioRemove(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = remove(Param[0]->Val->Pointer);
}

/* rename: 重命名文件 */
void StdioRename(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = rename(Param[0]->Val->Pointer, Param[1]->Val->Pointer);
}

/* rewind: 重置文件位置指针到开头 */
void StdioRewind(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    rewind(Param[0]->Val->Pointer);
}

/* tmpfile: 创建临时文件 */
void StdioTmpfile(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Pointer = tmpfile();
}

/* clearerr: 清除文件流的错误和 EOF 标志 */
void StdioClearerr(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    clearerr((FILE *)Param[0]->Val->Pointer);
}

/* feof: 检查文件流是否到达末尾 */
void StdioFeof(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = feof((FILE *)Param[0]->Val->Pointer);
}

/* ferror: 检查文件流是否有错误 */
void StdioFerror(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = ferror((FILE *)Param[0]->Val->Pointer);
}

/* fileno: 获取文件流对应的文件描述符 */
void StdioFileno(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
#ifndef WIN32
    ReturnValue->Val->Integer = fileno(Param[0]->Val->Pointer);
#else
    ReturnValue->Val->Integer = _fileno(Param[0]->Val->Pointer);
#endif
}

/* fflush: 刷新文件流缓冲区 */
void StdioFflush(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = fflush(Param[0]->Val->Pointer);
}

/* fgetpos: 获取文件流的当前位置 */
void StdioFgetpos(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = fgetpos(Param[0]->Val->Pointer, Param[1]->Val->Pointer);
}

/* fsetpos: 设置文件流的当前位置 */
void StdioFsetpos(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = fsetpos(Param[0]->Val->Pointer, Param[1]->Val->Pointer);
}

/* fputc: 向文件流写入一个字符 */
void StdioFputc(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = fputc(Param[0]->Val->Integer, Param[1]->Val->Pointer);
}

/* fputs: 向文件流写入字符串 */
void StdioFputs(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = fputs(Param[0]->Val->Pointer, Param[1]->Val->Pointer);
}

/* ftell: 获取文件流当前偏移量 */
void StdioFtell(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = ftell(Param[0]->Val->Pointer);
}

/* fseek: 移动文件流位置指针 (SEEK_SET/SEEK_CUR/SEEK_END) */
void StdioFseek(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = fseek(Param[0]->Val->Pointer, Param[1]->Val->Integer, Param[2]->Val->Integer);
}

/* perror: 打印 errno 对应的错误信息 */
void StdioPerror(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    perror(Param[0]->Val->Pointer);
}

/* putc: 向文件流写入一个字符 (等同 fputc) */
void StdioPutc(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = putc(Param[0]->Val->Integer, Param[1]->Val->Pointer);
}

/* putchar / fputchar: 向标准输出写入一个字符 */
void StdioPutchar(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = putchar(Param[0]->Val->Integer);
}

/* setbuf: 设置文件流的缓冲区 */
void StdioSetbuf(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    setbuf(Param[0]->Val->Pointer, Param[1]->Val->Pointer);
}

/* setvbuf: 设置文件流的缓冲区及缓冲模式 (_IOFBF/_IOLBF/_IONBF) */
void StdioSetvbuf(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    setvbuf(Param[0]->Val->Pointer, Param[1]->Val->Pointer, Param[2]->Val->Integer, Param[3]->Val->Integer);
}

/* ungetc: 将字符退回输入流 */
void StdioUngetc(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = ungetc(Param[0]->Val->Integer, Param[1]->Val->Pointer);
}

/* puts: 向标准输出写入字符串并追加换行 */
void StdioPuts(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = puts(Param[0]->Val->Pointer);
}

/* gets: 从标准输入读取一行 (不推荐使用，无缓冲区溢出保护) */
void StdioGets(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Pointer = fgets(Param[0]->Val->Pointer, GETS_MAXValue, stdin);
    if (ReturnValue->Val->Pointer != NULL)
    {
        char *EOLPos = strchr(Param[0]->Val->Pointer, '\n');
        if (EOLPos != NULL)
            *EOLPos = '\0';
    }
}

/* getchar: 从标准输入读取一个字符 */
void StdioGetchar(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = getchar();
}

/* =========================================================================
 * printf 系列 - 格式化输出函数
 * ========================================================================= */

/* printf: 格式化输出到标准输出 stdout */
void StdioPrintf(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    struct StdVararg PrintfArgs;

    PrintfArgs.Param = Param;
    PrintfArgs.NumArgs = NumArgs-1;
    ReturnValue->Val->Integer = StdioBasePrintf(Parser, stdout, NULL, 0, Param[0]->Val->Pointer, &PrintfArgs);
}

/* vprintf: 使用 va_list 格式化输出到 stdout */
void StdioVprintf(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = StdioBasePrintf(Parser, stdout, NULL, 0, Param[0]->Val->Pointer, Param[1]->Val->Pointer);
}

/* fprintf: 格式化输出到指定文件流 */
void StdioFprintf(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    struct StdVararg PrintfArgs;

    PrintfArgs.Param = Param + 1;
    PrintfArgs.NumArgs = NumArgs-2;
    ReturnValue->Val->Integer = StdioBasePrintf(Parser, Param[0]->Val->Pointer, NULL, 0, Param[1]->Val->Pointer, &PrintfArgs);
}

/* vfprintf: 使用 va_list 格式化输出到指定文件流 */
void StdioVfprintf(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = StdioBasePrintf(Parser, Param[0]->Val->Pointer, NULL, 0, Param[1]->Val->Pointer, Param[2]->Val->Pointer);
}

/* sprintf: 格式化输出到字符串 (无长度限制，不安全) */
void StdioSprintf(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    struct StdVararg PrintfArgs;

    PrintfArgs.Param = Param + 1;
    PrintfArgs.NumArgs = NumArgs-2;
    ReturnValue->Val->Integer = StdioBasePrintf(Parser, NULL, Param[0]->Val->Pointer, -1, Param[1]->Val->Pointer, &PrintfArgs);
}

/* snprintf: 格式化输出到字符串，带最大长度限制 */
void StdioSnprintf(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    struct StdVararg PrintfArgs;

    PrintfArgs.Param = Param+2;
    PrintfArgs.NumArgs = NumArgs-3;
    ReturnValue->Val->Integer = StdioBasePrintf(Parser, NULL, Param[0]->Val->Pointer, Param[1]->Val->Integer, Param[2]->Val->Pointer, &PrintfArgs);
}

/* =========================================================================
 * scanf 系列 - 格式化输入函数
 * ========================================================================= */

/* scanf: 从标准输入 stdin 格式化读取 */
void StdioScanf(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    struct StdVararg ScanfArgs;

    ScanfArgs.Param = Param;
    ScanfArgs.NumArgs = NumArgs-1;
    ReturnValue->Val->Integer = StdioBaseScanf(Parser, stdin, NULL, Param[0]->Val->Pointer, &ScanfArgs);
}

/* fscanf: 从指定文件流格式化读取 */
void StdioFscanf(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    struct StdVararg ScanfArgs;

    ScanfArgs.Param = Param+1;
    ScanfArgs.NumArgs = NumArgs-2;
    ReturnValue->Val->Integer = StdioBaseScanf(Parser, Param[0]->Val->Pointer, NULL, Param[1]->Val->Pointer, &ScanfArgs);
}

/* sscanf: 从字符串格式化读取 */
void StdioSscanf(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    struct StdVararg ScanfArgs;

    ScanfArgs.Param = Param+1;
    ScanfArgs.NumArgs = NumArgs-2;
    ReturnValue->Val->Integer = StdioBaseScanf(Parser, NULL, Param[0]->Val->Pointer, Param[1]->Val->Pointer, &ScanfArgs);
}

/* =========================================================================
 * v 系列 - va_list 版本的格式化函数
 * ========================================================================= */

/* vsprintf: 使用 va_list 格式化输出到字符串 */
void StdioVsprintf(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = StdioBasePrintf(Parser, NULL, Param[0]->Val->Pointer, -1, Param[1]->Val->Pointer, Param[2]->Val->Pointer);
}

/* vsnprintf: 使用 va_list 格式化输出到字符串，带长度限制 */
void StdioVsnprintf(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = StdioBasePrintf(Parser, NULL, Param[0]->Val->Pointer, Param[1]->Val->Integer, Param[2]->Val->Pointer, Param[3]->Val->Pointer);
}

/* vscanf: 使用 va_list 从 stdin 格式化读取 */
void StdioVscanf(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = StdioBaseScanf(Parser, stdin, NULL, Param[0]->Val->Pointer, Param[1]->Val->Pointer);
}

/* vfscanf: 使用 va_list 从指定文件流格式化读取 */
void StdioVfscanf(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = StdioBaseScanf(Parser, Param[0]->Val->Pointer, NULL, Param[1]->Val->Pointer, Param[2]->Val->Pointer);
}

/* vsscanf: 使用 va_list 从字符串格式化读取 */
void StdioVsscanf(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = StdioBaseScanf(Parser, NULL, Param[0]->Val->Pointer, Param[1]->Val->Pointer, Param[2]->Val->Pointer);
}

/* PicoC 类型定义: FILE 和 va_list 结构体 */
const char StdioDefs[] = "\
typedef struct __va_listStruct va_list; \
typedef struct __FILEStruct FILE;\
";

/* stdio 函数注册表: 将 PicoC 内部函数名映射到 C 原型声明 */
struct LibraryFunction StdioFunctions[] =
{
    { StdioFopen,   "FILE *fopen(char *, char *);" },
    { StdioFreopen, "FILE *freopen(char *, char *, FILE *);" },
    { StdioFclose,  "int fclose(FILE *);" },
    { StdioFread,   "int fread(void *, int, int, FILE *);" },
    { StdioFwrite,  "int fwrite(void *, int, int, FILE *);" },
    { StdioFgetc,   "int fgetc(FILE *);" },
    { StdioFgetc,   "int getc(FILE *);" },
    { StdioFgets,   "char *fgets(char *, int, FILE *);" },
    { StdioFputc,   "int fputc(int, FILE *);" },
    { StdioFputs,   "int fputs(char *, FILE *);" },
    { StdioRemove,  "int remove(char *);" },
    { StdioRename,  "int rename(char *, char *);" },
    { StdioRewind,  "void rewind(FILE *);" },
    { StdioTmpfile, "FILE *tmpfile();" },
    { StdioClearerr,"void clearerr(FILE *);" },
    { StdioFeof,    "int feof(FILE *);" },
    { StdioFerror,  "int ferror(FILE *);" },
    { StdioFileno,  "int fileno(FILE *);" },
    { StdioFflush,  "int fflush(FILE *);" },
    { StdioFgetpos, "int fgetpos(FILE *, int *);" },
    { StdioFsetpos, "int fsetpos(FILE *, int *);" },
    { StdioFtell,   "int ftell(FILE *);" },
    { StdioFseek,   "int fseek(FILE *, int, int);" },
    { StdioPerror,  "void perror(char *);" },
    { StdioPutc,    "int putc(char *, FILE *);" },
    { StdioPutchar, "int putchar(int);" },
    { StdioPutchar, "int fputchar(int);" },
    { StdioSetbuf,  "void setbuf(FILE *, char *);" },
    { StdioSetvbuf, "void setvbuf(FILE *, char *, int, int);" },
    { StdioUngetc,  "int ungetc(int, FILE *);" },
    { StdioPuts,    "int puts(char *);" },
    { StdioGets,    "char *gets(char *);" },
    { StdioGetchar, "int getchar();" },
    { StdioPrintf,  "int printf(char *, ...);" },
    { StdioFprintf, "int fprintf(FILE *, char *, ...);" },
    { StdioSprintf, "int sprintf(char *, char *, ...);" },
    { StdioSnprintf,"int snprintf(char *, int, char *, ...);" },
    { StdioScanf,   "int scanf(char *, ...);" },
    { StdioFscanf,  "int fscanf(FILE *, char *, ...);" },
    { StdioSscanf,  "int sscanf(char *, char *, ...);" },
    { StdioVprintf, "int vprintf(char *, va_list);" },
    { StdioVfprintf,"int vfprintf(FILE *, char *, va_list);" },
    { StdioVsprintf,"int vsprintf(char *, char *, va_list);" },
    { StdioVsnprintf,"int vsnprintf(char *, int, char *, va_list);" },
    { StdioVscanf,   "int vscanf(char *, va_list);" },
    { StdioVfscanf,  "int vfscanf(FILE *, char *, va_list);" },
    { StdioVsscanf,  "int vsscanf(char *, char *, va_list);" },
    { NULL,         NULL }
};

/* StdioSetupFunc: 初始化 stdio 库，注册类型、常量和全局变量
 * - 创建 struct __FILEStruct (不透明，sizeof(FILE))
 * - 创建 struct __va_listStruct (不透明)
 * - 注册 EOF, SEEK_SET, BUFSIZ 等宏常量
 * - 注册 stdin, stdout, stderr 全局变量 */
void StdioSetupFunc(Picoc *pc)
{
    struct ValueType *StructFileType;
    struct ValueType *FilePtrType;

    /* 创建与原生 FILE 结构相同大小的 struct __FILEStruct */
    StructFileType = TypeCreateOpaqueStruct(pc, NULL, TableStrRegister(pc, "__FILEStruct"), sizeof(FILE));

    /* 获取 FILE * 类型 */
    FilePtrType = TypeGetMatching(pc, NULL, StructFileType, TypePointer, 0, pc->StrEmpty, TRUE);

    /* 创建与 struct StdVararg 相同大小的 struct __va_listStruct */
    TypeCreateOpaqueStruct(pc, NULL, TableStrRegister(pc, "__va_listStruct"), sizeof(FILE));

    /* 注册标准 I/O 宏常量 */
    VariableDefinePlatformVar(pc, NULL, "EOF", &pc->IntType, (union AnyValue *)&EOFValue, FALSE);
    VariableDefinePlatformVar(pc, NULL, "SEEK_SET", &pc->IntType, (union AnyValue *)&SEEK_SETValue, FALSE);
    VariableDefinePlatformVar(pc, NULL, "SEEK_CUR", &pc->IntType, (union AnyValue *)&SEEK_CURValue, FALSE);
    VariableDefinePlatformVar(pc, NULL, "SEEK_END", &pc->IntType, (union AnyValue *)&SEEK_ENDValue, FALSE);
    VariableDefinePlatformVar(pc, NULL, "BUFSIZ", &pc->IntType, (union AnyValue *)&BUFSIZValue, FALSE);
    VariableDefinePlatformVar(pc, NULL, "FILENAME_MAX", &pc->IntType, (union AnyValue *)&FILENAME_MAXValue, FALSE);
    VariableDefinePlatformVar(pc, NULL, "_IOFBF", &pc->IntType, (union AnyValue *)&_IOFBFValue, FALSE);
    VariableDefinePlatformVar(pc, NULL, "_IOLBF", &pc->IntType, (union AnyValue *)&_IOLBFValue, FALSE);
    VariableDefinePlatformVar(pc, NULL, "_IONBF", &pc->IntType, (union AnyValue *)&_IONBFValue, FALSE);
    VariableDefinePlatformVar(pc, NULL, "L_tmpnam", &pc->IntType, (union AnyValue *)&L_tmpnamValue, FALSE);
    VariableDefinePlatformVar(pc, NULL, "GETS_MAX", &pc->IntType, (union AnyValue *)&GETS_MAXValue, FALSE);

    /* 注册标准流 stdin, stdout, stderr */
    VariableDefinePlatformVar(pc, NULL, "stdin", FilePtrType, (union AnyValue *)&stdinValue, FALSE);
    VariableDefinePlatformVar(pc, NULL, "stdout", FilePtrType, (union AnyValue *)&stdoutValue, FALSE);
    VariableDefinePlatformVar(pc, NULL, "stderr", FilePtrType, (union AnyValue *)&stderrValue, FALSE);

    /* 定义 NULL (如果尚未定义) */
    if (!VariableDefined(pc, TableStrRegister(pc, "NULL")))
        VariableDefinePlatformVar(pc, NULL, "NULL", &pc->IntType, (union AnyValue *)&Stdio_ZeroValue, FALSE);
}

/* =========================================================================
 * 跨平台 I/O 辅助函数 (供 PicoC 内部/其他模块使用)
 * ========================================================================= */

/* PrintCh: 输出单个字符到文件流 */
void PrintCh(char OutCh, FILE *Stream)
{
    putc(OutCh, Stream);
}

/* PrintSimpleInt: 以十进制格式输出长整数 */
void PrintSimpleInt(long Num, FILE *Stream)
{
    fprintf(Stream, "%ld", Num);
}

/* PrintStr: 输出字符串到文件流 */
void PrintStr(const char *Str, FILE *Stream)
{
    fputs(Str, Stream);
}

/* PrintFP: 以浮点格式输出双精度数 */
void PrintFP(double Num, FILE *Stream)
{
    fprintf(Stream, "%f", Num);
}

#endif /* !BUILTIN_MINI_STDLIB */
