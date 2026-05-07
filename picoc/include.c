/* picoc include system - can emulate system includes from built-in libraries
 * or it can include and parse files if the system has files */

/* PicoC 文件包含系统 — 支持 #include 指令; 在有文件系统的平台上可读取实际文件,
 * 嵌入式平台可通过 IncludeRegister 注册内置库 (源码字符串形式) 模拟系统头文件;
 * NO_HASH_INCLUDE 宏可完全禁用此功能 */

#include "picoc.h"
#include "interpreter.h"

#ifndef NO_HASH_INCLUDE


/* 初始化内建头文件库: 注册 ctype.h/math.h/stdio.h/stdlib.h/string.h 等标准库 */
void IncludeInit(Picoc *pc)
{
    IncludeRegister(pc, "ctype.h", NULL, &StdCtypeFunctions[0], NULL);
#ifndef BUILTIN_MINI_STDLIB
    IncludeRegister(pc, "errno.h", &StdErrnoSetupFunc, NULL, NULL);
#endif
# ifndef NO_FP
    IncludeRegister(pc, "math.h", &MathSetupFunc, &MathFunctions[0], NULL);
# endif
    IncludeRegister(pc, "stdbool.h", &StdboolSetupFunc, NULL, StdboolDefs);
#ifndef BUILTIN_MINI_STDLIB
    IncludeRegister(pc, "stdio.h", &StdioSetupFunc, &StdioFunctions[0], StdioDefs);
#endif
    IncludeRegister(pc, "stdlib.h", &StdlibSetupFunc, &StdlibFunctions[0], NULL);
    IncludeRegister(pc, "string.h", &StringSetupFunc, &StringFunctions[0], NULL);
#ifndef BUILTIN_MINI_STDLIB
    IncludeRegister(pc, "time.h", &StdTimeSetupFunc, &StdTimeFunctions[0], StdTimeDefs);
# ifndef WIN32
    IncludeRegister(pc, "unistd.h", &UnistdSetupFunc, &UnistdFunctions[0], UnistdDefs);
# endif
#endif
}

/* 清理包含系统: 释放所有注册的内建头文件链表 */
void IncludeCleanup(Picoc *pc)
{
    struct IncludeLibrary *ThisInclude = pc->IncludeLibList;
    struct IncludeLibrary *NextInclude;

    while (ThisInclude != NULL)
    {
        NextInclude = ThisInclude->NextLib;
        HeapFreeMem(pc, ThisInclude);
        ThisInclude = NextInclude;
    }

    pc->IncludeLibList = NULL;
}

/* 注册内建头文件。参数:
 * - IncludeName: 头文件名(如 "stdio.h")
 * - SetupFunction: 可选的初始化回调函数
 * - FuncList: 库函数列表(原型+函数指针)
 * - SetupCSource: 可选的 C 源码字符串(定义类型/宏等) */
void IncludeRegister(Picoc *pc, const char *IncludeName, void (*SetupFunction)(Picoc *pc), struct LibraryFunction *FuncList, const char *SetupCSource)
{
    struct IncludeLibrary *NewLib = HeapAllocMem(pc, sizeof(struct IncludeLibrary));
    NewLib->IncludeName = TableStrRegister(pc, IncludeName);
    NewLib->SetupFunction = SetupFunction;
    NewLib->FuncList = FuncList;
    NewLib->SetupCSource = SetupCSource;
    NewLib->NextLib = pc->IncludeLibList;
    pc->IncludeLibList = NewLib;
}

/* 一次性包含所有已注册的系统头文件(初始化时调用) */
void PicocIncludeAllSystemHeaders(Picoc *pc)
{
    struct IncludeLibrary *ThisInclude = pc->IncludeLibList;

    for (; ThisInclude != NULL; ThisInclude = ThisInclude->NextLib)
        IncludeFile(pc, ThisInclude->IncludeName);
}

/* 包含文件的核心函数。先在已注册的内建头文件列表中查找;
 * 找到则执行初始化回调、解析 C 源码、注册库函数，并通过 VariableDefined 防重复包含;
 * 找不到则从文件系统读取实际文件 */
void IncludeFile(Picoc *pc, char *FileName)
{
    struct IncludeLibrary *LInclude;

    /* 在内建头文件列表中查找 */
    for (LInclude = pc->IncludeLibList; LInclude != NULL; LInclude = LInclude->NextLib)
    {
        if (strcmp(LInclude->IncludeName, FileName) == 0)
        {
            /* 找到了 - 防止重复包含 */
            if (!VariableDefined(pc, FileName))
            {
                VariableDefine(pc, NULL, FileName, NULL, &pc->VoidType, FALSE);

                /* 执行初始化回调(如果有) */
                if (LInclude->SetupFunction != NULL)
                    (*LInclude->SetupFunction)(pc);

                /* 解析初始化 C 源码(可定义类型和宏) */
                if (LInclude->SetupCSource != NULL)
                    PicocParse(pc, FileName, LInclude->SetupCSource, strlen(LInclude->SetupCSource), TRUE, TRUE, FALSE, FALSE);

                /* 注册库函数 */
                if (LInclude->FuncList != NULL)
                    LibraryAdd(pc, &pc->GlobalTable, FileName, LInclude->FuncList);
            }

            return;
        }
    }

    /* 不是内建文件，从文件系统读取实际文件 */
    PicocPlatformScanFile(pc, FileName);
}

#endif /* NO_HASH_INCLUDE */
