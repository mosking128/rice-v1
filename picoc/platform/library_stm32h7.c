#include "../interpreter.h"
#include "stm32h7xx_hal.h"

static void *Stm32GpioAValue = GPIOA;
static void *Stm32GpioBValue = GPIOB;
static void *Stm32GpioCValue = GPIOC;
static void *Stm32GpioDValue = GPIOD;
static void *Stm32GpioEValue = GPIOE;
static void *Stm32GpioFValue = GPIOF;
static void *Stm32GpioGValue = GPIOG;
static void *Stm32GpioHValue = GPIOH;
static void *Stm32GpioIValue = GPIOI;
static void *Stm32GpioJValue = GPIOJ;
static void *Stm32GpioKValue = GPIOK;

static int Stm32GpioPin0Value = GPIO_PIN_0;
static int Stm32GpioPin1Value = GPIO_PIN_1;
static int Stm32GpioPin2Value = GPIO_PIN_2;
static int Stm32GpioPin3Value = GPIO_PIN_3;
static int Stm32GpioPin4Value = GPIO_PIN_4;
static int Stm32GpioPin5Value = GPIO_PIN_5;
static int Stm32GpioPin6Value = GPIO_PIN_6;
static int Stm32GpioPin7Value = GPIO_PIN_7;
static int Stm32GpioPin8Value = GPIO_PIN_8;
static int Stm32GpioPin9Value = GPIO_PIN_9;
static int Stm32GpioPin10Value = GPIO_PIN_10;
static int Stm32GpioPin11Value = GPIO_PIN_11;
static int Stm32GpioPin12Value = GPIO_PIN_12;
static int Stm32GpioPin13Value = GPIO_PIN_13;
static int Stm32GpioPin14Value = GPIO_PIN_14;
static int Stm32GpioPin15Value = GPIO_PIN_15;
static int Stm32GpioPinAllValue = GPIO_PIN_All;

static int Stm32GpioPinResetValue = GPIO_PIN_RESET;
static int Stm32GpioPinSetValue = GPIO_PIN_SET;

static int Stm32GpioModeInputValue = GPIO_MODE_INPUT;
static int Stm32GpioModeOutputPpValue = GPIO_MODE_OUTPUT_PP;
static int Stm32GpioModeOutputOdValue = GPIO_MODE_OUTPUT_OD;
static int Stm32GpioModeAnalogValue = GPIO_MODE_ANALOG;

static int Stm32GpioNopullValue = GPIO_NOPULL;
static int Stm32GpioPullupValue = GPIO_PULLUP;
static int Stm32GpioPulldownValue = GPIO_PULLDOWN;

static int Stm32GpioSpeedLowValue = GPIO_SPEED_FREQ_LOW;
static int Stm32GpioSpeedMediumValue = GPIO_SPEED_FREQ_MEDIUM;
static int Stm32GpioSpeedHighValue = GPIO_SPEED_FREQ_HIGH;
static int Stm32GpioSpeedVeryHighValue = GPIO_SPEED_FREQ_VERY_HIGH;

static void PicocHalGpioInit(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    (void)Parser;
    (void)ReturnValue;
    (void)NumArgs;

    HAL_GPIO_Init((GPIO_TypeDef *)Param[0]->Val->Pointer,
                  (GPIO_InitTypeDef *)Param[1]->Val->Pointer);
}

static void PicocHalGpioDeInit(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    (void)Parser;
    (void)ReturnValue;
    (void)NumArgs;

    HAL_GPIO_DeInit((GPIO_TypeDef *)Param[0]->Val->Pointer,
                    (uint32_t)Param[1]->Val->Integer);
}

static void PicocHalGpioReadPin(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    (void)Parser;
    (void)NumArgs;

    ReturnValue->Val->Integer = HAL_GPIO_ReadPin((GPIO_TypeDef *)Param[0]->Val->Pointer,
                                                 (uint16_t)Param[1]->Val->Integer);
}

static void PicocHalGpioWritePin(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    (void)Parser;
    (void)ReturnValue;
    (void)NumArgs;

    HAL_GPIO_WritePin((GPIO_TypeDef *)Param[0]->Val->Pointer,
                      (uint16_t)Param[1]->Val->Integer,
                      (GPIO_PinState)Param[2]->Val->Integer);
}

static void PicocHalGpioTogglePin(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    (void)Parser;
    (void)ReturnValue;
    (void)NumArgs;

    HAL_GPIO_TogglePin((GPIO_TypeDef *)Param[0]->Val->Pointer,
                       (uint16_t)Param[1]->Val->Integer);
}

static void PicocHalDelay(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    (void)Parser;
    (void)ReturnValue;
    (void)NumArgs;

    HAL_Delay((uint32_t)Param[0]->Val->Integer);
}

static void Stm32SetupFunc(Picoc *pc)
{
    VariableDefinePlatformVar(pc, NULL, "GPIOA", pc->VoidPtrType, (union AnyValue *)&Stm32GpioAValue, FALSE);
    VariableDefinePlatformVar(pc, NULL, "GPIOB", pc->VoidPtrType, (union AnyValue *)&Stm32GpioBValue, FALSE);
    VariableDefinePlatformVar(pc, NULL, "GPIOC", pc->VoidPtrType, (union AnyValue *)&Stm32GpioCValue, FALSE);
    VariableDefinePlatformVar(pc, NULL, "GPIOD", pc->VoidPtrType, (union AnyValue *)&Stm32GpioDValue, FALSE);
    VariableDefinePlatformVar(pc, NULL, "GPIOE", pc->VoidPtrType, (union AnyValue *)&Stm32GpioEValue, FALSE);
    VariableDefinePlatformVar(pc, NULL, "GPIOF", pc->VoidPtrType, (union AnyValue *)&Stm32GpioFValue, FALSE);
    VariableDefinePlatformVar(pc, NULL, "GPIOG", pc->VoidPtrType, (union AnyValue *)&Stm32GpioGValue, FALSE);
    VariableDefinePlatformVar(pc, NULL, "GPIOH", pc->VoidPtrType, (union AnyValue *)&Stm32GpioHValue, FALSE);
    VariableDefinePlatformVar(pc, NULL, "GPIOI", pc->VoidPtrType, (union AnyValue *)&Stm32GpioIValue, FALSE);
    VariableDefinePlatformVar(pc, NULL, "GPIOJ", pc->VoidPtrType, (union AnyValue *)&Stm32GpioJValue, FALSE);
    VariableDefinePlatformVar(pc, NULL, "GPIOK", pc->VoidPtrType, (union AnyValue *)&Stm32GpioKValue, FALSE);

    VariableDefinePlatformVar(pc, NULL, "GPIO_PIN_0", &pc->IntType, (union AnyValue *)&Stm32GpioPin0Value, FALSE);
    VariableDefinePlatformVar(pc, NULL, "GPIO_PIN_1", &pc->IntType, (union AnyValue *)&Stm32GpioPin1Value, FALSE);
    VariableDefinePlatformVar(pc, NULL, "GPIO_PIN_2", &pc->IntType, (union AnyValue *)&Stm32GpioPin2Value, FALSE);
    VariableDefinePlatformVar(pc, NULL, "GPIO_PIN_3", &pc->IntType, (union AnyValue *)&Stm32GpioPin3Value, FALSE);
    VariableDefinePlatformVar(pc, NULL, "GPIO_PIN_4", &pc->IntType, (union AnyValue *)&Stm32GpioPin4Value, FALSE);
    VariableDefinePlatformVar(pc, NULL, "GPIO_PIN_5", &pc->IntType, (union AnyValue *)&Stm32GpioPin5Value, FALSE);
    VariableDefinePlatformVar(pc, NULL, "GPIO_PIN_6", &pc->IntType, (union AnyValue *)&Stm32GpioPin6Value, FALSE);
    VariableDefinePlatformVar(pc, NULL, "GPIO_PIN_7", &pc->IntType, (union AnyValue *)&Stm32GpioPin7Value, FALSE);
    VariableDefinePlatformVar(pc, NULL, "GPIO_PIN_8", &pc->IntType, (union AnyValue *)&Stm32GpioPin8Value, FALSE);
    VariableDefinePlatformVar(pc, NULL, "GPIO_PIN_9", &pc->IntType, (union AnyValue *)&Stm32GpioPin9Value, FALSE);
    VariableDefinePlatformVar(pc, NULL, "GPIO_PIN_10", &pc->IntType, (union AnyValue *)&Stm32GpioPin10Value, FALSE);
    VariableDefinePlatformVar(pc, NULL, "GPIO_PIN_11", &pc->IntType, (union AnyValue *)&Stm32GpioPin11Value, FALSE);
    VariableDefinePlatformVar(pc, NULL, "GPIO_PIN_12", &pc->IntType, (union AnyValue *)&Stm32GpioPin12Value, FALSE);
    VariableDefinePlatformVar(pc, NULL, "GPIO_PIN_13", &pc->IntType, (union AnyValue *)&Stm32GpioPin13Value, FALSE);
    VariableDefinePlatformVar(pc, NULL, "GPIO_PIN_14", &pc->IntType, (union AnyValue *)&Stm32GpioPin14Value, FALSE);
    VariableDefinePlatformVar(pc, NULL, "GPIO_PIN_15", &pc->IntType, (union AnyValue *)&Stm32GpioPin15Value, FALSE);
    VariableDefinePlatformVar(pc, NULL, "GPIO_PIN_All", &pc->IntType, (union AnyValue *)&Stm32GpioPinAllValue, FALSE);

    VariableDefinePlatformVar(pc, NULL, "GPIO_PIN_RESET", &pc->IntType, (union AnyValue *)&Stm32GpioPinResetValue, FALSE);
    VariableDefinePlatformVar(pc, NULL, "GPIO_PIN_SET", &pc->IntType, (union AnyValue *)&Stm32GpioPinSetValue, FALSE);

    VariableDefinePlatformVar(pc, NULL, "GPIO_MODE_INPUT", &pc->IntType, (union AnyValue *)&Stm32GpioModeInputValue, FALSE);
    VariableDefinePlatformVar(pc, NULL, "GPIO_MODE_OUTPUT_PP", &pc->IntType, (union AnyValue *)&Stm32GpioModeOutputPpValue, FALSE);
    VariableDefinePlatformVar(pc, NULL, "GPIO_MODE_OUTPUT_OD", &pc->IntType, (union AnyValue *)&Stm32GpioModeOutputOdValue, FALSE);
    VariableDefinePlatformVar(pc, NULL, "GPIO_MODE_ANALOG", &pc->IntType, (union AnyValue *)&Stm32GpioModeAnalogValue, FALSE);

    VariableDefinePlatformVar(pc, NULL, "GPIO_NOPULL", &pc->IntType, (union AnyValue *)&Stm32GpioNopullValue, FALSE);
    VariableDefinePlatformVar(pc, NULL, "GPIO_PULLUP", &pc->IntType, (union AnyValue *)&Stm32GpioPullupValue, FALSE);
    VariableDefinePlatformVar(pc, NULL, "GPIO_PULLDOWN", &pc->IntType, (union AnyValue *)&Stm32GpioPulldownValue, FALSE);

    VariableDefinePlatformVar(pc, NULL, "GPIO_SPEED_FREQ_LOW", &pc->IntType, (union AnyValue *)&Stm32GpioSpeedLowValue, FALSE);
    VariableDefinePlatformVar(pc, NULL, "GPIO_SPEED_FREQ_MEDIUM", &pc->IntType, (union AnyValue *)&Stm32GpioSpeedMediumValue, FALSE);
    VariableDefinePlatformVar(pc, NULL, "GPIO_SPEED_FREQ_HIGH", &pc->IntType, (union AnyValue *)&Stm32GpioSpeedHighValue, FALSE);
    VariableDefinePlatformVar(pc, NULL, "GPIO_SPEED_FREQ_VERY_HIGH", &pc->IntType, (union AnyValue *)&Stm32GpioSpeedVeryHighValue, FALSE);
}

static const char Stm32Defs[] = "\
typedef struct { \
    unsigned int Pin; \
    unsigned int Mode; \
    unsigned int Pull; \
    unsigned int Speed; \
    unsigned int Alternate; \
} GPIO_InitTypeDef; \
";

static struct LibraryFunction Stm32Functions[] =
{
    { PicocHalGpioInit,      "void HAL_GPIO_Init(void *,GPIO_InitTypeDef *);" },
    { PicocHalGpioDeInit,    "void HAL_GPIO_DeInit(void *,int);" },
    { PicocHalGpioReadPin,   "int HAL_GPIO_ReadPin(void *,int);" },
    { PicocHalGpioWritePin,  "void HAL_GPIO_WritePin(void *,int,int);" },
    { PicocHalGpioTogglePin, "void HAL_GPIO_TogglePin(void *,int);" },
    { PicocHalDelay,         "void HAL_Delay(int);" },
    { NULL, NULL }
};

void PlatformLibraryInit(Picoc *pc)
{
    IncludeRegister(pc, "stdio.h", NULL, NULL, NULL);
    IncludeRegister(pc, "picoc_stm32.h", &Stm32SetupFunc, &Stm32Functions[0], Stm32Defs);
}
