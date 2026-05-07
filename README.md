# PicoC on STM32H750

[English](README.md) | [中文](README_CN.md)

本项目将 PicoC 移植到 STM32H750，在 `USART1` 上提供交互式 REPL、整文件上传执行和 Windows 上位机工具，目标是用尽量小的代价跑通 MCU 侧 C 脚本解释工作流，并沉淀一套可复用的 PicoC 嵌入式移植思路。

This project ports PicoC to STM32H750 and provides a `USART1`-based REPL, whole-file source upload/execution, and a Windows host tool for a practical embedded C scripting workflow.

## Features

- Interactive PicoC REPL over `USART1`
- File upload mode with `:load / :end / :abort`
- Multi-line source input and whole-file execution
- **Interactive debugger**: breakpoints, single-step, expression evaluation
- STM32H750 Keil MDK project ready for flashing
- Windows host tool for serial console, file upload, batch testing, and debugging

## Debugger

The host tool provides an interactive debugger with breakpoint, single-step, and expression evaluation support.

### Debug Toolbar

The debug toolbar is always visible at the bottom of the host tool window.

| Section | Controls | Description |
|---------|----------|-------------|
| Breakpoint | `行号` input + `设断点` / `清断点` | Set or clear a line breakpoint before uploading |
| Execution | `继续` / `单步` | Continue execution or step one statement |
| Expression | `表达式` input + `求值` | Evaluate a C expression in the current scope |

### Workflow

1. Connect the board and wait for `picoc> `.
2. Enter a line number (e.g. `5`) and click `设断点` — the console shows `设置断点: 第5行`.
3. Upload a `.c` file and execute it.
4. Execution pauses at the breakpoint — the toolbar label shows `已中断: 第5行`.
5. Click `单步` to step statement by statement, or enter an expression like `x + 1` and click `求值`.
6. Click `继续` to resume until the next breakpoint or file end.
7. Breakpoints are automatically cleared after each file execution.

### Current Limits

- Debugging currently targets uploaded source executed as `serial_load`.
- Breakpoint line numbers refer to the uploaded file content, not REPL history.
- `:eval` currently wraps the expression with `printf("%d\\n", (...))`, so integer-like expressions are the safest choice.
- Breakpoints are copied into the isolated file-run instance and cleared after that run finishes.

### Debug Protocol

The host and device communicate via structured UART protocol extensions:

| Direction | Command | Description |
|-----------|---------|-------------|
| Host → Device | `:bkpt <file> <line>` | Set breakpoint |
| Host → Device | `:bkptclear <file> <line>` | Clear breakpoint |
| Host → Device | `:cont` | Continue execution |
| Host → Device | `:step` | Single-step one statement |
| Host → Device | `:eval <expr>` | Evaluate expression |
| Device → Host | `:break <file> <line> <col>` | Breakpoint hit notification |
| Device → Host | `:step <file> <line> <col>` | Step break notification |

## Repository Layout

- [Core](Core): STM32 application code
- [picoc](picoc): upstream PicoC source plus STM32 platform port
- [MDK-ARM](MDK-ARM): Keil project files
- [tools/picoc_host/src](tools/picoc_host/src): host tool source code
- [tools/picoc_host/src/README.md](tools/picoc_host/src/README.md): host tool usage
- [docs/移植流程.md](docs/%E7%A7%BB%E6%A4%8D%E6%B5%81%E7%A8%8B.md): porting workflow notes
- [docs/调试使用说明.md](docs/%E8%B0%83%E8%AF%95%E4%BD%BF%E7%94%A8%E8%AF%B4%E6%98%8E.md): breakpoint and single-step guide

## Current Scope

- Script input comes from serial only
- No target-side filesystem loading
- Minimal built-in standard library first
- Focused on stable bring-up and practical interaction

## Firmware Quick Start

1. Open [MDK-ARM/UART_DMA_H750.uvprojx](MDK-ARM/UART_DMA_H750.uvprojx) in Keil MDK.
2. Build and flash the STM32H750 target.
3. Open `USART1` at `115200`.
4. After boot, wait for `picoc> `.

## Host Tool Quick Start

Source mode:

```powershell
cd tools/picoc_host
run_host.bat
```

Packaged mode:

- Build with `build_exe.bat`
- Deliver the whole `tools/picoc_host/release/PicoCHost` folder to other users

## Typical Workflow

### File Upload & Execution

1. Connect the board and open the host tool.
2. Wait for `picoc> `.
3. Add `.c` files to the list, select one, and click `执行选中`.
4. Source code is uploaded, executed, and results appear in the console.

### Debugging

1. Connect and set a breakpoint by entering a line number and clicking `设断点`.
2. Upload and execute a file — execution pauses at the breakpoint.
3. Use `单步`, `求值`, and `继续` to inspect program state.
4. Breakpoints are cleared automatically after each run.

## Notes

- The current board-side upload buffer is RAM-based.
- This repository keeps source and packaged host-tool output separated.
- Generated build artifacts and local packaging caches are ignored by Git.

## License

MIT. See [LICENSE](LICENSE).
