/* picoc variable storage. This provides ways of defining and accessing
 * variables */

/* PicoC 变量管理 — 变量定义(VariableDefine)、查找(VariableGet，先局部后全局)、
 * 作用域管理(VariableScopeBegin/End, OutOfScope标记)、栈帧压入/弹出(函数调用时);
 * VariableAllocValueAndData 分配 struct Value+AnyValue 联合体存储 */

#include "interpreter.h"

/* 临时拷贝缓冲区最大大小，用于在创建变量时暂存值 */
#define MAX_TMP_COPY_BUF 256


/* 初始化变量系统: 创建全局变量表和字符串字面量表 */
void VariableInit(Picoc *pc)
{
    TableInitTable(&(pc->GlobalTable), &(pc->GlobalHashTable)[0], GLOBAL_TABLE_SIZE, TRUE);
    TableInitTable(&pc->StringLiteralTable, &pc->StringLiteralHashTable[0], STRING_LITERAL_TABLE_SIZE, TRUE);
    pc->TopStackFrame = NULL;
}

/* 释放变量的堆上内容: 函数体、宏体、AnyValue 和 Value 自身 */
void VariableFree(Picoc *pc, struct Value *Val)
{
    if (Val->ValOnHeap || Val->AnyValOnHeap)
    {
        /* 释放函数体 */
        if (Val->Typ == &pc->FunctionType && Val->Val->FuncDef.Intrinsic == NULL && Val->Val->FuncDef.Body.Pos != NULL)
            HeapFreeMem(pc, (void *)Val->Val->FuncDef.Body.Pos);

        /* 释放宏体 */
        if (Val->Typ == &pc->MacroType)
            HeapFreeMem(pc, (void *)Val->Val->MacroDef.Body.Pos);

        /* 释放 AnyValue */
        if (Val->AnyValOnHeap)
            HeapFreeMem(pc, Val->Val);
    }

    /* 释放 Value 结构本身 */
    if (Val->ValOnHeap)
        HeapFreeMem(pc, Val);
}

/* 遍历哈希表，释放所有条目和其中的变量 */
void VariableTableCleanup(Picoc *pc, struct Table *HashTable)
{
    struct TableEntry *Entry;
    struct TableEntry *NextEntry;
    int Count;

    for (Count = 0; Count < HashTable->Size; Count++)
    {
        for (Entry = HashTable->HashTable[Count]; Entry != NULL; Entry = NextEntry)
        {
            NextEntry = Entry->Next;
            VariableFree(pc, Entry->p.v.Val);

            /* 释放哈希表条目 */
            HeapFreeMem(pc, Entry);
        }
    }
}

/* 清理变量系统: 释放全局表和字符串字面量表 */
void VariableCleanup(Picoc *pc)
{
    VariableTableCleanup(pc, &pc->GlobalTable);
    VariableTableCleanup(pc, &pc->StringLiteralTable);
}

/* 在堆或栈上分配 Size 字节内存，内存不足时报错 */
void *VariableAlloc(Picoc *pc, struct ParseState *Parser, int Size, int OnHeap)
{
    void *NewValue;

    if (OnHeap)
        NewValue = HeapAllocMem(pc, Size);
    else
        NewValue = HeapAllocStack(pc, Size);

    if (NewValue == NULL)
        ProgramFail(Parser, "out of memory");

#ifdef DEBUG_HEAP
    if (!OnHeap)
        printf("pushing %d at 0x%lx\n", Size, (unsigned long)NewValue);
#endif

    return NewValue;
}

/* 分配 Value + 附加数据空间。Value 结构体和 AnyValue 数据连续存放，
 * Val 指针指向紧跟 Value 结构体之后的数据区 */
struct Value *VariableAllocValueAndData(Picoc *pc, struct ParseState *Parser, int DataSize, int IsLValue, struct Value *LValueFrom, int OnHeap)
{
    struct Value *NewValue = VariableAlloc(pc, Parser, MEM_ALIGN(sizeof(struct Value)) + DataSize, OnHeap);
    NewValue->Val = (union AnyValue *)((char *)NewValue + MEM_ALIGN(sizeof(struct Value)));
    NewValue->ValOnHeap = OnHeap;
    NewValue->AnyValOnHeap = FALSE;
    NewValue->ValOnStack = !OnHeap;
    NewValue->IsLValue = IsLValue;
    NewValue->LValueFrom = LValueFrom;
    if (Parser)
        NewValue->ScopeID = Parser->ScopeID;

    NewValue->OutOfScope = 0;

    return NewValue;
}

/* 根据指定类型 Typ 分配 Value，数据区大小由类型决定 */
struct Value *VariableAllocValueFromType(Picoc *pc, struct ParseState *Parser, struct ValueType *Typ, int IsLValue, struct Value *LValueFrom, int OnHeap)
{
    int Size = TypeSize(Typ, Typ->ArraySize, FALSE);
    struct Value *NewValue = VariableAllocValueAndData(pc, Parser, Size, IsLValue, LValueFrom, OnHeap);
    assert(Size >= 0 || Typ == &pc->VoidType);
    NewValue->Typ = Typ;

    return NewValue;
}

/* 从已有值拷贝创建新 Value。通过临时缓冲区处理可能的内存重叠 */
struct Value *VariableAllocValueAndCopy(Picoc *pc, struct ParseState *Parser, struct Value *FromValue, int OnHeap)
{
    struct ValueType *DType = FromValue->Typ;
    struct Value *NewValue;
    char TmpBuf[MAX_TMP_COPY_BUF];
    int CopySize = TypeSizeValue(FromValue, TRUE);

    assert(CopySize <= MAX_TMP_COPY_BUF);
    memcpy((void *)&TmpBuf[0], (void *)FromValue->Val, CopySize);
    NewValue = VariableAllocValueAndData(pc, Parser, CopySize, FromValue->IsLValue, FromValue->LValueFrom, OnHeap);
    NewValue->Typ = DType;
    memcpy((void *)NewValue->Val, (void *)&TmpBuf[0], CopySize);

    return NewValue;
}

/* 从已有 AnyValue 和类型创建 Value(浅引用，不拷贝数据，Val 直接指向 FromValue) */
struct Value *VariableAllocValueFromExistingData(struct ParseState *Parser, struct ValueType *Typ, union AnyValue *FromValue, int IsLValue, struct Value *LValueFrom)
{
    struct Value *NewValue = VariableAlloc(Parser->pc, Parser, sizeof(struct Value), FALSE);
    NewValue->Typ = Typ;
    NewValue->Val = FromValue;
    NewValue->ValOnHeap = FALSE;
    NewValue->AnyValOnHeap = FALSE;
    NewValue->ValOnStack = FALSE;
    NewValue->IsLValue = IsLValue;
    NewValue->LValueFrom = LValueFrom;

    return NewValue;
}

/* 共享已有 Value(浅拷贝，共享底层同类型、同数据) */
struct Value *VariableAllocValueShared(struct ParseState *Parser, struct Value *FromValue)
{
    return VariableAllocValueFromExistingData(Parser, FromValue->Typ, FromValue->Val, FromValue->IsLValue, FromValue->IsLValue ? FromValue : NULL);
}

/* 重新分配变量的数据区为新大小。释放旧 AnyValOnHeap 数据，在堆上分配新空间 */
void VariableRealloc(struct ParseState *Parser, struct Value *FromValue, int NewSize)
{
    if (FromValue->AnyValOnHeap)
        HeapFreeMem(Parser->pc, FromValue->Val);

    FromValue->Val = VariableAlloc(Parser->pc, Parser, NewSize, TRUE);
    FromValue->AnyValOnHeap = TRUE;
}

/* 开始新作用域: 计算新 ScopeID，遍历哈希表将之前离开该作用域的变量重新激活(OutOfScope=FALSE) */
int VariableScopeBegin(struct ParseState * Parser, int* OldScopeID)
{
    struct TableEntry *Entry;
    struct TableEntry *NextEntry;
    Picoc * pc = Parser->pc;
    int Count;
    #ifdef VAR_SCOPE_DEBUG
    int FirstPrint = 0;
    #endif

    struct Table * HashTable = (pc->TopStackFrame == NULL) ? &(pc->GlobalTable) : &(pc->TopStackFrame)->LocalTable;

    if (Parser->ScopeID == -1) return -1;

    /* ScopeID 由源码文本地址和位置计算，用于标识同一作用域 */
    *OldScopeID = Parser->ScopeID;
    Parser->ScopeID = (int)(intptr_t)(Parser->SourceText) * ((int)(intptr_t)(Parser->Pos) / sizeof(char*));
    /* 或者用更易读的哈希用于调试: */
    /* Parser->ScopeID = Parser->Line * 0x10000 + Parser->CharacterPos; */

    for (Count = 0; Count < HashTable->Size; Count++)
    {
        for (Entry = HashTable->HashTable[Count]; Entry != NULL; Entry = NextEntry)
        {
            NextEntry = Entry->Next;
            if (Entry->p.v.Val->ScopeID == Parser->ScopeID && Entry->p.v.Val->OutOfScope)
            {
                Entry->p.v.Val->OutOfScope = FALSE;
                Entry->p.v.Key = (char*)((intptr_t)Entry->p.v.Key & ~1); /* 恢复键名使变量可被正常查找 */
                #ifdef VAR_SCOPE_DEBUG
                if (!FirstPrint) { PRINT_SOURCE_POS; }
                FirstPrint = 1;
                printf(">>> back into scope: %s %x %d\n", Entry->p.v.Key, Entry->p.v.Val->ScopeID, Entry->p.v.Val->Val->Integer);
                #endif
            }
        }
    }

    return Parser->ScopeID;
}

/* 结束当前作用域: 将该 ScopeID 的变量标记为 OutOfScope，并修改键名使其在正常查找中不可见 */
void VariableScopeEnd(struct ParseState * Parser, int ScopeID, int PrevScopeID)
{
    struct TableEntry *Entry;
    struct TableEntry *NextEntry;
    Picoc * pc = Parser->pc;
    int Count;
    #ifdef VAR_SCOPE_DEBUG
    int FirstPrint = 0;
    #endif

    struct Table * HashTable = (pc->TopStackFrame == NULL) ? &(pc->GlobalTable) : &(pc->TopStackFrame)->LocalTable;

    if (ScopeID == -1) return;

    for (Count = 0; Count < HashTable->Size; Count++)
    {
        for (Entry = HashTable->HashTable[Count]; Entry != NULL; Entry = NextEntry)
        {
            NextEntry = Entry->Next;
            if (Entry->p.v.Val->ScopeID == ScopeID && !Entry->p.v.Val->OutOfScope)
            {
                #ifdef VAR_SCOPE_DEBUG
                if (!FirstPrint) { PRINT_SOURCE_POS; }
                FirstPrint = 1;
                printf(">>> out of scope: %s %x %d\n", Entry->p.v.Key, Entry->p.v.Val->ScopeID, Entry->p.v.Val->Val->Integer);
                #endif
                Entry->p.v.Val->OutOfScope = TRUE;
                Entry->p.v.Key = (char*)((intptr_t)Entry->p.v.Key | 1); /* 修改键名最低位使正常查找找不到 */
            }
        }
    }

    Parser->ScopeID = PrevScopeID;
}

/* 检查变量是否已定义但离开了作用域(用于更友好的错误提示) */
int VariableDefinedAndOutOfScope(Picoc * pc, const char* Ident)
{
    struct TableEntry *Entry;
    int Count;

    struct Table * HashTable = (pc->TopStackFrame == NULL) ? &(pc->GlobalTable) : &(pc->TopStackFrame)->LocalTable;
    for (Count = 0; Count < HashTable->Size; Count++)
    {
        for (Entry = HashTable->HashTable[Count]; Entry != NULL; Entry = Entry->Next)
        {
            if (Entry->p.v.Val->OutOfScope && (char*)((intptr_t)Entry->p.v.Key & ~1) == Ident)
                return TRUE;
        }
    }
    return FALSE;
}

/* 定义新变量。有 InitValue 则拷贝初始化，否则按类型分配默认空间。
 * 变量插入到当前作用域的符号表中，重名则报错 */
struct Value *VariableDefine(Picoc *pc, struct ParseState *Parser, char *Ident, struct Value *InitValue, struct ValueType *Typ, int MakeWritable)
{
    struct Value * AssignValue;
    struct Table * currentTable = (pc->TopStackFrame == NULL) ? &(pc->GlobalTable) : &(pc->TopStackFrame)->LocalTable;

    int ScopeID = Parser ? Parser->ScopeID : -1;
#ifdef VAR_SCOPE_DEBUG
    if (Parser) fprintf(stderr, "def %s %x (%s:%d:%d)\n", Ident, ScopeID, Parser->FileName, Parser->Line, Parser->CharacterPos);
#endif

    if (InitValue != NULL)
        AssignValue = VariableAllocValueAndCopy(pc, Parser, InitValue, pc->TopStackFrame == NULL);
    else
        AssignValue = VariableAllocValueFromType(pc, Parser, Typ, MakeWritable, NULL, pc->TopStackFrame == NULL);

    AssignValue->IsLValue = MakeWritable;
    AssignValue->ScopeID = ScopeID;
    AssignValue->OutOfScope = FALSE;

    if (!TableSet(pc, currentTable, Ident, AssignValue, Parser ? ((char *)Parser->FileName) : NULL, Parser ? Parser->Line : 0, Parser ? Parser->CharacterPos : 0))
        ProgramFail(Parser, "'%s' is already defined", Ident);

    return AssignValue;
}

/* 定义变量，但如果同一源位置重定义(同行号同列号)则忽略错误。
 * static 变量通过混淆名存储到全局表，并在局部作用域创建镜像变量 */
struct Value *VariableDefineButIgnoreIdentical(struct ParseState *Parser, char *Ident, struct ValueType *Typ, int IsStatic, int *FirstVisit)
{
    Picoc *pc = Parser->pc;
    struct Value *ExistingValue;
    const char *DeclFileName;
    int DeclLine;
    int DeclColumn;

    /* 检查类型是否只是前向声明(尚未完整定义) */
    if (TypeIsForwardDeclared(Parser, Typ))
        ProgramFail(Parser, "type '%t' isn't defined", Typ);

    if (IsStatic)
    {
        char MangledName[LINEBUFFER_MAX];
        char *MNPos = &MangledName[0];
        char *MNEnd = &MangledName[LINEBUFFER_MAX-1];
        const char *RegisteredMangledName;

        /* 构造 static 变量的混淆名: /文件名/函数名/变量名 */
        memset((void *)&MangledName, '\0', sizeof(MangledName));
        *MNPos++ = '/';
        strncpy(MNPos, (char *)Parser->FileName, MNEnd - MNPos);
        MNPos += strlen(MNPos);

        if (pc->TopStackFrame != NULL)
        {
            /* 在函数内部，追加函数名 */
            if (MNEnd - MNPos > 0) *MNPos++ = '/';
            strncpy(MNPos, (char *)pc->TopStackFrame->FuncName, MNEnd - MNPos);
            MNPos += strlen(MNPos);
        }

        if (MNEnd - MNPos > 0) *MNPos++ = '/';
        strncpy(MNPos, Ident, MNEnd - MNPos);
        RegisteredMangledName = TableStrRegister(pc, MangledName);

        /* 检查混淆名的 static 变量是否已存在 */
        if (!TableGet(&pc->GlobalTable, RegisteredMangledName, &ExistingValue, &DeclFileName, &DeclLine, &DeclColumn))
        {
            /* 在全局作用域创建混淆名的 static 变量存储 */
            ExistingValue = VariableAllocValueFromType(Parser->pc, Parser, Typ, TRUE, NULL, TRUE);
            TableSet(pc, &pc->GlobalTable, (char *)RegisteredMangledName, ExistingValue, (char *)Parser->FileName, Parser->Line, Parser->CharacterPos);
            *FirstVisit = TRUE;
        }

        /* 在当前作用域用短名创建镜像变量，共享全局存储 */
        VariableDefinePlatformVar(Parser->pc, Parser, Ident, ExistingValue->Typ, ExistingValue->Val, TRUE);
        return ExistingValue;
    }
    else
    {
        if (Parser->Line != 0 && TableGet((pc->TopStackFrame == NULL) ? &pc->GlobalTable : &pc->TopStackFrame->LocalTable, Ident, &ExistingValue, &DeclFileName, &DeclLine, &DeclColumn)
                && DeclFileName == Parser->FileName && DeclLine == Parser->Line && DeclColumn == Parser->CharacterPos)
            return ExistingValue;
        else
            return VariableDefine(Parser->pc, Parser, Ident, NULL, Typ, TRUE);
    }
}

/* 检查变量是否已定义。先查局部表，再查全局表 */
int VariableDefined(Picoc *pc, const char *Ident)
{
    struct Value *FoundValue;

    if (pc->TopStackFrame == NULL || !TableGet(&pc->TopStackFrame->LocalTable, Ident, &FoundValue, NULL, NULL, NULL))
    {
        if (!TableGet(&pc->GlobalTable, Ident, &FoundValue, NULL, NULL, NULL))
            return FALSE;
    }

    return TRUE;
}

/* 获取变量的值。先查局部表，再查全局表。找不到则报错:
 * "out of scope"(已定义但离开作用域) 或 "undefined"(完全未定义) */
void VariableGet(Picoc *pc, struct ParseState *Parser, const char *Ident, struct Value **LVal)
{
    if (pc->TopStackFrame == NULL || !TableGet(&pc->TopStackFrame->LocalTable, Ident, LVal, NULL, NULL, NULL))
    {
        if (!TableGet(&pc->GlobalTable, Ident, LVal, NULL, NULL, NULL))
        {
            if (VariableDefinedAndOutOfScope(pc, Ident))
                ProgramFail(Parser, "'%s' is out of scope", Ident);
            else
                ProgramFail(Parser, "'%s' is undefined", Ident);
        }
    }
}

/* 定义与平台全局变量共享存储的变量。Val 直接指向平台提供的数据地址 */
void VariableDefinePlatformVar(Picoc *pc, struct ParseState *Parser, char *Ident, struct ValueType *Typ, union AnyValue *FromValue, int IsWritable)
{
    struct Value *SomeValue = VariableAllocValueAndData(pc, NULL, 0, IsWritable, NULL, TRUE);
    SomeValue->Typ = Typ;
    SomeValue->Val = FromValue;

    if (!TableSet(pc, (pc->TopStackFrame == NULL) ? &pc->GlobalTable : &pc->TopStackFrame->LocalTable, TableStrRegister(pc, Ident), SomeValue, Parser ? Parser->FileName : NULL, Parser ? Parser->Line : 0, Parser ? Parser->CharacterPos : 0))
        ProgramFail(Parser, "'%s' is already defined", Ident);
}

/* 释放并弹出栈顶变量。根据 ValOnHeap/ValOnStack 标志选择释放方式 */
void VariableStackPop(struct ParseState *Parser, struct Value *Var)
{
    int Success;

#ifdef DEBUG_HEAP
    if (Var->ValOnStack)
        printf("popping %ld at 0x%lx\n", (unsigned long)(sizeof(struct Value) + TypeSizeValue(Var, FALSE)), (unsigned long)Var);
#endif

    if (Var->ValOnHeap)
    {
        if (Var->Val != NULL)
            HeapFreeMem(Parser->pc, Var->Val);

        Success = HeapPopStack(Parser->pc, Var, sizeof(struct Value));                       /* 从堆上释放 */
    }
    else if (Var->ValOnStack)
        Success = HeapPopStack(Parser->pc, Var, sizeof(struct Value) + TypeSizeValue(Var, FALSE));  /* 从栈上释放 */
    else
        Success = HeapPopStack(Parser->pc, Var, sizeof(struct Value));                       /* 值不由我们管理 */

    if (!Success)
        ProgramFail(Parser, "stack underrun");
}

/* 函数调用时添加栈帧: 保存返回解析器状态，分配参数数组和局部变量表 */
void VariableStackFrameAdd(struct ParseState *Parser, const char *FuncName, int NumParams)
{
    struct StackFrame *NewFrame;

    HeapPushStackFrame(Parser->pc);
    NewFrame = HeapAllocStack(Parser->pc, sizeof(struct StackFrame) + sizeof(struct Value *) * NumParams);
    if (NewFrame == NULL)
        ProgramFail(Parser, "out of memory");

    ParserCopy(&NewFrame->ReturnParser, Parser);
    NewFrame->FuncName = FuncName;
    NewFrame->Parameter = (NumParams > 0) ? ((void *)((char *)NewFrame + sizeof(struct StackFrame))) : NULL;
    TableInitTable(&NewFrame->LocalTable, &NewFrame->LocalHashTable[0], LOCAL_TABLE_SIZE, FALSE);
    NewFrame->PreviousStackFrame = Parser->pc->TopStackFrame;
    Parser->pc->TopStackFrame = NewFrame;
}

/* 弹出当前栈帧: 恢复返回解析器状态和上一个栈帧指针 */
void VariableStackFramePop(struct ParseState *Parser)
{
    if (Parser->pc->TopStackFrame == NULL)
        ProgramFail(Parser, "stack is empty - can't go back");

    ParserCopy(Parser, &Parser->pc->TopStackFrame->ReturnParser);
    Parser->pc->TopStackFrame = Parser->pc->TopStackFrame->PreviousStackFrame;
    HeapPopStackFrame(Parser->pc);
}

/* 获取字符串字面量。未找到返回 NULL */
struct Value *VariableStringLiteralGet(Picoc *pc, char *Ident)
{
    struct Value *LVal = NULL;

    if (TableGet(&pc->StringLiteralTable, Ident, &LVal, NULL, NULL, NULL))
        return LVal;
    else
        return NULL;
}

/* 定义字符串字面量到字符串字面量表 */
void VariableStringLiteralDefine(Picoc *pc, char *Ident, struct Value *Val)
{
    TableSet(pc, &pc->StringLiteralTable, Ident, Val, NULL, 0, 0);
}

/* 解引用指针: 返回指针指向的实际数据地址，同时输出解引用类型和 LValue 信息 */
void *VariableDereferencePointer(struct ParseState *Parser, struct Value *PointerValue, struct Value **DerefVal, int *DerefOffset, struct ValueType **DerefType, int *DerefIsLValue)
{
    if (DerefVal != NULL)
        *DerefVal = NULL;

    if (DerefType != NULL)
        *DerefType = PointerValue->Typ->FromType;

    if (DerefOffset != NULL)
        *DerefOffset = 0;

    if (DerefIsLValue != NULL)
        *DerefIsLValue = TRUE;

    return PointerValue->Val->Pointer;
}
