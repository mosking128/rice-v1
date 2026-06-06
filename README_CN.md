# RICE v1 — Runtime Interactive C Environment

[English](README.md) | [中文](README_CN.md)

RICE v1 将 **PicoC** C 语言解释器移植到 **STM32H750VBTx**（Cortex-M7, 480 MHz），通过串口提供交互式 C 脚本环境，具备完整的调试支持——断点、单步执行、变量监视和表达式求值。无需操作系统、无需文件系统、无需 PC 端工具。

用任意串口终端（PuTTY、TeraTerm、sscom、minicom、screen 等）连接 `USART1`（115200 8N1），即可开始交互式 C 编程。

## 功能特性

- 基于 `USART1` 的 PicoC 交互式 REPL
- 通过 `:load` / `:end` / `:abort` 协议实现文件上传执行
- 多行源码输入，自动分析语句完整性
- 交互式调试：断点、单步、表达式求值、变量查看与修改
- 提供 STM32H750 的 Keil MDK 工程，直接编译烧录
- 裸机超级循环架构——无 RTOS 依赖

## 硬件要求

- **MCU:** STM32H750VBTx（Cortex-M7, 480 MHz, 128 KB SRAM）
- **串口:** USART1（PA9 TX, PA10 RX），115200 波特率，8N1
- **烧录器:** ST-Link (SWD)
- **供电:** USB 或外部 3.3V

## 快速开始

1. 用 Keil MDK 打开 `MDK-ARM/UART_DMA_H750.uvprojx`。
2. 编译（F7）并通过 ST-Link 烧录。
3. 用串口终端连接 `USART1`，波特率 `115200`，数据位 `8N1`。
4. 上电后等待出现 `picoc>` 提示符。

## 使用指南

所有交互通过串口发送纯文本命令完成。任意串口终端均可使用。

### REPL 模式

默认模式。直接输入 C 语句，立即查看结果。多行输入（如函数定义、循环体）会被自动识别——解释器会等待输入语句完整后再执行。

```
picoc> int x = 42;
picoc> printf("x = %d\n", x);
x = 42
picoc> 3 + 5 * 2
13
```

多行代码块：

```
picoc> for (int i = 0; i < 3; i++) {
...     printf("i = %d\n", i);
... }
i = 0
i = 1
i = 2
```

### 文件上传

上传完整的 C 源文件进行执行。文件在隔离的 PicoC 实例中运行，不会污染 REPL 环境。

```
:load
#include <stdio.h>

int main() {
    for (int i = 0; i < 10; i++) {
        printf("count: %d\n", i);
    }
    return 0;
}
:end
```

`:load` 进入上传模式（显示 `load>` 提示符）。发送源码行后用 `:end` 执行。在上传模式中可用 `:abort` 取消上传，不执行。

### 心跳检测

`:ping` 命令检查设备是否存活：

```
:ping
:pong
```

### 重置

将解释器重置为干净状态（清除所有变量、函数和断点）：

```
:reset
:ok
picoc>
```

## 调试功能

RICE v1 包含完整的交互式调试器，支持断点、单步执行、变量监视和表达式求值。

### 设置断点

在上传文件的任意行设置断点。上传文件的文件名为 `serial_load`。

```
:bkpt serial_load 5
:ok bkpt
:bkpt serial_load 12
:ok bkpt
:bkpt serial_load 20
:ok bkpt
```

支持同时设置多个断点。当执行到断点行时，调试器暂停并报告位置：

```
:break serial_load 5 0
```

清除指定断点：

```
:bkptclear serial_load 5
:ok bkptclear
```

### 单步执行

命中断点后，使用 `:step` 逐条语句执行：

```
:break serial_load 5 0       ← 命中断点
:step
:step serial_load 6 0        ← 前进到下一行
:step
:step serial_load 7 0        ← 继续
```

每条 `:step` 响应显示新的执行位置（文件名、行号、列号）。

### 继续执行

恢复正常执行，直到下一个断点或程序结束：

```
:break serial_load 12 0      ← 命中断点
:cont
:ok ready                    ← 脚本执行完毕
picoc>
```

### 表达式求值

在断点暂停时，求值当前作用域中的任意 C 表达式：

```
:break serial_load 10 0
:eval x + 1
43
:ok eval
:eval arr[2]
99
:ok eval
```

### 查看变量

列出所有可见变量（局部变量和全局变量）及其类型和值：

```
:vars
:var i 5
:var x 42
:var c A
:var p 0x20001000
:ok vars
```

类型标识符：`i`=int, `s`=short, `c`=char, `l`=long, `I`=unsigned int, `S`=unsigned short, `C`=unsigned char, `L`=unsigned long, `f`=float, `p`=pointer。

### 修改变量

在断点暂停时修改变量的值：

```
:break serial_load 10 0
:set x 100
:ok set
:eval x
100
:ok eval
```

支持所有整数类型、浮点、指针和字符字面量（`'A'`）。

### 完整调试示例

完整的调试会话流程：

```
:load
#include <stdio.h>

int main() {
    int sum = 0;
    for (int i = 1; i <= 5; i++) {
        sum += i;
    }
    printf("sum = %d\n", sum);
    return 0;
}
:end
```

设置断点并运行：

```
:bkpt serial_load 5
:ok bkpt
:bkpt serial_load 7
:ok bkpt
:end                          ← 开始执行
:break serial_load 5 0        ← 在第 5 行暂停
:vars
:var sum 0
:ok vars
:step
:step serial_load 6 0         ← 进入循环
:step
:step serial_load 7 0         ← 到达 sum += i
:eval sum
1
:ok eval
:eval i
1
:ok eval
:cont                         ← 继续到下一个断点
:break serial_load 7 0        ← 再次命中（循环迭代）
:eval sum
6
:ok eval
:cont
:ok ready                     ← 脚本执行完毕
picoc>
```

### 调试命令速查

| 命令 | 说明 |
|------|------|
| `:bkpt <文件> <行号>` | 设置断点 |
| `:bkptclear <文件> <行号>` | 清除断点 |
| `:cont` | 继续执行 |
| `:step` | 单步执行一条语句 |
| `:eval <表达式>` | 在当前作用域中求值 |
| `:vars` | 列出可见变量 |
| `:set <变量名> <值>` | 修改变量值 |

## 协议参考

### 主机 → 设备命令

| 命令 | 说明 |
|------|------|
| `:load [size]` | 进入文件上传模式。可选 `size` 参数用于缓冲区预检查。 |
| `:end` | 执行已上传的源码 |
| `:abort` | 取消上传或中断正在运行的脚本 |
| `:ping` | 心跳检测（回复 `:pong`） |
| `:reset` | 重置 PicoC 解释器 |
| `:bkpt <文件> <行号>` | 在文件的指定行设置断点 |
| `:bkptclear <文件> <行号>` | 清除断点 |
| `:cont` | 断点后继续执行 |
| `:step` | 单步执行一条语句 |
| `:eval <表达式>` | 在当前调试作用域中求值表达式 |
| `:vars` | 枚举所有可见变量 |
| `:set <变量名> <值>` | 修改变量值 |

### 设备 → 主机响应

| 响应 | 说明 |
|------|------|
| `:ok [data]` | 成功。data 可选：`ready`, `bkpt`, `bkptclear`, `eval`, `set`, `vars` |
| `:err <msg>` | 错误，附带错误消息 |
| `:pong` | 对 `:ping` 的响应 |
| `:break <文件> <行号> <列号>` | 执行在断点处暂停 |
| `:step <文件> <行号> <列号>` | 单步后执行暂停 |
| `:var <类型> <名称> <值>` | 变量数据（每行一条，出现在 `:vars` 和 `:ok vars` 之间） |

## 仓库结构

```
├── Core/               STM32 应用代码（串口层、PicoC 应用层）
├── picoc/              PicoC 解释器源码及 STM32 平台适配
├── Drivers/            STM32 HAL 和 CMSIS 驱动
├── MDK-ARM/            Keil MDK 工程文件
├── README.md
├── README_CN.md
└── LICENSE
```

## 技术细节

- **MCU:** STM32H750VBTx（Cortex-M7, 480 MHz, 128 KB SRAM）
- **串口栈:** DMA 环形缓冲（1 KB）→ 环形缓冲区（8 KB RX + 8 KB TX，无锁 SPSC）→ PicoC 应用状态机（REPL / LOAD / DRAIN 三种模式）
- **上传缓冲区:** 8 KB RAM
- **PicoC 堆:** 64 KB（platform.h）
- **错误恢复:** `setjmp/longjmp`——脚本异常退出后干净回到 REPL 提示符

## 已知限制

- 死循环（`while(1){}`）会卡死 MCU——无协作式中断机制（参见 [RICE v2](https://github.com/mosking128/rice-v2) 获取 FreeRTOS 中断支持）
- 脚本执行期间 `:ping` 无响应
- 未启用电源管理/睡眠模式
- 最小内置 C 标准库

## 相关项目

- [RICE v2](https://github.com/mosking128/rice-v2) — 基于 FreeRTOS 的版本，支持协作式中断和任务隔离

## 许可证

MIT，详见 [LICENSE](LICENSE)。
