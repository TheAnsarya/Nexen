#pragma once
#include "pch.h"
#include "Utilities/ISerializable.h"
#include "SNES/IMemoryHandler.h"
#include "Shared/MemoryType.h"

/// <summary>
/// Base class for SNES cartridge coprocessors.
/// Provides interface for enhancement chips like SA-1, Super FX, DSP, etc.
/// </summary>
/// <remarks>
/// Coprocessors are additional chips in SNES cartridges that enhance
/// the console's capabilities. They run in parallel with the main CPU
/// and can access cartridge memory independently.
///
/// **Supported Coprocessors:**
/// - **SA-1**: Second 65816 @ 10.74 MHz with character conversion, bit-mapped RAM
/// - **Super FX (GSU)**: RISC processor for 3D polygon rendering
/// - **DSP-1/2/3/4**: Fixed-point math for Mode 7 effects, AI, etc.
/// - **Cx4**: Wireframe 3D and trigonometry (Mega Man X2/X3)
/// - **ST010/ST011**: AI processors for racing games
/// - **S-DD1/SPC7110**: Data decompression chips
/// - **OBC1**: Sprite attribute management
///
/// **Lifecycle:**
/// - `Reset()`: Initialize to power-on state
/// - `Run()`: Execute coprocessor until caught up with main CPU
/// - `ProcessEndOfFrame()`: Per-frame housekeeping
/// - `LoadBattery()`/`SaveBattery()`: Persist coprocessor work RAM
///
/// **Memory Interface:**
/// - Inherits IMemoryHandler for memory-mapped register access
/// - Coprocessors may have their own address spaces
/// </remarks>
class BaseCoprocessor : public ISerializable, public IMemoryHandler {
public:
	/// <summary>Constructs coprocessor with SNES register memory type.</summary>
	BaseCoprocessor() : IMemoryHandler(MemoryType::SnesRegister) {}

	/// <summary>Resets coprocessor to initial power-on state.</summary>
	virtual void Reset() = 0;

	/// <summary>Executes coprocessor to synchronize with main CPU.</summary>
	virtual void Run() {}

	/// <summary>Performs end-of-frame processing (housekeeping).</summary>
	virtual void ProcessEndOfFrame() {}

	/// <summary>Loads battery-backed data from file.</summary>
	virtual void LoadBattery() {}

	/// <summary>Saves battery-backed data to file.</summary>
	virtual void SaveBattery() {}
};