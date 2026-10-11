# RS-485 ↔ UART shield

![Shield, top](images/02_shield_top.jpg)

The shield replaces the entire legacy weighing board (BRD1) without
touching the stepper board (BRD2). It plugs into the exact connector that
used to carry the ribbon from the old HX711 PCB.

## What is on the board

Hand-wired on a green prototyping board. Visible from the top view:

- **MAX485-class RS-485 transceiver module** (the small blue sub-board,
  3.3 V I/O, 5 V bus, with built-in biasing resistors).
- **Small step-down DC-DC module** (the silver inductor + the "100" marking)
  providing the 3.3 V / 5 V split from the single power rail available on
  the stepper board's connector.
- **Screw terminals** for the RS-485 bus (A / B / GND and power).
- **2×5 IDC header** that fits the ribbon cable connector that previously
  carried HX711 CLK / DATA / power to the old weighing board. Two of those
  pins — the ones that used to be HX711 CLK and DATA on one of the five
  channels — are now routed through the shield as UART TX / RX from the
  stepper MCU.

## Why those two pins

Looking at the legacy board schematic shared by the previous engineer, the
MCU side of the HX711 CLK / DATA pair on channel 5 happened to be a pair
of GPIOs with alternate-function UART capability on the STM32F4 stepper
board. That meant I could re-map those pins in the stepper firmware from
GPIO to USART2, run the ribbon cable through the shield, and feed the same
two conductors into the MAX485 transceiver — all without changing a single
trace on the stepper board.

The side effect: pull the shield, plug the original HX711-side ribbon back
in, revert the firmware, and the machine is in the previous configuration.
That reversibility was a soft requirement from the plant.

## Signal flow

```
QLST slaves 1..5   ───(twisted pair + drain)───   RS-485 bus
                                                     │
                                                   A, B
                                                     │
                                 ┌───────────────────┴─────────────┐
                                 │     MAX485-class transceiver     │
                                 │                                   │
                                 │   RO ──→ UART RX of USART2       │
                                 │   DI ←── UART TX of USART2       │
                                 │   DE/RE   (see note below)       │
                                 └───────────────────┬─────────────┘
                                                     │
                              ┌──────────────────────┴─────────────────────┐
                              │ 2×5 IDC header → original BRD2 connector   │
                              │ (TX / RX pins reused; power/GND from BRD2) │
                              └────────────────────────────────────────────┘
```

## Direction control (DE / RE)

The transceiver sub-board is one of the common MAX485 modules with auto-
flow-direction (DE/RE tied together through its own TXD-controlled logic).
On the master side there is only one speaker at a time (we send a request,
we receive a response), and the master's own TX activity drives the
direction. In practice it has been reliable at 115 200 bps on a short
(~80 cm) run.

For the next hardware revision, driving DE/RE directly from a GPIO under
firmware control (set DE around `HAL_UART_Transmit`, clear it right after
TC flag) is the robust solution and will tolerate longer buses and more
slaves without edge-case glitches.

## EMI and ground

The initial deployment reused the original loom and shared ground over the
80 cm run back to the stepper board. After two episodes of intermittent
Modbus errors in service (see [`field-notes.md`](field-notes.md)) the loom
was rebuilt with:

- a **twisted pair** for A / B;
- the drain wire bonded to the **PG** pin on each QLST indicator;
- the shield ground and all QLST grounds commoned at the shield board
  (single-point reference);
- **ferrules** on every terminal strip;
- **snap-off plug connectors** on each column so the plant electrician can
  disconnect a station for service without screwdrivers.

What would be done differently on a next revision:

- Replace the shared ground with an **isolated DC-DC** on the RS-485 side
  and a true reference-ground scheme (ISO3082 or similar galvanic-isolated
  transceiver). I knew this at the time; the isolated converter was not
  procurable locally within the project schedule, so I lived with the
  shared ground and controlled it with the twisted pair + drain bond.
- Move to a proper manufactured 2-layer PCB with a ground plane under the
  transceiver.
- Fuse the bus-side power separately from the MCU-side logic.

## Why the shield exists at all

The alternative was to keep the legacy weighing MCU and bolt Modbus onto
it. I actually tried that first: identified an existing HX711 CLK / DATA
pair on the legacy board that mapped to UART-capable pins, unsoldered the
HX711 and ran UART through those pads to the first indicator. The machine
worked, but it inherited the legacy MCU's blocking main loop and its
calibration menu that lived inside the enclosure. The shield + removal of
BRD1 was the fix that let the whole weighing chain become operator-serviceable
and bus-scalable. See [`legacy.md`](legacy.md) for the "before" picture.
