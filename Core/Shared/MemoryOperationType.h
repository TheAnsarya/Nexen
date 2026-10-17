#pragma once

/// <summary>
/// Categorizes memory access operations by purpose for debugging and profiling.
/// Used by debugger for code/data logging (CDL), breakpoints, and trace logging.
/// </summary>
enum class MemoryOperationType {
	/// <summary>CPU reads data from memory</summary>
	Read = 0,

	/// <summary>CPU writes data to memory</summary>
	Write = 1,

	/// <summary>CPU fetches and executes opcode byte (instruction)</summary>
	/// <remarks>Used for code coverage tracking and execution breakpoints</remarks>
	ExecOpCode = 2,

	/// <summary>CPU fetches operand byte(s) following opcode</summary>
	/// <remarks>Used for code coverage tracking (operand bytes vs opcode bytes)</remarks>
	ExecOperand = 3,

	/// <summary>DMA controller reads from memory</summary>
	/// <remarks>SNES/NES/PCE DMA transfers - bypasses CPU</remarks>
	DmaRead = 4,

	/// <summary>DMA controller writes to memory</summary>
	/// <remarks>SNES/NES/PCE DMA transfers - bypasses CPU</remarks>
	DmaWrite = 5,

	/// <summary>Dummy read with no side effects (CPU open bus)</summary>
	/// <remarks>
	/// Some CPU cycles perform reads that don't use the result.
	/// Important for cycle-accurate emulation timing.
	/// </remarks>
	DummyRead = 6,

	/// <summary>Dummy write with no side effects</summary>
	/// <remarks>
	/// Some CPU cycles perform writes that are discarded.
	/// Important for cycle-accurate emulation timing.
	/// </remarks>
	DummyWrite = 7,

	/// <summary>PPU/VDP reads memory during rendering (background/sprite fetch)</summary>
	/// <remarks>Used for video memory access tracking and VRAM breakpoints</remarks>
	PpuRenderingRead = 8,

	/// <summary>CPU/PPU idle cycle (no memory access)</summary>
	/// <remarks>Used for accurate cycle counting and trace logging</remarks>
	Idle = 9
};

/// <summary>
/// Flags to indicate special conditions during memory access.
/// </summary>
enum class MemoryAccessFlags {
	/// <summary>Normal memory access with no special flags</summary>
	None = 0,

	/// <summary>Memory access performed by DSP coprocessor (SNES Super FX, NEC, etc.)</summary>
	/// <remarks>Used to distinguish DSP access from CPU access in debugger</remarks>
	DspAccess = 1,
};