# PicoC on STM32H750

本项目将 PicoC 移植到 STM32H750，在 `USART1` 上提供交互式 REPL、整文件上传执行和 Windows 上位机工具，目标是用尽量小的代价跑通 MCU 侧 C 脚本解释工作流。提供新的picoc嵌入式移植思路。

This project ports PicoC to STM32H750 and provides a `USART1`-based REPL, whole-file source upload/execution, and a Windows host tool for a practical embedded C scripting workflow.

## Features

- Interactive PicoC REPL over `USART1`
- File upload mode with `:load / :end / :abort`
- Multi-line source input and whole-file execution
- STM32H750 Keil MDK project ready for flashing
- Windows host tool for serial console, file upload, and batch testing

## Repository Layout

- [Core](Core): STM32 application code
- [picoc](picoc): upstream PicoC source plus STM32 platform port
- [MDK-ARM](MDK-ARM): Keil project files
- [tools/picoc_host/src](tools/picoc_host/src): host tool source code
- [tools/picoc_host/src/README.md](tools/picoc_host/src/README.md): host tool usage
- [docs/移植流程.md](docs/%E7%A7%BB%E6%A4%8D%E6%B5%81%E7%A8%8B.md): porting workflow notes

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

1. Connect the board and open the host tool.
2. Wait for `picoc> `.
3. For single-line interaction, send expressions or statements directly.
4. For whole-file execution, enter `:load`, send C source, then send `:end`.
5. After execution, the target stays in load mode and prints `ready for next file`.

## Notes

- The current board-side upload buffer is RAM-based.
- This repository keeps source and packaged host-tool output separated.
- Generated build artifacts and local packaging caches are ignored by Git.

## License

MIT. See [LICENSE](LICENSE).
