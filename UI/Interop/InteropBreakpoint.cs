using System;
using System.Runtime.InteropServices;
using Nexen.Debugger;

namespace Nexen.Interop; 
/// <summary>
/// Interop structure for passing breakpoint data to the native emulator core.
/// </summary>
/// <remarks>
/// <para>
/// This structure is marshaled to the native C++ emulator core via P/Invoke.
/// The layout must match the native <c>Breakpoint</c> structure exactly.
/// </para>
/// <para>
/// Created from <see cref="Breakpoint.ToInteropBreakpoint"/> when syncing
/// breakpoints with the emulator.
/// </para>
/// </remarks>
public struct InteropBreakpoint {
	/// <summary>
	/// Unique identifier for this breakpoint in the current session.
	/// </summary>
	public Int32 Id;

	/// <summary>
	/// The CPU type this breakpoint applies to.
	/// </summary>
	public CpuType CpuType;

	/// <summary>
	/// The memory type (address space) this breakpoint monitors.
	/// </summary>
	public MemoryType MemoryType;

	/// <summary>
	/// The access types that trigger this breakpoint (read/write/execute/forbid).
	/// </summary>
	public BreakpointTypeFlags Type;

	/// <summary>
	/// The start address of the breakpoint range (inclusive).
	/// </summary>
	public Int32 StartAddress;

	/// <summary>
	/// The end address of the breakpoint range (inclusive).
	/// </summary>
	public Int32 EndAddress;

	/// <summary>
	/// Whether the breakpoint is currently enabled.
	/// </summary>
	[MarshalAs(UnmanagedType.I1)]
	public bool Enabled;

	/// <summary>
	/// Whether to mark accesses as events in the event viewer.
	/// </summary>
	[MarshalAs(UnmanagedType.I1)]
	public bool MarkEvent;

	/// <summary>
	/// Whether to ignore dummy read/write operations.
	/// </summary>
	[MarshalAs(UnmanagedType.I1)]
	public bool IgnoreDummyOperations;

	/// <summary>
	/// The conditional expression as UTF-8 bytes (1000 byte buffer).
	/// </summary>
	/// <remarks>
	/// Null-terminated string containing the breakpoint condition expression.
	/// Empty for unconditional breakpoints.
	/// </remarks>
	[MarshalAs(UnmanagedType.ByValArray, SizeConst = 1000)]
	public byte[] Condition;
}
