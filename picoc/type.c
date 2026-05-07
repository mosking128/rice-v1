/* picoc data type module. This manages a tree of data types and has facilities
 * for parsing data types. */

/* PicoC 类型系统 — 管理以 UberType 为根的类型树; 基础类型 (int/char/float/void 等) 直挂在 UberType 下,
 * 派生类型 (指针/数组/结构体/联合体/枚举) 通过 FromType+DerivedTypeList 链链接;
 * TypeGetMatching 查找或创建匹配类型; TypeParse/TypeParseFront 解析 C 类型声明 */

#include "interpreter.h"

/* 基本类型的对齐字节数，由运行时自动检测 */
static int PointerAlignBytes;
static int IntAlignBytes;


/* 在类型树中添加新派生类型节点。新类型挂到 ParentType 的 DerivedTypeList 链表头 */
struct ValueType *TypeAdd(Picoc *pc, struct ParseState *Parser, struct ValueType *ParentType, enum BaseType Base, int ArraySize, const char *Identifier, int Sizeof, int AlignBytes)
{
    struct ValueType *NewType = VariableAlloc(pc, Parser, sizeof(struct ValueType), TRUE);
    NewType->Base = Base;
    NewType->ArraySize = ArraySize;
    NewType->Sizeof = Sizeof;
    NewType->AlignBytes = AlignBytes;
    NewType->Identifier = Identifier;
    NewType->Members = NULL;
    NewType->FromType = ParentType;
    NewType->DerivedTypeList = NULL;
    NewType->OnHeap = TRUE;
    NewType->Next = ParentType->DerivedTypeList;
    ParentType->DerivedTypeList = NewType;

    return NewType;
}

/* 查找或创建匹配的派生类型。根据 Base/ArraySize/Identifier 匹配，
 * AllowDuplicates=TRUE 则允许同名返回已有类型，FALSE 则重复时报错 */
struct ValueType *TypeGetMatching(Picoc *pc, struct ParseState *Parser, struct ValueType *ParentType, enum BaseType Base, int ArraySize, const char *Identifier, int AllowDuplicates)
{
    int Sizeof;
    int AlignBytes;
    struct ValueType *ThisType = ParentType->DerivedTypeList;
    while (ThisType != NULL && (ThisType->Base != Base || ThisType->ArraySize != ArraySize || ThisType->Identifier != Identifier))
        ThisType = ThisType->Next;

    if (ThisType != NULL)
    {
        if (AllowDuplicates)
            return ThisType;
        else
            ProgramFail(Parser, "data type '%s' is already defined", Identifier);
    }

    switch (Base)
    {
        case TypePointer:   Sizeof = sizeof(void *); AlignBytes = PointerAlignBytes; break;
        case TypeArray:     Sizeof = ArraySize * ParentType->Sizeof; AlignBytes = ParentType->AlignBytes; break;
        case TypeEnum:      Sizeof = sizeof(int); AlignBytes = IntAlignBytes; break;
        default:            Sizeof = 0; AlignBytes = 0; break;      /* struct/union 在添加成员后大小会增长 */
    }

    return TypeAdd(pc, Parser, ParentType, Base, ArraySize, Identifier, Sizeof, AlignBytes);
}

/* 获取值占用的栈空间大小(仅当 ValOnStack 时有效) */
int TypeStackSizeValue(struct Value *Val)
{
    if (Val != NULL && Val->ValOnStack)
        return TypeSizeValue(Val, FALSE);
    else
        return 0;
}

/* 获取值占用的内存大小。整数类型在非紧凑模式下预留扩展空间(ALIGN_TYPE 大小) */
int TypeSizeValue(struct Value *Val, int Compact)
{
    if (IS_INTEGER_NUMERIC(Val) && !Compact)
        return sizeof(ALIGN_TYPE);     /* 整数类型留出扩展空间 */
    else if (Val->Typ->Base != TypeArray)
        return Val->Typ->Sizeof;
    else
        return Val->Typ->FromType->Sizeof * Val->Typ->ArraySize;
}

/* 根据类型和数组大小计算所需内存大小 */
int TypeSize(struct ValueType *Typ, int ArraySize, int Compact)
{
    if (IS_INTEGER_NUMERIC_TYPE(Typ) && !Compact)
        return sizeof(ALIGN_TYPE);     /* 整数类型留出扩展空间 */
    else if (Typ->Base != TypeArray)
        return Typ->Sizeof;
    else
        return Typ->FromType->Sizeof * ArraySize;
}

/* 添加基本类型节点(非堆分配，静态嵌入 Picoc 结构体中)。挂到 UberType 的派生链上 */
void TypeAddBaseType(Picoc *pc, struct ValueType *TypeNode, enum BaseType Base, int Sizeof, int AlignBytes)
{
    TypeNode->Base = Base;
    TypeNode->ArraySize = 0;
    TypeNode->Sizeof = Sizeof;
    TypeNode->AlignBytes = AlignBytes;
    TypeNode->Identifier = pc->StrEmpty;
    TypeNode->Members = NULL;
    TypeNode->FromType = NULL;
    TypeNode->DerivedTypeList = NULL;
    TypeNode->OnHeap = FALSE;
    TypeNode->Next = pc->UberType.DerivedTypeList;
    pc->UberType.DerivedTypeList = TypeNode;
}

/* 初始化类型系统: 自动检测 int/指针的对齐，注册所有基本类型和常用派生类型 */
void TypeInit(Picoc *pc)
{
    struct IntAlign { char x; int y; } ia;
    struct ShortAlign { char x; short y; } sa;
    struct CharAlign { char x; char y; } ca;
    struct LongAlign { char x; long y; } la;
#ifndef NO_FP
    struct DoubleAlign { char x; double y; } da;
#endif
    struct PointerAlign { char x; void *y; } pa;

    IntAlignBytes = (char *)&ia.y - &ia.x;
    PointerAlignBytes = (char *)&pa.y - &pa.x;

    pc->UberType.DerivedTypeList = NULL;
    TypeAddBaseType(pc, &pc->IntType, TypeInt, sizeof(int), IntAlignBytes);
    TypeAddBaseType(pc, &pc->ShortType, TypeShort, sizeof(short), (char *)&sa.y - &sa.x);
    TypeAddBaseType(pc, &pc->CharType, TypeChar, sizeof(char), (char *)&ca.y - &ca.x);
    TypeAddBaseType(pc, &pc->LongType, TypeLong, sizeof(long), (char *)&la.y - &la.x);
    TypeAddBaseType(pc, &pc->UnsignedIntType, TypeUnsignedInt, sizeof(unsigned int), IntAlignBytes);
    TypeAddBaseType(pc, &pc->UnsignedShortType, TypeUnsignedShort, sizeof(unsigned short), (char *)&sa.y - &sa.x);
    TypeAddBaseType(pc, &pc->UnsignedLongType, TypeUnsignedLong, sizeof(unsigned long), (char *)&la.y - &la.x);
    TypeAddBaseType(pc, &pc->UnsignedCharType, TypeUnsignedChar, sizeof(unsigned char), (char *)&ca.y - &ca.x);
    TypeAddBaseType(pc, &pc->VoidType, TypeVoid, 0, 1);
    TypeAddBaseType(pc, &pc->FunctionType, TypeFunction, sizeof(int), IntAlignBytes);
    TypeAddBaseType(pc, &pc->MacroType, TypeMacro, sizeof(int), IntAlignBytes);
    TypeAddBaseType(pc, &pc->GotoLabelType, TypeGotoLabel, 0, 1);
#ifndef NO_FP
    TypeAddBaseType(pc, &pc->FPType, TypeFP, sizeof(double), (char *)&da.y - &da.x);
    TypeAddBaseType(pc, &pc->TypeType, Type_Type, sizeof(double), (char *)&da.y - &da.x);  /* 必须足够大以转换为 double */
#else
    TypeAddBaseType(pc, &pc->TypeType, Type_Type, sizeof(struct ValueType *), PointerAlignBytes);
#endif
    pc->CharArrayType = TypeAdd(pc, NULL, &pc->CharType, TypeArray, 0, pc->StrEmpty, sizeof(char), (char *)&ca.y - &ca.x);
    pc->CharPtrType = TypeAdd(pc, NULL, &pc->CharType, TypePointer, 0, pc->StrEmpty, sizeof(void *), PointerAlignBytes);
    pc->CharPtrPtrType = TypeAdd(pc, NULL, pc->CharPtrType, TypePointer, 0, pc->StrEmpty, sizeof(void *), PointerAlignBytes);
    pc->VoidPtrType = TypeAdd(pc, NULL, &pc->VoidType, TypePointer, 0, pc->StrEmpty, sizeof(void *), PointerAlignBytes);
}

/* 递归释放堆上分配的类型节点。struct/union 需先释放成员表和成员值 */
void TypeCleanupNode(Picoc *pc, struct ValueType *Typ)
{
    struct ValueType *SubType;
    struct ValueType *NextSubType;

    /* 清理并释放所有子节点 */
    for (SubType = Typ->DerivedTypeList; SubType != NULL; SubType = NextSubType)
    {
        NextSubType = SubType->Next;
        TypeCleanupNode(pc, SubType);
        if (SubType->OnHeap)
        {
            /* struct/union 需要先释放成员值和成员表 */
            if (SubType->Members != NULL)
            {
                VariableTableCleanup(pc, SubType->Members);
                HeapFreeMem(pc, SubType->Members);
            }

            /* 释放类型节点本身 */
            HeapFreeMem(pc, SubType);
        }
    }
}

/* 清理整个类型系统: 从 UberType 根节点递归释放 */
void TypeCleanup(Picoc *pc)
{
    TypeCleanupNode(pc, &pc->UberType);
}

/* 解析 struct 或 union 声明:
 * - 可选的类型名(匿名则自动生成 ^s0000 临时名)
 * - 无大括号: 使用前向声明或已定义的 struct/union
 * - 有大括号: 解析成员列表，计算对齐和偏移 */
void TypeParseStruct(struct ParseState *Parser, struct ValueType **Typ, int IsStruct)
{
    struct Value *LexValue;
    struct ValueType *MemberType;
    char *MemberIdentifier;
    char *StructIdentifier;
    struct Value *MemberValue;
    enum LexToken Token;
    int AlignBoundary;
    Picoc *pc = Parser->pc;

    Token = LexGetToken(Parser, &LexValue, FALSE);
    if (Token == TokenIdentifier)
    {
        LexGetToken(Parser, &LexValue, TRUE);
        StructIdentifier = LexValue->Val->Identifier;
        Token = LexGetToken(Parser, NULL, FALSE);
    }
    else
    {
        static char TempNameBuf[7] = "^s0000";
        StructIdentifier = PlatformMakeTempName(pc, TempNameBuf);
    }

    *Typ = TypeGetMatching(pc, Parser, &Parser->pc->UberType, IsStruct ? TypeStruct : TypeUnion, 0, StructIdentifier, TRUE);
    if (Token == TokenLeftBrace && (*Typ)->Members != NULL)
        ProgramFail(Parser, "data type '%t' is already defined", *Typ);

    Token = LexGetToken(Parser, NULL, FALSE);
    if (Token != TokenLeftBrace)
    {
        /* 使用已定义的结构(前向声明引用) */
#if 0
        if ((*Typ)->Members == NULL)
            ProgramFail(Parser, "structure '%s' isn't defined", LexValue->Val->Identifier);
#endif
        return;
    }

    if (pc->TopStackFrame != NULL)
        ProgramFail(Parser, "struct/union definitions can only be globals");

    LexGetToken(Parser, NULL, TRUE);
    (*Typ)->Members = VariableAlloc(pc, Parser, sizeof(struct Table) + STRUCT_TABLE_SIZE * sizeof(struct TableEntry), TRUE);
    (*Typ)->Members->HashTable = (struct TableEntry **)((char *)(*Typ)->Members + sizeof(struct Table));
    TableInitTable((*Typ)->Members, (struct TableEntry **)((char *)(*Typ)->Members + sizeof(struct Table)), STRUCT_TABLE_SIZE, TRUE);

    do {
        TypeParse(Parser, &MemberType, &MemberIdentifier, NULL);
        if (MemberType == NULL || MemberIdentifier == NULL)
            ProgramFail(Parser, "invalid type in struct");

        MemberValue = VariableAllocValueAndData(pc, Parser, sizeof(int), FALSE, NULL, TRUE);
        MemberValue->Typ = MemberType;
        if (IsStruct)
        {
            /* struct: 按对齐边界分配成员在结构内的偏移 */
            AlignBoundary = MemberValue->Typ->AlignBytes;
            if (((*Typ)->Sizeof & (AlignBoundary-1)) != 0)
                (*Typ)->Sizeof += AlignBoundary - ((*Typ)->Sizeof & (AlignBoundary-1));

            MemberValue->Val->Integer = (*Typ)->Sizeof;
            (*Typ)->Sizeof += TypeSizeValue(MemberValue, TRUE);
        }
        else
        {
            /* union: 成员偏移始终为 0，结构大小取最大成员大小 */
            MemberValue->Val->Integer = 0;
            if (MemberValue->Typ->Sizeof > (*Typ)->Sizeof)
                (*Typ)->Sizeof = TypeSizeValue(MemberValue, TRUE);
        }

        /* 确保对齐要求随最大成员更新 */
        if ((*Typ)->AlignBytes < MemberValue->Typ->AlignBytes)
            (*Typ)->AlignBytes = MemberValue->Typ->AlignBytes;

        /* 将成员定义写入成员表 */
        if (!TableSet(pc, (*Typ)->Members, MemberIdentifier, MemberValue, Parser->FileName, Parser->Line, Parser->CharacterPos))
            ProgramFail(Parser, "member '%s' already defined", &MemberIdentifier);

        if (LexGetToken(Parser, NULL, TRUE) != TokenSemicolon)
            ProgramFail(Parser, "semicolon expected");

    } while (LexGetToken(Parser, NULL, FALSE) != TokenRightBrace);

    /* 将结构总大小对齐到最大成员的对齐边界 */
    AlignBoundary = (*Typ)->AlignBytes;
    if (((*Typ)->Sizeof & (AlignBoundary-1)) != 0)
        (*Typ)->Sizeof += AlignBoundary - ((*Typ)->Sizeof & (AlignBoundary-1));

    LexGetToken(Parser, NULL, TRUE);
}

/* 创建系统内部不透明结构(对用户隐藏成员) */
struct ValueType *TypeCreateOpaqueStruct(Picoc *pc, struct ParseState *Parser, const char *StructName, int Size)
{
    struct ValueType *Typ = TypeGetMatching(pc, Parser, &pc->UberType, TypeStruct, 0, StructName, FALSE);

    /* 创建空成员表 */
    Typ->Members = VariableAlloc(pc, Parser, sizeof(struct Table) + STRUCT_TABLE_SIZE * sizeof(struct TableEntry), TRUE);
    Typ->Members->HashTable = (struct TableEntry **)((char *)Typ->Members + sizeof(struct Table));
    TableInitTable(Typ->Members, (struct TableEntry **)((char *)Typ->Members + sizeof(struct Table)), STRUCT_TABLE_SIZE, TRUE);
    Typ->Sizeof = Size;

    return Typ;
}

/* 解析 enum 声明:
 * - 可选类型名(匿名则自动生成 ^e0000)
 * - 无大括号: 引用已定义的枚举
 * - 有大括号: 解析枚举值列表，值默认从 0 递增，支持显式赋值 */
void TypeParseEnum(struct ParseState *Parser, struct ValueType **Typ)
{
    struct Value *LexValue;
    struct Value InitValue;
    enum LexToken Token;
    int EnumValue = 0;
    char *EnumIdentifier;
    Picoc *pc = Parser->pc;

    Token = LexGetToken(Parser, &LexValue, FALSE);
    if (Token == TokenIdentifier)
    {
        LexGetToken(Parser, &LexValue, TRUE);
        EnumIdentifier = LexValue->Val->Identifier;
        Token = LexGetToken(Parser, NULL, FALSE);
    }
    else
    {
        static char TempNameBuf[7] = "^e0000";
        EnumIdentifier = PlatformMakeTempName(pc, TempNameBuf);
    }

    TypeGetMatching(pc, Parser, &pc->UberType, TypeEnum, 0, EnumIdentifier, Token != TokenLeftBrace);
    *Typ = &pc->IntType;
    if (Token != TokenLeftBrace)
    {
        /* 使用已定义的枚举 */
        if ((*Typ)->Members == NULL)
            ProgramFail(Parser, "enum '%s' isn't defined", EnumIdentifier);

        return;
    }

    if (pc->TopStackFrame != NULL)
        ProgramFail(Parser, "enum definitions can only be globals");

    LexGetToken(Parser, NULL, TRUE);
    (*Typ)->Members = &pc->GlobalTable;
    memset((void *)&InitValue, '\0', sizeof(struct Value));
    InitValue.Typ = &pc->IntType;
    InitValue.Val = (union AnyValue *)&EnumValue;
    do {
        if (LexGetToken(Parser, &LexValue, TRUE) != TokenIdentifier)
            ProgramFail(Parser, "identifier expected");

        EnumIdentifier = LexValue->Val->Identifier;
        if (LexGetToken(Parser, NULL, FALSE) == TokenAssign)
        {
            LexGetToken(Parser, NULL, TRUE);
            EnumValue = ExpressionParseInt(Parser);
        }

        VariableDefine(pc, Parser, EnumIdentifier, &InitValue, NULL, FALSE);

        Token = LexGetToken(Parser, NULL, TRUE);
        if (Token != TokenComma && Token != TokenRightBrace)
            ProgramFail(Parser, "comma expected");

        EnumValue++;

    } while (Token == TokenComma);
}

/* 解析类型前部: 处理 static/extern 等限定符、signed/unsigned 前缀、基本类型名。
 * 返回 TRUE 表示成功解析到类型，FALSE 表示未找到类型声明 */
int TypeParseFront(struct ParseState *Parser, struct ValueType **Typ, int *IsStatic)
{
    struct ParseState Before;
    struct Value *LexerValue;
    enum LexToken Token;
    int Unsigned = FALSE;
    struct Value *VarValue;
    int StaticQualifier = FALSE;
    Picoc *pc = Parser->pc;
    *Typ = NULL;

    /* 忽略开头的类型限定符 */
    ParserCopy(&Before, Parser);
    Token = LexGetToken(Parser, &LexerValue, TRUE);
    while (Token == TokenStaticType || Token == TokenAutoType || Token == TokenRegisterType || Token == TokenExternType)
    {
        if (Token == TokenStaticType)
            StaticQualifier = TRUE;

        Token = LexGetToken(Parser, &LexerValue, TRUE);
    }

    if (IsStatic != NULL)
        *IsStatic = StaticQualifier;

    /* 处理独立的 signed/unsigned(后面没有具体类型名) */
    if (Token == TokenSignedType || Token == TokenUnsignedType)
    {
        enum LexToken FollowToken = LexGetToken(Parser, &LexerValue, FALSE);
        Unsigned = (Token == TokenUnsignedType);

        if (FollowToken != TokenIntType && FollowToken != TokenLongType && FollowToken != TokenShortType && FollowToken != TokenCharType)
        {
            if (Token == TokenUnsignedType)
                *Typ = &pc->UnsignedIntType;
            else
                *Typ = &pc->IntType;

            return TRUE;
        }

        Token = LexGetToken(Parser, &LexerValue, TRUE);
    }

    switch (Token)
    {
        case TokenIntType: *Typ = Unsigned ? &pc->UnsignedIntType : &pc->IntType; break;
        case TokenShortType: *Typ = Unsigned ? &pc->UnsignedShortType : &pc->ShortType; break;
        case TokenCharType: *Typ = Unsigned ? &pc->UnsignedCharType : &pc->CharType; break;
        case TokenLongType: *Typ = Unsigned ? &pc->UnsignedLongType : &pc->LongType; break;
#ifndef NO_FP
        case TokenFloatType: case TokenDoubleType: *Typ = &pc->FPType; break;
#endif
        case TokenVoidType: *Typ = &pc->VoidType; break;

        case TokenStructType: case TokenUnionType:
            if (*Typ != NULL)
                ProgramFail(Parser, "bad type declaration");

            TypeParseStruct(Parser, Typ, Token == TokenStructType);
            break;

        case TokenEnumType:
            if (*Typ != NULL)
                ProgramFail(Parser, "bad type declaration");

            TypeParseEnum(Parser, Typ);
            break;

        case TokenIdentifier:
            /* 标识符类型必须是 typedef 定义的 */
            VariableGet(pc, Parser, LexerValue->Val->Identifier, &VarValue);
            *Typ = VarValue->Val->Typ;
            break;

        default: ParserCopy(Parser, &Before); return FALSE;
    }

    return TRUE;
}

/* 解析类型后部: 标识符之后的数组维度 [N] 等后缀。
 * 支持不定长数组 [] 和定长数组 [expr] */
struct ValueType *TypeParseBack(struct ParseState *Parser, struct ValueType *FromType)
{
    enum LexToken Token;
    struct ParseState Before;

    ParserCopy(&Before, Parser);
    Token = LexGetToken(Parser, NULL, TRUE);
    if (Token == TokenLeftSquareBracket)
    {
        /* 数组声明 */
        if (LexGetToken(Parser, NULL, FALSE) == TokenRightSquareBracket)
        {
            /* 不定长数组 [] */
            LexGetToken(Parser, NULL, TRUE);
            return TypeGetMatching(Parser->pc, Parser, TypeParseBack(Parser, FromType), TypeArray, 0, Parser->pc->StrEmpty, TRUE);
        }
        else
        {
            /* 定长数组 [expr] */
            enum RunMode OldMode = Parser->Mode;
            int ArraySize;
            Parser->Mode = RunModeRun;
            ArraySize = ExpressionParseInt(Parser);
            Parser->Mode = OldMode;

            if (LexGetToken(Parser, NULL, TRUE) != TokenRightSquareBracket)
                ProgramFail(Parser, "']' expected");

            return TypeGetMatching(Parser->pc, Parser, TypeParseBack(Parser, FromType), TypeArray, ArraySize, Parser->pc->StrEmpty, TRUE);
        }
    }
    else
    {
        /* 类型规范结束 */
        ParserCopy(Parser, &Before);
        return FromType;
    }
}

/* 解析类型声明中每个标识符对应的部分: 处理 * 指针、() 函数括号、标识符名 */
void TypeParseIdentPart(struct ParseState *Parser, struct ValueType *BasicTyp, struct ValueType **Typ, char **Identifier)
{
    struct ParseState Before;
    enum LexToken Token;
    struct Value *LexValue;
    int Done = FALSE;
    *Typ = BasicTyp;
    *Identifier = Parser->pc->StrEmpty;

    while (!Done)
    {
        ParserCopy(&Before, Parser);
        Token = LexGetToken(Parser, &LexValue, TRUE);
        switch (Token)
        {
            case TokenOpenBracket:
                if (*Typ != NULL)
                    ProgramFail(Parser, "bad type declaration");

                TypeParse(Parser, Typ, Identifier, NULL);
                if (LexGetToken(Parser, NULL, TRUE) != TokenCloseBracket)
                    ProgramFail(Parser, "')' expected");
                break;

            case TokenAsterisk:
                if (*Typ == NULL)
                    ProgramFail(Parser, "bad type declaration");

                *Typ = TypeGetMatching(Parser->pc, Parser, *Typ, TypePointer, 0, Parser->pc->StrEmpty, TRUE);
                break;

            case TokenIdentifier:
                if (*Typ == NULL || *Identifier != Parser->pc->StrEmpty)
                    ProgramFail(Parser, "bad type declaration");

                *Identifier = LexValue->Val->Identifier;
                Done = TRUE;
                break;

            default: ParserCopy(Parser, &Before); Done = TRUE; break;
        }
    }

    if (*Typ == NULL)
        ProgramFail(Parser, "bad type declaration");

    if (*Identifier != Parser->pc->StrEmpty)
    {
        /* 解析标识符之后的类型部分(如数组维度) */
        *Typ = TypeParseBack(Parser, *Typ);
    }
}

/* 解析完整类型声明(包括标识符): 先调用 TypeParseFront 解析基本类型，
 * 再调用 TypeParseIdentPart 解析指针/数组等修饰符和标识符名 */
void TypeParse(struct ParseState *Parser, struct ValueType **Typ, char **Identifier, int *IsStatic)
{
    struct ValueType *BasicType;

    TypeParseFront(Parser, &BasicType, IsStatic);
    TypeParseIdentPart(Parser, BasicType, Typ, Identifier);
}

/* 检查类型是否只是前向声明(struct/union 有名称但 Members 为空)。
 * 数组类型递归检查其 FromType */
int TypeIsForwardDeclared(struct ParseState *Parser, struct ValueType *Typ)
{
    if (Typ->Base == TypeArray)
        return TypeIsForwardDeclared(Parser, Typ->FromType);

    if ( (Typ->Base == TypeStruct || Typ->Base == TypeUnion) && Typ->Members == NULL)
        return TRUE;

    return FALSE;
}
