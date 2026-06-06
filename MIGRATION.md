# RICE v1 — Porting Workflow

Complete guide for porting RICE v1 (PicoC embedded C interpreter, bare-metal architecture) to different MCU platforms.

---

## 1. Architecture Layers

```
┌───────────────────────────────────────────────────────────────────┐
│  Layer 1: PicoC Interpreter Core (fully reusable, no changes)     │
│  Files: picoc/*.c (lex.c, parse.c, expression.c, heap.c,        │
│         table.c, type.c, variable.c, clibrary.c, include.c,      │
│         debug.c, platform.c)                                      │
│  Note: Pure C logic, no hardware API dependencies                 │
├───────────────────────────────────────────────────────────────────┤
│  Layer 2: Platform Adaptation (porting focus: rewrite per chip)   │
│  Files: picoc/platform/platform_*.c — PicoC I/O bridge            │
│         picoc/platform/library_*.c  — Peripheral function bindings│
│         picoc/platform.h            — Heap size, feature flags    │
│         picoc/picoc.h               — Platform macro guard        │
│         picoc/interpreter.h         — Picoc_Struct sizing         │
├───────────────────────────────────────────────────────────────────┤
│  Layer 3: Application Layer (minor adaptation)                    │
│  Files: Core/Src/picoc_app.c   — REPL state machine (100% reuse) │
│         Core/Src/serial_app.c  — Serial DMA driver (~80% reuse)  │
├───────────────────────────────────────────────────────────────────┤
│  Layer 4: HAL Driver Layer (CubeMX-generated, full rewrite)       │
│  Files: main.c, usart.c, dma.c, gpio.c, stm32h7xx_it.c,        │
│         stm32h7xx_hal_msp.c, stm32h7xx_hal_conf.h,              │
│         system_stm32h7xx.c, startup_stm32h750xx.s                │
└───────────────────────────────────────────────────────────────────┘
```

**v1 vs v2 key difference:** v1 is bare-metal super-loop architecture with no FreeRTOS. The main loop is `while(1) { PicocApp_Task(); }` — serial reading and PicoC execution run in the same context. There is no inter-task communication and no cooperative abort (`:abort` only works via `setjmp`/`longjmp` from within the debugger).

---

## 2. File Dependency Matrix

### Files that MUST be fully rewritten (CubeMX-generated)

| File | Lines | Reason for rewrite |
|------|-------|--------------------|
| `main.c` | ~250 | Clock tree (HSE 25MHz → PLL → 480MHz), MPU, power config, HAL callback routing |
| `usart.c` | ~190 | UART peripheral init, DMA stream assignment, GPIO AF mapping, NVIC priorities |
| `dma.c` | ~58 | DMA clock enable, IRQ priority setup |
| `gpio.c` | ~66 | GPIO clock enable, pin config |
| `stm32h7xx_it.c` | ~240 | Interrupt vector names (DMA1_Stream0_IRQHandler, USART1_IRQHandler) |
| `stm32h7xx_hal_msp.c` | ~83 | Global MSP init |
| `stm32h7xx_hal_conf.h` | ~515 | HAL module selection, HSE_VALUE, VDD_VALUE |
| `system_stm32h7xx.c` | ~450 | CMSIS system init |
| Startup file | — | Vector table |
| Linker script | — | Memory region definitions |

### Files that need minor modifications

| File | What to change | What to keep |
|------|---------------|-------------|
| `serial_app.c` | `huart1` → new UART handle; `USART1` → new peripheral instance; `HAL_UARTEx_ReceiveToIdle_DMA` / `HAL_UART_Transmit_DMA` → new HAL API | Ring buffer logic, `SerialApp_Read`/`SerialApp_Write`, `SerialApp_ProcessRxDma`, `SerialApp_RxRingWrite`, `SerialApp_TxDmaKick` |
| `serial_app.h` | `UART_HandleTypeDef` type (if switching away from STM32 HAL) | Public API declarations |
| `picoc/platform.h` | Add `#ifdef YOUR_MCU_HOST` block, set `HEAP_SIZE` | Hash table sizes, prompt strings |
| `picoc/picoc.h` | Add `defined(YOUR_MCU_HOST)` to `#if` guard | Public API prototypes |

### Fully reusable files (zero modifications)

| File | Lines | Description |
|------|-------|-------------|
| `picoc_app.c` | ~850 | REPL state machine, protocol parsing (100% reuse in v1, no RTOS dependency) |
| `picoc_app.h` | ~50 | Application layer API declarations |
| `picoc/debug.c` | ~783 | Debugger (breakpoint hash table, step, eval, variable inspection) |
| `picoc/parse.c` | ~1000 | Parser (v1 uses setjmp/longjmp, no AbortRequested checkpoints) |
| `picoc/expression.c` | ~2000 | Expression evaluator |
| `picoc/lex.c` | ~800 | Lexer |
| `picoc/heap.c` | ~200 | Memory allocator |
| `picoc/table.c` | ~300 | Hash table implementation |
| `picoc/type.c` | ~500 | Type system |
| `picoc/variable.c` | ~400 | Variable management |
| `picoc/clibrary.c` | ~200 | C standard library init |
| `picoc/platform.c` | ~100 | Platform-agnostic wrappers |
| `picoc/include.c` | ~200 | `#include` system |
| `picoc/cstdlib/*.c` | 9 files | Mini C stdlib |

---

## 3. Step-by-Step Porting Guide

### Step 1: CubeMX Project Setup

Create a CubeMX project for the target MCU with these peripherals:

**UART Configuration:**
- Mode: Asynchronous
- Baud: 115200, 8N1, no hardware flow control
- Pins: Select available TX/RX pins
- GPIO speed: Very High

**DMA Configuration (critical):**
- RX DMA: Circular mode, byte alignment, high priority, FIFO disabled
- TX DMA: Normal mode, byte alignment, medium priority, FIFO disabled
- Must support DMA circular receive (otherwise must switch to interrupt mode)

**NVIC Configuration (v1 specifics):**
- DMA RX/TX interrupt priority: 0 (highest)
- USART interrupt priority: 0 (highest)
- SysTick interrupt priority: 15 (lowest)
- **Note:** v1 bare-metal has no FreeRTOS, so interrupt priorities can be set to highest (0). No FromISR API calls needed.

**Clock Configuration:**
- HSE: Configure external crystal frequency (current 25 MHz)
- PLL: Calculate target SYSCLK (current 480 MHz)

### Step 2: Serial Driver Adaptation (serial_app.c)

**3 hardware call sites to modify:**

```c
// 1. DMA receive start
SerialApp_StartRxDma():
  HAL_UARTEx_ReceiveToIdle_DMA(&huart1, rx_dma_buffer, 1024)
  // Change to new UART handle

// 2. DMA transmit start
SerialApp_TxDmaKick():
  HAL_UART_Transmit_DMA(&huart1, &tx_ring[tx_tail], len)
  // Change to new UART handle

// 3. Peripheral instance filter
SerialApp_RxEventCallback():
  if (huart->Instance == USART1)  // Change to new peripheral instance
  if (huart->Instance == USART1)  // Same in TX complete callback
```

**What NOT to modify:**
- Ring buffer arrays (rx_ring[8192], tx_ring[8192], rx_dma_buffer[1024])
- `SerialApp_Read()` / `SerialApp_Write()` — pure buffer operations
- `SerialApp_ProcessRxDma()` — DMA-to-ring copy with wrap-around handling
- `SerialApp_RxRingWrite()` — ring buffer write
- `SerialApp_TxDmaKick()` — TX DMA trigger logic

**Non-STM32 serial adaptation:**
1. Implement equivalent DMA circular receive (or switch to interrupt receive + callback filling rx_ring)
2. Implement equivalent DMA transmit (or switch to interrupt transmit + reading from tx_ring)
3. Copy data from DMA buffer to rx_ring in the receive completion callback

### Step 3: Platform I/O Adaptation (platform_*.c)

Create `picoc/platform/platform_your_mcu.c` implementing these 8 functions:

```c
// Must implement 2 low-level I/O primitives:
int PicocApp_ConsoleGetCharBlocking(void);  // Blocking single-char read
// Implementation: loop calling SerialApp_Read(), simple busy-wait when no data

void SerialApp_Write(const uint8_t *data, uint32_t len);
// Already implemented in serial_app.c, no rewrite needed

// The following can be copied directly from existing platform_stm32h7.c:
char *PlatformGetLineQuiet(char *Buf, int MaxLen);
char *PlatformGetLine(char *Buf, int MaxLen, const char *Prompt);
int PlatformGetCharacter(void);
void PlatformPutc(unsigned char ch, union OutputStreamInfo *Stream);
void PlatformInit(Picoc *pc);       // Empty function
void PlatformCleanup(Picoc *pc);    // Empty function
void PlatformExit(Picoc *pc, int RetVal);  // longjmp
char *PlatformReadFile(Picoc *pc, const char *FileName);  // Returns error
void PicocPlatformScanFile(Picoc *pc, const char *FileName);  // Returns error
```

**v1-specific: PicocApp_ConsoleGetCharBlocking implementation (no RTOS):**
```c
int PicocApp_ConsoleGetCharBlocking(void)
{
    uint8_t ch;
    while (SerialApp_Read(&ch, 1U) == 0U)
    {
        // v1 bare-metal: simple busy-wait, no osDelay
        // Optional: __WFI() for power saving
    }
    return (int)ch;
}
```

**Difference from v2:** The v2 version includes `AbortRequested` check and `osDelay(1)` to yield CPU. The v1 version has neither because there is no RTOS or cooperative abort mechanism.

### Step 4: Peripheral Function Bindings (library_*.c)

Create `picoc/platform/library_your_mcu.c` to expose target MCU peripheral functions to PicoC scripts.

**Three-step registration:**

```c
// Step 1: Write wrapper function
static void PicocHalDelay(struct ParseState *Parser,
                           struct Value *ReturnValue,
                           struct Value **Param, int NumArgs)
{
    (void)Parser; (void)ReturnValue; (void)NumArgs;
    HAL_Delay((uint32_t)Param[0]->Val->Integer);
}

// Step 2: Declare function prototype
const char HalDelay[] = "void delay(int ms);";

// Step 3: Add to registration table
const LibraryFunction Stm32Functions[] = {
    { PicocHalDelay, HalDelay },
    { NULL, NULL }
};

void PlatformLibraryInit(Picoc *pc)
{
    IncludeRegister(pc, "stm32.h", &Stm32Defs, &Stm32Functions[0], NULL);
}
```

**Recommended peripheral bindings:**

| Peripheral | Wrapper example | PicoC prototype |
|-----------|----------------|-----------------|
| GPIO | `PicocHalGpioWritePin` | `void digitalWrite(void *port, int pin, int state)` |
| GPIO | `PicocHalGpioReadPin` | `int digitalRead(void *port, int pin)` |
| GPIO | `PicocHalGpioInit` | `void pinMode(void *port, int pin, int mode, int pull, int speed)` |
| Delay | `PicocHalDelay` | `void delay(int ms)` |
| ADC | `PicocAnalogRead` | `int analogRead(int channel)` |
| PWM | `PicocAnalogWrite` | `void analogWrite(int pin, int duty)` |

**Constants to register:**
- GPIO port addresses (GPIOA ~ GPIOn)
- GPIO pin numbers (GPIO_PIN_0 ~ GPIO_PIN_15)
- GPIO modes, pull, speed constants

### Step 5: PicoC Configuration (platform.h)

Add a platform configuration block in `picoc/platform.h`:

```c
#ifdef YOUR_MCU_HOST
# define BUILTIN_MINI_STDLIB        // Use PicoC's built-in mini stdlib
# define HEAP_SIZE (64*1024)        // PicoC heap size (adjust per target RAM)
# define PICOC_MATH_LIBRARY         // Enable math functions
# define FEATURE_AUTO_DECLARE_VARIABLES  // Allow implicit int declarations
# define FANCY_ERROR_MESSAGES       // Detailed error messages
#endif
```

**HEAP_SIZE adjustment guide:**

| Target MCU SRAM | Recommended HEAP_SIZE | Notes |
|----------------|----------------------|-------|
| ≥ 256 KB | 64 KB | Current H750 config |
| 128~256 KB | 32 KB | Smaller heap, features mostly intact |
| 64~128 KB | 16 KB | Minimum usable, complex scripts may fail |
| < 64 KB | Not recommended | PicoC itself needs substantial memory |

### Step 6: Main Loop Adaptation (main.c)

v1's main loop is very simple:

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
        PicocApp_Task();  // Poll serial + execute PicoC
    }
}
```

**When porting, only change:**
1. `SystemClock_Config()` — full rewrite (clock tree, PLL, voltage config)
2. `MX_USART1_UART_Init()` — CubeMX regenerate
3. `MX_DMA_Init()` — CubeMX regenerate
4. `MX_GPIO_Init()` — CubeMX regenerate
5. ISR callback routing (keep in main.c):
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

### Step 7: Build Configuration

**Keil MDK:**

Preprocessor defines:
```
USE_PWR_LDO_SUPPLY    // STM32 power mode
USE_HAL_DRIVER        // Enable HAL library
STM32H750xx           // Chip model macro (change to target)
YOUR_MCU_HOST         // PicoC platform selection macro
```

Include paths:
```
../Core/Inc
../Drivers/<target_series>_HAL_Driver/Inc
../Drivers/CMSIS/Device/ST/<target_series>/Include
../Drivers/CMSIS/Include
../picoc
../picoc/platform
../picoc/cstdlib
```

Source groups (6 groups):
1. **Startup** — startup_<chip>.s
2. **App/Core** — serial_app.c, picoc_app.c, main.c, gpio.c, dma.c, usart.c, stm32<series>_it.c, stm32<series>_hal_msp.c
3. **App/PicoC** — clibrary.c, debug.c, expression.c, heap.c, include.c, lex.c, parse.c, platform.c, table.c, type.c, variable.c, platform_<chip>.c, library_<chip>.c
4. **HAL Driver** — hal_uart, hal_dma, hal_gpio, hal_rcc, hal_flash, hal_pwr, hal_cortex, hal_exti, hal_i2c, hal_tim, etc.
5. **CMSIS** — system_<series>.c
6. **PicoC/cstdlib** — ctype.c, errno.c, math.c, stdbool.c, stdio.c, stdlib.c, string.c, time.c, unistd.c

### Step 8: Memory Budget Verification

**v1 Memory Budget (no FreeRTOS):**

| Consumer | Size | Source |
|----------|------|--------|
| PicoC heap (`HeapMemory`) | 64 KB | Static array in `Picoc_Struct` |
| PicoC interpreter stack | 64 KB | Allocated from C heap at runtime (`PICOC_APP_STACK_SIZE`) |
| RX DMA buffer | 1 KB | Static in `serial_app.c` |
| RX ring buffer | 8 KB | Static in `serial_app.c` |
| TX ring buffer | 8 KB | Static in `serial_app.c` |
| Source line buffer | 2 KB | Static in `picoc_app.c` |
| Load buffer | 8 KB | Static in `picoc_app.c` |
| Keil configured heap | 128 KB | C library heap (used for PicoC interpreter stack allocation) |
| Keil configured stack | 8 KB | C stack |
| **Total** | **~160 KB** | |

**Minimum RAM required: 192 KB**

---

## 4. v1 vs v2 Architecture Differences

| Aspect | v1 (bare-metal) | v2 (FreeRTOS) |
|--------|-----------------|----------------|
| Main loop | `while(1) { PicocApp_Task(); }` | FreeRTOS task scheduling |
| Serial reading | `PicocApp_Task()` directly calls `SerialApp_Read()` | serialTask calls, sends via queue to picocTask |
| Script execution | Same context, blocks everything | Isolated picocTask, doesn't block serial |
| Abort script | Not supported (`while(1)` hangs MCU) | Supported (cooperative `AbortRequested` flag) |
| Heartbeat | Unresponsive during script execution | Always responds (separate task handles) |
| Debug input | `DebugCheckStatement` reads serial directly | `g_debug_input_active` flag coordinates two tasks |
| NVIC priorities | 0 (highest) | 5 (can call FreeRTOS FromISR APIs) |
| Interrupt vectors | Includes SVC/PendSV/SysTick | SVC/PendSV/SysTick handled by FreeRTOS port |
| Memory management | No RTOS heap | FreeRTOS heap_4 (48 KB) |
| HAL timebase | SysTick | TIM6 (SysTick reserved for FreeRTOS) |
| Porting complexity | Low | Medium |

---

## 5. MCU Family Porting Reference

### Between STM32 Families

| Source | Target | Effort | Key differences |
|--------|--------|--------|-----------------|
| STM32H750 | STM32H743 | Minimal | Same family, change model macro |
| STM32H750 | STM32F407 | Medium | Different DMA controller, no FIFO threshold, different clock tree, no MPU |
| STM32H750 | STM32F103 | Large | No DMA circular receive (must use interrupt mode), no `HAL_UARTEx_ReceiveToIdle_DMA` |
| STM32H750 | STM32G0B1 | Medium | Simpler DMA, no MPU, different clock tree |
| STM32H750 | STM32L476 | Medium | Low-power series, different DMA controller |

### Cross-Vendor Porting

| Vendor | Recommended approach | Serial adaptation strategy |
|--------|---------------------|---------------------------|
| NXP (LPC/IMXRT) | MCUXpresso + LPUART + eDMA | Rewrite serial_app.c HAL calls, reuse ring buffers |
| Nuvoton (M480) | NuMaker + UART + PDMA | HAL style similar to STM32, minimal changes |
| RP2040 | Pico SDK + UART + DMA | Simple DMA API |
| ESP32 | ESP-IDF + UART | Consider using v2 (native FreeRTOS support) |
| GD32 | GD32 HAL + USART + DMA | Highly compatible with STM32 HAL |

---

## 6. Porting Verification Checklist

| Step | What to verify | Test method | Expected result |
|------|---------------|-------------|-----------------|
| 1 | Basic serial communication | `SerialApp_Read` + `SerialApp_Write` echo loop | Input echoed back |
| 2 | REPL mode | Enter `printf("hello\n");` | Output `hello` |
| 3 | Expression evaluation | Enter `3 + 5 * 2` | Output `13` |
| 4 | Multi-line input | Enter `for` loop | Auto-detect completeness, execute |
| 5 | File upload | `:load` → source → `:end` | Execute and output results |
| 6 | Heartbeat | `:ping` | Receive `:pong` |
| 7 | Reset | `:reset` | Receive `:ok` |
| 8 | Set breakpoint | `:bkpt serial_load 5` | Receive `:ok bkpt` |
| 9 | Breakpoint hit | Upload script with breakpoint, execute | Receive `:break serial_load 5 0` |
| 10 | Single-step | `:step` | Receive `:step serial_load 6 0` |
| 11 | Continue | `:cont` | Continue to next breakpoint or end |
| 12 | Expression eval (debug) | `:eval x + 1` | Output evaluation result |
| 13 | Variable inspection | `:vars` | Receive `:var` list and `:ok vars` |
| 14 | Variable modification | `:set x 100` | Receive `:ok set` |
| 15 | Multiple breakpoints | Set 3 breakpoints, hit each | All pause and continue correctly |
| 16 | Peripheral bindings | Call `delay(100)` in PicoC script | Normal delay |
| 17 | Stress test | Upload 68+ PicoC test cases | All pass |

---

## 7. FAQ

**Q: Compile error `undefined reference to PlatformXxx`**
A: Platform adaptation functions not implemented. Check that `platform_your_mcu.c` contains all 8 required functions.

**Q: Serial can receive but not transmit**
A: Check DMA TX configuration, verify `HAL_UART_TxCpltCallback` routes to `SerialApp_TxCpltCallback`, confirm NVIC interrupt is enabled.

**Q: REPL unresponsive, no `picoc>` prompt**
A: Check `main()` call order: `SerialApp_Init()` → `PicocApp_Init()` → `while(1) { PicocApp_Task(); }`.

**Q: No output after file upload**
A: Check that the source code defines a `main()` function. PicoC auto-calls `main()`.

**Q: Breakpoints don't work**
A: 1) Confirm breakpoint filename matches upload filename (default `serial_load`). 2) Confirm `DebugCheckStatement()` is called before every statement.

**Q: Stack overflow (HardFault)**
A: 1) Increase Keil project Heap Size (current 128 KB). 2) Increase Stack Size (current 8 KB). 3) Decrease `HEAP_SIZE` (current 64 KB). 4) Check target MCU total SRAM ≥ 192 KB.

**Q: Compile error `jmp_buf` undefined**
A: Confirm compiler supports `<setjmp.h>`. Confirm platform macro guard in `picoc/picoc.h` includes your target macro.

**Q: `:abort` doesn't work**
A: In v1 bare-metal, `:abort` only works during script execution via `setjmp`/`longjmp`. If the script is stuck in `while(1){}` without calling any PicoC interpreter functions, `:abort` cannot interrupt (this is why v2 exists).

**Q: `:ping` unresponsive during script execution**
A: This is a known v1 limitation. During script execution, the main loop is blocked and cannot process new commands. See [RICE v2](https://github.com/mosking128/rice-v2) for FreeRTOS task isolation support.
