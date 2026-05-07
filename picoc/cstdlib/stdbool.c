/* ============================================================================
 * stdbool.c - PicoC 布尔类型支持 (stdbool.h)
 *
 * 为 PicoC 解释器提供 C99 标准 <stdbool.h> 头文件中定义的布尔类型
 * 和相关宏。将 bool 实现为 int 的 typedef 别名。
 *
 * 定义内容:
 *   - bool:                        布尔类型 (typedef int bool)
 *   - true:                        布尔真值 (整数 1)
 *   - false:                       布尔假值 (整数 0)
 *   - __bool_true_false_are_defined: 兼容性检查宏 (值为 1)
 *     用于在用户代码中检测 <stdbool.h> 是否已被包含:
 *     #ifndef __bool_true_false_are_defined
 *     #include <stdbool.h>
 *     #endif
 * ============================================================================ */

#include "../interpreter.h"

static int trueValue = 1;
static int falseValue = 0;


/* bool 类型定义: 将 bool 定义为 int 的别名 */
const char StdboolDefs[] = "typedef int bool;";

/* StdboolSetupFunc: 注册 bool 类型和 true/false 常量 */
void StdboolSetupFunc(Picoc *pc)
{
    /* 注册布尔值宏 */
    VariableDefinePlatformVar(pc, NULL, "true", &pc->IntType, (union AnyValue *)&trueValue, FALSE);
    VariableDefinePlatformVar(pc, NULL, "false", &pc->IntType, (union AnyValue *)&falseValue, FALSE);
    VariableDefinePlatformVar(pc, NULL, "__bool_true_false_are_defined", &pc->IntType, (union AnyValue *)&trueValue, FALSE);
}
