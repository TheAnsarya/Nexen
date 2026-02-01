using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading;
using System.Threading.Tasks;
using Nexen.Debugger;
using Nexen.Interop;
using Nexen.ViewModels;
using Nexen.Windows;

namespace Nexen.Debugger.Labels {
	/// <summary>
	/// Central manager for debugger labels (symbols).
	/// Handles label storage, lookup, import/export, and synchronization with the C++ core.
	/// </summary>
	/// <remarks>
	/// <para>
	/// LabelManager maintains an in-memory dictionary of all labels and synchronizes them
	/// with the native debugger core via <see cref="DebugApi"/>. Labels are indexed by
	/// both address (for fast lookup) and name (for reverse lookup).
	/// </para>
	/// <para>
	/// Multi-byte labels are supported: a label with Length > 1 is registered at each
	/// address in its range with "+offset" suffixes (e.g., "Buffer+0", "Buffer+1").
	/// </para>
	/// <para>
	/// The manager also processes assert directives in label comments using the syntax:
	/// <c>assert(condition)</c> which creates breakpoints when the condition is false.
	/// </para>
	/// </remarks>
	public class LabelManager {
		/// <summary>
		/// Regex pattern for valid label names: must start with letter/underscore/@, 
		/// followed by letters/digits/underscore/@.
		/// </summary>
		public static Regex LabelRegex { get; } = new Regex("^[@_a-zA-Z]+[@_a-zA-Z0-9]*$", RegexOptions.Compiled);

		/// <summary>
		/// Regex to find invalid characters in label names (for sanitization).
		/// </summary>
		public static Regex InvalidLabelRegex { get; } = new Regex("[^@_a-zA-Z0-9]", RegexOptions.Compiled);

		/// <summary>
		/// Regex to extract assert conditions from label comments.
		/// Matches: assert(condition)
		/// </summary>
		public static Regex AssertRegex { get; } = new Regex(@"assert\((.*)\)", RegexOptions.Compiled);

		/// <summary>Primary lookup: (address, memType) → label.</summary>
		private static Dictionary<UInt64, CodeLabel> _labelsByKey = new();

		/// <summary>Set of all labels for enumeration.</summary>
		private static HashSet<CodeLabel> _labels = [];

		/// <summary>Reverse lookup: label name → label.</summary>
		private static Dictionary<string, CodeLabel> _reverseLookup = new();

		/// <summary>
		/// Event raised when labels are added, removed, or modified.
		/// </summary>
		public static event EventHandler? OnLabelUpdated;

		/// <summary>Counter for suspending events during bulk operations.</summary>
		private static int _suspendEvents = 0;

		/// <summary>
		/// Suspends label update events. Use with <see cref="ResumeEvents"/> for bulk operations.
		/// </summary>
		public static void SuspendEvents() { Interlocked.Increment(ref _suspendEvents); }

		/// <summary>
		/// Resumes label update events and fires pending update if any.
		/// </summary>
		public static void ResumeEvents() { Interlocked.Decrement(ref _suspendEvents); ProcessLabelUpdate(); }

		/// <summary>
		/// Clears all labels from both managed storage and native debugger core.
		/// </summary>
		public static void ResetLabels() {
			DebugApi.ClearLabels();
			_labels.Clear();
			_labelsByKey.Clear();
			_reverseLookup.Clear();

			ProcessLabelUpdate();
		}

		/// <summary>
		/// Gets a label at a specific address and memory type.
		/// </summary>
		/// <param name="address">The absolute address.</param>
		/// <param name="type">The memory type.</param>
		/// <returns>The label if found; otherwise, <c>null</c>.</returns>
		public static CodeLabel? GetLabel(UInt32 address, MemoryType type) {
			CodeLabel? label;
			_labelsByKey.TryGetValue(GetKey(address, type), out label);
			return label;
		}

		/// <summary>
		/// Gets a label at an address, with automatic absolute address resolution.
		/// </summary>
		/// <param name="addr">The address info (may be relative or absolute).</param>
		/// <returns>The label if found; otherwise, <c>null</c>.</returns>
		/// <remarks>
		/// If the address is relative memory and no label is found, attempts to resolve
		/// to absolute address and search again.
		/// </remarks>
		public static CodeLabel? GetLabel(AddressInfo addr) {
			CodeLabel? label = GetLabel((UInt32)addr.Address, addr.Type);
			if (label is not null) {
				return label;
			}

			if (addr.Type.IsRelativeMemory()) {
				AddressInfo absAddress = DebugApi.GetAbsoluteAddress(addr);
				if (absAddress.Address >= 0) {
					return GetLabel((UInt32)absAddress.Address, absAddress.Type);
				}
			}

			return null;
		}

		/// <summary>
		/// Gets a label by its name (reverse lookup).
		/// </summary>
		/// <param name="label">The label name to search for.</param>
		/// <returns>The label if found; otherwise, <c>null</c>.</returns>
		public static CodeLabel? GetLabel(string label) {
			return _reverseLookup.ContainsKey(label) ? _reverseLookup[label] : null;
		}

		/// <summary>
		/// Checks if a label exists in the manager.
		/// </summary>
		/// <param name="label">The label to check.</param>
		/// <returns><c>true</c> if the label exists; otherwise, <c>false</c>.</returns>
		public static bool ContainsLabel(CodeLabel label) {
			return _labels.Contains(label);
		}

		/// <summary>
		/// Sets multiple labels at once, with optional event suppression.
		/// </summary>
		/// <param name="labels">The labels to add.</param>
		/// <param name="raiseEvents">Whether to raise the <see cref="OnLabelUpdated"/> event after completion.</param>
		/// <remarks>
		/// Labels with invalid memory types (size = 0) are silently skipped.
		/// </remarks>
		public static void SetLabels(IEnumerable<CodeLabel> labels, bool raiseEvents = true) {
			Dictionary<MemoryType, bool> isAvailable = new();

			foreach (CodeLabel label in labels) {
				//Check if label memory type is valid before adding it to the list
				if (!isAvailable.TryGetValue(label.MemoryType, out bool available)) {
					available = DebugApi.GetMemorySize(label.MemoryType) > 0;
					isAvailable[label.MemoryType] = available;
				}

				if (available) {
					SetLabel(label, false);
				}
			}

			if (raiseEvents) {
				ProcessLabelUpdate();
			}
		}

		/// <summary>
		/// Gets all labels accessible by a specific CPU type.
		/// </summary>
		/// <param name="cpu">The CPU type to filter by.</param>
		/// <returns>A list of labels the CPU can access.</returns>
		public static List<CodeLabel> GetLabels(CpuType cpu) {
			return _labels.Where((lbl) => lbl.Matches(cpu)).ToList<CodeLabel>();
		}

		/// <summary>
		/// Gets all labels regardless of CPU type.
		/// </summary>
		/// <returns>A list containing all labels.</returns>
		public static List<CodeLabel> GetAllLabels() {
			return _labels.ToList();
		}

		/// <summary>
		/// Generates a unique key for label lookup from address and memory type.
		/// </summary>
		/// <param name="address">The absolute address.</param>
		/// <param name="memType">The memory type.</param>
		/// <returns>A 64-bit key combining address and memory type.</returns>
		private static UInt64 GetKey(UInt32 address, MemoryType memType) {
			return address | ((UInt64)memType << 32);
		}

		/// <summary>
		/// Convenience method to create and set a label with the specified parameters.
		/// </summary>
		/// <param name="address">The absolute address.</param>
		/// <param name="memType">The memory type.</param>
		/// <param name="label">The label name.</param>
		/// <param name="comment">The optional comment.</param>
		public static void SetLabel(uint address, MemoryType memType, string label, string comment) {
			LabelManager.SetLabel(new CodeLabel() {
				Address = address,
				MemoryType = memType,
				Label = label,
				Comment = comment
			}, false);
		}

		/// <summary>
		/// Sets or updates a label. Handles duplicate removal and multi-byte registration.
		/// </summary>
		/// <param name="label">The label to set.</param>
		/// <param name="raiseEvent">Whether to raise the <see cref="OnLabelUpdated"/> event.</param>
		/// <returns><c>true</c> on success (always returns true).</returns>
		/// <remarks>
		/// <para>
		/// If a label with the same name already exists, it is removed first.
		/// If labels exist at any address in the range, they are removed.
		/// </para>
		/// <para>
		/// Multi-byte labels are registered at each address with "+offset" suffixes:
		/// e.g., "Buffer+0", "Buffer+1", etc. Only the first byte gets the comment.
		/// </para>
		/// </remarks>
		public static bool SetLabel(CodeLabel label, bool raiseEvent) {
			if (_reverseLookup.ContainsKey(label.Label)) {
				//Another identical label exists, we need to remove it
				DeleteLabel(_reverseLookup[label.Label], false);
			}

			string comment = label.Comment;
			for (UInt32 i = label.Address; i < label.Address + label.Length; i++) {
				UInt64 key = GetKey(i, label.MemoryType);
				CodeLabel? existingLabel;
				if (_labelsByKey.TryGetValue(key, out existingLabel)) {
					DeleteLabel(existingLabel, false);
					_reverseLookup.Remove(existingLabel.Label);
				}

				_labelsByKey[key] = label;

				if (label.Length == 1) {
					DebugApi.SetLabel(i, label.MemoryType, label.Label, comment.Replace(Environment.NewLine, "\n"));
				} else {
					DebugApi.SetLabel(i, label.MemoryType, label.Label + "+" + (i - label.Address).ToString(), comment.Replace(Environment.NewLine, "\n"));

					//Only set the comment on the first byte of multi-byte comments
					comment = "";
				}
			}

			_labels.Add(label);
			if (label.Label.Length > 0) {
				_reverseLookup[label.Label] = label;
			}

			if (raiseEvent) {
				ProcessLabelUpdate();
			}

			return true;
		}

		/// <summary>
		/// Deletes a label from managed storage and native debugger core.
		/// </summary>
		/// <param name="label">The label to delete.</param>
		/// <param name="raiseEvent">Whether to raise the <see cref="OnLabelUpdated"/> event.</param>
		public static void DeleteLabel(CodeLabel label, bool raiseEvent) {
			bool needEvent = false;

			_labels.Remove(label);
			for (UInt32 i = label.Address; i < label.Address + label.Length; i++) {
				UInt64 key = GetKey(i, label.MemoryType);
				if (_labelsByKey.ContainsKey(key)) {
					_reverseLookup.Remove(_labelsByKey[key].Label);
				}

				if (_labelsByKey.Remove(key)) {
					DebugApi.SetLabel(i, label.MemoryType, string.Empty, string.Empty);
					if (raiseEvent) {
						needEvent = true;
					}
				}
			}

			if (needEvent) {
				ProcessLabelUpdate();
			}
		}

		/// <summary>
		/// Deletes multiple labels at once, raising a single update event at the end.
		/// </summary>
		/// <param name="labels">The labels to delete.</param>
		public static void DeleteLabels(IEnumerable<CodeLabel> labels) {
			foreach (CodeLabel label in labels) {
				DeleteLabel(label, false);
			}

			ProcessLabelUpdate();
		}

		/// <summary>
		/// Re-synchronizes all labels with the native debugger core.
		/// Clears native labels and re-registers all managed labels.
		/// </summary>
		/// <param name="raiseEvent">Whether to raise the <see cref="OnLabelUpdated"/> event.</param>
		public static void RefreshLabels(bool raiseEvent) {
			DebugApi.ClearLabels();
			LabelManager.SetLabels([.. _labels], raiseEvent);
		}

		/// <summary>
		/// Raises the <see cref="OnLabelUpdated"/> event if events are not suspended.
		/// Also updates assert breakpoints from label comments.
		/// </summary>
		private static void ProcessLabelUpdate() {
			if (_suspendEvents == 0) {
				OnLabelUpdated?.Invoke(null, EventArgs.Empty);
				UpdateAssertBreakpoints();
			}
		}

		/// <summary>
		/// Scans all label comments for assert() directives and creates breakpoints.
		/// Assert breakpoints trigger when the condition evaluates to false.
		/// </summary>
		/// <remarks>
		/// Comment syntax: assert(condition)
		/// Example: assert(A &gt; 0) - breaks if A is zero or negative
		/// </remarks>
		private static void UpdateAssertBreakpoints() {
			List<Breakpoint> asserts = [];

			Action<CodeLabel, string, CpuType> addAssert = (CodeLabel label, string condition, CpuType cpuType) => asserts.Add(new Breakpoint() {
				BreakOnExec = true,
				MemoryType = label.MemoryType,
				CpuType = cpuType,
				StartAddress = label.Address,
				EndAddress = label.Address,
				Condition = "!(" + condition + ")",
				IsAssert = true
			});

			foreach (CodeLabel label in _labels) {
				foreach (string commentLine in label.Comment.Split('\n')) {
					Match m = LabelManager.AssertRegex.Match(commentLine);
					if (m.Success) {
						foreach (CpuType cpuType in MainWindowViewModel.Instance.RomInfo.CpuTypes) {
							if (cpuType.CanAccessMemoryType(label.MemoryType)) {
								addAssert(label, m.Groups[1].Value, cpuType);
							}
						}
					}
				}
			}

			BreakpointManager.Asserts = asserts;
			BreakpointManager.SetBreakpoints();
		}
	}

	/// <summary>
	/// Flags that modify label behavior.
	/// </summary>
	[Flags]
	public enum CodeLabelFlags {
		/// <summary>No special flags.</summary>
		None = 0,

		/// <summary>Label was automatically generated from a jump/call target analysis.</summary>
		AutoJumpLabel = 1
	}
}
