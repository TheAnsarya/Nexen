using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Reactive.Linq;
using Avalonia.Controls;
using Nexen.Debugger.Labels;
using Nexen.Interop;
using Nexen.Localization;
using Nexen.ViewModels;
using ReactiveUI;
using ReactiveUI.Fody.Helpers;

namespace Nexen.Debugger.ViewModels; 
/// <summary>
/// ViewModel for the label edit dialog, providing validation and two-way binding for label properties.
/// </summary>
/// <remarks>
/// <para>
/// This ViewModel handles:
/// <list type="bullet">
///   <item><description>Validation of label names against naming rules</description></item>
///   <item><description>Validation of address ranges against memory type limits</description></item>
///   <item><description>Conflict detection with existing labels</description></item>
///   <item><description>Dynamic filtering of available memory types based on CPU</description></item>
/// </list>
/// </para>
/// <para>
/// Uses ReactiveUI observables to provide real-time validation feedback as the user types.
/// </para>
/// </remarks>
public sealed class LabelEditViewModel : DisposableViewModel {
	/// <summary>
	/// Gets or sets the reactive code label being edited.
	/// </summary>
	[Reactive] public ReactiveCodeLabel Label { get; set; }

	/// <summary>
	/// Gets whether the OK button should be enabled (all validation passed).
	/// </summary>
	[ObservableAsProperty] public bool OkEnabled { get; }

	/// <summary>
	/// Gets the maximum address display string for the current memory type.
	/// </summary>
	[ObservableAsProperty] public string MaxAddress { get; } = "";

	/// <summary>
	/// Gets or sets the current validation error message, or empty string if valid.
	/// </summary>
	[Reactive] public string ErrorMessage { get; private set; } = "";

	/// <summary>
	/// Gets whether the delete button should be shown (editing existing label).
	/// </summary>
	public bool AllowDelete { get; } = false;

	/// <summary>
	/// Gets the array of memory types available for labels on this CPU.
	/// </summary>
	public Enum[] AvailableMemoryTypes { get; private set; } = [];

	/// <summary>
	/// Gets the CPU type for this label editing context.
	/// </summary>
	public CpuType CpuType { get; }

	private CodeLabel? _originalLabel;

	/// <summary>
	/// Designer-only constructor. Do not use in code.
	/// </summary>
	[Obsolete("For designer only")]
	public LabelEditViewModel() : this(CpuType.Snes, new CodeLabel()) { }

	/// <summary>
	/// Initializes a new instance of the <see cref="LabelEditViewModel"/> class.
	/// </summary>
	/// <param name="cpuType">The CPU type, which determines available memory types.</param>
	/// <param name="label">The code label to edit.</param>
	/// <param name="originalLabel">The original label being edited (null if creating new label).</param>
	/// <remarks>
	/// Sets up reactive subscriptions for validation and max address display.
	/// When editing an existing label, preserves the original for conflict detection.
	/// </remarks>
	public LabelEditViewModel(CpuType cpuType, CodeLabel label, CodeLabel? originalLabel = null) {
		_originalLabel = originalLabel;

		Label = new ReactiveCodeLabel(label);
		AllowDelete = originalLabel is not null;

		if (Design.IsDesignMode) {
			return;
		}

		CpuType = cpuType;
		// Filter memory types to those accessible by this CPU that support labels
		AvailableMemoryTypes = Enum.GetValues<MemoryType>().Where(t => cpuType.CanAccessMemoryType(t) && t.SupportsLabels() && DebugApi.GetMemorySize(t) > 0).Cast<Enum>().ToArray();
		if (!AvailableMemoryTypes.Contains(Label.MemoryType)) {
			Label.MemoryType = (MemoryType)AvailableMemoryTypes[0];
		}

		// Update max address display when memory type changes
		AddDisposable(this.WhenAnyValue(x => x.Label.MemoryType, (memoryType) => {
			int maxAddress = DebugApi.GetMemorySize(memoryType) - 1;
			if (maxAddress <= 0) {
				return "(unavailable)";
			} else {
				return "(Max: $" + maxAddress.ToString("X4") + ")";
			}
		}).ToPropertyEx(this, x => x.MaxAddress));

		// Comprehensive validation when any relevant property changes
		AddDisposable(this.WhenAnyValue(x => x.Label.Label, x => x.Label.Comment, x => x.Label.Length, x => x.Label.MemoryType, x => x.Label.Address, (label, comment, length, memoryType, address) => {
			CodeLabel? sameLabel = LabelManager.GetLabel(label);
			int maxAddress = DebugApi.GetMemorySize(memoryType) - 1;

			// Check for address conflicts with existing labels
			for (UInt32 i = 0; i < length; i++) {
				CodeLabel? sameAddress = LabelManager.GetLabel(address + i, memoryType);
				if (sameAddress is not null) {
					if (originalLabel is null || (sameAddress.Label != originalLabel.Label && !sameAddress.Label.StartsWith(originalLabel.Label + "+"))) {
						//A label already exists, we're trying to edit an existing label, but the existing label
						//and the label we're editing aren't the same label.  Can't override an existing label with a different one.
						ErrorMessage = ResourceHelper.GetMessage("AddressHasOtherLabel", sameAddress.Label.Length > 0 ? sameAddress.Label : sameAddress.Comment);
						return false;
					}
				}
			}

			// Check address is within valid range
			if (address + (length - 1) > maxAddress) {
				ErrorMessage = ResourceHelper.GetMessage("AddressOutOfRange");
				return false;
			}

			// Require at least a label name or a comment
			if (label.Length == 0 && comment.Length == 0) {
				ErrorMessage = ResourceHelper.GetMessage("LabelOrCommentRequired");
				return false;
			}

			// Validate label name format (alphanumeric + underscore, cannot start with digit)
			if (label.Length > 0 && !LabelManager.LabelRegex.IsMatch(label)) {
				ErrorMessage = ResourceHelper.GetMessage("InvalidLabel");
				return false;
			}

			// Check for label name conflicts
			if (sameLabel is not null && sameLabel != originalLabel) {
				ErrorMessage = ResourceHelper.GetMessage("LabelNameInUse");
				return false;
			}

			// Final validation: length must be valid and comment cannot contain separator
			if (length >= 1 && length <= 65536 && !comment.Contains('\x1')) {
				ErrorMessage = "";
				return true;
			}

			return false;
		}).ToPropertyEx(this, x => x.OkEnabled));
	}

	/// <summary>
	/// Deletes the original label from the label manager.
	/// </summary>
	/// <remarks>
	/// Only callable when editing an existing label (AllowDelete is true).
	/// </remarks>
	public void DeleteLabel() {
		if (_originalLabel is not null) {
			LabelManager.DeleteLabel(_originalLabel, true);
		}
	}

	/// <summary>
	/// Commits the edited label values to the underlying code label.
	/// </summary>
	public void Commit() {
		Label.Commit();
	}

	/// <summary>
	/// Reactive wrapper around <see cref="CodeLabel"/> that provides property change notifications.
	/// </summary>
	/// <remarks>
	/// <para>
	/// This class wraps a <see cref="CodeLabel"/> instance to provide ReactiveUI-compatible
	/// property change notifications, enabling real-time validation in the edit dialog.
	/// </para>
	/// <para>
	/// Call <see cref="Commit"/> to save changes back to the original label.
	/// </para>
	/// </remarks>
	public sealed class ReactiveCodeLabel : ReactiveObject {
		private CodeLabel _originalLabel;

		/// <summary>
		/// Initializes a new instance of the <see cref="ReactiveCodeLabel"/> class.
		/// </summary>
		/// <param name="label">The code label to wrap.</param>
		public ReactiveCodeLabel(CodeLabel label) {
			_originalLabel = label;

			Address = label.Address;
			Label = label.Label;
			Comment = label.Comment;
			MemoryType = label.MemoryType;
			Flags = label.Flags;
			Length = label.Length;
		}

		/// <summary>
		/// Commits the current property values back to the original code label.
		/// </summary>
		public void Commit() {
			_originalLabel.Address = Address;
			_originalLabel.Label = Label;
			_originalLabel.Comment = Comment;
			_originalLabel.MemoryType = MemoryType;
			_originalLabel.Flags = Flags;
			_originalLabel.Length = Length;
		}

		/// <summary>Gets or sets the memory address of the label.</summary>
		[Reactive] public UInt32 Address { get; set; }

		/// <summary>Gets or sets the memory type where the label resides.</summary>
		[Reactive] public MemoryType MemoryType { get; set; }

		/// <summary>Gets or sets the label name (must be a valid identifier).</summary>
		[Reactive] public string Label { get; set; } = "";

		/// <summary>Gets or sets the comment text associated with the label.</summary>
		[Reactive] public string Comment { get; set; } = "";

		/// <summary>Gets or sets the label flags (function start, auto-generated, etc.).</summary>
		[Reactive] public CodeLabelFlags Flags { get; set; }

		/// <summary>Gets or sets the length in bytes covered by this label (for data tables).</summary>
		[Reactive] public UInt32 Length { get; set; } = 1;
	}
}
