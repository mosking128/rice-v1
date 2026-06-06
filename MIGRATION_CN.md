# RICE v1 移植工作流 / Porting Workflow

将 RICE v1（PicoC 嵌入式 C 解释器，裸机架构）移植到不同 MCU 平台的完整指南。

---

## 1. 架构分层

```
┌───────────────────────────────────────────────────────────────────┐
│  层 1：PicoC 解释器核心（完全复用，无需修改）                        │
│  文件：picoc/*.c（lex.c, parse.c, expression.c, heap.c,          │
│        table.c, type.c, variable.c, clibrary.c, include.c,       │
│        debug.c, platform.c）                                      │
│  特点：纯 C 逻辑，不依赖任何硬件 API                                │
├───────────────────────────────────────────────────────────────────┤
│  层 2：平台适配层（移植重点：需为新芯片重写）                        │
│  文件：picoc/platform/platform_*.c — PicoC I/O 桥接                │
│        picoc/platform/library_*.c  — 外设函数绑定                  │
│        picoc/platform.h            — 堆大小、功能开关               │
│        picoc/picoc.h               — 平台宏守卫                    │
│        picoc/interpreter.h         — Picoc_Struct 大小             │
├───────────────────────────────────────────────────────────────────┤
│  层 3：应用层（少量适配）                                           │
│  文件：Core/Src/picoc_app.c   — REPL 状态机、协议解析（100% 复用）  │
│        Core/Src/serial_app.c  — 串口 DMA 收发（~80% 复用）          │
├───────────────────────────────────────────────────────────────────┤
│  层 4：HAL 驱动层（CubeMX 生成，需完全重写）                        │
│  文件：main.c, usart.c, dma.c, gpio.c, stm32h7xx_it.c,           │
│        stm32h7xx_hal_msp.c, stm32h7xx_hal_conf.h,                │
│        system_stm32h7xx.c, startup_stm32h750xx.s                  │
└───────────────────────────────────────────────────────────────────┘
```

**v1 与 v2 的关键区别：** v1 是裸机超级循环架构，没有 FreeRTOS。主循环为 `while(1) { PicocApp_Task(); }`，串口读取和 PicoC 执行在同一个上下文中。没有任务间通信、没有协作式 abort（`:abort` 仅通过 `setjmp`/`longjmp` 从调试器内部触发）。

---

## 2. 文件依赖矩阵

### 必须完全重写的文件（CubeMX 生成）

| 文件 | 行数 | 重写原因 |
|------|------|---------|
| `main.c` | ~250 | 时钟树（HSE 25MHz → PLL → 480MHz）、MPU、电源配置、HAL 回调路由 |
| `usart.c` | ~190 | UART 外设初始化、DMA 流分配、GPIO AF 映射、NVIC 优先级 |
| `dma.c` | ~58 | DMA 时钟使能、IRQ 优先级设置 |
| `gpio.c` | ~66 | GPIO 时钟使能、引脚配置 |
| `stm32h7xx_it.c` | ~240 | 中断向量名（DMA1_Stream0_IRQHandler、USART1_IRQHandler） |
| `stm32h7xx_hal_msp.c` | ~83 | 全局 MSP 初始化 |
| `stm32h7xx_hal_conf.h` | ~515 | HAL 模块选择、HSE_VALUE、VDD_VALUE |
| `system_stm32h7xx.c` | ~450 | CMSIS 系统初始化 |
| 启动文件 | — | 向量表 |
| 链接脚本 | — | 内存区域定义 |

### 需要少量修改的文件

| 文件 | 需要改什么 | 不需要改什么 |
|------|-----------|-------------|
| `serial_app.c` | `huart1` → 新 UART 句柄；`USART1` → 新外设实例；`HAL_UARTEx_ReceiveToIdle_DMA` / `HAL_UART_Transmit_DMA` → 新 HAL API | 环形缓冲区逻辑、`SerialApp_Read`/`SerialApp_Write`、`SerialApp_ProcessRxDma`、`SerialApp_RxRingWrite`、`SerialApp_TxDmaKick` |
| `serial_app.h` | `UART_HandleTypeDef` 类型（如果换非 STM32 HAL） | 公共 API 声明 |
| `picoc/platform.h` | 添加 `#ifdef YOUR_MCU_HOST` 块、设置 `HEAP_SIZE` | 哈希表大小、提示符字符串 |
| `picoc/picoc.h` | 在 `#if` 守卫中添加 `defined(YOUR_MCU_HOST)` | 公共 API 原型 |

### 完全可复用的文件（零修改）

| 文件 | 行数 | 说明 |
|------|------|------|
| `picoc_app.c` | ~850 | REPL 状态机、协议解析（v1 中 100% 复用，无 RTOS 依赖） |
| `picoc_app.h` | ~50 | 应用层 API 声明 |
| `picoc/debug.c` | ~783 | 调试器（断点哈希表、单步、求值、变量监视） |
| `picoc/parse.c` | ~1000 | 解析器（v1 中无 AbortRequested 检查点，使用 setjmp/longjmp） |
| `picoc/expression.c` | ~2000 | 表达式求值器 |
| `picoc/lex.c` | ~800 | 词法分析器 |
| `picoc/heap.c` | ~200 | 内存分配器 |
| `picoc/table.c` | ~300 | 哈希表实现 |
| `picoc/type.c` | ~500 | 类型系统 |
| `picoc/variable.c` | ~400 | 变量管理 |
| `picoc/clibrary.c` | ~200 | C 标准库初始化 |
| `picoc/platform.c` | ~100 | 平台无关包装 |
| `picoc/include.c` | ~200 | `#include` 系统 |
| `picoc/cstdlib/*.c` | 9 文件 | 迷你 C 标准库 |

---

## 3. 逐步移植指南

### 第 1 步：CubeMX 工程配置

在 CubeMX 中为新 MCU 创建工程，配置以下外设：

**UART 配置：**
- 模式：Asynchronous（异步）
- 波特率：115200，数据位 8，停止位 1，无校验
- 引脚：选择可用的 TX/RX 引脚
- GPIO 速度：Very High（高速）

**DMA 配置（关键）：**
- RX DMA：循环模式（Circular）、字节对齐、高优先级、FIFO 禁用
- TX DMA：普通模式（Normal）、字节对齐、中优先级、FIFO 禁用
- 必须支持 DMA 循环接收（否则需要改用中断模式）

**NVIC 配置（v1 特点）：**
- DMA RX/TX 中断优先级：0（最高）
- USART 中断优先级：0（最高）
- SysTick 中断优先级：15（最低）
- **注意：** v1 裸机版不需要调用 FreeRTOS FromISR API，所以中断优先级可以设为最高（0）

**时钟配置：**
- HSE：配置外部晶振频率（当前 25 MHz）
- PLL：计算目标 SYSCLK（当前 480 MHz）

### 第 2 步：串口驱动适配（serial_app.c）

**需要修改的 3 处硬件调用：**

```c
// 1. DMA 接收启动
SerialApp_StartRxDma():
  HAL_UARTEx_ReceiveToIdle_DMA(&huart1, rx_dma_buffer, 1024)
  // 改为新 UART 句柄

// 2. DMA 发送启动
SerialApp_TxDmaKick():
  HAL_UART_Transmit_DMA(&huart1, &tx_ring[tx_tail], len)
  // 改为新 UART 句柄

// 3. 外设实例过滤
SerialApp_RxEventCallback():
  if (huart->Instance == USART1)  // 改为新外设实例
  if (huart->Instance == USART1)  // TX 完成回调中同样修改
```

**不需要修改的部分：**
- 环形缓冲区数组（rx_ring[8192]、tx_ring[8192]、rx_dma_buffer[1024]）
- `SerialApp_Read()` / `SerialApp_Write()` — 纯缓冲区操作
- `SerialApp_ProcessRxDma()` — DMA 到环形缓冲区的拷贝逻辑
- `SerialApp_RxRingWrite()` — 环形缓冲区写入
- `SerialApp_TxDmaKick()` — TX DMA 触发逻辑

**非 STM32 平台的串口适配：**
1. 实现等效的 DMA 循环接收（或改为中断接收 + 回调填充 rx_ring）
2. 实现等效的 DMA 发送（或改为中断发送 + 从 tx_ring 取数据）
3. 在接收完成回调中将数据从 DMA 缓冲区拷贝到 rx_ring

### 第 3 步：平台 I/O 适配（platform_*.c）

创建 `picoc/platform/platform_your_mcu.c`，实现以下 8 个函数：

```c
// 必须实现的 2 个底层 I/O 原语：
int PicocApp_ConsoleGetCharBlocking(void);  // 阻塞读取一个字符
// 实现：循环调用 SerialApp_Read()，无数据时简单循环等待

void SerialApp_Write(const uint8_t *data, uint32_t len);
// 已在 serial_app.c 中实现，无需重写

// 以下函数可直接复用现有 platform_stm32h7.c 的实现：
char *PlatformGetLineQuiet(char *Buf, int MaxLen);
char *PlatformGetLine(char *Buf, int MaxLen, const char *Prompt);
int PlatformGetCharacter(void);
void PlatformPutc(unsigned char ch, union OutputStreamInfo *Stream);
void PlatformInit(Picoc *pc);       // 空函数
void PlatformCleanup(Picoc *pc);    // 空函数
void PlatformExit(Picoc *pc, int RetVal);  // longjmp
char *PlatformReadFile(Picoc *pc, const char *FileName);  // 返回错误
void PicocPlatformScanFile(Picoc *pc, const char *FileName);  // 返回错误
```

**v1 特有：PicocApp_ConsoleGetCharBlocking 实现（无 RTOS）：**
```c
int PicocApp_ConsoleGetCharBlocking(void)
{
    uint8_t ch;
    while (SerialApp_Read(&ch, 1U) == 0U)
    {
        // v1 裸机版：简单循环等待，无 osDelay
        // 可选：__WFI() 降低功耗
    }
    return (int)ch;
}
```

**与 v2 的区别：** v2 版本中此函数包含 `AbortRequested` 检查和 `osDelay(1)` 让出 CPU。v1 版本没有这些，因为没有 RTOS 和协作式 abort 机制。

### 第 4 步：外设函数绑定（library_*.c）

创建 `picoc/platform/library_your_mcu.c`，将目标 MCU 的外设函数暴露给 PicoC 脚本。

**三步注册流程：**

```c
// 第 1 步：写包装函数
static void PicocHalDelay(struct ParseState *Parser,
                           struct Value *ReturnValue,
                           struct Value **Param, int NumArgs)
{
    (void)Parser; (void)ReturnValue; (void)NumArgs;
    HAL_Delay((uint32_t)Param[0]->Val->Integer);
}

// 第 2 步：声明函数原型
const char HalDelay[] = "void delay(int ms);";

// 第 3 步：在注册表中添加
const LibraryFunction Stm32Functions[] = {
    { PicocHalDelay, HalDelay },
    { NULL, NULL }
};

void PlatformLibraryInit(Picoc *pc)
{
    IncludeRegister(pc, "stm32.h", &Stm32Defs, &Stm32Functions[0], NULL);
}
```

**建议暴露的外设函数：**

| 外设 | 包装函数示例 | PicoC 原型 |
|------|------------|-----------|
| GPIO | `PicocHalGpioWritePin` | `void digitalWrite(void *port, int pin, int state)` |
| GPIO | `PicocHalGpioReadPin` | `int digitalRead(void *port, int pin)` |
| GPIO | `PicocHalGpioInit` | `void pinMode(void *port, int pin, int mode, int pull, int speed)` |
| 延时 | `PicocHalDelay` | `void delay(int ms)` |
| ADC | `PicocAnalogRead` | `int analogRead(int channel)` |
| PWM | `PicocAnalogWrite` | `void analogWrite(int pin, int duty)` |

**需要注册的常量：**
- GPIO 端口地址（GPIOA ~ GPIOn）
- GPIO 引脚编号（GPIO_PIN_0 ~ GPIO_PIN_15）
- GPIO 模式、上拉/下拉、速度常量

### 第 5 步：PicoC 配置（platform.h）

在 `picoc/platform.h` 中添加新平台的配置块：

```c
#ifdef YOUR_MCU_HOST
# define BUILTIN_MINI_STDLIB        // 使用 PicoC 内置迷你标准库
# define HEAP_SIZE (64*1024)        // PicoC 堆大小（根据目标 RAM 调整）
# define PICOC_MATH_LIBRARY         // 启用数学函数
# define FEATURE_AUTO_DECLARE_VARIABLES  // 允许隐式 int 声明
# define FANCY_ERROR_MESSAGES       // 详细错误信息
#endif
```

**HEAP_SIZE 调整指南：**

| 目标 MCU SRAM | 推荐 HEAP_SIZE | 说明 |
|--------------|---------------|------|
| ≥ 256 KB | 64 KB | 当前 H750 配置 |
| 128~256 KB | 32 KB | 减小堆，功能基本完整 |
| 64~128 KB | 16 KB | 最小可用，复杂脚本可能失败 |
| < 64 KB | 不建议移植 | PicoC 本身需要较大内存 |

### 第 6 步：主循环适配（main.c）

v1 的主循环非常简单：

```c
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_DMA_Init();
    MX_GPIO_Init();
    MX_USART1_UART_Init();
    SerialApp_Init();
    PicocApp_Init();

    while (1)
    {
        PicocApp_Task();  // 轮询串口 + 执行 PicoC
    }
}
```

**移植时只需改：**
1. `SystemClock_Config()` — 完全重写（时钟树、PLL、电压配置）
2. `MX_USART1_UART_Init()` — CubeMX 重新生成
3. `MX_DMA_Init()` — CubeMX 重新生成
4. `MX_GPIO_Init()` — CubeMX 重新生成
5. ISR 回调路由（保留在 main.c 中）：
```c
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    SerialApp_RxEventCallback(huart, Size);
}
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    SerialApp_TxCpltCallback(huart);
}
```

### 第 7 步：编译配置

**Keil MDK：**

预处理宏定义：
```
USE_PWR_LDO_SUPPLY    // STM32 电源模式
USE_HAL_DRIVER        // 启用 HAL 库
STM32H750xx           // 芯片型号宏（改为目标型号）
YOUR_MCU_HOST         // PicoC 平台选择宏
```

包含路径：
```
../Core/Inc
../Drivers/<目标系列>_HAL_Driver/Inc
../Drivers/CMSIS/Device/ST/<目标系列>/Include
../Drivers/CMSIS/Include
../picoc
../picoc/platform
../picoc/cstdlib
```

源文件分组（6 组）：
1. **启动** — startup_<芯片>.s
2. **应用/核心** — serial_app.c, picoc_app.c, main.c, gpio.c, dma.c, usart.c, stm32<系列>_it.c, stm32<系列>_hal_msp.c
3. **应用/PicoC** — clibrary.c, debug.c, expression.c, heap.c, include.c, lex.c, parse.c, platform.c, table.c, type.c, variable.c, platform_<芯片>.c, library_<芯片>.c
4. **HAL 驱动** — hal_uart, hal_dma, hal_gpio, hal_rcc, hal_flash, hal_pwr, hal_cortex, hal_exti, hal_i2c, hal_tim 等
5. **CMSIS** — system_<系列>.c
6. **PicoC/cstdlib** — ctype.c, errno.c, math.c, stdbool.c, stdio.c, stdlib.c, string.c, time.c, unistd.c

### 第 8 步：内存预算验证

**v1 内存预算（无 FreeRTOS）：**

| 消费者 | 大小 | 来源 |
|--------|------|------|
| PicoC 堆 (`HeapMemory`) | 64 KB | `Picoc_Struct` 中的静态数组 |
| PicoC 解释器栈 | 64 KB | 运行时从 C 堆分配（`PICOC_APP_STACK_SIZE`） |
| RX DMA 缓冲区 | 1 KB | `serial_app.c` 静态分配 |
| RX 环形缓冲区 | 8 KB | `serial_app.c` 静态分配 |
| TX 环形缓冲区 | 8 KB | `serial_app.c` 静态分配 |
| 源码行缓冲区 | 2 KB | `picoc_app.c` 静态分配 |
| 加载缓冲区 | 8 KB | `picoc_app.c` 静态分配 |
| Keil 配置堆 | 128 KB | C 库堆（用于 PicoC 解释器栈分配） |
| Keil 配置栈 | 8 KB | C 栈 |
| **总计** | **~160 KB** | |

**最低 RAM 要求：192 KB**

---

## 4. v1 与 v2 架构差异

| 方面 | v1（裸机） | v2（FreeRTOS） |
|------|-----------|----------------|
| 主循环 | `while(1) { PicocApp_Task(); }` | FreeRTOS 任务调度 |
| 串口读取 | `PicocApp_Task()` 直接调用 `SerialApp_Read()` | serialTask 调用，通过队列发给 picocTask |
| 脚本执行 | 同一上下文，阻塞一切 | 独立 picocTask，不阻塞串口 |
| 中断脚本 | 不支持（`while(1)` 卡死 MCU） | 支持（协作式 `AbortRequested` 标志） |
| 心跳响应 | 脚本运行期间无响应 | 始终响应（独立任务处理） |
| 调试输入 | `DebugCheckStatement` 直接读串口 | `g_debug_input_active` 标志协调两任务 |
| NVIC 优先级 | 0（最高） | 5（可调用 FreeRTOS FromISR API） |
| 中断向量 | 包含 SVC/PendSV/SysTick | SVC/PendSV/SysTick 由 FreeRTOS port 处理 |
| 内存管理 | 无 RTOS 堆 | FreeRTOS heap_4（48 KB） |
| HAL 时基 | SysTick | TIM6（SysTick 给 FreeRTOS） |
| 移植复杂度 | 低 | 中 |

---

## 5. 芯片族移植参考

### STM32 系列间移植

| 原芯片 | 目标芯片 | 改动量 | 关键差异 |
|--------|---------|--------|---------|
| STM32H750 | STM32H743 | 极小 | 同系列，改型号宏即可 |
| STM32H750 | STM32F407 | 中 | DMA 控制器不同、无 FIFO 阈值配置、时钟树不同、无 MPU |
| STM32H750 | STM32F103 | 大 | 无 DMA 循环接收（需改用中断模式）、无 `HAL_UARTEx_ReceiveToIdle_DMA` |
| STM32H750 | STM32G0B1 | 中 | DMA 较简单、无 MPU、时钟树不同 |
| STM32H750 | STM32L476 | 中 | 低功耗系列、DMA 控制器不同 |

### 跨厂商移植

| 厂商 | 推荐方案 | 串口适配策略 |
|------|---------|-------------|
| NXP (LPC/IMXRT) | MCUXpresso + LPUART + eDMA | 重写 serial_app.c 的 HAL 调用，环形缓冲区复用 |
| Nuvoton (M480) | NuMaker + UART + PDMA | HAL 风格类似 STM32，改动较小 |
| RP2040 | Pico SDK + UART + DMA | DMA API 简单 |
| ESP32 | ESP-IDF + UART | 建议使用 v2（FreeRTOS 原生支持） |
| GD32 | GD32 HAL + USART + DMA | 与 STM32 HAL 高度兼容 |

---

## 6. 移植验证清单

| 步骤 | 验证内容 | 测试方法 | 预期结果 |
|------|---------|---------|---------|
| 1 | 串口基本通信 | `SerialApp_Read` + `SerialApp_Write` 循环回显 | 输入什么回显什么 |
| 2 | REPL 模式 | 输入 `printf("hello\n");` | 输出 `hello` |
| 3 | 表达式求值 | 输入 `3 + 5 * 2` | 输出 `13` |
| 4 | 多行输入 | 输入 `for` 循环 | 自动检测完整性并执行 |
| 5 | 文件上传 | `:load` → 源码 → `:end` | 执行并输出结果 |
| 6 | 心跳检测 | `:ping` | 收到 `:pong` |
| 7 | 重置 | `:reset` | 收到 `:ok` |
| 8 | 设置断点 | `:bkpt serial_load 5` | 收到 `:ok bkpt` |
| 9 | 断点命中 | 上传含断点的脚本并执行 | 收到 `:break serial_load 5 0` |
| 10 | 单步执行 | `:step` | 收到 `:step serial_load 6 0` |
| 11 | 继续执行 | `:cont` | 继续到下一个断点或结束 |
| 12 | 表达式求值（调试中） | `:eval x + 1` | 输出求值结果 |
| 13 | 变量监视 | `:vars` | 收到 `:var` 列表和 `:ok vars` |
| 14 | 修改变量 | `:set x 100` | 收到 `:ok set` |
| 15 | 多断点 | 设置 3 个断点，逐个命中 | 全部正常暂停和继续 |
| 16 | 外设绑定 | PicoC 脚本中调用 `delay(100)` | 正常延时 |
| 17 | 压力测试 | 上传 68+ PicoC 测试用例 | 全部通过 |

---

## 7. 常见问题

**Q: 编译报 `undefined reference to PlatformXxx`**
A: 未实现平台适配函数。检查 `platform_your_mcu.c` 是否包含所有 8 个必需函数。

**Q: 串口能收不能发**
A: 检查 DMA TX 配置、`HAL_UART_TxCpltCallback` 是否路由到 `SerialApp_TxCpltCallback`、NVIC 中断是否使能。

**Q: REPL 无响应，看不到 `picoc>` 提示符**
A: 检查 `main()` 中调用顺序：`SerialApp_Init()` → `PicocApp_Init()` → `while(1) { PicocApp_Task(); }`。

**Q: 文件上传后无输出**
A: 检查源码中是否定义了 `main()` 函数。PicoC 会自动调用 `main()`。

**Q: 断点不生效**
A: 1) 确认断点文件名与上传文件名一致（默认 `serial_load`）。2) 确认 `DebugCheckStatement()` 在每个语句前被调用。

**Q: 堆栈溢出（HardFault）**
A: 1) 增大 Keil 工程中的 Heap Size（当前 128 KB）。2) 增大 Stack Size（当前 8 KB）。3) 减小 `HEAP_SIZE`（当前 64 KB）。4) 检查目标 MCU 的总 SRAM 是否 ≥ 192 KB。

**Q: 编译报 `jmp_buf` 未定义**
A: 确认编译器支持 `<setjmp.h>`。确认 `picoc/picoc.h` 中的平台宏守卫包含了你的目标宏。

**Q: `:abort` 不生效**
A: v1 裸机版中 `:abort` 仅在脚本执行期间通过 `setjmp`/`longjmp` 生效。如果脚本卡在 `while(1){}` 中且没有调用任何 PicoC 解释器函数，`:abort` 无法中断（这就是 v2 存在的原因）。

**Q: 脚本运行时 `:ping` 无响应**
A: 这是 v1 的已知限制。脚本执行期间主循环被阻塞，无法处理新命令。参见 [RICE v2](https://github.com/mosking128/rice-v2) 获取 FreeRTOS 任务隔离支持。
