# RICE — Runtime Interactive C Environment

[English](README.md) | [中文](README_CN.md)

RICE 将 **PicoC** C 语言解释器移植到 **STM32H750VBTx**（Cortex-M7, 480 MHz），通过串口提供交互式 C 脚本环境——无需操作系统、无需文件系统、无需 PC 端工具。

用任意串口终端（PuTTY、TeraTerm、sscom、minicom、screen 等）连接 `USART1`（115200 8N1），即可开始交互式 C 编程。

## 功能特性

- 基于 `USART1` 的 PicoC 交互式 REPL
- 通过 `:load / :end / :abort` 协议实现文件上传执行（从终端粘贴源码即可）
- 多行源码输入，自动分析语句完整性
- 基于串口的交互式调试：断点、单步、表达式求值、变量查看与修改、Watch 监视
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

## 手动串口使用方法

所有交互通过串口发送纯文本命令完成，可使用任意串口终端软件。

### REPL 模式

在 `picoc>` 提示符下直接输入 C 语句。多行输入（如函数定义、循环体）会被自动识别——解释器会等待你的输入语句完整后再执行。

```
picoc> int x = 42;
picoc> printf("x = %d\n", x);
x = 42
picoc> for (int i = 0; i < 3; i++) { printf("%d\n", i); }
0
1
2
```

### 文件上传模式

发送 `:load` 进入上传模式，粘贴 C 源码，然后发送 `:end` 执行。源码在独立的 PicoC 实例中运行，不会污染 REPL 环境。

```
:load
#include <stdio.h>

int main() {
    printf("Hello from uploaded code!\n");
    return 0;
}
:end
```

发送 `:abort` 可取消上传或中断正在运行的脚本。

### 协议命令

| 命令 | 说明 |
|------|------|
| `:load [size]` | 进入文件上传模式 |
| `:end` | 执行已上传的源码 |
| `:abort` | 取消上传或中断正在运行的脚本 |
| `:ping` | 心跳检测（回复 `:pong`） |
| `:reset` | 重置 PicoC 解释器到干净状态 |

### 交互式调试

当 REPL 空闲或脚本停在断点时，可以发送以下调试命令：

| 命令 | 说明 |
|------|------|
| `:bkpt <文件> <行号>` | 设置断点 |
| `:bkptclear <文件> <行号>` | 清除断点 |
| `:cont` | 继续执行 |
| `:step` | 单步执行一条语句 |
| `:eval <表达式>` | 在当前作用域中求值 C 表达式 |
| `:vars` | 列出所有可见变量 |
| `:set <变量名> <值>` | 修改变量值 |

板端 → 主机的调试通知：

| 通知 | 说明 |
|------|------|
| `:break <文件> <行号> <列号>` | 断点命中 |
| `:step <文件> <行号> <列号>` | 单步完成 |
| `:var <类型> <变量名> <值>` | 变量数据（每行一个） |
| `:ok vars` | 变量枚举结束 |
| `:ok set` / `:err set ...` | 变量修改结果 |

**典型调试流程：**

1. 设置断点：发送 `:bkpt serial_load 5`
2. 通过 `:load` ... `:end` 上传并执行源码
3. 断点命中后，板端发送 `:break serial_load 5 0`
4. 用 `:vars` 查看变量，用 `:eval x + 1` 求值表达式
5. 用 `:set x 100` 修改变量值
6. 用 `:step` 单步或 `:cont` 继续
7. 每次文件执行结束后，断点自动清空

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

## 当前范围

- 脚本输入仅来自串口
- 目标端不依赖文件系统
- 最小内置 C 标准库
- 重点稳定跑通交互式 REPL 和文件执行流程

## 技术说明

- 上传缓冲区使用 RAM（8 KB）。
- PicoC 使用 `setjmp/longjmp` 进行错误恢复——脚本异常退出后干净回到 REPL 提示符。
- 串口栈分三层：DMA/ISR → 环形缓冲区（8 KB RX + 8 KB TX，无锁 SPSC）→ PicoC 应用状态机（REPL / LOAD / DRAIN 三种模式）。

## 许可证

MIT，详见 [LICENSE](LICENSE)。
