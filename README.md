# RICE v1 — Runtime Interactive C Environment

[English](README.md) | [中文](README_CN.md)

RICE v1 ports the **PicoC** C interpreter to **STM32H750VBTx** (Cortex-M7, 480 MHz), providing an interactive C scripting environment over a serial port with full debug support — breakpoints, single-stepping, variable inspection, and expression evaluation. No OS, no filesystem, no PC-side tooling required.

Connect via `USART1` (115200 8N1) with any serial terminal (PuTTY, TeraTerm, sscom, minicom, screen, etc.) and start writing C interactively.

## Features

- Interactive PicoC REPL over `USART1`
- File upload execution via `:load` / `:end` / `:abort` protocol
- Multi-line source input with automatic completeness analysis
- Interactive debugging: breakpoints, single-step, expression evaluation, variable inspection and modification
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

## Usage Guide

All interaction through plain-text serial commands. Any terminal emulator works.

### REPL Mode

The default mode. Type C statements directly and see results immediately. Multi-line input (e.g. function definitions, loops) is automatically detected — the interpreter waits until your input is syntactically complete before executing.

```
picoc> int x = 42;
picoc> printf("x = %d\n", x);
x = 42
picoc> 3 + 5 * 2
13
```

Multi-line code blocks:

```
picoc> for (int i = 0; i < 3; i++) {
...     printf("i = %d\n", i);
... }
i = 0
i = 1
i = 2
```

### File Upload

Upload a complete C source file for execution. The file is buffered and executed in an isolated PicoC instance (does not pollute REPL state).

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

`:load` enters upload mode (`load>` prompt). Send source lines, then `:end` to execute. Use `:abort` in upload mode to cancel without executing.

### Heartbeat

The `:ping` command checks whether the device is alive:

```
:ping
:pong
```

### Reset

Reset the interpreter to a clean state (clears all variables, functions, and breakpoints):

```
:reset
:ok
picoc>
```

## Debugging

RICE v1 includes a full interactive debugger with breakpoints, single-stepping, variable inspection, and expression evaluation.

### Setting Breakpoints

Set a breakpoint on any line of an uploaded file. The filename for uploaded files is `serial_load`.

```
:bkpt serial_load 5
:ok bkpt
:bkpt serial_load 12
:ok bkpt
:bkpt serial_load 20
:ok bkpt
```

Multiple breakpoints can be set simultaneously. When execution reaches a breakpoint line, the debugger pauses and reports the location:

```
:break serial_load 5 0
```

Clear a specific breakpoint:

```
:bkptclear serial_load 5
:ok bkptclear
```

### Single-Stepping

After hitting a breakpoint, use `:step` to execute one statement at a time:

```
:break serial_load 5 0       ← breakpoint hit
:step
:step serial_load 6 0        ← advanced to next line
:step
:step serial_load 7 0        ← and the next
```

Each `:step` response shows the new execution position (filename, line, column).

### Continue Execution

Resume normal execution until the next breakpoint or program end:

```
:break serial_load 12 0      ← breakpoint hit
:cont
:ok ready                    ← script finished
picoc>
```

### Evaluating Expressions

Evaluate any C expression in the current scope while paused at a breakpoint:

```
:break serial_load 10 0
:eval x + 1
43
:ok eval
:eval arr[2]
99
:ok eval
```

### Inspecting Variables

List all visible variables (locals and globals) with their types and values:

```
:vars
:var i 5
:var x 42
:var c A
:var p 0x20001000
:ok vars
```

Type characters: `i`=int, `s`=short, `c`=char, `l`=long, `I`=unsigned int, `S`=unsigned short, `C`=unsigned char, `L`=unsigned long, `f`=float, `p`=pointer.

### Modifying Variables

Change a variable's value while paused:

```
:break serial_load 10 0
:set x 100
:ok set
:eval x
100
:ok eval
```

Supports all integer types, floats, pointers, and character literals (`'A'`).

### Debug Workflow Example

Complete debugging session:

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

Set breakpoints and run:

```
:bkpt serial_load 5
:ok bkpt
:bkpt serial_load 7
:ok bkpt
:end                          ← start execution
:break serial_load 5 0        ← paused at line 5
:vars
:var sum 0
:ok vars
:step
:step serial_load 6 0         ← entered loop
:step
:step serial_load 7 0         ← at sum += i
:eval sum
1
:ok eval
:eval i
1
:ok eval
:cont                         ← continue to next breakpoint
:break serial_load 7 0        ← hit again (loop iteration)
:eval sum
6
:ok eval
:cont
:ok ready                     ← script finished
picoc>
```

### Debug Command Reference

| Command | Description |
|---------|-------------|
| `:bkpt <file> <line>` | Set breakpoint |
| `:bkptclear <file> <line>` | Clear breakpoint |
| `:cont` | Continue execution |
| `:step` | Single-step one statement |
| `:eval <expr>` | Evaluate C expression in current scope |
| `:vars` | List visible variables |
| `:set <name> <value>` | Modify a variable |

## Protocol Reference

### Host → Device Commands

| Command | Description |
|---------|-------------|
| `:load [size]` | Enter file upload mode. Optional `size` for buffer pre-check. |
| `:end` | Execute uploaded source |
| `:abort` | Cancel upload or abort running script |
| `:ping` | Heartbeat (responds `:pong`) |
| `:reset` | Reset PicoC interpreter |
| `:bkpt <file> <line>` | Set breakpoint at line in file |
| `:bkptclear <file> <line>` | Clear breakpoint |
| `:cont` | Continue execution after breakpoint |
| `:step` | Single-step one statement |
| `:eval <expr>` | Evaluate expression in current debug scope |
| `:vars` | Enumerate all visible variables |
| `:set <name> <value>` | Modify variable value |

### Device → Host Responses

| Response | Description |
|----------|-------------|
| `:ok [data]` | Success. Data: `ready`, `bkpt`, `bkptclear`, `eval`, `set`, `vars` |
| `:err <msg>` | Error with message |
| `:pong` | Response to `:ping` |
| `:break <file> <line> <col>` | Execution paused at breakpoint |
| `:step <file> <line> <col>` | Execution paused after single-step |
| `:var <type> <name> <value>` | Variable data (one per line, between `:vars` and `:ok vars`) |

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

## Technical Details

- **MCU:** STM32H750VBTx (Cortex-M7, 480 MHz, 128 KB SRAM)
- **Serial stack:** DMA circular (1 KB) → ring buffers (8 KB RX + 8 KB TX, lock-free SPSC) → PicoC app state machine (REPL / LOAD / DRAIN modes)
- **Upload buffer:** 8 KB RAM-based
- **PicoC heap:** 64 KB (platform.h)
- **Error recovery:** `setjmp/longjmp` — script errors return to REPL cleanly

## Known Limitations

- Infinite loops (`while(1){}`) will hang the MCU — no cooperative abort mechanism (see [RICE v2](https://github.com/mosking128/rice-v2) for FreeRTOS-based abort support)
- `:ping` is unresponsive during script execution
- No power management / sleep modes enabled
- Minimal built-in C standard library

## Related Projects

- [RICE v2](https://github.com/mosking128/rice-v2) — FreeRTOS-based version with cooperative abort and task isolation

## License

MIT. See [LICENSE](LICENSE).
