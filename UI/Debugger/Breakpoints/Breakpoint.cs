using System;
using System.Reactive.Linq;
using System.Text;
using Avalonia.Media;
using Nexen.Config;
using Nexen.Debugger.Labels;
using Nexen.Interop;
using Nexen.Utilities;
using ReactiveUI;
using ReactiveUI.Fody.Helpers;

namespace Nexen.Debugger {
	/// <summary>
	/// Represents a debugger breakpoint that can trigger on memory access or execution.
	/// </summary>
	/// <remarks>
	/// <para>
	/// Breakpoints are the primary debugging mechanism, allowing the debugger to pause
	/// execution when specific conditions are met. They support:
	/// <list type="bullet">
	///   <item><description>Read breakpoints - trigger when memory is read</description></item>
	///   <item><description>Write breakpoints - trigger when memory is written</description></item>
	///   <item><description>Execute breakpoints - trigger when code is executed</description></item>
	///   <item><description>Forbid breakpoints - prevent access entirely (for debugging)</description></item>
	///   <item><description>Conditional breakpoints - trigger only when a condition evaluates true</description></item>
	/// </list>
	/// </para>
	/// <para>
	/// Breakpoints can target a single address or an address range, and can be set on
	/// either relative (CPU-visible) or absolute (ROM/RAM file) addresses.
	/// </para>
	/// <para>
	/// This class uses ReactiveUI with Fody weaving for automatic property change notifications.
	/// </para>
	/// </remarks>
	public class Breakpoint : ReactiveObject {
		/// <summary>
		/// Gets or sets whether to break when the address is read from.
		/// </summary>
		[Reactive] public bool BreakOnRead { get; set; }

		/// <summary>
		/// Gets or sets whether to break when the address is written to.
		/// </summary>
		[Reactive] public bool BreakOnWrite { get; set; }

		/// <summary>
		/// Gets or sets whether to break when code at the address is executed.
		/// </summary>
		/// <remarks>
		/// Only applicable if <see cref="SupportsExec"/> is <c>true</c> for the memory type.
		/// </remarks>
		[Reactive] public bool BreakOnExec { get; set; }

		/// <summary>
		/// Gets or sets whether this is a "forbid" breakpoint that prevents access.
		/// </summary>
		/// <remarks>
		/// Forbid breakpoints always break when accessed, useful for detecting unexpected access.
		/// </remarks>
		[Reactive] public bool Forbid { get; set; }

		/// <summary>
		/// Gets or sets whether the breakpoint is currently enabled.
		/// </summary>
		/// <value>Default is <c>true</c>.</value>
		[Reactive] public bool Enabled { get; set; } = true;

		/// <summary>
		/// Gets or sets whether to mark this access as an event in the event viewer.
		/// </summary>
		/// <remarks>
		/// When enabled, accesses to this address are highlighted in the event viewer
		/// without necessarily pausing execution.
		/// </remarks>
		[Reactive] public bool MarkEvent { get; set; }

		/// <summary>
		/// Gets or sets whether to ignore dummy read/write operations.
		/// </summary>
		/// <value>Default is <c>true</c>.</value>
		/// <remarks>
		/// Some CPU instructions perform dummy reads/writes that don't represent real data access.
		/// Setting this to <c>true</c> skips triggering on these operations.
		/// </remarks>
		[Reactive] public bool IgnoreDummyOperations { get; set; } = true;

		/// <summary>
		/// Gets or sets the memory type this breakpoint applies to.
		/// </summary>
		/// <remarks>
		/// Examples: CpuMemory, PrgRom, WorkRam, SaveRam, etc.
		/// </remarks>
		[Reactive] public MemoryType MemoryType { get; set; }

		/// <summary>
		/// Gets or sets the start address of the breakpoint range (inclusive).
		/// </summary>
		[Reactive] public UInt32 StartAddress { get; set; }

		/// <summary>
		/// Gets or sets the end address of the breakpoint range (inclusive).
		/// </summary>
		/// <remarks>
		/// For single-address breakpoints, this equals <see cref="StartAddress"/>.
		/// </remarks>
		[Reactive] public UInt32 EndAddress { get; set; }

		/// <summary>
		/// Gets or sets the CPU type this breakpoint applies to.
		/// </summary>
		/// <remarks>
		/// Multi-CPU systems (e.g., SNES with SA-1) require specifying which CPU.
		/// </remarks>
		[Reactive] public CpuType CpuType { get; set; }

		/// <summary>
		/// Gets or sets whether this breakpoint applies to any address.
		/// </summary>
		/// <value>Default is <c>false</c>.</value>
		[Reactive] public bool AnyAddress { get; set; } = false;

		/// <summary>
		/// Gets or sets whether this breakpoint is an assertion.
		/// </summary>
		/// <value>Default is <c>false</c>.</value>
		/// <remarks>
		/// Assert breakpoints trigger when their condition is violated, useful for
		/// detecting invariant violations during development.
		/// </remarks>
		[Reactive] public bool IsAssert { get; set; } = false;

		/// <summary>
		/// Gets or sets the conditional expression for this breakpoint.
		/// </summary>
		/// <value>An expression string, or empty for unconditional breakpoints.</value>
		/// <remarks>
		/// Supports expressions like "A == $80", "[PlayerHP] &lt; 10", etc.
		/// </remarks>
		[Reactive] public string Condition { get; set; } = "";

		/// <summary>
		/// Initializes a new instance of the <see cref="Breakpoint"/> class.
		/// </summary>
		public Breakpoint() {
		}

		/// <summary>
		/// Gets whether the breakpoint uses absolute (ROM/RAM file) addressing.
		/// </summary>
		/// <value>
		/// <c>true</c> if the address is absolute; <c>false</c> if it's relative (CPU-visible).
		/// </value>
		public bool IsAbsoluteAddress { get { return !MemoryType.IsRelativeMemory(); } }

		/// <summary>
		/// Gets whether the memory type supports execution breakpoints.
		/// </summary>
		public bool SupportsExec { get { return MemoryType.SupportsExecBreakpoints(); } }

		/// <summary>
		/// Gets whether this breakpoint targets a single address.
		/// </summary>
		public bool IsSingleAddress { get { return StartAddress == EndAddress; } }

		/// <summary>
		/// Gets whether this breakpoint targets an address range.
		/// </summary>
		public bool IsAddressRange { get { return StartAddress != EndAddress; } }

		/// <summary>
		/// Gets the combined breakpoint type flags indicating which access types trigger the breakpoint.
		/// </summary>
		/// <value>
		/// A combination of <see cref="BreakpointTypeFlags"/> values.
		/// </value>
		/// <remarks>
		/// If <see cref="Forbid"/> is set, returns only <see cref="BreakpointTypeFlags.Forbid"/>.
		/// Otherwise, returns the combination of Read, Write, and Execute flags based on settings.
		/// </remarks>
		public BreakpointTypeFlags Type {
			get {
				BreakpointTypeFlags type = BreakpointTypeFlags.None;
				if (BreakOnRead) {
					type |= BreakpointTypeFlags.Read;
				}

				if (BreakOnWrite) {
					type |= BreakpointTypeFlags.Write;
				}

				if (BreakOnExec && SupportsExec) {
					type |= BreakpointTypeFlags.Execute;
				}

				if (Forbid) {
					type = BreakpointTypeFlags.Forbid;
				}

				return type;
			}
		}

		/// <summary>
		/// Determines whether this breakpoint matches the specified address, memory type, and CPU.
		/// </summary>
		/// <param name="address">The address to check.</param>
		/// <param name="type">The memory type of the address.</param>
		/// <param name="cpuType">The CPU type, or <c>null</c> to match any CPU.</param>
		/// <returns>
		/// <c>true</c> if the breakpoint matches the specified parameters; otherwise, <c>false</c>.
		/// </returns>
		public bool Matches(UInt32 address, MemoryType type, CpuType? cpuType) {
			if (cpuType.HasValue && cpuType.Value != CpuType) {
				return false;
			}

			return type == MemoryType && address >= StartAddress && address <= EndAddress;
		}

		/// <summary>
		/// Gets the relative (CPU-visible) address for this breakpoint's start address.
		/// </summary>
		/// <returns>
		/// The relative address if the breakpoint uses absolute addressing and supports execution;
		/// otherwise, the start address unchanged.
		/// </returns>
		/// <remarks>
		/// Used for display purposes and when the debugger needs to show the CPU's view of the address.
		/// </remarks>
		public int GetRelativeAddress() {
			if (SupportsExec && this.IsAbsoluteAddress) {
				return DebugApi.GetRelativeAddress(new AddressInfo() { Address = (int)StartAddress, Type = this.MemoryType }, this.CpuType).Address;
			} else {
				return (int)StartAddress;
			}
		}

		/// <summary>
		/// Gets the relative (CPU-visible) address for this breakpoint's end address.
		/// </summary>
		/// <returns>
		/// The relative address if this is a range breakpoint using absolute addressing;
		/// otherwise, <c>-1</c>.
		/// </returns>
		private int GetRelativeAddressEnd() {
			if (StartAddress != EndAddress) {
				if (SupportsExec && this.IsAbsoluteAddress) {
					return DebugApi.GetRelativeAddress(new AddressInfo() { Address = (int)this.EndAddress, Type = this.MemoryType }, this.CpuType).Address;
				} else {
					return (int)this.EndAddress;
				}
			}

			return -1;
		}

		/// <summary>
		/// Converts this breakpoint to an interop structure for passing to the emulator core.
		/// </summary>
		/// <param name="breakpointId">The unique identifier to assign to this breakpoint.</param>
		/// <returns>
		/// An <see cref="InteropBreakpoint"/> structure containing the breakpoint configuration.
		/// </returns>
		/// <remarks>
		/// The condition string is converted to UTF-8 bytes with newlines replaced by spaces.
		/// </remarks>
		public InteropBreakpoint ToInteropBreakpoint(int breakpointId) {
			InteropBreakpoint bp = new InteropBreakpoint() {
				Id = breakpointId,
				CpuType = CpuType,
				MemoryType = MemoryType,
				Type = Type,
				MarkEvent = MarkEvent,
				IgnoreDummyOperations = IgnoreDummyOperations,
				Enabled = Enabled,
				StartAddress = (Int32)StartAddress,
				EndAddress = (Int32)EndAddress
			};

			bp.Condition = new byte[1000];
			byte[] condition = Encoding.UTF8.GetBytes(Condition.Replace(Environment.NewLine, " ").Trim());
			Array.Copy(condition, bp.Condition, condition.Length);
			return bp;
		}

		/// <summary>
		/// Gets a formatted string representation of the breakpoint's address or range.
		/// </summary>
		/// <param name="showLabel">If <c>true</c>, appends the label name in brackets if one exists.</param>
		/// <returns>
		/// A string like "$1234" for single addresses or "$1234 - $5678" for ranges,
		/// optionally followed by "[LabelName]".
		/// </returns>
		public string GetAddressString(bool showLabel) {
			string addr = "";
			string format = MemoryType.GetFormatString();
			if (StartAddress == EndAddress) {
				addr += $"${StartAddress.ToString(format)}";
			} else {
				addr = $"${StartAddress.ToString(format)} - ${EndAddress.ToString(format)}";
			}

			if (showLabel) {
				string label = GetAddressLabel();
				if (!string.IsNullOrWhiteSpace(label)) {
					addr += " [" + label + "]";
				}
			}

			return addr;
		}

		/// <summary>
		/// Gets the label name associated with this breakpoint's start address, if any.
		/// </summary>
		/// <returns>
		/// The label name, or an empty string if no label exists or the memory type doesn't support labels.
		/// </returns>
		public string GetAddressLabel() {
			if (MemoryType.SupportsLabels()) {
				CodeLabel? label = LabelManager.GetLabel(new AddressInfo() { Address = (int)StartAddress, Type = MemoryType });
				return label?.Label ?? string.Empty;
			}

			return string.Empty;
		}

		/// <summary>
		/// Gets a human-readable string representation of the breakpoint type.
		/// </summary>
		/// <returns>
		/// A string like "PRG:RW‒" indicating memory type and access flags,
		/// or "PRG:🛇" for forbid breakpoints.
		/// </returns>
		/// <remarks>
		/// Format: "{MemoryTypeShortName}:{R|‒}{W|‒}{X|‒}" where:
		/// <list type="bullet">
		///   <item><description>R = Read breakpoint enabled</description></item>
		///   <item><description>W = Write breakpoint enabled</description></item>
		///   <item><description>X = Execute breakpoint enabled (if supported)</description></item>
		///   <item><description>‒ = Flag not set</description></item>
		///   <item><description>🛇 = Forbid breakpoint</description></item>
		/// </list>
		/// </remarks>
		public string ToReadableType() {
			string type = MemoryType.GetShortName();
			type += ":";
			if (Forbid) {
				type += "🛇";
			} else {
				type += BreakOnRead ? "R" : "‒";
				type += BreakOnWrite ? "W" : "‒";
				if (SupportsExec) {
					type += BreakOnExec ? "X" : "‒";
				}
			}

			return type;
		}

		/// <summary>
		/// Gets the display color for this breakpoint based on its type.
		/// </summary>
		/// <returns>
		/// A <see cref="Color"/> from the debugger configuration:
		/// forbid color for forbid breakpoints, execution color for exec breakpoints,
		/// write color for write breakpoints, or read color otherwise.
		/// </returns>
		public Color GetColor() {
			DebuggerConfig config = ConfigManager.Config.Debug.Debugger;
			if (Forbid) {
				return Color.FromUInt32(config.ForbidBreakpointColor);
			}

			return Color.FromUInt32(BreakOnExec ? config.CodeExecBreakpointColor : (BreakOnWrite ? config.CodeWriteBreakpointColor : config.CodeReadBreakpointColor));
		}

		/// <summary>
		/// Creates a deep copy of this breakpoint.
		/// </summary>
		/// <returns>A new <see cref="Breakpoint"/> instance with the same property values.</returns>
		public Breakpoint Clone() {
			return JsonHelper.Clone(this);
		}

		/// <summary>
		/// Copies all property values from another breakpoint to this instance.
		/// </summary>
		/// <param name="copy">The breakpoint to copy values from.</param>
		public void CopyFrom(Breakpoint copy) {
			StartAddress = copy.StartAddress;
			EndAddress = copy.EndAddress;
			MemoryType = copy.MemoryType;
			MarkEvent = copy.MarkEvent;
			IgnoreDummyOperations = copy.IgnoreDummyOperations;
			Enabled = copy.Enabled;
			Condition = copy.Condition;
			BreakOnExec = copy.BreakOnExec;
			BreakOnRead = copy.BreakOnRead;
			BreakOnWrite = copy.BreakOnWrite;
			Forbid = copy.Forbid;
			CpuType = copy.CpuType;
		}
	}

	/// <summary>
	/// Specifies the types of memory access that trigger a breakpoint.
	/// </summary>
	/// <remarks>
	/// This is a flags enumeration allowing combinations of access types.
	/// For example, <c>Read | Write</c> triggers on both read and write access.
	/// </remarks>
	[Flags]
	public enum BreakpointTypeFlags {
		/// <summary>
		/// No access types selected.
		/// </summary>
		None = 0,

		/// <summary>
		/// Trigger on memory read access.
		/// </summary>
		Read = 1,

		/// <summary>
		/// Trigger on memory write access.
		/// </summary>
		Write = 2,

		/// <summary>
		/// Trigger on code execution at the address.
		/// </summary>
		Execute = 4,

		/// <summary>
		/// Forbid any access to the address (always triggers).
		/// </summary>
		Forbid = 8,
	}
}
