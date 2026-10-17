#pragma once

#include "pch.h"
#include "Debugger/DebugTypes.h"

/// <summary>
/// Dummy (side-effect-free) 65C02 CPU for the Lynx debugger.
///
/// DummyLynxCpu is a compile-time variant of LynxCpu that performs no
/// actual memory writes or side effects. It is used by the debugger for:
///   - Predictive disassembly (determining operand addresses without executing)
///   - Breakpoint condition evaluation without modifying state
///   - Stepping through code in the disassembly view
///
/// Implementation uses the DUMMYCPU macro to redefine LynxCpu → DummyLynxCpu
/// at include time, creating a separate class with the same instruction
/// decode logic but stubbed memory operations.
///
/// This pattern is shared across all CPU cores in Nexen (NES, SNES, PCE, etc.)
/// and allows the debugger to safely evaluate arbitrary breakpoint expressions
/// like "A == $42 &amp;&amp; [PC+1] == $80" without risk of modifying emulation state.
///
/// References:
///   - Core/Debugger/DummyCpu.h (pattern origin)
///   - Core/NES/Debugger/DummyNesCpu.h (similar implementation)
///   - ~docs/DEBUGGER.md (Section: Breakpoint Evaluation)
/// </summary>
#define DUMMYCPU
#define LynxCpu DummyLynxCpu
#include "Lynx/LynxCpu.h"
#undef LynxCpu
#undef DUMMYCPU
