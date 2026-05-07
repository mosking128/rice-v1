/* STM32H7 硬件库 — 暴露给 PicoC 脚本的 C 函数注册表
 *
 * 通过 PlatformLibraryInit() 将 STM32H7 平台相关的 C 函数注册到
 * PicoC 解释器中。PicoC 脚本可通过 #include <picoc_stm32.h> 引入
 * 这些函数并在脚本中直接调用。
 *
 * 当前函数表为空占位，可在此添加硬件控制函数（GPIO、定时器、ADC 等），
 * 按照 { "函数名", 函数指针 } 格式填入 Stm32Functions 数组即可。
 */

#include "../interpreter.h"

/* STM32 库设置函数（PicoC 首次引用 picoc_stm32.h 时调用） */
static void Stm32SetupFunc(Picoc *pc)
{
    (void)pc;
}

/* STM32 硬件函数注册表
 * 格式：{ "C函数名", 函数指针 }
 * 末尾以 { NULL, NULL } 哨兵结束
 */
static struct LibraryFunction Stm32Functions[] =
{
    { NULL, NULL }
};

/* 初始化平台库：注册头文件和函数表 */
void PlatformLibraryInit(Picoc *pc)
{
    IncludeRegister(pc, "stdio.h", NULL, NULL, NULL);
    IncludeRegister(pc, "picoc_stm32.h", &Stm32SetupFunc, &Stm32Functions[0], NULL);
}
