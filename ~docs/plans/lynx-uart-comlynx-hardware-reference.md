# Atari Lynx UART / ComLynx Serial Port — Hardware Reference

> **Purpose**: Comprehensive technical reference for implementing the Lynx UART/ComLynx
> serial port in the Nexen emulator. Synthesized from the Epyx/Atari hardware documentation
> (monlynx.de), Handy emulator source (bspruck/handy-fork), and Mednafen/Beetle Lynx
> (libretro/beetle-lynx-libretro).

---

## 1. Architecture Overview

The Lynx UART is part of the **Mikey** custom ASIC (address space `$FD00`–`$FDFF`). It provides
a single-wire half-duplex serial link called **ComLynx**, designed for daisy-chaining up to 18
Lynx units via 3.5 mm stereo jacks.

### Key characteristics

| Property              | Value                                    |
|-----------------------|------------------------------------------|
| Wire format           | 11-bit frames (1 start + 8 data + 1 parity/mark + 1 stop) |
| Baud rate source      | Timer 4 output (programmable via TIM4BKUP + TIM4CTLA)     |
| Bus topology          | Open-collector wired-OR (active low)     |
| Self-loopback         | Mandatory — TX always echoes to RX       |
| Max devices           | 18 (practical, depends on cable loading) |
| Interrupt line        | Timer 4 / serial interrupt (INTSET bit 4 = `$10`) |
| IRQ behaviour         | **Level-sensitive** (not edge-triggered) — hardware bug, see §9 |

### Register map

| Address | Name    | R/W    | Description                      |
|---------|---------|--------|----------------------------------|
| `$FD8C` | SERCTL  | R/W    | Serial Control (write) / Status (read) |
| `$FD8D` | SERDAT  | R/W    | Serial Data Tx (write) / Rx (read)     |
| `$FD90` | SDONEACK| W      | Suzy Done Acknowledge (unrelated, but adjacent) |
| `$FD10` | TIM4BKUP| R/W    | Timer 4 backup value (baud rate period) |
| `$FD11` | TIM4CTLA| R/W    | Timer 4 control A (clock source, enables) |
| `$FD12` | TIM4CNT | R/W    | Timer 4 current count             |
| `$FD13` | TIM4CTLB| R/W    | Timer 4 control B (dynamic status) |
| `$FD9C` | Mtest0  | W      | Test register — bit 4 = UARTturbo (1 MBd mode) |

---

## 2. SERCTL Register — Write (`$FD8C`)

Writing to `$FD8C` configures the UART. This is a **write-only** view; reading the same
address returns the status register (§3).

```
Bit 7   Bit 6   Bit 5   Bit 4   Bit 3     Bit 2    Bit 1   Bit 0
TXINTEN RXINTEN (rsvd)  PAREN   RESETERR  TXOPEN   TXBRK   PAREVEN
```

| Bit | Name      | Description |
|-----|-----------|-------------|
| 7   | TXINTEN   | **TX interrupt enable.** When set, an IRQ fires whenever the transmitter is idle (buffer empty). Because the IRQ is level-sensitive, it fires continuously while TX is idle and this bit is set. |
| 6   | RXINTEN   | **RX interrupt enable.** When set, an IRQ fires whenever received data is ready. Same level-sensitive caveat. |
| 5   | (reserved)| Documented as "0 for future compatibility" in the Epyx reference. Must be written as 0. |
| 4   | PAREN     | **Parity enable.** When set, the UART calculates and sends/checks parity. When clear, the PAREVEN bit value is sent as the 9th bit instead (allows using the parity bit as a mark/space indicator). |
| 3   | RESETERR  | **Reset all errors.** Writing a 1 clears the overrun and framing error flags. Self-clearing (no need to write 0 after). |
| 2   | TXOPEN    | **TX open collector mode.** 1 = open-collector driver (for multi-device ComLynx bus). 0 = TTL push-pull driver. Open collector mode is required for proper ComLynx bus arbitration where multiple devices can drive the line. **Note:** Neither Handy nor Mednafen implement this bit — they ignore it entirely. |
| 1   | TXBRK     | **Send break.** As long as this bit is set, the transmitter continuously sends break codes (all zeros). The break auto-repeats every 11 Timer 4 ticks. The transmitted break is looped back to the receiver. |
| 0   | PAREVEN   | **Even parity / mark bit.** When PAREN=1: selects even (1) vs odd (0) parity. When PAREN=0: this bit's value is directly sent as the 9th bit of the frame. |

### ⚠ Note on Nexen's current stub

The current Nexen SERCTL write comment has the bits shifted incorrectly (PAREN at bit 5,
RESETERR at bit 4, TXOPEN at bit 3, etc.). The correct layout matches what Handy/Mednafen
use in their Poke handlers and matches the Epyx hardware documentation as shown above.

---

## 3. SERCTL Register — Read (`$FD8C`)

Reading `$FD8C` returns a completely different set of status bits:

```
Bit 7   Bit 6   Bit 5    Bit 4   Bit 3    Bit 2    Bit 1   Bit 0
TXRDY   RXRDY   TXEMPTY  PARERR  OVERRUN  FRAMERR  RXBRK   PARBIT
```

| Bit | Name    | Description |
|-----|---------|-------------|
| 7   | TXRDY   | **TX buffer empty.** 1 = the transmit holding register can accept a new byte. In practice, set when `TX_COUNTDOWN` is inactive (sentinel `0x80000000`). |
| 6   | RXRDY   | **RX data ready.** 1 = a received byte is available in SERDAT for reading. Cleared when SERDAT is read. |
| 5   | TXEMPTY | **TX completely idle.** 1 = both the TX holding register and the internal shift register are empty. In the simplified emulator model (no separate shift register), TXRDY and TXEMPTY are always the same — both set when TX is inactive. Emulators return `0xA0` (bits 7+5) together. |
| 4   | PARERR  | **Parity error.** 1 = the received byte had a parity mismatch. **Not implemented** in either Handy or Mednafen — always returns 0. |
| 3   | OVERRUN | **Receiver overrun.** 1 = a new byte was received before the previous one was read. Variable: `mUART_Rx_overun_error`. Set in the Timer 4 update when `mUART_RX_READY` is already set at receive completion. |
| 2   | FRAMERR | **Framing error.** 1 = the stop bit was not detected correctly. Variable: `mUART_Rx_framing_error`. **Not actually generated** by either emulator (only cleared, never set). |
| 1   | RXBRK   | **Break received.** 1 = a break condition was detected (≥24 bit periods of continuous low). Detected by checking `mUART_RX_DATA & UART_BREAK_CODE (0x8000)`. |
| 0   | PARBIT  | **9th bit / parity bit.** The received parity/mark bit. Extracted as `mUART_RX_DATA & 0x0100` (bit 8 of the internal 16-bit RX data word). |

### ⚠ Known comment bug in Handy/Mednafen

Both emulators' source code has **swapped comments** on bits 3 and 2:
```cpp
retval|=(mUART_Rx_overun_error)?0x08:0x0;  // Comment says "Framing error" — WRONG
retval|=(mUART_Rx_framing_error)?0x04:0x00; // Comment says "Rx overrun"  — WRONG
```
The **variable mapping is correct** (overrun → bit 3, framing → bit 2), matching the
hardware documentation. Only the inline comments are swapped.

---

## 4. SERDAT Register (`$FD8D`)

### Write (TX)

Writing to `$FD8D` loads the transmit data register and starts the 11-tick transmission
countdown:

```cpp
mUART_TX_DATA = value;                // Store 8-bit data

if (mUART_PARITY_ENABLE) {
    // Calculate parity over data bits
    // NOTE: Both Handy and Mednafen have "Leave at zero !!" here
    //       — parity calculation is UNIMPLEMENTED in both emulators
} else {
    // Parity disabled: send PAREVEN bit as the 9th bit
    if (mUART_PARITY_EVEN) data |= 0x0100;
}

mUART_TX_COUNTDOWN = UART_TX_TIME_PERIOD;  // = 11

// ComLynx loopback: TX is always echoed back to RX
// because the ComLynx port has only one I/O pin (TX and RX shorted)
ComLynxTxLoopback(mUART_TX_DATA);
```

### Read (RX)

Reading `$FD8D` returns the received data byte and clears RXRDY:

```cpp
mUART_RX_READY = 0;            // Clear the "data ready" flag
return mUART_RX_DATA & 0xFF;   // Return lower 8 bits only
```

The 9th bit (parity/mark) is available separately via SERCTL read bit 0 (PARBIT).

---

## 5. Timer 4 — Baud Rate Generator

Timer 4 (`$FD10`–`$FD13`) is the dedicated UART clock. Unlike other timers, it **does not
generate normal timer interrupts** through the standard timer IRQ path. Instead, its
borrow-out signal clocks the UART shift register and may trigger UART-specific IRQs via
the serial interrupt line (INTSET bit 4):

### Clock division formula

```
divide = 4 + 3 + TIM4_LINKING
```

Where:
- **4** = master clock 16 MHz → ÷16 = 1 MHz base (the minimum timer prescale)
- **3** = additional ÷8 for "8 clocks per bit transmit" (oversampling)
- **TIM4_LINKING** = bits [2:0] of TIM4CTLA (the clock source selector)

The effective prescaler from the master clock is `2^(4+3+linking) = 2^(7+linking)`:

| CTLA[2:0] | Linking | Total divide | Base frequency | Example baud (BKUP=0) |
|-----------|---------|-------------|----------------|-----------------------|
| 0         | 0       | 2^7 = 128   | 125 kHz        | ~11,364 baud          |
| 1         | 1       | 2^8 = 256   | 62.5 kHz       | ~5,682 baud           |
| 2         | 2       | 2^9 = 512   | 31.25 kHz      | ~2,841 baud           |
| 3         | 3       | 2^10 = 1024 | 15.625 kHz     | ~1,420 baud           |
| 4         | 4       | 2^11 = 2048 | 7,812.5 Hz     | ~710 baud             |
| 5         | 5       | 2^12 = 4096 | 3,906.25 Hz    | ~355 baud             |
| 6         | 6       | 2^13 = 8192 | 1,953.125 Hz   | ~178 baud             |
| 7         | linked  | cascade     | (varies)       | Timer 3 drives Timer 4 |

### Baud rate calculation

```
baud_rate = master_clock / (2^(7 + linking) × (backup_value + 1) × 11)
```

Where 11 = bits per frame (1 start + 8 data + 1 parity + 1 stop).

### Timer 4 update path

Each time Timer 4 underflows (count goes negative), it:

1. Sets `BORROW_OUT = true`
2. Updates the UART TX countdown: decrements `mUART_TX_COUNTDOWN`
3. Updates the UART RX countdown: decrements `mUART_RX_COUNTDOWN`
4. Reloads the count: `current += backup + 1`

The TX and RX countdowns count down from 11 (UART_TX_TIME_PERIOD / UART_RX_TIME_PERIOD).
When a countdown reaches 0, the corresponding transfer is complete.

### Common baud rate: 62500

The default ComLynx baud rate used by most games is **62,500 baud**:
- TIM4CTLA clock source = 0 (1 µs / 1 MHz)
- TIM4BKUP = 0 (divide by 1)
- Effective: 1,000,000 Hz ÷ (8 × (0+1)) = 125,000 half-bits/s → 1 bit per 8 clocks
- But the UART counts 11 timer ticks per frame, and each Timer 4 tick at source=0
  with BKUP=0 fires every 128 master clocks.
- Actual bit time = 128 master clocks = 8 µs → 125,000 bps → ÷11 = ~11,364 frames/s
  (but the UART baud rate is typically quoted per-bit, not per-frame)

**Practical note**: Most games set Timer 4 for ~62,500 baud using
`BKUP=$00, CTLA=$18` (enable count + enable reload + source=0).

### UARTturbo mode

Setting bit 4 of `Mtest0` (`$FD9C`) switches the baud rate to **1 Mbit/s**. This is a test
feature documented in the hardware reference but not implemented in Handy or Mednafen.

---

## 6. TX Mechanics — Complete Flow

### 6.1 Normal transmission

1. **Software writes SERDAT** → `mUART_TX_DATA = value`
2. Parity bit is computed (or PAREVEN substituted when parity disabled)
3. `mUART_TX_COUNTDOWN = 11` (UART_TX_TIME_PERIOD)
4. **Loopback immediately**: `ComLynxTxLoopback(mUART_TX_DATA)` — inserts data at
   the front of the RX queue (because ComLynx is wired-OR, you always hear yourself)
5. Timer 4 ticks down `TX_COUNTDOWN` each borrow event
6. When `TX_COUNTDOWN` reaches 0:
   - If `SENDBREAK` is set: load `TX_DATA = UART_BREAK_CODE (0x8000)`, restart countdown,
     loop back the break code → auto-repeats
   - If `SENDBREAK` is clear: set `TX_COUNTDOWN = UART_TX_INACTIVE (0x80000000)` → idle
   - Fire TX callback (if external networking object is attached)
7. When inactive, SERCTL read returns `TXRDY=1, TXEMPTY=1` (bits 7+5 = `0xA0`)

### 6.2 Break transmission

1. Software sets **TXBRK** (bit 1) in SERCTL write
2. In the SERCTL Poke handler: `TX_COUNTDOWN = 11`, `ComLynxTxLoopback(UART_BREAK_CODE)`
3. Every time the TX countdown reaches 0, if SENDBREAK is still set:
   - `TX_DATA = UART_BREAK_CODE`
   - Restart: `TX_COUNTDOWN = 11`
   - Loop back: `ComLynxTxLoopback(TX_DATA)`
4. Break continues until software clears TXBRK

### 6.3 Key constant

```cpp
#define UART_TX_INACTIVE   0x80000000  // Sentinel: TX is idle
#define UART_TX_TIME_PERIOD  11        // Timer 4 ticks for one 11-bit frame
```

---

## 7. RX Mechanics — Complete Flow

The receiver uses a **circular queue** (32 entries) and a separate countdown:

### 7.1 External data arrival: `ComLynxRxData(int data)`

```cpp
void ComLynxRxData(int data) {
    if (mUART_Rx_waiting < UART_MAX_RX_QUEUE) {   // Queue not full (32 max)
        // Trigger receive countdown only if queue was previously empty
        if (!mUART_Rx_waiting)
            mUART_RX_COUNTDOWN = UART_RX_TIME_PERIOD;  // = 11

        // Append to back of queue
        mUART_Rx_input_queue[mUART_Rx_input_ptr] = data;
        mUART_Rx_input_ptr = (mUART_Rx_input_ptr + 1) % UART_MAX_RX_QUEUE;
        mUART_Rx_waiting++;
    }
    // If queue full: byte is silently dropped (overrun at queue level)
}
```

### 7.2 TX self-loopback: `ComLynxTxLoopback(int data)`

Identical to `ComLynxRxData` except the byte is inserted at the **front** of the queue
(so loopback data is received before any externally-queued data):

```cpp
void ComLynxTxLoopback(int data) {
    if (mUART_Rx_waiting < UART_MAX_RX_QUEUE) {
        if (!mUART_Rx_waiting)
            mUART_RX_COUNTDOWN = UART_RX_TIME_PERIOD;

        // INSERT at front (decrement output pointer)
        mUART_Rx_output_ptr = (mUART_Rx_output_ptr - 1) % UART_MAX_RX_QUEUE;
        mUART_Rx_input_queue[mUART_Rx_output_ptr] = data;
        mUART_Rx_waiting++;
    }
}
```

### 7.3 Timer 4 driven reception (in `Update()`)

When `mUART_RX_COUNTDOWN` reaches 0 (decremented by Timer 4 borrow events):

```cpp
if (!mUART_RX_COUNTDOWN) {
    // Fetch next byte from the queue
    if (mUART_Rx_waiting > 0) {
        mUART_RX_DATA = mUART_Rx_input_queue[mUART_Rx_output_ptr];
        mUART_Rx_output_ptr = (mUART_Rx_output_ptr + 1) % UART_MAX_RX_QUEUE;
        mUART_Rx_waiting--;
    }

    // If more bytes are waiting, set delay before next receive
    if (mUART_Rx_waiting > 0)
        mUART_RX_COUNTDOWN = UART_RX_TIME_PERIOD + UART_RX_NEXT_DELAY;  // 11 + 44 = 55
    else
        mUART_RX_COUNTDOWN = UART_RX_INACTIVE;  // 0x80000000

    // Overrun detection: if previous byte not yet read
    if (mUART_RX_READY)
        mUART_Rx_overun_error = 1;

    // Flag data as available
    mUART_RX_READY = 1;
}
else if (!(mUART_RX_COUNTDOWN & UART_RX_INACTIVE)) {
    mUART_RX_COUNTDOWN--;  // Count down if not already inactive
}
```

### 7.4 Key constants

```cpp
#define UART_RX_INACTIVE     0x80000000  // Sentinel: RX is idle
#define UART_RX_TIME_PERIOD  11          // Timer 4 ticks for one 11-bit frame
#define UART_RX_NEXT_DELAY   44          // Extra delay between queued receives
#define UART_MAX_RX_QUEUE    32          // Circular buffer size
#define UART_BREAK_CODE      0x00008000  // Marker for break condition in data word
```

---

## 8. 11-Bit Frame Format

Each UART frame on the wire is 11 bits:

```
[Start][D0][D1][D2][D3][D4][D5][D6][D7][Parity/Mark][Stop]
  0     LSB ← — — — — — — — → MSB       P/M          1
```

| Field       | Bits | Description |
|-------------|------|-------------|
| Start bit   | 1    | Always 0 (low). Signals frame start. |
| Data bits   | 8    | LSB first (standard UART order). The byte written to SERDAT. |
| Parity/Mark | 1    | If PAREN=1: computed parity (even/odd per PAREVEN). If PAREN=0: the PAREVEN bit value is sent directly (acts as a mark/space bit). |
| Stop bit    | 1    | Always 1 (high). |

- **Break** = the line held low for ≥24 bit periods (no valid stop bit detected).
- In the emulator model, the 11 bits are "transmitted" as a countdown of 11 Timer 4 ticks
  rather than shifting bits individually.

---

## 9. Hardware Bugs and Errata

### Bug 1: Level-sensitive UART IRQ (documented in Epyx ref §13.2)

The UART interrupt is **level-sensitive**, not edge-triggered. This means:

- If TXINTEN is set and the TX buffer is empty → IRQ fires **continuously** on every
  update cycle, not just once when TX becomes empty
- If RXINTEN is set and RX data is ready → IRQ fires continuously until the byte is read
- Games must **clear the interrupt enable bits** or **service the condition** to stop the
  flood of IRQs

From the emulator source:
```cpp
// Emulate the UART bug where UART IRQ is level sensitive
// in that it will continue to generate interrupts as long
// as they are enabled and the interrupt condition is true

if ((mUART_TX_COUNTDOWN & UART_TX_INACTIVE) && mUART_TX_IRQ_ENABLE)
    mTimerStatusFlags |= 0x10;  // Assert serial IRQ

if (mUART_RX_READY && mUART_RX_IRQ_ENABLE)
    mTimerStatusFlags |= 0x10;  // Assert serial IRQ
```

The serial interrupt is bit 4 (`$10`) in the interrupt status/mask registers at `$FD80`/`$FD81`.

### Bug 2: TXD starts high at power-up (§13.3)

At power-up, the TXD line starts in a high state. When a Lynx is connected to the ComLynx
bus, this initial high-to-low transition can be misinterpreted as the start of a break
signal by other connected Lynx units. Games that use ComLynx must include logic to handle
or ignore this spurious initial "break".

### Bug 3: ALGO 3 sprite sizing is broken

(Not UART-related, but documented in the same section: "SET IT TO ZERO!!!!, algo 3 is broke")

### ⚠ Known emulation gaps

| Feature | Status in Handy/Mednafen |
|---------|-------------------------|
| Parity calculation (PAREN=1) | **UNIMPLEMENTED** — comment says "Leave at zero !!" |
| PARERR (SERCTL read bit 4) | **UNIMPLEMENTED** — always returns 0 |
| FRAMERR generation | **NEVER SET** — variable exists but is only cleared, never set |
| TXOPEN (open-collector mode) | **IGNORED** — bit 2 of SERCTL write is not decoded |
| UARTturbo (Mtest0 bit 4) | **UNIMPLEMENTED** — test mode for 1 Mbit/s |

These gaps haven't caused practical problems because:
- Most games don't use parity (they disable it and use PAREVEN as a mark bit)
- No game relies on parity error detection
- Framing errors don't occur in emulation (perfect bit timing)
- Open-collector vs TTL distinction doesn't matter with only 2 emulated units
- UARTturbo is a test/debug feature

---

## 10. UART IRQ Integration

The UART interrupt is multiplexed onto the **Timer 4** interrupt line (bit 4 = `$10`):

```
INTSET ($FD81) / INTRST ($FD80):
   Bit 7 = Timer 7
   Bit 6 = Timer 6
   Bit 5 = Timer 5
   Bit 4 = Serial interrupt  ← UART lives here
   Bit 3 = Timer 3
   Bit 2 = Timer 2 (vertical line counter)
   Bit 1 = Timer 1
   Bit 0 = Timer 0 (horizontal line timer)
```

Timer 4 itself does **not** generate normal timer interrupts. Its interrupt bit is used
exclusively for the UART. The Timer 4 borrow-out event is consumed internally to clock
the UART shift register.

---

## 11. ComLynx Bus Protocol

### Physical layer

- **Single-wire** half-duplex, active-low open-collector with pull-up
- 3.5 mm stereo jack: Tip = data, Ring = ground, Sleeve = ground
- Multiple units wired in parallel (daisy-chain topology)
- Any unit pulling the line low is seen by all units (wired-OR / wired-AND)

### Self-loopback

Because TX and RX share the same physical pin, every byte transmitted is simultaneously
received by the sender. This is fundamental to the protocol — games use it for:

1. **Collision detection**: If the received byte doesn't match what was sent, another unit
   was transmitting simultaneously (bus contention)
2. **Echo verification**: Confirms successful transmission
3. **Protocol coordination**: Master/slave protocols use the loopback to confirm ownership

### Emulator modelling

In the emulator, loopback is handled by `ComLynxTxLoopback()` which inserts the TX data
at the **front** of the RX queue (priority over externally received data). External data
from another emulated Lynx arrives via `ComLynxRxData()` which appends to the **back**
of the same queue.

For multi-unit emulation, the external callback mechanism is:
```cpp
void ComLynxTxCallback(void (*function)(int data, uint32 objref), uint32 objref);
```
When a byte transmission completes, this callback fires with the data and a user-provided
reference object, allowing a networking layer to forward the byte to other emulated units.

---

## 12. UART State Variables (for serialization)

All variables that must be saved/restored for save states:

```cpp
// Configuration (from SERCTL write)
uint32  mUART_RX_IRQ_ENABLE;     // RX interrupt enable
uint32  mUART_TX_IRQ_ENABLE;     // TX interrupt enable
uint32  mUART_PARITY_ENABLE;     // Parity calculation enable
uint32  mUART_PARITY_EVEN;       // Even parity / mark bit value
uint32  mUART_SENDBREAK;         // Break transmission active

// Data registers
uint32  mUART_TX_DATA;            // Last transmitted data (wider than 8 bits for parity/break)
uint32  mUART_RX_DATA;            // Last received data (wider for parity bit 8 + break bit 15)
uint32  mUART_RX_READY;           // 1 = received byte available for reading

// Countdown timers (clocked by Timer 4 borrow events)
uint32  mUART_TX_COUNTDOWN;       // 11→0 for transmission, 0x80000000 = inactive
uint32  mUART_RX_COUNTDOWN;       // 11→0 for reception, 0x80000000 = inactive

// Error flags
int     mUART_Rx_framing_error;   // Framing error detected
int     mUART_Rx_overun_error;    // Overrun error (previous byte not read)

// Receive queue (circular buffer)
int     mUART_Rx_input_queue[32]; // RX data queue
uint32  mUART_Rx_input_ptr;       // Write pointer (back of queue)
uint32  mUART_Rx_output_ptr;      // Read pointer (front of queue)
int     mUART_Rx_waiting;         // Number of bytes in queue

// External I/O
int     mUART_CABLE_PRESENT;      // Cable detection flag
```

---

## 13. Reset State

On reset, all UART state initializes to:

```cpp
mUART_RX_IRQ_ENABLE   = 0;
mUART_TX_IRQ_ENABLE   = 0;
mUART_TX_COUNTDOWN    = UART_TX_INACTIVE;   // 0x80000000
mUART_RX_COUNTDOWN    = UART_RX_INACTIVE;   // 0x80000000
mUART_Rx_input_ptr    = 0;
mUART_Rx_output_ptr   = 0;
mUART_Rx_waiting      = 0;
mUART_Rx_framing_error = 0;
mUART_Rx_overun_error  = 0;
mUART_SENDBREAK       = 0;
mUART_TX_DATA         = 0;
mUART_RX_DATA         = 0;
mUART_RX_READY        = 0;
mUART_PARITY_ENABLE   = 0;
mUART_PARITY_EVEN     = 0;
```

Timer 4 also resets to 0 (backup=0, count=0, all controls cleared).

---

## 14. Implementation Recommendations for Nexen

### 14.1 Fix SERCTL write bit layout in current stub

The current Nexen code at `LynxMikey.cpp` has incorrect bit assignments in the SERCTL
write handler comment. The correct mapping is:

```
B7=TXINTEN, B6=RXINTEN, B5=reserved, B4=PAREN,
B3=RESETERR, B2=TXOPEN, B1=TXBRK, B0=PAREVEN
```

### 14.2 Stub-adequate behaviour (single-player, no cable)

For games that just check the serial port and move on (most single-player games):

- **SERCTL read**: Return `$A0` (TXRDY + TXEMPTY, no RX data, no errors) — current behaviour
- **SERDAT write**: Accept and discard — current behaviour
- **SERDAT read**: Return 0 or last written value — current behaviour
- **SERCTL write**: Parse TXINTEN/RXINTEN. If TXINTEN is set and the stub always reports
  TX idle, it will trigger level-sensitive IRQs continuously. This **must** be handled or
  games will hang in an IRQ storm. Current stub needs to avoid triggering serial IRQs or
  properly handle the level-sensitive nature.

### 14.3 Full implementation (for multiplayer/ComLynx)

Required additions:
1. UART state variables as listed in §12
2. Timer 4 integration: hook Timer 4 borrow into UART TX/RX countdown decrements
3. Self-loopback: TX writes immediately queue into RX (front-insertion)
4. Level-sensitive IRQ: assert serial IRQ on every update while condition is true
5. External callback: for multi-instance networking

### 14.4 Games known to use ComLynx

Almost all multiplayer Lynx games use ComLynx, including:
- Todd's Adventures in Slime World (up to 8 players)
- Checkered Flag (2 players)
- Warbirds (2–4 players)
- Chips Challenge
- Slime World
- Many others — ComLynx was a key Lynx feature

---

## 15. Source References

| Source | URL / Path | Notes |
|--------|-----------|-------|
| Epyx hardware reference | monlynx.de/lynx/hardware.html | SERCTL bit definitions, Mtest0 UARTturbo |
| Handy 0.95 (patched) | bspruck/handy-fork `handy-win32src-0.95-patched/core/mikie.cpp` | Primary reference implementation |
| Handy 0.98 | bspruck/handy-fork `handy-sdl-master/src/handy-0.98/mikie.cpp` | Latest version with minor fixes |
| Mednafen / Beetle Lynx | libretro/beetle-lynx-libretro `mednafen/lynx/mikie.cpp` | Derived from Handy, nearly identical UART |
| Nexen current stub | `Core/Lynx/LynxMikey.cpp:304-326` (read), `443-464` (write) | Minimal stub, needs correction |
