# RS-485 ↔ UART shield

<img src="images/02_shield_top.jpg" alt="Shield, top" width="280">

The shield replaces the legacy weighing board (BRD1) without touching the
stepper board (BRD2). It plugs into the connector that used to carry the
ribbon to BRD1. That ribbon already had UART between the two MCUs.

## What is on the board

Hand-wired on a green prototyping board. Visible from the top view:

- **MAX485-class RS-485 transceiver module** (the small blue sub-board,
  3.3 V I/O, 5 V bus, with built-in biasing resistors).
- **Small step-down DC-DC module** (the silver inductor + the "100" marking)
  providing the 3.3 V / 5 V split from the single power rail available on
  the stepper board's connector.
- **Screw terminals** for the RS-485 bus (A / B / GND and power).
- **2×5 IDC header** that fits the ribbon connector on the stepper board.
  The two conductors that were already UART TX / RX toward the weighing
  board are routed into the transceiver. No HX711 signals ever lived on
  this connector.

## Why those two pins

The weighing board and the stepper board already spoke UART over this
ribbon. After BRD1 was removed, those same pins were the natural place
to attach the RS-485 transceiver: the stepper PCB did not need a respin,
and plugging the old ribbon back in restores the previous link.

The two HX711 pins that also happened to be UART-capable were a different
experiment, on BRD1, while I was still trying to keep that board. That
attempt is described below. It is not how the shield is wired.

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

The first attempt kept BRD1. On its schematic, one of the four HX711 chips
was wired to two MCU pins that also had a UART alternate function. I
removed that HX711 and soldered the indicator leads to those pads. The
line ran, but with jitter, delay, and overfill: the other three HX711s
were still bit-banged in the blocking main loop. Removing BRD1 and moving
the link onto the existing UART ribbon, through this shield, is what made
the weighing chain operator-serviceable and bus-scalable. See
[`legacy.md`](legacy.md) for the "before" picture.
