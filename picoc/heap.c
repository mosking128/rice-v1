/* picoc heap memory allocation. This is a complete (but small) memory
 * allocator for embedded systems which have no memory allocator. Alternatively
 * you can define USE_MALLOC_HEAP to use your system's own malloc() allocator */

/* PicoC 内存分配器 — 在单一内存池 HeapMemory[] 中同时管理"栈"(向上增长，存放临时值和调用帧)
 * 和"堆"(向下增长，存放持久变量); 嵌入式平台使用 HEAP_SIZE 静态数组作为内存池;
 * UNIX/WIN32 平台可选 USE_MALLOC_STACK/USE_MALLOC_HEAP 使用系统 malloc */

/* stack grows up from the bottom and heap grows down from the top of heap space */
#include "interpreter.h"

#ifdef DEBUG_HEAP
/* 调试: 打印大块空闲链表内容 */
void ShowBigList(Picoc *pc)
{
    struct AllocNode *LPos;

    printf("Heap: bottom=0x%lx 0x%lx-0x%lx, big freelist=", (long)pc->HeapBottom, (long)&(pc->HeapMemory)[0], (long)&(pc->HeapMemory)[HEAP_SIZE]);
    for (LPos = pc->FreeListBig; LPos != NULL; LPos = LPos->NextFree)
        printf("0x%lx:%d ", (long)LPos, LPos->Size);

    printf("\n");
}
#endif

/* 初始化栈和堆: 设置 HeapBottom/StackFrame/HeapStackTop 边界，对齐内存，清空空闲链表 */
void HeapInit(Picoc *pc, int StackOrHeapSize)
{
    int Count;
    int AlignOffset = 0;

#ifdef USE_MALLOC_STACK
    pc->HeapMemory = malloc(StackOrHeapSize);
    pc->HeapBottom = NULL;                     /* 堆底(向下增长) */
    pc->StackFrame = NULL;                     /* 当前栈帧 */
    pc->HeapStackTop = NULL;                          /* 栈顶 */
#else
# ifdef SURVEYOR_HOST
    pc->HeapMemory = (unsigned char *)C_HEAPSTART;      /* 全部内存(栈+堆) */
    pc->HeapBottom = (void *)C_HEAPSTART + HEAP_SIZE;  /* 堆底(向下增长) */
    pc->StackFrame = (void *)C_HEAPSTART;              /* 当前栈帧 */
    pc->HeapStackTop = (void *)C_HEAPSTART;                   /* 栈顶 */
    pc->HeapMemStart = (void *)C_HEAPSTART;
# else
    pc->HeapBottom = &pc->HeapMemory[HEAP_SIZE];   /* 堆底(向下增长) */
    pc->StackFrame = &pc->HeapMemory[0];           /* 当前栈帧 */
    pc->HeapStackTop = &pc->HeapMemory[0];         /* 栈顶 */
# endif
#endif

    while (((unsigned long)&pc->HeapMemory[AlignOffset] & (sizeof(ALIGN_TYPE)-1)) != 0)
        AlignOffset++;

    pc->StackFrame = &(pc->HeapMemory)[AlignOffset];
    pc->HeapStackTop = &(pc->HeapMemory)[AlignOffset];
    *(void **)(pc->StackFrame) = NULL;
    pc->HeapBottom = &(pc->HeapMemory)[StackOrHeapSize-sizeof(ALIGN_TYPE)+AlignOffset];
    pc->FreeListBig = NULL;
    for (Count = 0; Count < FREELIST_BUCKETS; Count++)
        pc->FreeListBucket[Count] = NULL;
}

/* 清理堆内存(仅 USE_MALLOC_STACK 模式下释放 malloc 分配的内存池) */
void HeapCleanup(Picoc *pc)
{
#ifdef USE_MALLOC_STACK
    free(pc->HeapMemory);
#endif
}

/* 在栈上分配 Size 字节(已对齐)，清零并返回指针。栈空间不足返回 NULL */
void *HeapAllocStack(Picoc *pc, int Size)
{
    char *NewMem = pc->HeapStackTop;
    char *NewTop = (char *)pc->HeapStackTop + MEM_ALIGN(Size);
#ifdef DEBUG_HEAP
    printf("HeapAllocStack(%ld) at 0x%lx\n", (unsigned long)MEM_ALIGN(Size), (unsigned long)pc->HeapStackTop);
#endif
    if (NewTop > (char *)pc->HeapBottom)
        return NULL;

    pc->HeapStackTop = (void *)NewTop;
    memset((void *)NewMem, '\0', Size);
    return NewMem;
}

/* 撤销栈弹出: 将栈顶指针向上移，恢复之前弹出的空间 */
void HeapUnpopStack(Picoc *pc, int Size)
{
#ifdef DEBUG_HEAP
    printf("HeapUnpopStack(%ld) at 0x%lx\n", (unsigned long)MEM_ALIGN(Size), (unsigned long)pc->HeapStackTop);
#endif
    pc->HeapStackTop = (void *)((char *)pc->HeapStackTop + MEM_ALIGN(Size));
}

/* 从栈顶释放 Size 字节。Addr 非 NULL 时验证栈顶指针是否匹配。返回 TRUE 表示成功 */
int HeapPopStack(Picoc *pc, void *Addr, int Size)
{
    int ToLose = MEM_ALIGN(Size);
    if (ToLose > ((char *)pc->HeapStackTop - (char *)&(pc->HeapMemory)[0]))
        return FALSE;

#ifdef DEBUG_HEAP
    printf("HeapPopStack(0x%lx, %ld) back to 0x%lx\n", (unsigned long)Addr, (unsigned long)MEM_ALIGN(Size), (unsigned long)pc->HeapStackTop - ToLose);
#endif
    pc->HeapStackTop = (void *)((char *)pc->HeapStackTop - ToLose);
    assert(Addr == NULL || pc->HeapStackTop == Addr);

    return TRUE;
}

/* 压入新栈帧: 保存当前 StackFrame 指针到栈顶，并将 StackFrame 指向新帧位置 */
void HeapPushStackFrame(Picoc *pc)
{
#ifdef DEBUG_HEAP
    printf("Adding stack frame at 0x%lx\n", (unsigned long)pc->HeapStackTop);
#endif
    *(void **)pc->HeapStackTop = pc->StackFrame;
    pc->StackFrame = pc->HeapStackTop;
    pc->HeapStackTop = (void *)((char *)pc->HeapStackTop + MEM_ALIGN(sizeof(ALIGN_TYPE)));
}

/* 弹出当前栈帧: 释放帧内所有内存，恢复上一帧的 StackFrame。无更多帧时返回 FALSE */
int HeapPopStackFrame(Picoc *pc)
{
    if (*(void **)pc->StackFrame != NULL)
    {
        pc->HeapStackTop = pc->StackFrame;
        pc->StackFrame = *(void **)pc->StackFrame;
#ifdef DEBUG_HEAP
        printf("Popping stack frame back to 0x%lx\n", (unsigned long)pc->HeapStackTop);
#endif
        return TRUE;
    }
    else
        return FALSE;
}

/* 动态分配堆内存(向下增长方向)。分配策略:
 * 1) 优先从小块桶空闲链表(FREELIST_BUCKETS)分配
 * 2) 其次从大块空闲链表查找合适的块，大小接近则不分割以减少碎片
 * 3) 最后向下扩展堆边界 HeapBottom
 * 返回清零后的内存，内存不足返回 NULL */
void *HeapAllocMem(Picoc *pc, int Size)
{
#ifdef USE_MALLOC_HEAP
    return calloc(Size, 1);
#else
    struct AllocNode *NewMem = NULL;
    struct AllocNode **FreeNode;
    int AllocSize = MEM_ALIGN(Size) + MEM_ALIGN(sizeof(NewMem->Size));
    int Bucket;
    void *ReturnMem;

    if (Size == 0)
        return NULL;

    assert(Size > 0);

    /* 确保能容纳 AllocNode 头部 */
    if (AllocSize < sizeof(struct AllocNode))
        AllocSize = sizeof(struct AllocNode);

    Bucket = AllocSize >> 2;
    if (Bucket < FREELIST_BUCKETS && pc->FreeListBucket[Bucket] != NULL)
    {
        /* 从小块桶空闲链表中分配 */
#ifdef DEBUG_HEAP
        printf("allocating %d(%d) from bucket", Size, AllocSize);
#endif
        NewMem = pc->FreeListBucket[Bucket];
        assert((unsigned long)NewMem >= (unsigned long)&(pc->HeapMemory)[0] && (unsigned char *)NewMem - &(pc->HeapMemory)[0] < HEAP_SIZE);
        pc->FreeListBucket[Bucket] = *(struct AllocNode **)NewMem;
        assert(pc->FreeListBucket[Bucket] == NULL || ((unsigned long)pc->FreeListBucket[Bucket] >= (unsigned long)&(pc->HeapMemory)[0] && (unsigned char *)pc->FreeListBucket[Bucket] - &(pc->HeapMemory)[0] < HEAP_SIZE));
        NewMem->Size = AllocSize;
    }
    else if (pc->FreeListBig != NULL)
    {
        /* 从小块桶分配失败，从大块空闲链表找合适的块 */
        for (FreeNode = &pc->FreeListBig; *FreeNode != NULL && (*FreeNode)->Size < AllocSize; FreeNode = &(*FreeNode)->NextFree)
        {}

        if (*FreeNode != NULL)
        {
            assert((unsigned long)*FreeNode >= (unsigned long)&(pc->HeapMemory)[0] && (unsigned char *)*FreeNode - &(pc->HeapMemory)[0] < HEAP_SIZE);
            assert((*FreeNode)->Size < HEAP_SIZE && (*FreeNode)->Size > 0);
            if ((*FreeNode)->Size < AllocSize + SPLIT_MEM_THRESHOLD)
            {
                /* 大小接近，不分割以减少碎片 */
#ifdef DEBUG_HEAP
               printf("allocating %d(%d) from freelist, no split (%d)", Size, AllocSize, (*FreeNode)->Size);
#endif
                NewMem = *FreeNode;
                assert((unsigned long)NewMem >= (unsigned long)&(pc->HeapMemory)[0] && (unsigned char *)NewMem - &(pc->HeapMemory)[0] < HEAP_SIZE);
                *FreeNode = NewMem->NextFree;
            }
            else
            {
                /* 块远大于需求，分割 */
#ifdef DEBUG_HEAP
                printf("allocating %d(%d) from freelist, split chunk (%d)", Size, AllocSize, (*FreeNode)->Size);
#endif
                NewMem = (void *)((char *)*FreeNode + (*FreeNode)->Size - AllocSize);
                assert((unsigned long)NewMem >= (unsigned long)&(pc->HeapMemory)[0] && (unsigned char *)NewMem - &(pc->HeapMemory)[0] < HEAP_SIZE);
                (*FreeNode)->Size -= AllocSize;
                NewMem->Size = AllocSize;
            }
        }
    }

    if (NewMem == NULL)
    {
        /* 空闲链表分配失败，向下扩展堆边界 */
#ifdef DEBUG_HEAP
        printf("allocating %d(%d) at bottom of heap (0x%lx-0x%lx)", Size, AllocSize, (long)((char *)pc->HeapBottom - AllocSize), (long)pc->HeapBottom);
#endif
        if ((char *)pc->HeapBottom - AllocSize < (char *)pc->HeapStackTop)
            return NULL;

        pc->HeapBottom = (void *)((char *)pc->HeapBottom - AllocSize);
        NewMem = pc->HeapBottom;
        NewMem->Size = AllocSize;
    }

    ReturnMem = (void *)((char *)NewMem + MEM_ALIGN(sizeof(NewMem->Size)));
    memset(ReturnMem, '\0', AllocSize - MEM_ALIGN(sizeof(NewMem->Size)));
#ifdef DEBUG_HEAP
    printf(" = %lx\n", (unsigned long)ReturnMem);
#endif
    return ReturnMem;
#endif
}

/* 释放动态分配的内存。根据块位置和大小采用不同回收策略:
 * - 位于堆底: 收缩堆边界
 * - 小块: 放入对应桶链表
 * - 大块: 放入大块空闲链表 */
void HeapFreeMem(Picoc *pc, void *Mem)
{
#ifdef USE_MALLOC_HEAP
    free(Mem);
#else
    struct AllocNode *MemNode = (struct AllocNode *)((char *)Mem - MEM_ALIGN(sizeof(MemNode->Size)));
    int Bucket = MemNode->Size >> 2;

#ifdef DEBUG_HEAP
    printf("HeapFreeMem(0x%lx)\n", (unsigned long)Mem);
#endif
    assert((unsigned long)Mem >= (unsigned long)&(pc->HeapMemory)[0] && (unsigned char *)Mem - &(pc->HeapMemory)[0] < HEAP_SIZE);
    assert(MemNode->Size < HEAP_SIZE && MemNode->Size > 0);
    if (Mem == NULL)
        return;

    if ((void *)MemNode == pc->HeapBottom)
    {
        /* 位于堆底，直接收缩堆边界 */
#ifdef DEBUG_HEAP
        printf("freeing %d from bottom of heap\n", MemNode->Size);
#endif
        pc->HeapBottom = (void *)((char *)pc->HeapBottom + MemNode->Size);
#ifdef DEBUG_HEAP
        ShowBigList(pc);
#endif
    }
    else if (Bucket < FREELIST_BUCKETS)
    {
        /* 小块放入桶链表 */
#ifdef DEBUG_HEAP
        printf("freeing %d to bucket\n", MemNode->Size);
#endif
        assert(pc->FreeListBucket[Bucket] == NULL || ((unsigned long)pc->FreeListBucket[Bucket] >= (unsigned long)&(pc->HeapMemory)[0] && (unsigned char *)pc->FreeListBucket[Bucket] - &pc->HeapMemory[0] < HEAP_SIZE));
        *(struct AllocNode **)MemNode = pc->FreeListBucket[Bucket];
        pc->FreeListBucket[Bucket] = (struct AllocNode *)MemNode;
    }
    else
    {
        /* 大块放入大块空闲链表 */
#ifdef DEBUG_HEAP
        printf("freeing %lx:%d to freelist\n", (unsigned long)Mem, MemNode->Size);
#endif
        assert(pc->FreeListBig == NULL || ((unsigned long)pc->FreeListBig >= (unsigned long)&(pc->HeapMemory)[0] && (unsigned char *)pc->FreeListBig - &(pc->HeapMemory)[0] < HEAP_SIZE));
        MemNode->NextFree = pc->FreeListBig;
        pc->FreeListBig = MemNode;
#ifdef DEBUG_HEAP
        ShowBigList(pc);
#endif
    }
#endif
}
