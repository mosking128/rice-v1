# RICE 移植工作流 / Porting Workflow

本文档为开发者提供将 RICE（PicoC 嵌入式 C 解释器）移植到不同 MCU 平台的通用方法论。适用于 RICE v1（裸机）和 v2（FreeRTOS）。

This document provides a universal methodology for porting RICE (PicoC embedded C interpreter) to different MCU platforms. Applicable to both RICE v1 (bare-metal) and v2 (FreeRTOS).

---

## 移植架构总览 / Architecture Overview

RICE 采用分层架构，移植时只需修改底层硬件相关代码，上层逻辑完全复用。

RICE uses a layered architecture. Only the bottom hardware-dependent layer needs modification; upper layers are fully reusable.

```
┌─────────────────────────────────────────────────────────────┐
│  PicoC 解释器核心（无需修改 / No changes needed）             │
│  picoc/*.c  — 解释器、解析器、调试器                          │
├─────────────────────────────────────────────────────────────┤
│  平台适配层（移植重点 / Porting focus）                       │
│  picoc/platform/platform_*.c  — I/O 桥接                     │
│  picoc/platform/library_*.c   — 外设函数绑定                  │
├─────────────────────────────────────────────────────────────┤
│  应用层（少量适配 / Minor adaptation）                        │
│  Core/Src/picoc_app.c   — REPL 状态机、协议解析               │
│  Core/Src/serial_app.c  — 串口 DMA 收发                      │
├─────────────────────────────────────────────────────────────┤
│  HAL 驱动层（需完全重写 / Must rewrite）                      │
│  usart.c, dma.c, gpio.c  — CubeMX 生成或手写                 │
└─────────────────────────────────────────────────────────────┘
```

---

## 移植检查清单 / Porting Checklist

### 第一步：硬件抽象层配置 / Step 1: Hardware Abstraction Layer

| 任务 / Task | 文件 / Files | 说明 / Notes |
|-------------|-------------|-------------|
| CubeMX 配置新 MCU | `.ioc` 文件 | 生成 HAL 初始化代码 |
| 配置 UART 外设 | `usart.c`, `usart.h` | 选择可用的 USART，配置引脚 |
| 配置 DMA | `dma.c`, `dma.h` | RX: 循环模式; TX: 普通模式 |
| 配置中断 | `stm32h7xx_it.c` | USART IDLE 中断 + DMA 完成中断 |
| 时钟配置 | `SystemClock_Config()` | 确保 CPU 和外设时钟正确 |
| MPU 配置 | `MPU_Config()` | Cortex-M7 需要，M4/M3 可省略 |

**关键约束 / Key constraints:**
- UART 必须支持 DMA 收发
- 必须有 IDLE 线检测中断（或等效机制）
- DMA RX 必须支持循环模式

### 第二步：串口驱动层 / Step 2: Serial Driver Layer

修改 `serial_app.c`，适配新 MCU 的 DMA/UART 寄存器。

Modify `serial_app.c` to match the new MCU's DMA/UART registers.

**需要修改的部分 / What to change:**

| 组件 | 现状 | 移植时需改 |
|------|------|-----------|
| DMA 缓冲区 | `rx_dma_buffer[1024]` | 不改 |
| 环形缓冲区 | `rx_ring[8192]` / `tx_ring[8192]` | 不改 |
| DMA 启动 | `HAL_UARTEx_ReceiveToIdle_DMA(&huart1, ...)` | 改 `huart1` 为新 UART 句柄 |
| DMA 发送 | `HAL_UART_Transmit_DMA(&huart1, ...)` | 同上 |
| 回调路由 | `main.c` 中的 HAL 回调 | 不改（HAL 回调机制通用） |

**v2 额外修改 / v2 additional changes:**
- `SerialApp_InitMutex()` — FreeRTOS 互斥锁，与芯片无关
- `SerialApp_RxEventCallback()` 中的 `xTaskNotifyFromISR` — 与芯片无关

### 第三步：平台 I/O 适配 / Step 3: Platform I/O Adaptation

创建 `picoc/platform/platform_<芯片型号>.c`，实现以下函数：

Create `picoc/platform/platform_<chip>.c`, implementing these functions:

| 函数 | 签名 | 说明 |
|------|------|------|
| `PlatformInit` | `void PlatformInit(Picoc *pc)` | 平台初始化（通常为空） |
| `PlatformCleanup` | `void PlatformCleanup(Picoc *pc)` | 平台清理（通常为空） |
| `PlatformGetLine` | `char *PlatformGetLine(char *Buf, int MaxLen, const char *Prompt)` | 读取一行输入（带回显） |
| `PlatformGetLineQuiet` | `char *PlatformGetLineQuiet(char *Buf, int MaxLen)` | 读取一行输入（无回显，调试器用） |
| `PlatformGetCharacter` | `int PlatformGetCharacter(void)` | 读取单个字符（阻塞） |
| `PlatformPutc` | `void PlatformPutc(unsigned char ch, union OutputStreamInfo *Stream)` | 输出单个字符 |
| `PlatformReadFile` | `char *PlatformReadFile(Picoc *pc, const char *FileName)` | 读取文件（嵌入式通常不支持） |
| `PlatformExit` | `void PlatformExit(Picoc *pc, int RetVal)` | 退出执行（`longjmp`） |

**模板代码 / Template:**

```c
#include "../picoc.h"
#include "../interpreter.h"
#include "picoc_app.h"
#include "serial_app.h"

char *PlatformGetLineQuiet(char *Buf, int MaxLen)
{
    int len = 0;
    if (Buf == NULL || MaxLen <= 1) return NULL;
    for (;;) {
        int ch = PicocApp_ConsoleGetCharBlocking();
        if (ch == '\r' || ch == '\n') {
            Buf[len++] = '\n';
            Buf[len] = '\0';
            return Buf;
        }
        if (ch == '\b' || ch == 0x7f) {
            if (len > 0) len--;
            continue;
        }
        if (len < MaxLen - 2)
            Buf[len++] = (char)ch;
    }
}

// PlatformGetLine, PlatformGetCharacter, PlatformPutc 等
// 参见现有 platform_stm32h7.c 的实现
// See existing platform_stm32h7.c for implementation

void PlatformExit(Picoc *pc, int RetVal)
{
    pc->PicocExitValue = RetVal;
    longjmp(pc->PicocExitBuf, 1);
}
```

**关键点 / Key points:**
- 所有 I/O 最终调用 `PicocApp_ConsoleGetCharBlocking()`（输入）和 `SerialApp_Write()`（输出）
- `PlatformExit` 必须使用 `longjmp`，不能用 `return`
- `PlatformReadFile` 在嵌入式上返回错误即可

### 第四步：外设函数绑定 / Step 4: Peripheral Function Bindings

修改 `picoc/platform/library_<芯片型号>.c`，将新 MCU 的 HAL 函数暴露给 PicoC 脚本。

Modify `picoc/platform/library_<chip>.c` to expose the new MCU's HAL functions to PicoC scripts.

**三步注册流程 / Three-step registration:**

```c
// 1. 写包装函数 / Write wrapper function
static void PicocHalDelay(struct ParseState *Parser,
                           struct Value *ReturnValue,
                           struct Value **Param, int NumArgs)
{
    (void)Parser; (void)ReturnValue; (void)NumArgs;
    HAL_Delay((uint32_t)Param[0]->Val->Integer);
}

// 2. 声明函数原型（供 PicoC 解析器使用）
//    Declare function prototype (for PicoC parser)
const char HalDelay[] = "void delay(int ms);";

// 3. 在注册表中添加条目 / Add entry to registration table
// 在 PlatformLibrary[] 数组中：
//   { PicocHalDelay, HalDelay },
```

**需要绑定的外设 / Peripherals to bind:**

| 外设 | 推荐暴露的函数 | 说明 |
|------|--------------|------|
| GPIO | `digitalWrite(pin, val)`, `digitalRead(pin)` | 基础 I/O |
| Timer | `delay(ms)`, `micros()` | 时间控制 |
| ADC | `analogRead(channel)` | 模拟输入 |
| PWM | `analogWrite(pin, duty)` | 脉宽调制 |
| UART | `serialWrite(port, data)`, `serialRead(port)` | 多串口 |
| I2C | `i2cWrite(addr, data, len)` | 总线通信 |
| SPI | `spiTransfer(cs, tx, rx, len)` | 高速通信 |

### 第五步：PicoC 配置 / Step 5: PicoC Configuration

修改 `picoc/platform.h` 中的平台宏：

Modify platform macros in `picoc/platform.h`:

| 宏 | 说明 | 典型值 |
|----|------|--------|
| `HEAP_SIZE` | PicoC 堆大小 | 64 KB（根据 SRAM 调整） |
| `NO_FP` | 禁用浮点 | 不定义（保留浮点支持） |
| `NO_DEBUGGER` | 禁用调试器 | 不定义（保留调试支持） |
| `NO_HASH_INCLUDE` | 禁用 `#include` | 不定义 |
| `BUILTIN_MINI_STDLIB` | 使用最小标准库 | 定义（嵌入式推荐） |

### 第六步：应用层适配 / Step 6: Application Layer Adaptation

**v1（裸机 / bare-metal）：**

修改 `main.c` 中的主循环：

```c
// main() 中：
SerialApp_Init();
PicocApp_Init();
while (1) {
    PicocApp_Task();  // 轮询串口 + 执行 PicoC
}
```

**v2（FreeRTOS）：**

修改 `freertos.c` 中的任务实现：

```c
void StartSerialTask(void *argument) {
    uint8_t buf[256];
    TaskMsg msg;
    for (;;) {
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(10));
        uint32_t len = SerialApp_Read(buf, sizeof(buf));
        if (len > 0) {
            PicocApp_ProcessChars(buf, len, &msg);
            osMessageQueuePut(uartQueneHandle, &msg, 0U, 0U);
        }
    }
}

void StartPicocTask(void *argument) {
    TaskMsg msg;
    for (;;) {
        if (osMessageQueueGet(uartQueneHandle, &msg, NULL, osWaitForever) == osOK) {
            switch (msg.type) {
                case MSG_SOURCE_LINE: PicocApp_RunSourceLine(msg.data); break;
                case MSG_LOAD_END:    PicocApp_ExecuteLoadSource(); break;
                case MSG_RESET:       PicocApp_Reset(); break;
                // ...
            }
        }
    }
}
```

### 第七步：编译配置 / Step 7: Build Configuration

**Keil MDK：**
1. 在 Manage Project Items 中添加 PicoC 源文件组
2. Include Paths 添加 `picoc/`、`picoc/platform/`、`Core/Inc/`
3. C 预处理宏：`USE_HAL_DRIVER`、`<芯片型号>`（如 `STM32F407xx`）
4. 堆栈大小：Heap ≥ 64 KB（PicoC 堆），Stack ≥ 8 KB

**GCC / CMake：**
```cmake
# 添加 PicoC 源文件
file(GLOB PICOCSRC picoc/*.c picoc/platform/*.c)
target_sources(${PROJECT_NAME} PRIVATE ${PICOCSRC})
target_include_directories(${PROJECT_NAME} PRIVATE picoc picoc/platform Core/Inc)
target_compile_definitions(${PROJECT_NAME} PRIVATE USE_HAL_DRIVER STM32F407xx)
```

---

## 芯片族移植指南 / MCU Family Guide

### STM32 系列间移植 / Between STM32 Families

| 原芯片 | 目标芯片 | 改动量 | 说明 |
|--------|---------|--------|------|
| STM32H750 | STM32H743 | 极小 | 同系列，改型号宏即可 |
| STM32H750 | STM32F407 | 小 | DMA 控制器不同，需适配 DMA 配置 |
| STM32H750 | STM32F103 | 中 | 无 DMA 循环接收，需改用中断模式 |
| STM32H750 | STM32G0B1 | 中 | DMA 较简单，需适配 |

**DMA 差异要点 / DMA differences:**

| 特性 | H7 | F4 | F1 | G0 |
|------|----|----|----|----|
| DMA 循环模式 | 支持 | 支持 | 不支持 | 支持 |
| IDLE 中断 | `HAL_UARTEx_ReceiveToIdle_DMA` | 同左 | 需手动检测 | 同左 |
| DMA 请求映射 | MUX | 固定 | 固定 | 固定 |

### 跨厂商移植 / Cross-Vendor Porting

| 厂商 | 推荐方案 | 关键适配点 |
|------|---------|-----------|
| NXP (LPC/IMX) | MCUXpresso + UART DMA | DMA API 不同，需重写 serial_app |
| Infineon (XMC) | DAVE + UART | 无 DMA 循环模式，用中断接收 |
| Nuvoton (M480) | NuMaker + UART DMA | HAL 风格类似 STM32，较易移植 |
| ESP32 | ESP-IDF + UART | FreeRTOS 原生支持，v2 架构更合适 |
| RP2040 | Pico SDK + UART | DMA API 简单，移植较易 |

---

## v1 与 v2 架构差异 / v1 vs v2 Architecture Differences

| 方面 | v1（裸机） | v2（FreeRTOS） |
|------|-----------|----------------|
| 主循环 | `while(1) { PicocApp_Task(); }` | FreeRTOS 任务调度 |
| 串口读取 | `PicocApp_Task()` 直接调用 `SerialApp_Read()` | serialTask 调用，通过队列发给 picocTask |
| 脚本执行 | 同一上下文，阻塞一切 | 独立 picocTask，不阻塞串口 |
| 中断脚本 | 不支持（`while(1)` 卡死 MCU） | 支持（协作式 `AbortRequested` 标志） |
| 心跳响应 | 脚本运行期间无响应 | 始终响应（独立任务处理） |
| 调试输入 | `DebugCheckStatement` 直接读串口 | `g_debug_input_active` 标志协调两任务 |
| 内存管理 | 无 RTOS 堆 | FreeRTOS heap_4（30 KB） |
| 移植复杂度 | 低（无 RTOS 依赖） | 中（需配置 FreeRTOS） |

**移植建议 / Migration advice:**
- 资源紧张的小型 MCU（< 64 KB SRAM）：使用 v1 裸机架构
- 需要中断脚本或实时响应的场景：使用 v2 FreeRTOS 架构
- 已有 FreeRTOS 的项目：直接集成 v2 架构

---

## 移植验证 / Porting Verification

按以下顺序逐层验证：

Verify in this bottom-up order:

| 步骤 | 验证内容 | 方法 |
|------|---------|------|
| 1 | 串口回显 | `SerialApp_Read` + `SerialApp_Write` 循环 |
| 2 | REPL 交互 | 输入 `printf("hello\n");` 看输出 |
| 3 | 多行输入 | 输入 `for` 循环，检查自动完整性检测 |
| 4 | 文件上传 | `:load` → 源码 → `:end`，检查执行结果 |
| 5 | 中断脚本 | 执行 `while(1){}` 后发 `:abort`（仅 v2） |
| 6 | 调试功能 | 设断点 → 执行 → 命中 → `:step` / `:cont` |
| 7 | 外设绑定 | PicoC 脚本中调用 `delay(100)` 等绑定函数 |
| 8 | 压力测试 | 上传 68+ PicoC 测试用例全部通过 |

---

## 常见问题 / FAQ

**Q: PicoC 编译报 `undefined reference to PlatformXxx`**
A: 未实现平台适配函数。检查 `platform_<chip>.c` 是否包含所有必需函数。

**Q: 串口能收不能发**
A: 检查 DMA TX 配置和 `SerialApp_TxCpltCallback` 是否正确路由到 `SerialApp_TxCpltCallback`。

**Q: REPL 无响应，看不到 `picoc>` 提示符**
A: 检查 `PicocApp_Init()` 是否在 `main()` 中调用，以及 `PicocApp_Task()` 是否在主循环中。

**Q: 文件上传后无输出**
A: 检查 `main()` 函数是否在源码中定义。PicoC 会自动调用 `main()`。

**Q: 断点不生效**
A: 确保断点文件名与上传时的文件名一致（默认为 `serial_load`）。

**Q: 堆栈溢出（HardFault）**
A: 增大 `HEAP_SIZE`（PicoC 堆）和任务栈大小（v2）。推荐 PicoC 堆 64 KB，picocTask 栈 16 KB。
