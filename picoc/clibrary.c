/* picoc mini standard C library - provides an optional tiny C standard library
 * if BUILTIN_MINI_STDLIB is defined */

/* PicoC 迷你 C 标准库 — 在嵌入式平台 (BUILTIN_MINI_STDLIB) 下提供 printf/putchar/sprintf 等精简实现;
 * 输出通过平台底层 PrintCh/PrintStr/PrintSimpleInt/PrintFP 完成，最终到达 PlatformPutc 串口输出 */

#include "picoc.h"
#include "interpreter.h"


/* 大小端检测 */
static const int __ENDIAN_CHECK__ = 1;
static int BigEndian;
static int LittleEndian;


/* 库系统全局初始化: 定义版本宏和大小端宏 */
void LibraryInit(Picoc *pc)
{

    /* 定义版本号宏 */
    pc->VersionString = TableStrRegister(pc, PICOC_VERSION);
    VariableDefinePlatformVar(pc, NULL, "PICOC_VERSION", pc->CharPtrType, (union AnyValue *)&pc->VersionString, FALSE);

    /* 定义大小端宏 */
    BigEndian = ((*(char*)&__ENDIAN_CHECK__) == 0);
    LittleEndian = ((*(char*)&__ENDIAN_CHECK__) == 1);

    VariableDefinePlatformVar(pc, NULL, "BIG_ENDIAN", &pc->IntType, (union AnyValue *)&BigEndian, FALSE);
    VariableDefinePlatformVar(pc, NULL, "LITTLE_ENDIAN", &pc->IntType, (union AnyValue *)&LittleEndian, FALSE);
}

/* 添加库函数列表: 解析每个函数的原型字符串，创建内建函数 Value 并注册 */
void LibraryAdd(Picoc *pc, struct Table *GlobalTable, const char *LibraryName, struct LibraryFunction *FuncList)
{
    struct ParseState Parser;
    int Count;
    char *Identifier;
    struct ValueType *ReturnType;
    struct Value *NewValue;
    void *Tokens;
    char *IntrinsicName = TableStrRegister(pc, "c library");

    /* 遍历所有库函数定义 */
    for (Count = 0; FuncList[Count].Prototype != NULL; Count++)
    {
        Tokens = LexAnalyse(pc, IntrinsicName, FuncList[Count].Prototype, strlen((char *)FuncList[Count].Prototype), NULL);
        LexInitParser(&Parser, pc, FuncList[Count].Prototype, Tokens, IntrinsicName, TRUE, FALSE);
        TypeParse(&Parser, &ReturnType, &Identifier, NULL);
        NewValue = ParseFunctionDefinition(&Parser, ReturnType, Identifier);
        NewValue->Val->FuncDef.Intrinsic = FuncList[Count].Func;
        HeapFreeMem(pc, Tokens);
    }
}

/* 将类型名打印到输出流(不使用 printf/sprintf)。递归处理指针和数组类型 */
void PrintType(struct ValueType *Typ, IOFILE *Stream)
{
    switch (Typ->Base)
    {
        case TypeVoid:          PrintStr("void", Stream); break;
        case TypeInt:           PrintStr("int", Stream); break;
        case TypeShort:         PrintStr("short", Stream); break;
        case TypeChar:          PrintStr("char", Stream); break;
        case TypeLong:          PrintStr("long", Stream); break;
        case TypeUnsignedInt:   PrintStr("unsigned int", Stream); break;
        case TypeUnsignedShort: PrintStr("unsigned short", Stream); break;
        case TypeUnsignedLong:  PrintStr("unsigned long", Stream); break;
        case TypeUnsignedChar:  PrintStr("unsigned char", Stream); break;
#ifndef NO_FP
        case TypeFP:            PrintStr("double", Stream); break;
#endif
        case TypeFunction:      PrintStr("function", Stream); break;
        case TypeMacro:         PrintStr("macro", Stream); break;
        case TypePointer:       if (Typ->FromType) PrintType(Typ->FromType, Stream); PrintCh('*', Stream); break;
        case TypeArray:         PrintType(Typ->FromType, Stream); PrintCh('[', Stream); if (Typ->ArraySize != 0) PrintSimpleInt(Typ->ArraySize, Stream); PrintCh(']', Stream); break;
        case TypeStruct:        PrintStr("struct ", Stream); PrintStr( Typ->Identifier, Stream); break;
        case TypeUnion:         PrintStr("union ", Stream); PrintStr(Typ->Identifier, Stream); break;
        case TypeEnum:          PrintStr("enum ", Stream); PrintStr(Typ->Identifier, Stream); break;
        case TypeGotoLabel:     PrintStr("goto label ", Stream); break;
        case Type_Type:         PrintStr("type ", Stream); break;
    }
}


#ifdef BUILTIN_MINI_STDLIB

/*
 * 嵌入式系统的简化标准库。不需要系统 stdio 支持。
 * 更完整的标准库实现位于 library_XXX.c 文件中。
 */

static int TRUEValue = 1;
static int ZeroValue = 0;

/* 初始化基本 I/O: 将标准输出回调设置为平台串口输出函数 */
void BasicIOInit(Picoc *pc)
{
    pc->CStdOutBase.Putch = &PlatformPutc;
    pc->CStdOut = &pc->CStdOutBase;
}

/* 初始化 C 库: 定义 NULL、TRUE、FALSE 常量 */
void CLibraryInit(Picoc *pc)
{
    /* 定义常量 */
    VariableDefinePlatformVar(pc, NULL, "NULL", &pc->IntType, (union AnyValue *)&ZeroValue, FALSE);
    VariableDefinePlatformVar(pc, NULL, "TRUE", &pc->IntType, (union AnyValue *)&TRUEValue, FALSE);
    VariableDefinePlatformVar(pc, NULL, "FALSE", &pc->IntType, (union AnyValue *)&ZeroValue, FALSE);
}

/* 字符串输出流的 putc: 将字符追加到字符串缓冲区 */
void SPutc(unsigned char Ch, union OutputStreamInfo *Stream)
{
    struct StringOutputStream *Out = &Stream->Str;
    *Out->WritePos++ = Ch;
}

/* 输出单个字符到输出流(通过回调 Putch) */
void PrintCh(char OutCh, struct OutputStream *Stream)
{
    (*Stream->Putch)(OutCh, &Stream->i);
}

/* 输出以 '\0' 结尾的字符串到输出流 */
void PrintStr(const char *Str, struct OutputStream *Stream)
{
    while (*Str != 0)
        PrintCh(*Str++, Stream);
}

/* 重复输出指定字符 Length 次 */
void PrintRepeatedChar(char ShowChar, int Length, struct OutputStream *Stream)
{
    while (Length-- > 0)
        PrintCh(ShowChar, Stream);
}

/* 输出无符号整数到流。支持:
 * - Base: 进制(10/16/2)
 * - FieldWidth: 最小字段宽度
 * - ZeroPad: 用 '0' 填充
 * - LeftJustify: 左对齐 */
void PrintUnsigned(unsigned long Num, unsigned int Base, int FieldWidth, int ZeroPad, int LeftJustify, struct OutputStream *Stream)
{
    char Result[33];
    int ResPos = sizeof(Result);

    Result[--ResPos] = '\0';
    if (Num == 0)
        Result[--ResPos] = '0';

    while (Num > 0)
    {
        unsigned long NextNum = Num / Base;
        unsigned long Digit = Num - NextNum * Base;
        if (Digit < 10)
            Result[--ResPos] = '0' + Digit;
        else
            Result[--ResPos] = 'a' + Digit - 10;

        Num = NextNum;
    }

    if (FieldWidth > 0 && !LeftJustify)
        PrintRepeatedChar(ZeroPad ? '0' : ' ', FieldWidth - (sizeof(Result) - 1 - ResPos), Stream);

    PrintStr(&Result[ResPos], Stream);

    if (FieldWidth > 0 && LeftJustify)
        PrintRepeatedChar(' ', FieldWidth - (sizeof(Result) - 1 - ResPos), Stream);
}

/* 简单输出有符号整数(无格式选项) */
void PrintSimpleInt(long Num, struct OutputStream *Stream)
{
    PrintInt(Num, -1, FALSE, FALSE, Stream);
}

/* 输出有符号整数到流。负数打印 '-' 号，然后调用 PrintUnsigned */
void PrintInt(long Num, int FieldWidth, int ZeroPad, int LeftJustify, struct OutputStream *Stream)
{
    if (Num < 0)
    {
        PrintCh('-', Stream);
        Num = -Num;
        if (FieldWidth != 0)
            FieldWidth--;
    }

    PrintUnsigned((unsigned long)Num, 10, FieldWidth, ZeroPad, LeftJustify, Stream);
}

#ifndef NO_FP
/* 输出浮点数到流(不使用 printf/sprintf)。格式化为科学计数法:
 * 整数部分.小数部分e指数 */
void PrintFP(double Num, struct OutputStream *Stream)
{
    int Exponent = 0;
    int MaxDecimal;

    if (Num < 0)
    {
        PrintCh('-', Stream);
        Num = -Num;
    }

    if (Num >= 1e7)
        Exponent = log10(Num);
    else if (Num <= 1e-7 && Num != 0.0)
        Exponent = log10(Num) - 0.999999999;

    Num /= pow(10.0, Exponent);
    PrintInt((long)Num, 0, FALSE, FALSE, Stream);
    PrintCh('.', Stream);
    Num = (Num - (long)Num) * 10;
    if (abs(Num) >= 1e-7)
    {
        for (MaxDecimal = 6; MaxDecimal > 0 && abs(Num) >= 1e-7; Num = (Num - (long)(Num + 1e-7)) * 10, MaxDecimal--)
            PrintCh('0' + (long)(Num + 1e-7), Stream);
    }
    else
        PrintCh('0', Stream);

    if (Exponent != 0)
    {
        PrintCh('e', Stream);
        PrintInt(Exponent, 0, FALSE, FALSE, Stream);
    }
}
#endif

/* 通用 printf 格式化核心。解析格式字符串中的 % 说明符:
 * %s - 字符串, %d - 有符号整数, %u - 无符号整数
 * %x - 十六进制, %b - 二进制, %c - 字符, %f - 浮点数(可选)
 * 支持标志: '-' 左对齐, '0' 零填充, 数字字段宽度 */
void GenericPrintf(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs, struct OutputStream *Stream)
{
    char *FPos;
    struct Value *NextArg = Param[0];
    struct ValueType *FormatType;
    int ArgCount = 1;
    int LeftJustify = FALSE;
    int ZeroPad = FALSE;
    int FieldWidth = 0;
    char *Format = Param[0]->Val->Pointer;

    for (FPos = Format; *FPos != '\0'; FPos++)
    {
        if (*FPos == '%')
        {
            FPos++;
		    FieldWidth = 0;
            if (*FPos == '-')
            {
                /* '-' 表示左对齐 */
                LeftJustify = TRUE;
                FPos++;
            }

            if (*FPos == '0')
            {
                /* '0' 表示数字零填充 */
                ZeroPad = TRUE;
                FPos++;
            }

            /* 获取字段宽度数值 */
            while (isdigit((int)*FPos))
                FieldWidth = FieldWidth * 10 + (*FPos++ - '0');

            /* 检查格式字符类型 */
            switch (*FPos)
            {
                case 's': FormatType = Parser->pc->CharPtrType; break;
                case 'd': case 'u': case 'x': case 'b': case 'c': FormatType = &Parser->pc->IntType; break;
#ifndef NO_FP
                case 'f': FormatType = &Parser->pc->FPType; break;
#endif
                case '%': PrintCh('%', Stream); FormatType = NULL; break;
                case '\0': FPos--; FormatType = NULL; break;
                default:  PrintCh(*FPos, Stream); FormatType = NULL; break;
            }

            if (FormatType != NULL)
            {
                /* 有格式化输出 */
                if (ArgCount >= NumArgs)
                    PrintStr("XXX", Stream);   /* 参数不够 */
                else
                {
                    NextArg = (struct Value *)((char *)NextArg + MEM_ALIGN(sizeof(struct Value) + TypeStackSizeValue(NextArg)));
                    if (NextArg->Typ != FormatType &&
                            !((FormatType == &Parser->pc->IntType || *FPos == 'f') && IS_NUMERIC_COERCIBLE(NextArg)) &&
                            !(FormatType == Parser->pc->CharPtrType && (NextArg->Typ->Base == TypePointer ||
                                                             (NextArg->Typ->Base == TypeArray && NextArg->Typ->FromType->Base == TypeChar) ) ) )
                        PrintStr("XXX", Stream);   /* 类型不匹配 */
                    else
                    {
                        switch (*FPos)
                        {
                            case 's':
                            {
                                char *Str;

                                if (NextArg->Typ->Base == TypePointer)
                                    Str = NextArg->Val->Pointer;
                                else
                                    Str = &NextArg->Val->ArrayMem[0];

                                if (Str == NULL)
                                    PrintStr("NULL", Stream);
                                else
                                    PrintStr(Str, Stream);
                                break;
                            }
                            case 'd': PrintInt(ExpressionCoerceInteger(NextArg), FieldWidth, ZeroPad, LeftJustify, Stream); break;
                            case 'u': PrintUnsigned(ExpressionCoerceUnsignedInteger(NextArg), 10, FieldWidth, ZeroPad, LeftJustify, Stream); break;
                            case 'x': PrintUnsigned(ExpressionCoerceUnsignedInteger(NextArg), 16, FieldWidth, ZeroPad, LeftJustify, Stream); break;
                            case 'b': PrintUnsigned(ExpressionCoerceUnsignedInteger(NextArg), 2, FieldWidth, ZeroPad, LeftJustify, Stream); break;
                            case 'c': PrintCh(ExpressionCoerceUnsignedInteger(NextArg), Stream); break;
#ifndef NO_FP
                            case 'f': PrintFP(ExpressionCoerceFP(NextArg), Stream); break;
#endif
                        }
                    }
                }

                ArgCount++;
            }
        }
        else
            PrintCh(*FPos, Stream);
    }
}

/* printf(): 格式化输出到控制台(通过 PlatformPutc) */
void LibPrintf(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    struct OutputStream ConsoleStream;

    ConsoleStream.Putch = &PlatformPutc;
    GenericPrintf(Parser, ReturnValue, Param, NumArgs, &ConsoleStream);
}

/* sprintf(): 格式化输出到字符串缓冲区 */
void LibSPrintf(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    struct OutputStream StrStream;

    StrStream.Putch = &SPutc;
    StrStream.i.Str.Parser = Parser;
    StrStream.i.Str.WritePos = Param[0]->Val->Pointer;

    GenericPrintf(Parser, ReturnValue, Param+1, NumArgs-1, &StrStream);
    PrintCh(0, &StrStream);
    ReturnValue->Val->Pointer = *Param;
}

/* gets(): 从平台读取一行输入，自动去除末尾换行符 */
void LibGets(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Pointer = PlatformGetLine(Param[0]->Val->Pointer, GETS_BUF_MAX, NULL);
    if (ReturnValue->Val->Pointer != NULL)
    {
        char *EOLPos = strchr(Param[0]->Val->Pointer, '\n');
        if (EOLPos != NULL)
            *EOLPos = '\0';
    }
}

/* getchar(): 从平台读取单个字符 */
void LibGetc(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = PlatformGetCharacter();
}

/* 所有库函数及其原型列表 */
struct LibraryFunction CLibrary[] =
{
    { LibPrintf,        "void printf(char *, ...);" },
    { LibSPrintf,       "char *sprintf(char *, char *, ...);" },
    { LibGets,          "char *gets(char *);" },
    { LibGetc,          "int getchar();" },
    { NULL,             NULL }
};

#endif /* BUILTIN_MINI_STDLIB */
