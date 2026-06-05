# RICE — Runtime Interactive C Environment

[English](README.md) | [中文](README_CN.md)

RICE ports the **PicoC** C interpreter to **STM32H750VBTx** (Cortex-M7, 480 MHz), providing an interactive C scripting environment over a serial port — no OS, no filesystem, no PC-side tooling required.

Connect via `USART1` (115200 8N1) with any serial terminal (PuTTY, TeraTerm, sscom, minicom, screen, etc.) and start writing C interactively.

## Features

- Interactive PicoC REPL over `USART1`
- File upload execution via `:load / :end / :abort` protocol (paste source from any terminal)
- Multi-line source input with automatic completeness analysis
- Interactive debugging over serial: breakpoints, single-step, expression evaluation, variable inspection and modification, watchpoints
- STM32H750 Keil MDK project ready to build and flash
- Bare-metal super-loop architecture — no RTOS dependency

## Hardware

- **MCU:** STM32H750VBTx (Cortex-M7, 480 MHz, 128 KB SRAM)
- **Serial:** USART1 (PA9 TX, PA10 RX), 115200 baud, 8N1
- **Programmer:** ST-Link (SWD)
- **Power:** USB or external 3.3V

## Quick Start

1. Open `MDK-ARM/UART_DMA_H750.uvprojx` in Keil MDK.
2. Build (F7) and flash via ST-Link.
3. Open a serial terminal on `USART1` at `115200 8N1`.
4. After boot, you should see the `picoc>` prompt.

## Manual Serial Usage

All interaction happens through plain-text commands over the serial port. Use any terminal emulator.

### REPL Mode

Type C statements directly at the `picoc>` prompt. Multi-line input (e.g. function definitions, loops) is automatically detected — the interpreter waits until your input is syntactically complete before executing.

```
picoc> int x = 42;
picoc> printf("x = %d\n", x);
x = 42
picoc> for (int i = 0; i < 3; i++) { printf("%d\n", i); }
0
1
2
```

### File Upload Mode

Send `:load` to enter upload mode, paste your C source code, then send `:end` to execute. The source runs in an isolated PicoC instance (does not pollute the REPL namespace).

```
:load
#include <stdio.h>

int main() {
    printf("Hello from uploaded code!\n");
    return 0;
}
:end
```

Send `:abort` to cancel an upload or interrupt a running script.

### Protocol Commands

| Command | Description |
|---------|-------------|
| `:load [size]` | Enter file upload mode |
| `:end` | Execute uploaded source |
| `:abort` | Cancel upload or abort running script |
| `:ping` | Heartbeat check (responds `:pong`) |
| `:reset` | Reset PicoC interpreter to clean state |

### Interactive Debugging

Debug commands can be sent at any time when the REPL is idle or a script is stopped at a breakpoint.

| Command | Description |
|---------|-------------|
| `:bkpt <file> <line>` | Set a breakpoint |
| `:bkptclear <file> <line>` | Clear a breakpoint |
| `:cont` | Continue execution |
| `:step` | Single-step one statement |
| `:eval <expr>` | Evaluate a C expression in the current scope |
| `:vars` | List all visible variables |
| `:set <name> <value>` | Modify a variable value |

Device-to-host debug notifications:

| Notification | Description |
|-------------|-------------|
| `:break <file> <line> <col>` | Breakpoint hit |
| `:step <file> <line> <col>` | Step completed |
| `:var <type> <name> <value>` | Variable data (one per line) |
| `:ok vars` | Variable enumeration finished |
| `:ok set` / `:err set ...` | Variable write result |

**Typical debug workflow:**

1. Set a breakpoint: send `:bkpt serial_load 5`
2. Upload and execute a source file via `:load` ... `:end`
3. When the breakpoint hits, the device sends `:break serial_load 5 0`
4. Inspect variables with `:vars`, evaluate expressions with `:eval x + 1`
5. Modify a variable with `:set x 100`
6. Step with `:step` or continue with `:cont`
7. Breakpoints auto-clear after each file execution

## Repository Layout

```
├── Core/               STM32 application code (serial, PicoC app layer)
├── picoc/              PicoC interpreter source plus STM32 platform port
├── Drivers/            STM32 HAL and CMSIS drivers
├── MDK-ARM/            Keil MDK project files
├── README.md
├── README_CN.md
└── LICENSE
```

## Scope

- Script input via serial port only
- No target-side filesystem
- Minimal built-in C standard library
- Focus on stable REPL interaction and file execution

## Platform Notes

- The upload buffer is RAM-based (8 KB).
- PicoC uses `setjmp/longjmp` for error recovery — script errors return to the REPL prompt cleanly.
- The serial stack is three layers: DMA/ISR → ring buffer (8 KB RX + 8 KB TX, lock-free SPSC) → PicoC app state machine (REPL / LOAD / DRAIN modes).

## License

MIT. See [LICENSE](LICENSE).
