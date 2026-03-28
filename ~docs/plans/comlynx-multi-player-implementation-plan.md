# ComLynx Multi-Player Implementation Plan

> **Issue**: [#955 — [950.5] ComLynx Multi-Player Not Implemented](https://github.com/TheAnsarya/Nexen/issues/955)
> **Parent**: [#950 — [Epic] Lynx Emulation Completeness & Quality](https://github.com/TheAnsarya/Nexen/issues/950)
> **Date**: 2026-03-26
> **Status**: In Progress

---

## 1. Problem Statement

The Lynx UART / ComLynx serial port hardware is **fully implemented** at the register level:

- SERCTL register (read/write mux, bit-field extraction)
- SERDAT register (TX/RX with parity, 9th bit)
- TX/RX countdown state machines driven by Timer 4 underflows
- 32-entry circular RX queue with overrun detection
- Self-loopback (mandatory on open-collector bus)
- Break signal continuous retransmission
- Level-sensitive IRQ (hardware bug §9.1)
- Full serialization for save states

**What's missing**: Inter-instance communication. `ComLynxRxData()` exists as the public
injection point for external data but nothing calls it from another console instance.
Single-player games work correctly (self-loopback ensures TX echoes back) but multi-player
games see no data from other Lynx units.

---

## 2. Architecture Analysis

### Current Architecture (Single Instance)

```
Emulator → LynxConsole → LynxMikey → UART
                                       ├── TX → ComLynxTxLoopback() → own RX queue (front-insert)
                                       └── RX ← ComLynxRxData() ← [NOTHING — no external source]
```

### Constraint: Single Console per Emulator

The `Emulator` class owns exactly one `IConsole` instance (`safe_ptr<IConsole> _console`).
There is no multi-instance infrastructure. Loading a second ROM replaces the first console.

### Existing Precedent: No Cross-Platform Link

- **Game Boy**: SB/SC registers implemented as loopback only (shifts data, reads own output)
- **WonderSwan**: WsSerial has send buffer tracking but receive buffer is never populated
- **Netplay**: GameServer/GameClient synchronize controller input, not device protocols

### Design Decision

Implementing full multi-instance ComLynx requires either:

1. **Option A — Dual Emulator Instances**: Run two separate Emulator objects in one process
   with a virtual cable connecting their Mikey UARTs. Major architectural change.

2. **Option B — Virtual Cable within Single Emulator**: Create a ComLynx bus abstraction
   that connects multiple LynxMikey instances within the same Emulator. Requires second
   LynxConsole instance management.

3. **Option C — External IPC**: Use shared memory / named pipes between two Nexen processes.
   Least invasive but introduces timing synchronization challenges.

4. **Option D — ComLynx Cable Stub + Test Harness**: Create the cable abstraction and test
   infrastructure without modifying the Emulator architecture. Tests can instantiate two
   LynxMikey instances directly and wire them together. This proves the data path works
   and lays groundwork for Options A/B later.

**Chosen approach: Option D** — ComLynx virtual cable abstraction with comprehensive test
harness. This delivers:

- Proof that the data path works end-to-end
- A reusable cable abstraction for future multi-instance support
- No changes to the existing Emulator architecture
- All existing tests continue passing
- Foundation for future UI/netplay integration

---

## 3. Implementation Design

### 3.1 ComLynxCable Class

```cpp
// Core/Lynx/ComLynxCable.h
class ComLynxCable {
private:
    std::vector<LynxMikey*> _connectedUnits;

public:
    void Connect(LynxMikey* unit);
    void Disconnect(LynxMikey* unit);
    void Broadcast(LynxMikey* sender, uint16_t data);
    size_t GetConnectedCount() const;
    void DisconnectAll();
};
```

**Broadcast semantics** (matching real hardware §11):

- ComLynx is an open-collector bus — ALL units see ALL transmissions
- The sender already gets loopback via `ComLynxTxLoopback()` (front-insert)
- `Broadcast()` calls `ComLynxRxData()` on every unit EXCEPT the sender
- Data is back-inserted into each receiver's RX queue

### 3.2 LynxMikey Integration

Add a cable reference to LynxMikey:

```cpp
// In LynxMikey.h — private member
ComLynxCable* _comLynxCable = nullptr;

// In LynxMikey.h — public method
void SetComLynxCable(ComLynxCable* cable);
```

Modify `ComLynxTxLoopback()` to also broadcast:

```cpp
void LynxMikey::ComLynxTxLoopback(uint16_t data) {
    // Existing front-insert loopback (unchanged)
    if (_uartRxWaiting < UartMaxRxQueue) { ... }

    // NEW: Broadcast to other connected units
    if (_comLynxCable) {
        _comLynxCable->Broadcast(this, data);
    }
}
```

### 3.3 Test Harness: Dual-Mikey Wiring

Tests can directly wire two LynxMikey instances through a ComLynxCable without
needing the full Emulator/Console stack:

```cpp
TEST(ComLynxCableTest, TxOnUnit1_DeliveredToUnit2) {
    // Create two standalone test UART states + cable
    ComLynxCable cable;
    // Wire up, transmit on one, verify arrival on other
}
```

### 3.4 Files to Create/Modify

| File | Action | Description |
|------|--------|-------------|
| `Core/Lynx/ComLynxCable.h` | **CREATE** | Virtual cable class declaration |
| `Core/Lynx/ComLynxCable.cpp` | **CREATE** | Virtual cable implementation |
| `Core/Lynx/LynxMikey.h` | MODIFY | Add `_comLynxCable` member + `SetComLynxCable()` |
| `Core/Lynx/LynxMikey.cpp` | MODIFY | Broadcast in `ComLynxTxLoopback()` |
| `Core.Tests/Lynx/ComLynxCableTests.cpp` | **CREATE** | Multi-instance test suite |
| `Core.Benchmarks/Lynx/ComLynxCableBench.cpp` | **CREATE** | Broadcast performance benchmarks |

---

## 4. Test Plan

### Unit Tests (ComLynxCableTests.cpp)

1. **Cable connection management**
   - Connect/disconnect units
   - Connected count tracking
   - DisconnectAll cleanup

2. **Broadcast semantics**
   - TX on unit A → RX on unit B (2-unit cable)
   - TX on unit A → RX on B and C (3-unit cable)
   - Sender does NOT receive via Broadcast (only self-loopback)
   - Data integrity: transmitted data matches received data

3. **Queue behavior under multi-instance**
   - Receiver queue fills correctly
   - Overrun detection works for external data
   - Inter-byte delay applies to externally-received data

4. **Break signal propagation**
   - Break on unit A → break code received on unit B

5. **Null cable (no cable connected)**
   - Self-loopback still works when `_comLynxCable == nullptr`
   - No crashes or undefined behavior

### Benchmark Tests (ComLynxCableBench.cpp)

1. **Broadcast overhead** — 2-unit vs 4-unit cable
2. **TickUart with cable** — Verify no regression in hot path
3. **Null cable overhead** — Extra nullptr check cost

---

## 5. Risk Assessment

| Risk | Likelihood | Mitigation |
|------|-----------|------------|
| Regression in existing UART tests | Low | All tests run before/after. Cable is null by default. |
| Performance regression in TickUart | Low | Cable broadcast only happens during TX completion (rare path). Nullptr check is branch-predicted. |
| Thread safety for future multi-instance | Medium | Cable is designed for single-thread use. Thread-safe wrapper needed if Emulator spawns threads. |
| Save state compatibility | Low | `_comLynxCable` is not serialized (runtime wiring only). |

---

## 6. Success Criteria

- [ ] All 2288+ existing C++ tests pass
- [ ] All existing UART tests pass unchanged
- [ ] New ComLynxCable tests demonstrate TX→RX between two units
- [ ] Break signal propagates across cable
- [ ] No performance regression in UART benchmarks
- [ ] Cable abstraction works with nullptr (no cable = self-loopback only)
- [ ] Clean build with no warnings
