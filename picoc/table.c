/* picoc hash table module. This hash table code is used for both symbol tables
 * and the shared string table. */

/* PicoC 哈希表 — 用于全局/局部符号表、共享字符串表、字符串字面量表、保留字表;
 * 关键设计: 所有标识符通过 TableStrRegister 注册为唯一指针，哈希直接基于指针值而非字符串内容;
 * TableGet/TableSet 为基本操作，TableDelete 支持删除 */

#include "interpreter.h"

/* 初始化共享字符串系统: 创建空字符串表并注册空字符串 */
void TableInit(Picoc *pc)
{
    TableInitTable(&pc->StringTable, &pc->StringHashTable[0], STRING_TABLE_SIZE, TRUE);
    pc->StrEmpty = TableStrRegister(pc, "");
}

/* 字符串哈希函数: 将字符按偏移分散到无符号整型各位 */
static unsigned int TableHash(const char *Key, int Len)
{
    unsigned int Hash = Len;
    int Offset;
    int Count;

    for (Count = 0, Offset = 8; Count < Len; Count++, Offset+=7)
    {
        if (Offset > sizeof(unsigned int) * 8 - 7)
            Offset -= sizeof(unsigned int) * 8 - 6;

        Hash ^= *Key++ << Offset;
    }

    return Hash;
}

/* 初始化哈希表: 设置大小、分配策略和哈希桶数组 */
void TableInitTable(struct Table *Tbl, struct TableEntry **HashTable, int Size, int OnHeap)
{
    Tbl->Size = Size;
    Tbl->OnHeap = OnHeap;
    Tbl->HashTable = HashTable;
    memset((void *)HashTable, '\0', sizeof(struct TableEntry *) * Size);
}

/* 在哈希表中搜索已注册的共享字符串 Key。使用指针地址取模(无需重新哈希) */
static struct TableEntry *TableSearch(struct Table *Tbl, const char *Key, int *AddAt)
{
    struct TableEntry *Entry;
    int HashValue = ((unsigned long)Key) % Tbl->Size;   /* 共享字符串有唯一地址，直接用地址取模 */

    for (Entry = Tbl->HashTable[HashValue]; Entry != NULL; Entry = Entry->Next)
    {
        if (Entry->p.v.Key == Key)
            return Entry;   /* 找到了 */
    }

    *AddAt = HashValue;    /* 在链上未找到，记录插入位置 */
    return NULL;
}

/* 在哈希表中设置键值对。Key 必须来自 TableStrRegister()。
 * 已存在返回 FALSE(不覆盖)，成功插入返回 TRUE */
int TableSet(Picoc *pc, struct Table *Tbl, char *Key, struct Value *Val, const char *DeclFileName, int DeclLine, int DeclColumn)
{
    int AddAt;
    struct TableEntry *FoundEntry = TableSearch(Tbl, Key, &AddAt);

    if (FoundEntry == NULL)
    {   /* 添加到表中 */
        struct TableEntry *NewEntry = VariableAlloc(pc, NULL, sizeof(struct TableEntry), Tbl->OnHeap);
        NewEntry->DeclFileName = DeclFileName;
        NewEntry->DeclLine = DeclLine;
        NewEntry->DeclColumn = DeclColumn;
        NewEntry->p.v.Key = Key;
        NewEntry->p.v.Val = Val;
        NewEntry->Next = Tbl->HashTable[AddAt];
        Tbl->HashTable[AddAt] = NewEntry;
        return TRUE;
    }

    return FALSE;
}

/* 在哈希表中查找值。找到返回 TRUE 并填充 *Val，
 * 可选返回声明位置(文件名、行号、列号) */
int TableGet(struct Table *Tbl, const char *Key, struct Value **Val, const char **DeclFileName, int *DeclLine, int *DeclColumn)
{
    int AddAt;
    struct TableEntry *FoundEntry = TableSearch(Tbl, Key, &AddAt);
    if (FoundEntry == NULL)
        return FALSE;

    *Val = FoundEntry->p.v.Val;

    if (DeclFileName != NULL)
    {
        *DeclFileName = FoundEntry->DeclFileName;
        *DeclLine = FoundEntry->DeclLine;
        *DeclColumn = FoundEntry->DeclColumn;
    }

    return TRUE;
}

/* 从哈希表中删除条目并释放内存，返回被删除条目的 Value */
struct Value *TableDelete(Picoc *pc, struct Table *Tbl, const char *Key)
{
    struct TableEntry **EntryPtr;
    int HashValue = ((unsigned long)Key) % Tbl->Size;   /* 共享字符串有唯一地址，直接用地址取模 */

    for (EntryPtr = &Tbl->HashTable[HashValue]; *EntryPtr != NULL; EntryPtr = &(*EntryPtr)->Next)
    {
        if ((*EntryPtr)->p.v.Key == Key)
        {
            struct TableEntry *DeleteEntry = *EntryPtr;
            struct Value *Val = DeleteEntry->p.v.Val;
            *EntryPtr = DeleteEntry->Next;
            HeapFreeMem(pc, DeleteEntry);

            return Val;
        }
    }

    return NULL;
}

/* 在字符串表中搜索标识符。使用字符串内容(非指针)进行哈希和比较 */
static struct TableEntry *TableSearchIdentifier(struct Table *Tbl, const char *Key, int Len, int *AddAt)
{
    struct TableEntry *Entry;
    int HashValue = TableHash(Key, Len) % Tbl->Size;

    for (Entry = Tbl->HashTable[HashValue]; Entry != NULL; Entry = Entry->Next)
    {
        if (strncmp(&Entry->p.Key[0], (char *)Key, Len) == 0 && Entry->p.Key[Len] == '\0')
            return Entry;   /* 找到了 */
    }

    *AddAt = HashValue;    /* 在链上未找到 */
    return NULL;
}

/* 在字符串表中设置标识符。已存在返回已有指针，不存在则紧凑分配新条目。
 * 紧凑分配: 只分配 Key 字符串所需空间而非完整 TableEntry，节省内存 */
char *TableSetIdentifier(Picoc *pc, struct Table *Tbl, const char *Ident, int IdentLen)
{
    int AddAt;
    struct TableEntry *FoundEntry = TableSearchIdentifier(Tbl, Ident, IdentLen, &AddAt);

    if (FoundEntry != NULL)
        return &FoundEntry->p.Key[0];
    else
    {   /* 添加到表 - 紧凑分配以节省内存 */
        struct TableEntry *NewEntry = HeapAllocMem(pc, sizeof(struct TableEntry) - sizeof(union TableEntryPayload) + IdentLen + 1);
        if (NewEntry == NULL)
            ProgramFailNoParser(pc, "out of memory");

        strncpy((char *)&NewEntry->p.Key[0], (char *)Ident, IdentLen);
        NewEntry->p.Key[IdentLen] = '\0';
        NewEntry->Next = Tbl->HashTable[AddAt];
        Tbl->HashTable[AddAt] = NewEntry;
        return &NewEntry->p.Key[0];
    }
}

/* 在共享字符串表中注册指定长度的字符串，返回唯一指针 */
char *TableStrRegister2(Picoc *pc, const char *Str, int Len)
{
    return TableSetIdentifier(pc, &pc->StringTable, Str, Len);
}

/* 在共享字符串表中注册 C 字符串，返回唯一指针 */
char *TableStrRegister(Picoc *pc, const char *Str)
{
    return TableStrRegister2(pc, Str, strlen((char *)Str));
}

/* 释放共享字符串表中的所有字符串条目 */
void TableStrFree(Picoc *pc)
{
    struct TableEntry *Entry;
    struct TableEntry *NextEntry;
    int Count;

    for (Count = 0; Count < pc->StringTable.Size; Count++)
    {
        for (Entry = pc->StringTable.HashTable[Count]; Entry != NULL; Entry = NextEntry)
        {
            NextEntry = Entry->Next;
            HeapFreeMem(pc, Entry);
        }
    }
}
