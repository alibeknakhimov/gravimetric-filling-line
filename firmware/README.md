# Firmware snapshot

This directory contains the firmware sources for the STM32F4 stepper /
fill controller. It is the closest snapshot of the production firmware
that I have local access to: see the "A note on the firmware snapshot"
section of the top-level [`README.md`](../README.md).

## Target

| | |
|---|---|
| MCU | STM32F407VG (STM32F4 Discovery board as the carrier) |
| Toolchain | STM32CubeIDE / arm-none-eabi-gcc |
| HAL | STM32F4 HAL (Cube) |
| Linker scripts | `STM32F407VGTX_FLASH.ld`, `STM32F407VGTX_RAM.ld` |
| Configurator | STM32CubeMX (`FitoF4.ioc`) |
| Original project origin | MDK-ARM (Keil), re-generated against CubeIDE early in this project |

## Peripherals in use

| Peripheral | Config | Purpose |
|---|---|---|
| TIM6 | prescaler 167, period 99 → ~10 kHz, update IRQ | motion tick: nozzle axes, pump pulsing, bottle counting, USB CDC background TX |
| EXTI line 5 | GPIO interrupt | drum-position event, decoupled from the TIM6 motion tick |
| USART2 | PA2 / PA3, 115 200 8N1, IT RX per byte | Modbus RTU master to the QLST weighing bus |
| USB_OTG_FS | PA11 / PA12, CDC class | HMI link to the Raspberry Pi (replaces long UART) |
| GPIO | ~30 lines on ports A/B/C/D/E | step/dir/enable to Darlington array, end-switches, operator buttons |

## How to build

Open `FitoF4.ioc` in STM32CubeIDE (`File → Open Projects from File System`),
let it generate the project files, and build with the included GCC
toolchain. The build has no external dependencies beyond the STM32F4 HAL
and the ST USB CDC device middleware, both vendored under `Drivers/` and
`Middlewares/`.

The following CubeMX vendor packs were trimmed from this snapshot because
the project does not use them (CubeMX will re-create any of them on demand
if `.ioc` is regenerated):

- `MDK-ARM/`: the original Keil project, migrated to CubeIDE early on;
- `Documents/`: vendor PDFs;
- `Drivers/CMSIS/Lib/`: ~69 MB of pre-compiled CMSIS-DSP libraries (DSP
  is not used);
- `Drivers/CMSIS/DSP/`, `NN/`, `RTOS/`, `RTOS2/`, `docs/`: unused.

## What is NOT in this snapshot

- The actually-deployed binary has a slightly different interrupt
  configuration (NVIC priorities and the split between the TIM6 motion
  tick and the drum-position event): see the top-level README note.
  Business logic (Modbus master, USB CDC HMI, fill algorithm with
  soft-start and nozzle tracking, drum/nozzle/pump motion) is identical.
- The legacy weighing-board firmware (STM32F1 + 4× HX711) and the legacy
  stepper-board firmware are the previous engineer's property and are
  **not** reproduced in this repository. The architectural "before"
  picture lives in [`../docs/legacy.md`](../docs/legacy.md) and
  [`../docs/architecture.md`](../docs/architecture.md).

## Where to start reading the code

| File | What it holds |
|---|---|
| `Core/Src/main.c` | everything application-level: Modbus master state machine, fill algorithm, USB CDC HMI, operator-tare, TIM6 motion ISR body (`IRQ_htim6`) |
| `Core/Src/stm32f4xx_it.c` | the actual IRQ vectors (`TIM6_DAC_IRQHandler`, `USART2_IRQHandler`, `EXTI9_5_IRQHandler`) and their routing into `main.c` |
| `FitoF4.ioc` | the CubeMX configuration: authoritative for pin mapping, NVIC priorities, clock tree |
| `USB_DEVICE/` | ST's standard CDC device template, unmodified business logic (the sending path from main.c lives in `usb_mass_poll()`) |

Entry points inside `main.c` (approximate line ranges):

- **Modbus master:** `modbus_process()`, `modbus_send_request()`,
  `Modbus_CRC16()`, UART IT callbacks `HAL_UART_RxCpltCallback` and
  `HAL_UART_ErrorCallback`.
- **Operator tare & cycle framing:** `tare_all_scales()`,
  `set_tare_active_scales()`, `cancel_tare_all_scales()`.
- **Fill algorithm:** `update_fill_channel()`, `fill_channel_force_stop()`,
  `calculate_speeds()`, plus the fill-mode section at the bottom of the
  main loop (`rezhim == 3` branch).
- **Motion:** `IRQ_htim6()`: all stepper pulse generation and axis state
  logic.
- **USB CDC HMI:** `usb_mass_request_send()`, `usb_mass_poll()`,
  `usb_append_int()`, plus the TIM6 call in the ISR body.
