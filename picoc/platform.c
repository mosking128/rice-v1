/* PicoC 平台抽象层 — 初始化/清理/main调用/错误报告/printf/临时名称
 * 平台相关的 I/O 代码在 platform/platform_XX.c 和 platform/library_XX.c 中 */

#include "picoc.h"
#include "interpreter.h"


/* ---- 初始化与清理 ---- */

/* 初始化整个 PicoC 系统: 平台→IO→堆→符号表→变量→词法→类型→include→库→调试器 */
void PicocInitialise(Picoc *pc, int StackSize)
{
    memset(pc, '\0', sizeof(*pc));
    PlatformInit(pc);
    BasicIOInit(pc);
    HeapInit(pc, StackSize);
    TableInit(pc);
    VariableInit(pc);
    LexInit(pc);
    TypeInit(pc);
#ifndef NO_HASH_INCLUDE
    IncludeInit(pc);
#endif
    LibraryInit(pc);
#ifdef BUILTIN_MINI_STDLIB
    LibraryAdd(pc, &pc->GlobalTable, "c library", &CLibrary[0]);
    CLibraryInit(pc);
#endif
    PlatformLibraryInit(pc);
    PicocIncludeAllSystemHeaders(pc);
    DebugInit(pc);
}

/* 释放 PicoC 全部资源 */
void PicocCleanup(Picoc *pc)
{
    DebugCleanup(pc);
#ifndef NO_HASH_INCLUDE
    IncludeCleanup(pc);
#endif
    ParseCleanup(pc);
    LexCleanup(pc);
    VariableCleanup(pc);
    TypeCleanup(pc);
    TableStrFree(pc);
    HeapCleanup(pc);
    PlatformCleanup(pc);
}


/* ---- main() 函数调用 (UNIX/WIN32/STM32H750 平台通用) ---- */

#if defined(UNIX_HOST) || defined(WIN32) || defined(STM32H750_HOST)

#define CALL_MAIN_NO_ARGS_RETURN_VOID "main();"
#define CALL_MAIN_WITH_ARGS_RETURN_VOID "main(__argc,__argv);"
#define CALL_MAIN_NO_ARGS_RETURN_INT "__exit_value = main();"
#define CALL_MAIN_WITH_ARGS_RETURN_INT "__exit_value = main(__argc,__argv);"

/* 调用脚本中定义的 main() 函数，自动判断是否有参数和返回值 */
void PicocCallMain(Picoc *pc, int argc, char **argv)
{
    struct Value *FuncValue = NULL;

    if (!VariableDefined(pc, TableStrRegister(pc, "main")))
        ProgramFailNoParser(pc, "main() is not defined");

    VariableGet(pc, NULL, TableStrRegister(pc, "main"), &FuncValue);
    if (FuncValue->Typ->Base != TypeFunction)
        ProgramFailNoParser(pc, "main is not a function - can't call it");

    if (FuncValue->Val->FuncDef.NumParams != 0)
    {
        /* 定义命令行参数: __argc, __argv */
        VariableDefinePlatformVar(pc, NULL, "__argc", &pc->IntType, (union AnyValue *)&argc, FALSE);
        VariableDefinePlatformVar(pc, NULL, "__argv", pc->CharPtrPtrType, (union AnyValue *)&argv, FALSE);
    }

    if (FuncValue->Val->FuncDef.ReturnType == &pc->VoidType)
    {
        if (FuncValue->Val->FuncDef.NumParams == 0)
            PicocParse(pc, "startup", CALL_MAIN_NO_ARGS_RETURN_VOID, strlen(CALL_MAIN_NO_ARGS_RETURN_VOID), TRUE, TRUE, FALSE, TRUE);
        else
            PicocParse(pc, "startup", CALL_MAIN_WITH_ARGS_RETURN_VOID, strlen(CALL_MAIN_WITH_ARGS_RETURN_VOID), TRUE, TRUE, FALSE, TRUE);
    }
    else
    {
        /* 定义返回值变量 __exit_value */
        VariableDefinePlatformVar(pc, NULL, "__exit_value", &pc->IntType, (union AnyValue *)&pc->PicocExitValue, TRUE);

        if (FuncValue->Val->FuncDef.NumParams == 0)
            PicocParse(pc, "startup", CALL_MAIN_NO_ARGS_RETURN_INT, strlen(CALL_MAIN_NO_ARGS_RETURN_INT), TRUE, TRUE, FALSE, TRUE);
        else
            PicocParse(pc, "startup", CALL_MAIN_WITH_ARGS_RETURN_INT, strlen(CALL_MAIN_WITH_ARGS_RETURN_INT), TRUE, TRUE, FALSE, TRUE);
    }
}
#endif


/* ---- 错误报告 ---- */

/* 打印源代码错误位置 (文件名:行号:列号) 并在出错行下方标注 ^ */
void PrintSourceTextErrorLine(IOFILE *Stream, const char *FileName, const char *SourceText, int Line, int CharacterPos)
{
    int LineCount;
    const char *LinePos;
    const char *CPos;
    int CCount;

    if (SourceText != NULL)
    {
        /* 定位到出错行 */
        for (LinePos = SourceText, LineCount = 1; *LinePos != '\0' && LineCount < Line; LinePos++)
        {
            if (*LinePos == '\n')
                LineCount++;
        }

        /* 打印该行内容 */
        for (CPos = LinePos; *CPos != '\n' && *CPos != '\0'; CPos++)
            PrintCh(*CPos, Stream);
        PrintCh('\n', Stream);

        /* 在出错位置下方打印 ^ 标记 */
        for (CPos = LinePos, CCount = 0; *CPos != '\n' && *CPos != '\0' && (CCount < CharacterPos || *CPos == ' '); CPos++, CCount++)
        {
            if (*CPos == '\t')
                PrintCh('\t', Stream);
            else
                PrintCh(' ', Stream);
        }
    }
    else
    {
        /* 交互模式: 对齐到输入提示符后的位置 */
        for (CCount = 0; CCount < CharacterPos + (int)strlen(INTERACTIVE_PROMPT_STATEMENT); CCount++)
            PrintCh(' ', Stream);
    }
    PlatformPrintf(Stream, "^\n%s:%d:%d ", FileName, Line, CharacterPos);
}

/* 解析期错误: 打印源码位置 + 格式化消息，然后 longjmp 退出 */
void ProgramFail(struct ParseState *Parser, const char *Message, ...)
{
    va_list Args;

    PrintSourceTextErrorLine(Parser->pc->CStdOut, Parser->FileName, Parser->SourceText, Parser->Line, Parser->CharacterPos);
    va_start(Args, Message);
    PlatformVPrintf(Parser->pc->CStdOut, Message, Args);
    va_end(Args);
    PlatformPrintf(Parser->pc->CStdOut, "\n");
    PlatformExit(Parser->pc, 1);
}

/* 非解析期错误: 无源码位置，仅格式化消息 */
void ProgramFailNoParser(Picoc *pc, const char *Message, ...)
{
    va_list Args;

    va_start(Args, Message);
    PlatformVPrintf(pc->CStdOut, Message, Args);
    va_end(Args);
    PlatformPrintf(pc->CStdOut, "\n");
    PlatformExit(pc, 1);
}

/* 赋值类型不匹配时的错误报告 */
void AssignFail(struct ParseState *Parser, const char *Format, struct ValueType *Type1, struct ValueType *Type2, int Num1, int Num2, const char *FuncName, int ParamNo)
{
    IOFILE *Stream = Parser->pc->CStdOut;

    PrintSourceTextErrorLine(Parser->pc->CStdOut, Parser->FileName, Parser->SourceText, Parser->Line, Parser->CharacterPos);
    PlatformPrintf(Stream, "can't %s ", (FuncName == NULL) ? "assign" : "set");

    if (Type1 != NULL)
        PlatformPrintf(Stream, Format, Type1, Type2);
    else
        PlatformPrintf(Stream, Format, Num1, Num2);

    if (FuncName != NULL)
        PlatformPrintf(Stream, " in argument %d of call to %s()", ParamNo, FuncName);

    PlatformPrintf(Stream, "\n");
    PlatformExit(Parser->pc, 1);
}

/* 词法分析期错误 */
void LexFail(Picoc *pc, struct LexState *Lexer, const char *Message, ...)
{
    va_list Args;

    PrintSourceTextErrorLine(pc->CStdOut, Lexer->FileName, Lexer->SourceText, Lexer->Line, Lexer->CharacterPos);
    va_start(Args, Message);
    PlatformVPrintf(pc->CStdOut, Message, Args);
    va_end(Args);
    PlatformPrintf(pc->CStdOut, "\n");
    PlatformExit(pc, 1);
}


/* ---- 输出函数 ---- */

/* 格式化输出 (类似 printf)，支持 %s %d %c %t %f %% */
void PlatformPrintf(IOFILE *Stream, const char *Format, ...)
{
    va_list Args;

    va_start(Args, Format);
    PlatformVPrintf(Stream, Format, Args);
    va_end(Args);
}

void PlatformVPrintf(IOFILE *Stream, const char *Format, va_list Args)
{
    const char *FPos;

    for (FPos = Format; *FPos != '\0'; FPos++)
    {
        if (*FPos == '%')
        {
            FPos++;
            switch (*FPos)
            {
            case 's': PrintStr(va_arg(Args, char *), Stream); break;
            case 'd': PrintSimpleInt(va_arg(Args, int), Stream); break;
            case 'c': PrintCh(va_arg(Args, int), Stream); break;
            case 't': PrintType(va_arg(Args, struct ValueType *), Stream); break;
#ifndef NO_FP
            case 'f': PrintFP(va_arg(Args, double), Stream); break;
#endif
            case '%': PrintCh('%', Stream); break;
            case '\0': FPos--; break;
            }
        }
        else
            PrintCh(*FPos, Stream);
    }
}

/* 生成递增的临时名称，用于内部匿名变量。接受 char[7] 缓冲区，初始值 "XX0000" */
char *PlatformMakeTempName(Picoc *pc, char *TempNameBuffer)
{
    int CPos = 5;

    while (CPos > 1)
    {
        if (TempNameBuffer[CPos] < '9')
        {
            TempNameBuffer[CPos]++;
            return TableStrRegister(pc, TempNameBuffer);
        }
        else
        {
            TempNameBuffer[CPos] = '0';
            CPos--;
        }
    }

    return TableStrRegister(pc, TempNameBuffer);
}
