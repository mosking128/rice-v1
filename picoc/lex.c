/* PicoC 词法分析器 (Lexer / Tokenizer)
 *
 * 将 C 源代码文本转换为 Token 序列，供解析器使用。
 * 识别：关键字、标识符、整数/浮点常量、字符串/字符常量（含转义）、运算符。
 * 同时处理预处理指令：#define、#include、#ifdef、#ifndef、#if、#else、#endif。
 *
 * 入口函数：
 *   LexAnalyse() — 分析一段源代码文本，返回 token 序列
 *   LexGetToken() — 解析器每次取一个 token（含预处理）
 */

#include "interpreter.h"

#ifdef NO_CTYPE
#define isalpha(c) (((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z'))
#define isdigit(c) ((c) >= '0' && (c) <= '9')
#define isalnum(c) (isalpha(c) || isdigit(c))
#define isspace(c) ((c) == ' ' || (c) == '\t' || (c) == '\r' || (c) == '\n')
#endif
#define isCidstart(c) (isalpha(c) || (c)=='_' || (c)=='#')
#define isCident(c) (isalnum(c) || (c)=='_')

#define IS_HEX_ALPHA_DIGIT(c) (((c) >= 'a' && (c) <= 'f') || ((c) >= 'A' && (c) <= 'F'))
#define IS_BASE_DIGIT(c,b) (((c) >= '0' && (c) < '0' + (((b)<10)?(b):10)) || (((b) > 10) ? IS_HEX_ALPHA_DIGIT(c) : FALSE))
#define GET_BASE_DIGIT(c) (((c) <= '9') ? ((c) - '0') : (((c) <= 'F') ? ((c) - 'A' + 10) : ((c) - 'a' + 10)))

/* 多字符运算符的便捷宏：根据下一个字符选择对应的 token */
#define NEXTIS(c,x,y) { if (NextChar == (c)) { LEXER_INC(Lexer); GotToken = (x); } else GotToken = (y); }
#define NEXTIS3(c,x,d,y,z) { if (NextChar == (c)) { LEXER_INC(Lexer); GotToken = (x); } else NEXTIS(d,y,z) }
#define NEXTIS4(c,x,d,y,e,z,a) { if (NextChar == (c)) { LEXER_INC(Lexer); GotToken = (x); } else NEXTIS3(d,y,e,z,a) }
#define NEXTIS3PLUS(c,x,d,y,e,z,a) { if (NextChar == (c)) { LEXER_INC(Lexer); GotToken = (x); } else if (NextChar == (d)) { if (Lexer->Pos[1] == (e)) { LEXER_INCN(Lexer, 2); GotToken = (z); } else { LEXER_INC(Lexer); GotToken = (y); } } else GotToken = (a); }
#define NEXTISEXACTLY3(c,d,y,z) { if (NextChar == (c) && Lexer->Pos[1] == (d)) { LEXER_INCN(Lexer, 2); GotToken = (y); } else GotToken = (z); }

#define LEXER_INC(l) ( (l)->Pos++, (l)->CharacterPos++ )
#define LEXER_INCN(l, n) ( (l)->Pos+=(n), (l)->CharacterPos+=(n) )
#define TOKEN_DATA_OFFSET 2

#define MAX_CHAR_VALUE 255      /* "char" 类型能表示的最大值 */


/* 保留字结构体 */
struct ReservedWord
{
    const char *Word;
    enum LexToken Token;
};

/* 关键字/保留字表 —— 包含所有 C 关键字和预处理指令 */
static struct ReservedWord ReservedWords[] =
{
    { "#define", TokenHashDefine },
    { "#else", TokenHashElse },
    { "#endif", TokenHashEndif },
    { "#if", TokenHashIf },
    { "#ifdef", TokenHashIfdef },
    { "#ifndef", TokenHashIfndef },
    { "#include", TokenHashInclude },
    { "auto", TokenAutoType },
    { "break", TokenBreak },
    { "case", TokenCase },
    { "char", TokenCharType },
    { "continue", TokenContinue },
    { "default", TokenDefault },
    { "delete", TokenDelete },
    { "do", TokenDo },
#ifndef NO_FP
    { "double", TokenDoubleType },
#endif
    { "else", TokenElse },
    { "enum", TokenEnumType },
    { "extern", TokenExternType },
#ifndef NO_FP
    { "float", TokenFloatType },
#endif
    { "for", TokenFor },
    { "goto", TokenGoto },
    { "if", TokenIf },
    { "int", TokenIntType },
    { "long", TokenLongType },
    { "new", TokenNew },
    { "register", TokenRegisterType },
    { "return", TokenReturn },
    { "short", TokenShortType },
    { "signed", TokenSignedType },
    { "sizeof", TokenSizeof },
    { "static", TokenStaticType },
    { "struct", TokenStructType },
    { "switch", TokenSwitch },
    { "typedef", TokenTypedef },
    { "union", TokenUnionType },
    { "unsigned", TokenUnsignedType },
    { "void", TokenVoidType },
    { "while", TokenWhile }
};



/* 初始化词法分析器：将保留字表填入哈希表 */
void LexInit(Picoc *pc)
{
    int Count;

    TableInitTable(&pc->ReservedWordTable, &pc->ReservedWordHashTable[0], sizeof(ReservedWords) / sizeof(struct ReservedWord) * 2, TRUE);

    for (Count = 0; Count < sizeof(ReservedWords) / sizeof(struct ReservedWord); Count++)
    {
        TableSet(pc, &pc->ReservedWordTable, TableStrRegister(pc, ReservedWords[Count].Word), (struct Value *)&ReservedWords[Count], NULL, 0, 0);
    }

    pc->LexValue.Typ = NULL;
    pc->LexValue.Val = &pc->LexAnyValue;
    pc->LexValue.LValueFrom = FALSE;
    pc->LexValue.ValOnHeap = FALSE;
    pc->LexValue.ValOnStack = FALSE;
    pc->LexValue.AnyValOnHeap = FALSE;
    pc->LexValue.IsLValue = FALSE;
}

/* 清理词法分析器：清除交互输入和保留字表 */
void LexCleanup(Picoc *pc)
{
    int Count;

    LexInteractiveClear(pc, NULL);

    for (Count = 0; Count < sizeof(ReservedWords) / sizeof(struct ReservedWord); Count++)
        TableDelete(pc, &pc->ReservedWordTable, TableStrRegister(pc, ReservedWords[Count].Word));
}

/* 检查一个标识符是否为保留字，是则返回对应 token */
enum LexToken LexCheckReservedWord(Picoc *pc, const char *Word)
{
    struct Value *val;

    if (TableGet(&pc->ReservedWordTable, Word, &val, NULL, NULL, NULL))
        return ((struct ReservedWord *)val)->Token;
    else
        return TokenNone;
}

/* 解析数字字面量：支持十进制、十六进制 (0x)、八进制 (0)、二进制 (0b)，
 * 支持浮点数（含小数点和科学计数法 e/E），以及 u/l 后缀 */
enum LexToken LexGetNumber(Picoc *pc, struct LexState *Lexer, struct Value *Value)
{
    long Result = 0;
    long Base = 10;
    enum LexToken ResultToken;
#ifndef NO_FP
    double FPResult;
    double FPDiv;
#endif
    /* long/unsigned flags */
#if 0 /* unused for now */
    char IsLong = 0;
    char IsUnsigned = 0;
#endif

    if (*Lexer->Pos == '0')
    {
        /* 二进制、八进制或十六进制字面量 */
        LEXER_INC(Lexer);
        if (Lexer->Pos != Lexer->End)
        {
            if (*Lexer->Pos == 'x' || *Lexer->Pos == 'X')
                { Base = 16; LEXER_INC(Lexer); }
            else if (*Lexer->Pos == 'b' || *Lexer->Pos == 'B')
                { Base = 2; LEXER_INC(Lexer); }
            else if (*Lexer->Pos != '.')
                Base = 8;
        }
    }

    /* 读取数值部分 */
    for (; Lexer->Pos != Lexer->End && IS_BASE_DIGIT(*Lexer->Pos, Base); LEXER_INC(Lexer))
        Result = Result * Base + GET_BASE_DIGIT(*Lexer->Pos);

    if (*Lexer->Pos == 'u' || *Lexer->Pos == 'U')
    {
        LEXER_INC(Lexer);
        /* IsUnsigned = 1; */
    }
    if (*Lexer->Pos == 'l' || *Lexer->Pos == 'L')
    {
        LEXER_INC(Lexer);
        /* IsLong = 1; */
    }

    Value->Typ = &pc->LongType; /* ignored? */
    Value->Val->LongInteger = Result;

    ResultToken = TokenIntegerConstant;

    if (Lexer->Pos == Lexer->End)
        return ResultToken;

#ifndef NO_FP
    if (Lexer->Pos == Lexer->End)
    {
        return ResultToken;
    }

    if (*Lexer->Pos != '.' && *Lexer->Pos != 'e' && *Lexer->Pos != 'E')
    {
        return ResultToken;
    }

    Value->Typ = &pc->FPType;
    FPResult = (double)Result;

    if (*Lexer->Pos == '.')
    {
        LEXER_INC(Lexer);
        for (FPDiv = 1.0/Base; Lexer->Pos != Lexer->End && IS_BASE_DIGIT(*Lexer->Pos, Base); LEXER_INC(Lexer), FPDiv /= (double)Base)
        {
            FPResult += GET_BASE_DIGIT(*Lexer->Pos) * FPDiv;
        }
    }

    if (Lexer->Pos != Lexer->End && (*Lexer->Pos == 'e' || *Lexer->Pos == 'E'))
    {
        int ExponentSign = 1;

        LEXER_INC(Lexer);
        if (Lexer->Pos != Lexer->End && *Lexer->Pos == '-')
        {
            ExponentSign = -1;
            LEXER_INC(Lexer);
        }

        Result = 0;
        while (Lexer->Pos != Lexer->End && IS_BASE_DIGIT(*Lexer->Pos, Base))
        {
            Result = Result * Base + GET_BASE_DIGIT(*Lexer->Pos);
            LEXER_INC(Lexer);
        }

        FPResult *= pow((double)Base, (double)Result * ExponentSign);
    }

    Value->Val->FP = FPResult;

    if (*Lexer->Pos == 'f' || *Lexer->Pos == 'F')
        LEXER_INC(Lexer);

    return TokenFPConstant;
#else
    return ResultToken;
#endif
}

/* 解析标识符或保留字：读取 C 标识符，查保留字表判断类型 */
enum LexToken LexGetWord(Picoc *pc, struct LexState *Lexer, struct Value *Value)
{
    const char *StartPos = Lexer->Pos;
    enum LexToken Token;

    do {
        LEXER_INC(Lexer);
    } while (Lexer->Pos != Lexer->End && isCident((int)*Lexer->Pos));

    Value->Typ = NULL;
    Value->Val->Identifier = TableStrRegister2(pc, StartPos, Lexer->Pos - StartPos);

    Token = LexCheckReservedWord(pc, Value->Val->Identifier);
    switch (Token)
    {
        case TokenHashInclude: Lexer->Mode = LexModeHashInclude; break;
        case TokenHashDefine: Lexer->Mode = LexModeHashDefine; break;
        default: break;
    }

    if (Token != TokenNone)
        return Token;

    if (Lexer->Mode == LexModeHashDefineSpace)
        Lexer->Mode = LexModeHashDefineSpaceIdent;

    return TokenIdentifier;
}

/* 从八进制的转义序列中解析字符值 */
unsigned char LexUnEscapeCharacterConstant(const char **From, const char *End, unsigned char FirstChar, int Base)
{
    unsigned char Total = GET_BASE_DIGIT(FirstChar);
    int CCount;
    for (CCount = 0; IS_BASE_DIGIT(**From, Base) && CCount < 2; CCount++, (*From)++)
        Total = Total * Base + GET_BASE_DIGIT(**From);

    return Total;
}

/* 从字符串或字符常量中解析转义字符（支持 \n、\t、\\、\xHH、\OOO 等） */
unsigned char LexUnEscapeCharacter(const char **From, const char *End)
{
    unsigned char ThisChar;

    while ( *From != End && **From == '\\' &&
            &(*From)[1] != End && (*From)[1] == '\n' )
        (*From) += 2;       /* 跳过 LF 换行转义 */

    while ( *From != End && **From == '\\' &&
            &(*From)[1] != End && &(*From)[2] != End && (*From)[1] == '\r' && (*From)[2] == '\n')
        (*From) += 3;       /* 跳过 CR/LF 换行转义 */

    if (*From == End)
        return '\\';

    if (**From == '\\')
    {
        /* 转义字符 */
        (*From)++;
        if (*From == End)
            return '\\';

        ThisChar = *(*From)++;
        switch (ThisChar)
        {
            case '\\': return '\\';
            case '\'': return '\'';
            case '"':  return '"';
            case 'a':  return '\a';
            case 'b':  return '\b';
            case 'f':  return '\f';
            case 'n':  return '\n';
            case 'r':  return '\r';
            case 't':  return '\t';
            case 'v':  return '\v';
            case '0': case '1': case '2': case '3': return LexUnEscapeCharacterConstant(From, End, ThisChar, 8);
            case 'x': return LexUnEscapeCharacterConstant(From, End, '0', 16);
            default:   return ThisChar;
        }
    }
    else
        return *(*From)++;
}

/* 解析字符串常量：处理转义、查重存储，返回 char* 指针 */
enum LexToken LexGetStringConstant(Picoc *pc, struct LexState *Lexer, struct Value *Value, char EndChar)
{
    int Escape = FALSE;
    const char *StartPos = Lexer->Pos;
    const char *EndPos;
    char *EscBuf;
    char *EscBufPos;
    char *RegString;
    struct Value *ArrayValue;

    while (Lexer->Pos != Lexer->End && (*Lexer->Pos != EndChar || Escape))
    {
        /* 找到字符串结束位置 */
        if (Escape)
        {
            if (*Lexer->Pos == '\r' && Lexer->Pos+1 != Lexer->End)
                Lexer->Pos++;

            if (*Lexer->Pos == '\n' && Lexer->Pos+1 != Lexer->End)
            {
                Lexer->Line++;
                Lexer->Pos++;
                Lexer->CharacterPos = 0;
                Lexer->EmitExtraNewlines++;
            }

            Escape = FALSE;
        }
        else if (*Lexer->Pos == '\\')
            Escape = TRUE;

        LEXER_INC(Lexer);
    }
    EndPos = Lexer->Pos;

    EscBuf = HeapAllocStack(pc, EndPos - StartPos);
    if (EscBuf == NULL)
        LexFail(pc, Lexer, "out of memory");

    for (EscBufPos = EscBuf, Lexer->Pos = StartPos; Lexer->Pos != EndPos;)
        *EscBufPos++ = LexUnEscapeCharacter(&Lexer->Pos, EndPos);

    /* 尝试查找已存在的字符串字面量以避免重复存储 */
    RegString = TableStrRegister2(pc, EscBuf, EscBufPos - EscBuf);
    HeapPopStack(pc, EscBuf, EndPos - StartPos);
    ArrayValue = VariableStringLiteralGet(pc, RegString);
    if (ArrayValue == NULL)
    {
        /* 创建并存储新的字符串字面量 */
        ArrayValue = VariableAllocValueAndData(pc, NULL, 0, FALSE, NULL, TRUE);
        ArrayValue->Typ = pc->CharArrayType;
        ArrayValue->Val = (union AnyValue *)RegString;
        VariableStringLiteralDefine(pc, RegString, ArrayValue);
    }

    /* 创建 char* 指针 */
    Value->Typ = pc->CharPtrType;
    Value->Val->Pointer = RegString;
    if (*Lexer->Pos == EndChar)
        LEXER_INC(Lexer);

    return TokenStringConstant;
}

/* 解析字符常量（如 'a'、'\n' 等） */
enum LexToken LexGetCharacterConstant(Picoc *pc, struct LexState *Lexer, struct Value *Value)
{
    Value->Typ = &pc->CharType;
    Value->Val->Character = LexUnEscapeCharacter(&Lexer->Pos, Lexer->End);
    if (Lexer->Pos != Lexer->End && *Lexer->Pos != '\'')
        LexFail(pc, Lexer, "expected \"'\"");

    LEXER_INC(Lexer);
    return TokenCharacterConstant;
}

/* 跳过注释：支持 C 风格块注释和 C++ 风格行注释 */
void LexSkipComment(struct LexState *Lexer, char NextChar, enum LexToken *ReturnToken)
{
    if (NextChar == '*')
    {
        /* C 风格块注释 */
        while (Lexer->Pos != Lexer->End && (*(Lexer->Pos-1) != '*' || *Lexer->Pos != '/'))
        {
            if (*Lexer->Pos == '\n')
                Lexer->EmitExtraNewlines++;

            LEXER_INC(Lexer);
        }

        if (Lexer->Pos != Lexer->End)
            LEXER_INC(Lexer);

        Lexer->Mode = LexModeNormal;
    }
    else
    {
        /* C++ 风格行注释 */
        while (Lexer->Pos != Lexer->End && *Lexer->Pos != '\n')
            LEXER_INC(Lexer);
    }
}

/* 扫描并返回一个 token：根据当前字符分派到标识符、数字、字符串、字符或运算符解析 */
enum LexToken LexScanGetToken(Picoc *pc, struct LexState *Lexer, struct Value **Value)
{
    char ThisChar;
    char NextChar;
    enum LexToken GotToken = TokenNone;

    /* 处理多行注释或字符串造成行号偏移的情况 */
    if (Lexer->EmitExtraNewlines > 0)
    {
        Lexer->EmitExtraNewlines--;
        return TokenEndOfLine;
    }

    /* 扫描 token */
    do
    {
        *Value = &pc->LexValue;
        while (Lexer->Pos != Lexer->End && isspace((int)*Lexer->Pos))
        {
            if (*Lexer->Pos == '\n')
            {
                Lexer->Line++;
                Lexer->Pos++;
                Lexer->Mode = LexModeNormal;
                Lexer->CharacterPos = 0;
                return TokenEndOfLine;
            }
            else if (Lexer->Mode == LexModeHashDefine || Lexer->Mode == LexModeHashDefineSpace)
                Lexer->Mode = LexModeHashDefineSpace;

            else if (Lexer->Mode == LexModeHashDefineSpaceIdent)
                Lexer->Mode = LexModeNormal;

            LEXER_INC(Lexer);
        }

        if (Lexer->Pos == Lexer->End || *Lexer->Pos == '\0')
            return TokenEOF;

        ThisChar = *Lexer->Pos;
        if (isCidstart((int)ThisChar))
            return LexGetWord(pc, Lexer, *Value);

        if (isdigit((int)ThisChar))
            return LexGetNumber(pc, Lexer, *Value);

        NextChar = (Lexer->Pos+1 != Lexer->End) ? *(Lexer->Pos+1) : 0;
        LEXER_INC(Lexer);
        switch (ThisChar)
        {
            case '"': GotToken = LexGetStringConstant(pc, Lexer, *Value, '"'); break;
            case '\'': GotToken = LexGetCharacterConstant(pc, Lexer, *Value); break;
            case '(': if (Lexer->Mode == LexModeHashDefineSpaceIdent) GotToken = TokenOpenMacroBracket; else GotToken = TokenOpenBracket; Lexer->Mode = LexModeNormal; break;
            case ')': GotToken = TokenCloseBracket; break;
            case '=': NEXTIS('=', TokenEqual, TokenAssign); break;
            case '+': NEXTIS3('=', TokenAddAssign, '+', TokenIncrement, TokenPlus); break;
            case '-': NEXTIS4('=', TokenSubtractAssign, '>', TokenArrow, '-', TokenDecrement, TokenMinus); break;
            case '*': NEXTIS('=', TokenMultiplyAssign, TokenAsterisk); break;
            case '/': if (NextChar == '/' || NextChar == '*') { LEXER_INC(Lexer); LexSkipComment(Lexer, NextChar, &GotToken); } else NEXTIS('=', TokenDivideAssign, TokenSlash); break;
            case '%': NEXTIS('=', TokenModulusAssign, TokenModulus); break;
            case '<': if (Lexer->Mode == LexModeHashInclude) GotToken = LexGetStringConstant(pc, Lexer, *Value, '>'); else { NEXTIS3PLUS('=', TokenLessEqual, '<', TokenShiftLeft, '=', TokenShiftLeftAssign, TokenLessThan); } break;
            case '>': NEXTIS3PLUS('=', TokenGreaterEqual, '>', TokenShiftRight, '=', TokenShiftRightAssign, TokenGreaterThan); break;
            case ';': GotToken = TokenSemicolon; break;
            case '&': NEXTIS3('=', TokenArithmeticAndAssign, '&', TokenLogicalAnd, TokenAmpersand); break;
            case '|': NEXTIS3('=', TokenArithmeticOrAssign, '|', TokenLogicalOr, TokenArithmeticOr); break;
            case '{': GotToken = TokenLeftBrace; break;
            case '}': GotToken = TokenRightBrace; break;
            case '[': GotToken = TokenLeftSquareBracket; break;
            case ']': GotToken = TokenRightSquareBracket; break;
            case '!': NEXTIS('=', TokenNotEqual, TokenUnaryNot); break;
            case '^': NEXTIS('=', TokenArithmeticExorAssign, TokenArithmeticExor); break;
            case '~': GotToken = TokenUnaryExor; break;
            case ',': GotToken = TokenComma; break;
            case '.': NEXTISEXACTLY3('.', '.', TokenEllipsis, TokenDot); break;
            case '?': GotToken = TokenQuestionMark; break;
            case ':': GotToken = TokenColon; break;
            default:  LexFail(pc, Lexer, "illegal character '%c'", ThisChar); break;
        }
    } while (GotToken == TokenNone);

    return GotToken;
}

/* 返回每种 token 类型对应的值所占字节数 */
int LexTokenSize(enum LexToken Token)
{
    switch (Token)
    {
        case TokenIdentifier: case TokenStringConstant: return sizeof(char *);
        case TokenIntegerConstant: return sizeof(long);
        case TokenCharacterConstant: return sizeof(unsigned char);
        case TokenFPConstant: return sizeof(double);
        default: return 0;
    }
}

/* 将源代码全部 token 化：循环调用 LexScanGetToken 并将结果序列化到堆缓冲区中 */
void *LexTokenise(Picoc *pc, struct LexState *Lexer, int *TokenLen)
{
    enum LexToken Token;
    void *HeapMem;
    struct Value *GotValue;
    int MemUsed = 0;
    int ValueSize;
    int ReserveSpace = (Lexer->End - Lexer->Pos) * 4 + 16;
    void *TokenSpace = HeapAllocStack(pc, ReserveSpace);
    char *TokenPos = (char *)TokenSpace;
    int LastCharacterPos = 0;

    if (TokenSpace == NULL)
        LexFail(pc, Lexer, "out of memory");

    do
    {
        /* 将 token 存储到栈区末尾 */
        Token = LexScanGetToken(pc, Lexer, &GotValue);

#ifdef DEBUG_LEXER
        printf("Token: %02x\n", Token);
#endif
        *(unsigned char *)TokenPos = Token;
        TokenPos++;
        MemUsed++;

        *(unsigned char *)TokenPos = (unsigned char)LastCharacterPos;
        TokenPos++;
        MemUsed++;

        ValueSize = LexTokenSize(Token);
        if (ValueSize > 0)
        {
            /* 附带值的 token：将值也拷贝进来 */
            memcpy((void *)TokenPos, (void *)GotValue->Val, ValueSize);
            TokenPos += ValueSize;
            MemUsed += ValueSize;
        }

        LastCharacterPos = Lexer->CharacterPos;

    } while (Token != TokenEOF);

    HeapMem = HeapAllocMem(pc, MemUsed);
    if (HeapMem == NULL)
        LexFail(pc, Lexer, "out of memory");

    assert(ReserveSpace >= MemUsed);
    memcpy(HeapMem, TokenSpace, MemUsed);
    HeapPopStack(pc, TokenSpace, ReserveSpace);
#ifdef DEBUG_LEXER
    {
        int Count;
        printf("Tokens: ");
        for (Count = 0; Count < MemUsed; Count++)
            printf("%02x ", *((unsigned char *)HeapMem+Count));
        printf("\n");
    }
#endif
    if (TokenLen)
        *TokenLen = MemUsed;

    return HeapMem;
}

/* 词法分析入口：初始化 Lexer 状态，调用 LexTokenise 生成 token 序列 */
void *LexAnalyse(Picoc *pc, const char *FileName, const char *Source, int SourceLen, int *TokenLen)
{
    struct LexState Lexer;

    Lexer.Pos = Source;
    Lexer.End = Source + SourceLen;
    Lexer.Line = 1;
    Lexer.FileName = FileName;
    Lexer.Mode = LexModeNormal;
    Lexer.EmitExtraNewlines = 0;
    Lexer.CharacterPos = 1;
    Lexer.SourceText = Source;

    return LexTokenise(pc, &Lexer, TokenLen);
}

/* 初始化解析器状态：绑定 picoc 实例、token 源、文件名、运行模式和调试标志 */
void LexInitParser(struct ParseState *Parser, Picoc *pc, const char *SourceText, void *TokenSource, char *FileName, int RunIt, int EnableDebugger)
{
    Parser->pc = pc;
    Parser->Pos = TokenSource;
    Parser->Line = 1;
    Parser->FileName = FileName;
    Parser->Mode = RunIt ? RunModeRun : RunModeSkip;
    Parser->SearchLabel = 0;
    Parser->HashIfLevel = 0;
    Parser->HashIfEvaluateToLevel = 0;
    Parser->CharacterPos = 0;
    Parser->SourceText = SourceText;
    Parser->DebugMode = EnableDebugger;
}

/* 获取下一个原始 token（不含预处理展开）。
 * 交互模式下，如果当前行 token 耗尽则自动读取新一行输入。 */
enum LexToken LexGetRawToken(struct ParseState *Parser, struct Value **Value, int IncPos)
{
    enum LexToken Token = TokenNone;
    int ValueSize;
    char *Prompt = NULL;
    Picoc *pc = Parser->pc;

    do
    {
        /* 获取下一个 token */
        if (Parser->Pos == NULL && pc->InteractiveHead != NULL)
            Parser->Pos = pc->InteractiveHead->Tokens;

        if (Parser->FileName != pc->StrEmpty || pc->InteractiveHead != NULL)
        {
            /* 跳过开头的换行 token */
            while ((Token = (enum LexToken)*(unsigned char *)Parser->Pos) == TokenEndOfLine)
            {
                Parser->Line++;
                Parser->Pos += TOKEN_DATA_OFFSET;
            }
        }

        if (Parser->FileName == pc->StrEmpty && (pc->InteractiveHead == NULL || Token == TokenEOF))
        {
            /* 交互模式：当前行结束，读取下一行输入 */
            char LineBuffer[LINEBUFFER_MAX];
            void *LineTokens;
            int LineBytes;
            struct TokenLine *LineNode;

            if (pc->InteractiveHead == NULL || (unsigned char *)Parser->Pos == &pc->InteractiveTail->Tokens[pc->InteractiveTail->NumBytes-TOKEN_DATA_OFFSET])
            {
                /* 读取一行新的交互输入 */
                if (pc->LexUseStatementPrompt)
                {
                    Prompt = INTERACTIVE_PROMPT_STATEMENT;
                    pc->LexUseStatementPrompt = FALSE;
                }
                else
                    Prompt = INTERACTIVE_PROMPT_LINE;

                if (PlatformGetLine(&LineBuffer[0], LINEBUFFER_MAX, Prompt) == NULL)
                    return TokenEOF;

                /* 将新行追加到交互输入链表末尾 */
                LineTokens = LexAnalyse(pc, pc->StrEmpty, &LineBuffer[0], strlen(LineBuffer), &LineBytes);
                LineNode = VariableAlloc(pc, Parser, sizeof(struct TokenLine), TRUE);
                LineNode->Tokens = LineTokens;
                LineNode->NumBytes = LineBytes;
                if (pc->InteractiveHead == NULL)
                {
                    /* 初始化为新链表 */
                    pc->InteractiveHead = LineNode;
                    Parser->Line = 1;
                    Parser->CharacterPos = 0;
                }
                else
                    pc->InteractiveTail->Next = LineNode;

                pc->InteractiveTail = LineNode;
                pc->InteractiveCurrentLine = LineNode;
                Parser->Pos = LineTokens;
            }
            else
            {
                /* 移动到下一个 token 行 */
                if (Parser->Pos != &pc->InteractiveCurrentLine->Tokens[pc->InteractiveCurrentLine->NumBytes-TOKEN_DATA_OFFSET])
                {
                    /* 扫描找到当前行 */
                    for (pc->InteractiveCurrentLine = pc->InteractiveHead; Parser->Pos != &pc->InteractiveCurrentLine->Tokens[pc->InteractiveCurrentLine->NumBytes-TOKEN_DATA_OFFSET]; pc->InteractiveCurrentLine = pc->InteractiveCurrentLine->Next)
                    { assert(pc->InteractiveCurrentLine->Next != NULL); }
                }

                assert(pc->InteractiveCurrentLine != NULL);
                pc->InteractiveCurrentLine = pc->InteractiveCurrentLine->Next;
                assert(pc->InteractiveCurrentLine != NULL);
                Parser->Pos = pc->InteractiveCurrentLine->Tokens;
            }

            Token = (enum LexToken)*(unsigned char *)Parser->Pos;
        }
    } while ((Parser->FileName == pc->StrEmpty && Token == TokenEOF) || Token == TokenEndOfLine);

    Parser->CharacterPos = *((unsigned char *)Parser->Pos + 1);
    ValueSize = LexTokenSize(Token);
    if (ValueSize > 0)
    {
        /* 该 token 附带一个值，解包它 */
        if (Value != NULL)
        {
            switch (Token)
            {
                case TokenStringConstant:       pc->LexValue.Typ = pc->CharPtrType; break;
                case TokenIdentifier:           pc->LexValue.Typ = NULL; break;
                case TokenIntegerConstant:      pc->LexValue.Typ = &pc->LongType; break;
                case TokenCharacterConstant:    pc->LexValue.Typ = &pc->CharType; break;
#ifndef NO_FP
                case TokenFPConstant:           pc->LexValue.Typ = &pc->FPType; break;
#endif
                default: break;
            }

            memcpy((void *)pc->LexValue.Val, (void *)((char *)Parser->Pos + TOKEN_DATA_OFFSET), ValueSize);
            pc->LexValue.ValOnHeap = FALSE;
            pc->LexValue.ValOnStack = FALSE;
            pc->LexValue.IsLValue = FALSE;
            pc->LexValue.LValueFrom = NULL;
            *Value = &pc->LexValue;
        }

        if (IncPos)
            Parser->Pos += ValueSize + TOKEN_DATA_OFFSET;
    }
    else
    {
        if (IncPos && Token != TokenEOF)
            Parser->Pos += TOKEN_DATA_OFFSET;
    }

#ifdef DEBUG_LEXER
    printf("Got token=%02x inc=%d pos=%d\n", Token, IncPos, Parser->CharacterPos);
#endif
    assert(Token <= TokenEndOfFunction);
    return Token;
}

/* 根据 IncPos 标志修正 token 位置（用于预处理指令宏） */
void LexHashIncPos(struct ParseState *Parser, int IncPos)
{
    if (!IncPos)
        LexGetRawToken(Parser, NULL, TRUE);
}

/* 处理 #ifdef / #ifndef 预处理指令 */
void LexHashIfdef(struct ParseState *Parser, int IfNot)
{
    /* 获取要检查的标识符 */
    struct Value *IdentValue;
    struct Value *SavedValue;
    int IsDefined;
    enum LexToken Token = LexGetRawToken(Parser, &IdentValue, TRUE);

    if (Token != TokenIdentifier)
        ProgramFail(Parser, "identifier expected");

    /* 检查标识符是否已定义 */
    IsDefined = TableGet(&Parser->pc->GlobalTable, IdentValue->Val->Identifier, &SavedValue, NULL, NULL, NULL);
    if (Parser->HashIfEvaluateToLevel == Parser->HashIfLevel && ( (IsDefined && !IfNot) || (!IsDefined && IfNot)) )
    {
        /* #if 条件成立，激活这一级 */
        Parser->HashIfEvaluateToLevel++;
    }

    Parser->HashIfLevel++;
}

/* 处理 #if 预处理指令（条件为宏或整型常量） */
void LexHashIf(struct ParseState *Parser)
{
    /* 获取要检查的符号 */
    struct Value *IdentValue;
    struct Value *SavedValue = NULL;
    struct ParseState MacroParser;
    enum LexToken Token = LexGetRawToken(Parser, &IdentValue, TRUE);

    if (Token == TokenIdentifier)
    {
        /* 从宏定义中查找值 */
        if (!TableGet(&Parser->pc->GlobalTable, IdentValue->Val->Identifier, &SavedValue, NULL, NULL, NULL))
            ProgramFail(Parser, "'%s' is undefined", IdentValue->Val->Identifier);

        if (SavedValue->Typ->Base != TypeMacro)
            ProgramFail(Parser, "value expected");

        ParserCopy(&MacroParser, &SavedValue->Val->MacroDef.Body);
        Token = LexGetRawToken(&MacroParser, &IdentValue, TRUE);
    }

    if (Token != TokenCharacterConstant && Token != TokenIntegerConstant)
        ProgramFail(Parser, "value expected");

    /* 检查标识符是否定义且值为真 */
    if (Parser->HashIfEvaluateToLevel == Parser->HashIfLevel && IdentValue->Val->Character)
    {
        /* #if 条件成立，激活这一级 */
        Parser->HashIfEvaluateToLevel++;
    }

    Parser->HashIfLevel++;
}

/* 处理 #else 预处理指令：翻转条件求值状态 */
void LexHashElse(struct ParseState *Parser)
{
    if (Parser->HashIfEvaluateToLevel == Parser->HashIfLevel - 1)
        Parser->HashIfEvaluateToLevel++;     /* #if 之前未激活，现在激活 #else 段 */

    else if (Parser->HashIfEvaluateToLevel == Parser->HashIfLevel)
    {
        /* #if 之前已激活，#else 段跳过 */
        if (Parser->HashIfLevel == 0)
            ProgramFail(Parser, "#else without #if");

        Parser->HashIfEvaluateToLevel--;
    }
}

/* 处理 #endif 预处理指令：结束一个条件块 */
void LexHashEndif(struct ParseState *Parser)
{
    if (Parser->HashIfLevel == 0)
        ProgramFail(Parser, "#endif without #if");

    Parser->HashIfLevel--;
    if (Parser->HashIfEvaluateToLevel > Parser->HashIfLevel)
        Parser->HashIfEvaluateToLevel = Parser->HashIfLevel;
}

#if 0 /* useful for debug */
void LexPrintToken(enum LexToken Token)
{
    char* TokenNames[] = {
        /* 0x00 */ "None",
        /* 0x01 */ "Comma",
        /* 0x02 */ "Assign", "AddAssign", "SubtractAssign", "MultiplyAssign", "DivideAssign", "ModulusAssign",
        /* 0x08 */ "ShiftLeftAssign", "ShiftRightAssign", "ArithmeticAndAssign", "ArithmeticOrAssign", "ArithmeticExorAssign",
        /* 0x0d */ "QuestionMark", "Colon",
        /* 0x0f */ "LogicalOr",
        /* 0x10 */ "LogicalAnd",
        /* 0x11 */ "ArithmeticOr",
        /* 0x12 */ "ArithmeticExor",
        /* 0x13 */ "Ampersand",
        /* 0x14 */ "Equal", "NotEqual",
        /* 0x16 */ "LessThan", "GreaterThan", "LessEqual", "GreaterEqual",
        /* 0x1a */ "ShiftLeft", "ShiftRight",
        /* 0x1c */ "Plus", "Minus",
        /* 0x1e */ "Asterisk", "Slash", "Modulus",
        /* 0x21 */ "Increment", "Decrement", "UnaryNot", "UnaryExor", "Sizeof", "Cast",
        /* 0x27 */ "LeftSquareBracket", "RightSquareBracket", "Dot", "Arrow",
        /* 0x2b */ "OpenBracket", "CloseBracket",
        /* 0x2d */ "Identifier", "IntegerConstant", "FPConstant", "StringConstant", "CharacterConstant",
        /* 0x32 */ "Semicolon", "Ellipsis",
        /* 0x34 */ "LeftBrace", "RightBrace",
        /* 0x36 */ "IntType", "CharType", "FloatType", "DoubleType", "VoidType", "EnumType",
        /* 0x3c */ "LongType", "SignedType", "ShortType", "StaticType", "AutoType", "RegisterType", "ExternType", "StructType", "UnionType", "UnsignedType", "Typedef",
        /* 0x46 */ "Continue", "Do", "Else", "For", "Goto", "If", "While", "Break", "Switch", "Case", "Default", "Return",
        /* 0x52 */ "HashDefine", "HashInclude", "HashIf", "HashIfdef", "HashIfndef", "HashElse", "HashEndif",
        /* 0x59 */ "New", "Delete",
        /* 0x5b */ "OpenMacroBracket",
        /* 0x5c */ "EOF", "EndOfLine", "EndOfFunction"
    };
    printf("{%s}", TokenNames[Token]);
}
#endif

/* 获取下一个 token（含预处理展开）。
 * 自动处理 #if/#ifdef/#ifndef/#else/#endif 条件编译指令，
 * 跳过不满足条件的 token 块，对外透明。 */
enum LexToken LexGetToken(struct ParseState *Parser, struct Value **Value, int IncPos)
{
    enum LexToken Token;
    int TryNextToken;

    /* 实现预处理 #if 系列指令 */
    do
    {
        int WasPreProcToken = TRUE;

        Token = LexGetRawToken(Parser, Value, IncPos);
        switch (Token)
        {
            case TokenHashIfdef:    LexHashIncPos(Parser, IncPos); LexHashIfdef(Parser, FALSE); break;
            case TokenHashIfndef:   LexHashIncPos(Parser, IncPos); LexHashIfdef(Parser, TRUE); break;
            case TokenHashIf:       LexHashIncPos(Parser, IncPos); LexHashIf(Parser); break;
            case TokenHashElse:     LexHashIncPos(Parser, IncPos); LexHashElse(Parser); break;
            case TokenHashEndif:    LexHashIncPos(Parser, IncPos); LexHashEndif(Parser); break;
            default:                WasPreProcToken = FALSE; break;
        }

        /* 如果 token 需要被跳过（条件不满足或本身就是预处理器 token），移动指针到下一个 */
        TryNextToken = (Parser->HashIfEvaluateToLevel < Parser->HashIfLevel && Token != TokenEOF) || WasPreProcToken;
        if (!IncPos && TryNextToken)
            LexGetRawToken(Parser, NULL, TRUE);

    } while (TryNextToken);

    return Token;
}

/* 窥视下一个 token 而不消耗它（跳过预处理） */
enum LexToken LexRawPeekToken(struct ParseState *Parser)
{
    return (enum LexToken)*(unsigned char *)Parser->Pos;
}

/* 跳过当前行剩余的所有 token */
void LexToEndOfLine(struct ParseState *Parser)
{
    while (TRUE)
    {
        enum LexToken Token = (enum LexToken)*(unsigned char *)Parser->Pos;
        if (Token == TokenEndOfLine || Token == TokenEOF)
            return;
        else
            LexGetRawToken(Parser, NULL, TRUE);
    }
}

/* 拷贝 StartParser 到 EndParser 之间的 token 到新内存中，
 * 移除 TokenEOF 并以 TokenEndOfFunction 结尾。
 * 用于函数体、宏体等需要独立存储 token 序列的场景。 */
void *LexCopyTokens(struct ParseState *StartParser, struct ParseState *EndParser)
{
    int MemSize = 0;
    int CopySize;
    unsigned char *Pos = (unsigned char *)StartParser->Pos;
    unsigned char *NewTokens;
    unsigned char *NewTokenPos;
    struct TokenLine *ILine;
    Picoc *pc = StartParser->pc;

    if (pc->InteractiveHead == NULL)
    {
        /* 非交互模式：直接拷贝 */
        MemSize = EndParser->Pos - StartParser->Pos;
        NewTokens = VariableAlloc(pc, StartParser, MemSize + TOKEN_DATA_OFFSET, TRUE);
        memcpy(NewTokens, (void *)StartParser->Pos, MemSize);
    }
    else
    {
        /* 交互模式：逐行累加 */
        for (pc->InteractiveCurrentLine = pc->InteractiveHead; pc->InteractiveCurrentLine != NULL && (Pos < &pc->InteractiveCurrentLine->Tokens[0] || Pos >= &pc->InteractiveCurrentLine->Tokens[pc->InteractiveCurrentLine->NumBytes]); pc->InteractiveCurrentLine = pc->InteractiveCurrentLine->Next)
        {} /* 找到当前位置所在的行 */

        if (EndParser->Pos >= StartParser->Pos && EndParser->Pos < &pc->InteractiveCurrentLine->Tokens[pc->InteractiveCurrentLine->NumBytes])
        {
            /* 所有 token 在同一行 */
            MemSize = EndParser->Pos - StartParser->Pos;
            NewTokens = VariableAlloc(pc, StartParser, MemSize + TOKEN_DATA_OFFSET, TRUE);
            memcpy(NewTokens, (void *)StartParser->Pos, MemSize);
        }
        else
        {
            /* 跨越多行 */
            MemSize = &pc->InteractiveCurrentLine->Tokens[pc->InteractiveCurrentLine->NumBytes-TOKEN_DATA_OFFSET] - Pos;

            for (ILine = pc->InteractiveCurrentLine->Next; ILine != NULL && (EndParser->Pos < &ILine->Tokens[0] || EndParser->Pos >= &ILine->Tokens[ILine->NumBytes]); ILine = ILine->Next)
                MemSize += ILine->NumBytes - TOKEN_DATA_OFFSET;

            assert(ILine != NULL);
            MemSize += EndParser->Pos - &ILine->Tokens[0];
            NewTokens = VariableAlloc(pc, StartParser, MemSize + TOKEN_DATA_OFFSET, TRUE);

            CopySize = &pc->InteractiveCurrentLine->Tokens[pc->InteractiveCurrentLine->NumBytes-TOKEN_DATA_OFFSET] - Pos;
            memcpy(NewTokens, Pos, CopySize);
            NewTokenPos = NewTokens + CopySize;
            for (ILine = pc->InteractiveCurrentLine->Next; ILine != NULL && (EndParser->Pos < &ILine->Tokens[0] || EndParser->Pos >= &ILine->Tokens[ILine->NumBytes]); ILine = ILine->Next)
            {
                memcpy(NewTokenPos, &ILine->Tokens[0], ILine->NumBytes - TOKEN_DATA_OFFSET);
                NewTokenPos += ILine->NumBytes-TOKEN_DATA_OFFSET;
            }
            assert(ILine != NULL);
            memcpy(NewTokenPos, &ILine->Tokens[0], EndParser->Pos - &ILine->Tokens[0]);
        }
    }

    NewTokens[MemSize] = (unsigned char)TokenEndOfFunction;

    return NewTokens;
}

/* 清除所有交互输入的 token 行（如 REPL 重置时调用） */
void LexInteractiveClear(Picoc *pc, struct ParseState *Parser)
{
    while (pc->InteractiveHead != NULL)
    {
        struct TokenLine *NextLine = pc->InteractiveHead->Next;

        HeapFreeMem(pc, pc->InteractiveHead->Tokens);
        HeapFreeMem(pc, pc->InteractiveHead);
        pc->InteractiveHead = NextLine;
    }

    if (Parser != NULL)
        Parser->Pos = NULL;

    pc->InteractiveTail = NULL;
}

/* 释放交互输入中已处理完毕的 token 行（已完成解析的前缀行） */
void LexInteractiveCompleted(Picoc *pc, struct ParseState *Parser)
{
    while (pc->InteractiveHead != NULL && !(Parser->Pos >= &pc->InteractiveHead->Tokens[0] && Parser->Pos < &pc->InteractiveHead->Tokens[pc->InteractiveHead->NumBytes]))
    {
        /* 该 token 行不再需要，释放 */
        struct TokenLine *NextLine = pc->InteractiveHead->Next;

        HeapFreeMem(pc, pc->InteractiveHead->Tokens);
        HeapFreeMem(pc, pc->InteractiveHead);
        pc->InteractiveHead = NextLine;

        if (pc->InteractiveHead == NULL)
        {
            /* 链表已清空 */
            Parser->Pos = NULL;
            pc->InteractiveTail = NULL;
        }
    }
}

/* 将下一次交互提示设为完整的语句提示符 */
void LexInteractiveStatementPrompt(Picoc *pc)
{
    pc->LexUseStatementPrompt = TRUE;
}
