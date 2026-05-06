#include "../interpreter.h"

static void Stm32SetupFunc(Picoc *pc)
{
    (void)pc;
}

static struct LibraryFunction Stm32Functions[] =
{
    { NULL, NULL }
};

void PlatformLibraryInit(Picoc *pc)
{
    IncludeRegister(pc, "stdio.h", NULL, NULL, NULL);
    IncludeRegister(pc, "picoc_stm32.h", &Stm32SetupFunc, &Stm32Functions[0], NULL);
}
