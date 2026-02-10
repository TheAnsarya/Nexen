using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using Nexen.Debugger.Utilities;
using Nexen.Interop;

namespace Nexen.Debugger;
/// <summary>
/// Manages all breakpoints for the debugger, including user-defined and temporary breakpoints.
/// </summary>
/// <remarks>
/// <para>
/// The BreakpointManager maintains three categories of breakpoints:
/// <list type="bullet">
///   <item><description>User breakpoints - Persistent breakpoints set by the user</description></item>
///   <item><description>Assert breakpoints - Breakpoints generated from label assertions</description></item>
///   <item><description>Temporary breakpoints - Short-lived breakpoints (e.g., "Run to cursor")</description></item>
/// </list>
/// </para>
/// <para>
/// Breakpoints are tracked per-CPU type for multi-CPU systems. Only breakpoints for
/// active CPU types are sent to the emulator core.
/// </para>
/// <para>
/// Changes to breakpoints trigger the <see cref="BreakpointsChanged"/> event and
/// automatically sync with <see cref="DebugWorkspaceManager"/> for persistence.
/// </para>
/// </remarks>
public static class BreakpointManager {
	/// <summary>
	/// Raised when the breakpoint collection changes (add, remove, enable/disable, or modification).
	/// </summary>
	/// <remarks>
	/// The sender parameter may be the specific <see cref="Breakpoint"/> that changed,
	/// or <c>null</c> for bulk operations.
	/// </remarks>
	public static event EventHandler? BreakpointsChanged;

	/// <summary>
	/// The list of user-defined breakpoints.
	/// </summary>
	private static List<Breakpoint> _breakpoints = [];

	/// <summary>
	/// Cached read-only view of breakpoints - invalidated when breakpoints change.
	/// </summary>
	private static ReadOnlyCollection<Breakpoint>? _cachedBreakpoints = null;

	/// <summary>
	/// The list of temporary breakpoints (e.g., "Run to cursor").
	/// </summary>
	private static List<Breakpoint> _temporaryBreakpoints = [];

	/// <summary>
	/// The set of CPU types currently active in the debugger.
	/// </summary>
	/// <remarks>
	/// Only breakpoints for active CPU types are sent to the emulator core.
	/// </remarks>
	private static HashSet<CpuType> _activeCpuTypes = [];

	/// <summary>
	/// Invalidates the cached breakpoints collection.
	/// Call this whenever _breakpoints is modified.
	/// </summary>
	private static void InvalidateCache() {
		_cachedBreakpoints = null;
	}

	/// <summary>
	/// Gets a read-only collection of all user-defined breakpoints.
	/// </summary>
	/// <value>
	/// A cached snapshot of the current breakpoints as a read-only collection.
	/// </value>
	/// <remarks>
	/// Returns a cached copy that is invalidated when breakpoints change.
	/// For performance-critical code, consider using <see cref="GetBreakpoints(CpuType)"/> with a specific CPU type.
	/// </remarks>
	public static ReadOnlyCollection<Breakpoint> Breakpoints {
		get {
			return _cachedBreakpoints ??= _breakpoints.ToList().AsReadOnly();
		}
	}

	/// <summary>
	/// Gets or sets the list of assertion breakpoints generated from labels.
	/// </summary>
	/// <remarks>
	/// Assert breakpoints are created from labels with assertion conditions.
	/// See <see cref="LabelManager.UpdateAssertBreakpoints"/>.
	/// </remarks>
	public static List<Breakpoint> Asserts { internal get; set; } = [];

	/// <summary>
	/// Gets all breakpoints for a specific CPU type.
	/// </summary>
	/// <param name="cpuType">The CPU type to filter by.</param>
	/// <returns>A list of breakpoints targeting the specified CPU.</returns>
	public static List<Breakpoint> GetBreakpoints(CpuType cpuType) {
		List<Breakpoint> breakpoints = [];
		foreach (Breakpoint bp in _breakpoints) {
			if (bp.CpuType == cpuType) {
				breakpoints.Add(bp);
			}
		}

		return breakpoints;
	}

	/// <summary>
	/// Registers a CPU type as active, enabling breakpoints for that CPU.
	/// </summary>
	/// <param name="cpuType">The CPU type to add.</param>
	/// <remarks>
	/// Called when opening a debugger window for a specific CPU.
	/// </remarks>
	public static void AddCpuType(CpuType cpuType) {
		_activeCpuTypes.Add(cpuType);
		SetBreakpoints();
	}

	/// <summary>
	/// Unregisters a CPU type, disabling breakpoints for that CPU.
	/// </summary>
	/// <param name="cpuType">The CPU type to remove.</param>
	/// <remarks>
	/// Called when closing a debugger window for a specific CPU.
	/// </remarks>
	public static void RemoveCpuType(CpuType cpuType) {
		_activeCpuTypes.Remove(cpuType);
		SetBreakpoints();
	}

	/// <summary>
	/// Notifies subscribers of breakpoint changes and synchronizes with the emulator.
	/// </summary>
	/// <param name="bp">The specific breakpoint that changed, or <c>null</c> for bulk operations.</param>
	public static void RefreshBreakpoints(Breakpoint? bp = null) {
		InvalidateCache();
		BreakpointsChanged?.Invoke(bp, EventArgs.Empty);
		SetBreakpoints();
	}

	/// <summary>
	/// Removes all user-defined breakpoints.
	/// </summary>
	public static void ClearBreakpoints() {
		_breakpoints = new();
		InvalidateCache();
		RefreshBreakpoints();
	}

	/// <summary>
	/// Adds multiple breakpoints in bulk.
	/// </summary>
	/// <param name="breakpoints">The breakpoints to add.</param>
	/// <remarks>
	/// Used when loading breakpoints from a workspace file.
	/// </remarks>
	public static void AddBreakpoints(List<Breakpoint> breakpoints) {
		_breakpoints.AddRange(breakpoints);
		RefreshBreakpoints();
	}

	/// <summary>
	/// Removes a single breakpoint.
	/// </summary>
	/// <param name="bp">The breakpoint to remove.</param>
	/// <remarks>
	/// Triggers <see cref="DebugWorkspaceManager.AutoSave"/> if the breakpoint was found and removed.
	/// </remarks>
	public static void RemoveBreakpoint(Breakpoint bp) {
		if (_breakpoints.Remove(bp)) {
			DebugWorkspaceManager.AutoSave();
		}

		RefreshBreakpoints(bp);
	}

	/// <summary>
	/// Removes multiple breakpoints in bulk.
	/// </summary>
	/// <param name="breakpoints">The breakpoints to remove.</param>
	public static void RemoveBreakpoints(IEnumerable<Breakpoint> breakpoints) {
		foreach (Breakpoint bp in breakpoints) {
			_breakpoints.Remove(bp);
		}

		RefreshBreakpoints(null);
	}

	/// <summary>
	/// Adds a single breakpoint if it doesn't already exist.
	/// </summary>
	/// <param name="bp">The breakpoint to add.</param>
	/// <remarks>
	/// Triggers <see cref="DebugWorkspaceManager.AutoSave"/> if the breakpoint was added.
	/// </remarks>
	public static void AddBreakpoint(Breakpoint bp) {
		if (!_breakpoints.Contains(bp)) {
			_breakpoints.Add(bp);
			DebugWorkspaceManager.AutoSave();
		}

		RefreshBreakpoints(bp);
	}

	/// <summary>
	/// Creates and adds a new breakpoint at the specified address.
	/// </summary>
	/// <param name="addr">The address information for the breakpoint.</param>
	/// <param name="cpuType">The CPU type for the breakpoint.</param>
	/// <remarks>
	/// Only creates a breakpoint if no matching breakpoint exists at the address.
	/// The new breakpoint breaks on read, write, and execute.
	/// </remarks>
	public static void AddBreakpoint(AddressInfo addr, CpuType cpuType) {
		if (BreakpointManager.GetMatchingBreakpoint(addr, cpuType) == null) {
			Breakpoint bp = new Breakpoint() {
				StartAddress = (uint)addr.Address,
				EndAddress = (uint)addr.Address,
				MemoryType = addr.Type,
				CpuType = cpuType,
				BreakOnExec = true,
				BreakOnWrite = true,
				BreakOnRead = true
			};

			BreakpointManager.AddBreakpoint(bp);
		}
	}

	/// <summary>
	/// Adds a temporary breakpoint that won't be persisted.
	/// </summary>
	/// <param name="bp">The temporary breakpoint to add.</param>
	/// <remarks>
	/// Used for "Run to cursor" and similar operations. Temporary breakpoints
	/// are cleared automatically when execution resumes normally.
	/// </remarks>
	public static void AddTemporaryBreakpoint(Breakpoint bp) {
		_temporaryBreakpoints.Add(bp);
		SetBreakpoints();
	}

	/// <summary>
	/// Removes all temporary breakpoints.
	/// </summary>
	/// <remarks>
	/// Called when execution resumes to clear "Run to cursor" type breakpoints.
	/// </remarks>
	public static void ClearTemporaryBreakpoints() {
		if (_temporaryBreakpoints.Count > 0) {
			_temporaryBreakpoints.Clear();
			SetBreakpoints();
		}
	}

	/// <summary>
	/// Finds a breakpoint matching the given address and predicate.
	/// </summary>
	/// <param name="info">The address information to match.</param>
	/// <param name="cpuType">The CPU type to match.</param>
	/// <param name="predicate">Additional filter predicate for the breakpoint.</param>
	/// <returns>The first matching breakpoint, or <c>null</c> if none found.</returns>
	/// <remarks>
	/// Searches both relative and absolute address representations to find matches.
	/// </remarks>
	private static Breakpoint? GetMatchingBreakpoint(AddressInfo info, CpuType cpuType, Func<Breakpoint, bool> predicate) {
		Breakpoint? bp = Breakpoints.Where((bp) => predicate(bp) && bp.Matches((UInt32)info.Address, info.Type, cpuType)).FirstOrDefault();

		if (bp is null) {
			AddressInfo altAddr = info.Type.IsRelativeMemory() ? DebugApi.GetAbsoluteAddress(info) : DebugApi.GetRelativeAddress(info, cpuType);
			if (altAddr.Address >= 0) {
				bp = Breakpoints.Where((bp) => predicate(bp) && bp.Matches((UInt32)altAddr.Address, altAddr.Type, cpuType)).FirstOrDefault();
			}
		}

		return bp;
	}

	/// <summary>
	/// Finds a breakpoint matching the given address.
	/// </summary>
	/// <param name="info">The address information to match.</param>
	/// <param name="cpuType">The CPU type to match.</param>
	/// <param name="ignoreRangedRwBp">
	/// If <c>true</c>, ignores range breakpoints that only break on read/write (not execute).
	/// </param>
	/// <returns>The first matching breakpoint, or <c>null</c> if none found.</returns>
	public static Breakpoint? GetMatchingBreakpoint(AddressInfo info, CpuType cpuType, bool ignoreRangedRwBp = false) {
		return GetMatchingBreakpoint(info, cpuType, (bp) => !ignoreRangedRwBp || bp.IsSingleAddress || bp.BreakOnExec);
	}

	/// <summary>
	/// Finds a forbid breakpoint matching the given address.
	/// </summary>
	/// <param name="info">The address information to match.</param>
	/// <param name="cpuType">The CPU type to match.</param>
	/// <returns>The first matching forbid breakpoint, or <c>null</c> if none found.</returns>
	public static Breakpoint? GetMatchingForbidBreakpoint(AddressInfo info, CpuType cpuType) {
		return GetMatchingBreakpoint(info, cpuType, (bp) => bp.Forbid);
	}

	/// <summary>
	/// Finds a breakpoint matching an exact address range.
	/// </summary>
	/// <param name="startAddress">The start address to match.</param>
	/// <param name="endAddress">The end address to match.</param>
	/// <param name="memoryType">The memory type to match.</param>
	/// <returns>The first matching breakpoint, or <c>null</c> if none found.</returns>
	public static Breakpoint? GetMatchingBreakpoint(UInt32 startAddress, UInt32 endAddress, MemoryType memoryType) {
		return Breakpoints.Where((bp) =>
				bp.MemoryType == memoryType &&
				bp.StartAddress == startAddress && bp.EndAddress == endAddress
			).FirstOrDefault();
	}

	/// <summary>
	/// Toggles the enabled state of a breakpoint at the specified address.
	/// </summary>
	/// <param name="info">The address information to match.</param>
	/// <param name="cpuType">The CPU type to match.</param>
	/// <returns>
	/// <c>true</c> if a breakpoint was found and toggled; otherwise, <c>false</c>.
	/// </returns>
	public static bool EnableDisableBreakpoint(AddressInfo info, CpuType cpuType) {
		Breakpoint? breakpoint = BreakpointManager.GetMatchingBreakpoint(info, cpuType);
		if (breakpoint is not null) {
			breakpoint.Enabled = !breakpoint.Enabled;
			DebugWorkspaceManager.AutoSave();
			RefreshBreakpoints();
			return true;
		}

		return false;
	}

	/// <summary>
	/// Toggles a breakpoint at the specified address (adds if none exists, removes if one does).
	/// </summary>
	/// <param name="info">The address information for the breakpoint.</param>
	/// <param name="cpuType">The CPU type for the breakpoint.</param>
	/// <remarks>
	/// <para>
	/// Uses CDL (Code/Data Log) data to intelligently determine breakpoint type:
	/// <list type="bullet">
	///   <item><description>If address contains code, creates an execution breakpoint</description></item>
	///   <item><description>If address contains data, creates a read/write breakpoint</description></item>
	///   <item><description>If unknown, creates a full read/write/execute breakpoint</description></item>
	/// </list>
	/// </para>
	/// </remarks>
	public static void ToggleBreakpoint(AddressInfo info, CpuType cpuType) {
		if (info.Address < 0) {
			return;
		}

		Breakpoint? breakpoint = BreakpointManager.GetMatchingForbidBreakpoint(info, cpuType) ?? BreakpointManager.GetMatchingBreakpoint(info, cpuType, true);
		if (breakpoint is not null) {
			BreakpointManager.RemoveBreakpoint(breakpoint);
		} else {
			bool execBreakpoint = true;
			bool readWriteBreakpoint = !info.Type.IsRomMemory() || info.Type.IsRelativeMemory();
			if (info.Type.SupportsCdl()) {
				CdlFlags cdlData = DebugApi.GetCdlData((uint)info.Address, 1, info.Type)[0];
				bool isCode = cdlData.HasFlag(CdlFlags.Code);
				bool isData = cdlData.HasFlag(CdlFlags.Data);
				if (isCode || isData) {
					readWriteBreakpoint = !isCode;
					execBreakpoint = isCode;
				}
			}

			breakpoint = new Breakpoint() {
				CpuType = cpuType,
				Enabled = true,
				BreakOnExec = execBreakpoint,
				BreakOnRead = readWriteBreakpoint,
				BreakOnWrite = readWriteBreakpoint,
				StartAddress = (UInt32)info.Address,
				EndAddress = (UInt32)info.Address
			};

			breakpoint.MemoryType = info.Type;
			BreakpointManager.AddBreakpoint(breakpoint);
		}
	}

	/// <summary>
	/// Toggles a forbid breakpoint at the specified address.
	/// </summary>
	/// <param name="addr">The address information for the breakpoint.</param>
	/// <param name="cpuType">The CPU type for the breakpoint.</param>
	/// <remarks>
	/// Forbid breakpoints prevent any access to an address, useful for detecting
	/// unexpected memory access during debugging.
	/// </remarks>
	public static void ToggleForbidBreakpoint(AddressInfo addr, CpuType cpuType) {
		if (addr.Address < 0) {
			return;
		}

		Breakpoint? breakpoint = GetMatchingForbidBreakpoint(addr, cpuType);
		if (breakpoint is not null) {
			BreakpointManager.RemoveBreakpoint(breakpoint);
		} else {
			breakpoint = new Breakpoint() {
				CpuType = cpuType,
				Enabled = true,
				Forbid = true,
				StartAddress = (UInt32)addr.Address,
				EndAddress = (UInt32)addr.Address
			};
			breakpoint.MemoryType = addr.Type;
			BreakpointManager.AddBreakpoint(breakpoint);
		}
	}

	/// <summary>
	/// Sends all active breakpoints to the emulator core.
	/// </summary>
	/// <remarks>
	/// <para>
	/// Combines user breakpoints, assertions, and temporary breakpoints into a single
	/// array and sends them to the native emulator via <see cref="DebugApi.SetBreakpoints"/>.
	/// </para>
	/// <para>
	/// Breakpoints are assigned sequential IDs that correspond to their position
	/// in the combined list (user → asserts → temporary).
	/// </para>
	/// </remarks>
	public static void SetBreakpoints() {
		List<InteropBreakpoint> breakpoints = [];

		int id = 0;
		void toInteropBreakpoints(IEnumerable<Breakpoint> bpList) {
			foreach (Breakpoint bp in bpList) {
				if (_activeCpuTypes.Contains(bp.CpuType)) {
					breakpoints.Add(bp.ToInteropBreakpoint(id));
				}

				id++;
			}
		}

		toInteropBreakpoints(BreakpointManager.Breakpoints);
		toInteropBreakpoints(BreakpointManager.Asserts);
		toInteropBreakpoints(BreakpointManager._temporaryBreakpoints);

		DebugApi.SetBreakpoints(breakpoints.ToArray(), (UInt32)breakpoints.Count);
	}

	/// <summary>
	/// Retrieves a breakpoint by its internal ID.
	/// </summary>
	/// <param name="breakpointId">
	/// The ID of the breakpoint, as assigned by <see cref="SetBreakpoints"/>.
	/// </param>
	/// <returns>
	/// The breakpoint with the specified ID, or <c>null</c> if the ID is invalid.
	/// </returns>
	/// <remarks>
	/// IDs are assigned in order: user breakpoints (0 to N-1), asserts (N to M-1),
	/// temporary breakpoints (M to end).
	/// </remarks>
	public static Breakpoint? GetBreakpointById(int breakpointId) {
		if (breakpointId < 0) {
			return null;
		} else if (breakpointId < _breakpoints.Count) {
			return _breakpoints[breakpointId];
		} else if (breakpointId < _breakpoints.Count + Asserts.Count) {
			return Asserts[breakpointId - _breakpoints.Count];
		} else if (breakpointId < _breakpoints.Count + Asserts.Count + _temporaryBreakpoints.Count) {
			return _temporaryBreakpoints[breakpointId - _breakpoints.Count - Asserts.Count];
		}

		return null;
	}
}
