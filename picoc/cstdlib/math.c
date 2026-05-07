/* ============================================================================
 * math.c - PicoC 数学库 (math.h)
 *
 * 为 PicoC 解释器提供 C 标准 <math.h> 头文件中定义的浮点数学函数
 * 和数学常量。仅在未定义 NO_FP (启用浮点支持) 时编译。
 *
 * 功能分组:
 *   - 三角函数:     sin / cos / tan / asin / acos / atan / atan2
 *   - 双曲函数:     sinh / cosh / tanh
 *   - 指数和对数:   exp / log / log10 / pow / sqrt
 *   - 取整和取模:   ceil / floor / round / fabs / fmod / modf
 *   - 浮点拆分:     frexp / ldexp
 *   - 数学常量:     M_E / M_PI / M_SQRT2 等
 *
 * 注意: round() 内部使用 ceil(x - 0.5) 实现，对负数可能有偏差。
 *       依赖平台 math.h (需要硬件 FPU 或软件浮点支持)。
 * ============================================================================ */

#include "../interpreter.h"

#ifndef NO_FP

/* 数学常量 (高精度 double) */
static double M_EValue =        2.7182818284590452354;   /* e - 自然对数的底 */
static double M_LOG2EValue =    1.4426950408889634074;   /* log_2 e */
static double M_LOG10EValue =   0.43429448190325182765;  /* log_10 e */
static double M_LN2Value =      0.69314718055994530942;  /* log_e 2 */
static double M_LN10Value =     2.30258509299404568402;  /* log_e 10 */
static double M_PIValue =       3.14159265358979323846;  /* pi - 圆周率 */
static double M_PI_2Value =     1.57079632679489661923;  /* pi/2 */
static double M_PI_4Value =     0.78539816339744830962;  /* pi/4 */
static double M_1_PIValue =     0.31830988618379067154;  /* 1/pi */
static double M_2_PIValue =     0.63661977236758134308;  /* 2/pi */
static double M_2_SQRTPIValue = 1.12837916709551257390;  /* 2/sqrt(pi) */
static double M_SQRT2Value =    1.41421356237309504880;  /* sqrt(2) */
static double M_SQRT1_2Value =  0.70710678118654752440;  /* 1/sqrt(2) */

/* =========================================================================
 * 三角函数 (参数和返回值均为弧度)
 * ========================================================================= */

/* sin: 正弦函数 */
void MathSin(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->FP = sin(Param[0]->Val->FP);
}

/* cos: 余弦函数 */
void MathCos(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->FP = cos(Param[0]->Val->FP);
}

/* tan: 正切函数 */
void MathTan(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->FP = tan(Param[0]->Val->FP);
}

/* asin: 反正弦函数，返回值范围 [-pi/2, pi/2] */
void MathAsin(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->FP = asin(Param[0]->Val->FP);
}

/* acos: 反余弦函数，返回值范围 [0, pi] */
void MathAcos(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->FP = acos(Param[0]->Val->FP);
}

/* atan: 反正切函数，返回值范围 [-pi/2, pi/2] */
void MathAtan(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->FP = atan(Param[0]->Val->FP);
}

/* atan2: 二参数反正切 atan2(y, x)，根据符号确定正确象限 */
void MathAtan2(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->FP = atan2(Param[0]->Val->FP, Param[1]->Val->FP);
}

/* =========================================================================
 * 双曲函数
 * ========================================================================= */

/* sinh: 双曲正弦 sinh(x) = (e^x - e^(-x)) / 2 */
void MathSinh(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->FP = sinh(Param[0]->Val->FP);
}

/* cosh: 双曲余弦 cosh(x) = (e^x + e^(-x)) / 2 */
void MathCosh(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->FP = cosh(Param[0]->Val->FP);
}

/* tanh: 双曲正切 tanh(x) = sinh(x)/cosh(x) */
void MathTanh(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->FP = tanh(Param[0]->Val->FP);
}

/* =========================================================================
 * 指数和对数函数
 * ========================================================================= */

/* exp: 计算 e 的 x 次幂 e^x */
void MathExp(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->FP = exp(Param[0]->Val->FP);
}

/* log: 自然对数 ln(x) = log_e(x), x > 0 */
void MathLog(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->FP = log(Param[0]->Val->FP);
}

/* log10: 以 10 为底的对数 log_10(x), x > 0 */
void MathLog10(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->FP = log10(Param[0]->Val->FP);
}

/* pow: 计算 x 的 y 次幂 x^y */
void MathPow(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->FP = pow(Param[0]->Val->FP, Param[1]->Val->FP);
}

/* sqrt: 平方根 sqrt(x), x >= 0 */
void MathSqrt(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->FP = sqrt(Param[0]->Val->FP);
}

/* =========================================================================
 * 取整和取模函数
 * ========================================================================= */

/* fabs: 浮点数的绝对值 */
void MathFabs(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->FP = fabs(Param[0]->Val->FP);
}

/* fmod: 浮点数取模 (x 除以 y 的余数) */
void MathFmod(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->FP = fmod(Param[0]->Val->FP, Param[1]->Val->FP);
}

/* frexp: 将浮点数拆分为尾数 [0.5,1.0) 和指数 (2 的幂) */
void MathFrexp(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->FP = frexp(Param[0]->Val->FP, Param[1]->Val->Pointer);
}

/* ldexp: 计算 x * 2^exp (frexp 的逆操作) */
void MathLdexp(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->FP = ldexp(Param[0]->Val->FP, Param[1]->Val->Integer);
}

/* modf: 将浮点数拆分为整数部分和小数部分，返回小数部分 */
void MathModf(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->FP = modf(Param[0]->Val->FP, Param[0]->Val->Pointer);
}

/* round: 四舍五入取整 (通过 ceil(x-0.5) 实现) */
void MathRound(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    /* round() 的定义因 math.h 声明不一致而采用此简化实现 */
    ReturnValue->Val->FP = ceil(Param[0]->Val->FP - 0.5);
}

/* ceil: 向上取整，返回不小于 x 的最小整数 */
void MathCeil(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->FP = ceil(Param[0]->Val->FP);
}

/* floor: 向下取整，返回不大于 x 的最大整数 */
void MathFloor(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->FP = floor(Param[0]->Val->FP);
}

/* math 函数注册表: 将函数指针映射到 C 原型声明 */
struct LibraryFunction MathFunctions[] =
{
    { MathAcos,         "float acos(float);" },
    { MathAsin,         "float asin(float);" },
    { MathAtan,         "float atan(float);" },
    { MathAtan2,        "float atan2(float, float);" },
    { MathCeil,         "float ceil(float);" },
    { MathCos,          "float cos(float);" },
    { MathCosh,         "float cosh(float);" },
    { MathExp,          "float exp(float);" },
    { MathFabs,         "float fabs(float);" },
    { MathFloor,        "float floor(float);" },
    { MathFmod,         "float fmod(float, float);" },
    { MathFrexp,        "float frexp(float, int *);" },
    { MathLdexp,        "float ldexp(float, int);" },
    { MathLog,          "float log(float);" },
    { MathLog10,        "float log10(float);" },
    { MathModf,         "float modf(float, float *);" },
    { MathPow,          "float pow(float,float);" },
    { MathRound,        "float round(float);" },
    { MathSin,          "float sin(float);" },
    { MathSinh,         "float sinh(float);" },
    { MathSqrt,         "float sqrt(float);" },
    { MathTan,          "float tan(float);" },
    { MathTanh,         "float tanh(float);" },
    { NULL,             NULL }
};

/* MathSetupFunc: 初始化 math 库，注册所有数学常量 */
void MathSetupFunc(Picoc *pc)
{
    VariableDefinePlatformVar(pc, NULL, "M_E", &pc->FPType, (union AnyValue *)&M_EValue, FALSE);
    VariableDefinePlatformVar(pc, NULL, "M_LOG2E", &pc->FPType, (union AnyValue *)&M_LOG2EValue, FALSE);
    VariableDefinePlatformVar(pc, NULL, "M_LOG10E", &pc->FPType, (union AnyValue *)&M_LOG10EValue, FALSE);
    VariableDefinePlatformVar(pc, NULL, "M_LN2", &pc->FPType, (union AnyValue *)&M_LN2Value, FALSE);
    VariableDefinePlatformVar(pc, NULL, "M_LN10", &pc->FPType, (union AnyValue *)&M_LN10Value, FALSE);
    VariableDefinePlatformVar(pc, NULL, "M_PI", &pc->FPType, (union AnyValue *)&M_PIValue, FALSE);
    VariableDefinePlatformVar(pc, NULL, "M_PI_2", &pc->FPType, (union AnyValue *)&M_PI_2Value, FALSE);
    VariableDefinePlatformVar(pc, NULL, "M_PI_4", &pc->FPType, (union AnyValue *)&M_PI_4Value, FALSE);
    VariableDefinePlatformVar(pc, NULL, "M_1_PI", &pc->FPType, (union AnyValue *)&M_1_PIValue, FALSE);
    VariableDefinePlatformVar(pc, NULL, "M_2_PI", &pc->FPType, (union AnyValue *)&M_2_PIValue, FALSE);
    VariableDefinePlatformVar(pc, NULL, "M_2_SQRTPI", &pc->FPType, (union AnyValue *)&M_2_SQRTPIValue, FALSE);
    VariableDefinePlatformVar(pc, NULL, "M_SQRT2", &pc->FPType, (union AnyValue *)&M_SQRT2Value, FALSE);
    VariableDefinePlatformVar(pc, NULL, "M_SQRT1_2", &pc->FPType, (union AnyValue *)&M_SQRT1_2Value, FALSE);
}

#endif /* !NO_FP */
