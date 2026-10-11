# Architecture: before and after

Both diagrams describe the machine from the electrical and firmware side.
The drum, nozzle axes, pumps, and load cells are drawn in grey. The fifth
column was added during this retrofit, so the "before" picture has four
stations and the "after" picture has five.

## Before: 2021 system

```mermaid
flowchart LR
    subgraph MECH[Mechanics - unchanged]
        DRUM[Rotating bottle drum]
        NZ[4 x Nozzle vertical axes]
        PMP[4 x Peristaltic pumps]
        LC[4 x Load cells]
    end

    subgraph BRD1[Weighing board - STM32F1]
        HX["4 x HX711 ADC, bit-banged GPIO, busy-wait on DRDY"]
        LCD["16x2 I2C LCD + 3 buttons<br/>on-board calibration menu"]
        FLASH["Internal flash: density and calibration gains"]
    end

    subgraph BRD2[Stepper board - STM32F4]
        IRQ["TIM6 ISR @ 10 kHz<br/>drum + nozzles + pumps + I/O<br/>everything in one interrupt"]
        DRV["Industrial step/dir drivers<br/>via Darlington array"]
    end

    PI[Raspberry Pi HMI]

    LC --> HX
    HX --> BRD1
    BRD1 -->|UART on the ribbon| BRD2
    BRD2 --> DRV
    DRV --> DRUM
    DRV --> NZ
    DRV --> PMP
    PI -->|long UART cable| BRD2

    classDef mech fill:#eee,stroke:#999,color:#333;
    class MECH,DRUM,NZ,PMP,LC mech;
```

Pain points this hardware configuration created:

1. **HX711 polling blocks everything on BRD1.** Each `Hx_Read_N()` is a GPIO
   bit-bang with a busy-wait on DRDY and a 1-million-iteration safety loop.
   Four of these in a row inside `while(1)`, plus a trailing `HAL_Delay(100)`,
   gives an effective sample rate in the single-digit Hz range.
2. **The ribbon was already UART, not a scale bus.** The two boards talked
   over UART on that cable. Adding a scale still meant another HX711 and
   another blocking read on BRD1. The shield later reused those same UART pins.
3. **Calibration hidden inside the enclosure.** The LCD and buttons sat on
   BRD1, which was mounted behind panels. Re-calibration meant opening the
   machine.
4. **BRD2 ISR did everything.** Drum stepping, nozzle up/down, pump pulses
   and input polling all shared one 10 kHz TIM6 interrupt. There was no room
   to add non-trivial sensing without hurting drum motion.

## After: current system

```mermaid
flowchart LR
    subgraph MECH[Mechanics - unchanged]
        DRUM[Rotating bottle drum]
        NZ[5 x Nozzle vertical axes]
        PMP[5 x Peristaltic pumps]
        LC[5 x Load cells]
    end

    subgraph QLST["5 x QLST weighing indicators<br/>Modbus RTU slaves, addr 1..5<br/>built-in filter, operator buttons + display"]
        Q1[QLST #1]
        Q2[QLST #2]
        Q3[QLST #3]
        Q4[QLST #4]
        Q5[QLST #5]
    end

    subgraph SHIELD["RS-485 to UART shield<br/>plugs into BRD2 original connector"]
        X485[MAX485-class transceiver]
    end

    subgraph BRD2["Stepper board - STM32F4 - firmware rewritten"]
        TIM["TIM6 ISR @ 10 kHz<br/>nozzles + pumps<br/>(motion tick only)"]
        EXTI["EXTI line 5 ISR<br/>drum-position event<br/>decoupled from TIM6"]
        MB["Modbus RTU master<br/>state machine from main loop<br/>HAL_UART_Receive_IT, CRC16"]
        FA["Fill algorithm:<br/>soft-start / fast / slow / drip tail<br/>+ nozzle-follows-mass"]
        CDC["USB CDC device<br/>HMI link"]
        DRV["Industrial step/dir drivers<br/>via Darlington array (unchanged)"]
    end

    PI[Raspberry Pi HMI]

    LC --> QLST
    Q1 ---|2-wire RS-485 bus, 115200 bps| X485
    Q2 --- X485
    Q3 --- X485
    Q4 --- X485
    Q5 --- X485
    X485 -->|USART2 on the original UART ribbon pins| MB
    MB --> FA
    FA --> TIM
    TIM --> DRV
    DRV --> DRUM
    DRV --> NZ
    DRV --> PMP
    PI -->|USB cable| CDC

    classDef mech fill:#eee,stroke:#999,color:#333;
    class MECH,DRUM,NZ,PMP,LC mech;
```

What the new picture is doing differently:

1. **Weighing is now a bus device, not a bit-banged peripheral.** QLST
   indicators run their own filter at ~40 Hz, timestamp-stable samples arrive
   over Modbus RTU; the stepper MCU just asks and waits with a timeout.
2. **BRD1 is gone.** One less PCB, one less MCU, one less failure point. The
   calibration UI is now on each QLST unit, which the plant operator can
   handle without opening the machine.
3. **Reversible retrofit.** The shield plugs into the existing BRD2 connector
   whose ribbon used to carry UART to the weighing board. Pull the shield,
   plug the old BRD1 ribbon back in: the machine is in the previous configuration.
4. **ISRs are specialised.** TIM6 drives the periodic motion tick (nozzle
   axes, pump pulsing, bottle counting). The drum-position event has been
   moved onto its own EXTI line, so it is no longer polled inside TIM6 at
   10 kHz and no longer competes with the motion tick. Modbus RX runs in the
   USART2 IRQ byte-by-byte, and fill-state logic plus USB CDC run from the
   main loop.

## MCU resources (current firmware)

| Peripheral | Config | Purpose |
|---|---|---|
| TIM6 | prescaler 167, period 99 → 10 kHz, update IRQ | motion tick: nozzle axes, pump pulsing, bottle counting |
| EXTI line 5 | GPIO interrupt | drum-related event, decoupled from the TIM6 motion tick |
| USART2 (PA2/PA3) | 115 200 8N1, async, RX IT per byte | Modbus RTU master link to QLST bus |
| USB_OTG_FS (PA11/PA12) | USB device, CDC class | HMI link to Raspberry Pi (replaces long UART) |
| GPIO | ~30 lines on A/B/C/D/E | step/dir/enable to Darlington array, end-switches, operator buttons |

NVIC: `TIM6_DAC_IRQn`, `USART2_IRQn` and `EXTI9_5_IRQn` are all at preemption
0 / subpriority 0 in the current build. Interaction between the motion tick,
the drum event and the Modbus RX is handled by the firmware state machines
instead of hardware preemption. This is called out in the "what I'd do
differently" section of the main README.
