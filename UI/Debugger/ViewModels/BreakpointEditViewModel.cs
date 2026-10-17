using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Reactive.Linq;
using Avalonia.Controls;
using Nexen.Debugger.Labels;
using Nexen.Debugger.Utilities;
using Nexen.Debugger.Windows;
using Nexen.Interop;
using Nexen.Localization;
using Nexen.ViewModels;
using ReactiveUI;
using ReactiveUI.SourceGenerators;

namespace Nexen.Debugger.ViewModels; 
/// <summary>
/// ViewModel for the breakpoint edit dialog, providing two-way binding for breakpoint properties.
/// </summary>
/// <remarks>
/// <para>
/// This ViewModel handles:
/// <list type="bullet">
///   <item><description>Validation of address ranges against memory type limits</description></item>
///   <item><description>Validation of condition expressions</description></item>
///   <item><description>Dynamic filtering of available memory types based on CPU</description></item>
///   <item><description>Automatic end address sync when start address changes</description></item>
/// </list>
/// </para>
/// <para>
/// Uses ReactiveUI observables to track property changes and update validation state.
/// </para>
/// </remarks>
public sealed partial class BreakpointEditViewModel : DisposableViewModel {
	/// <summary>
	/// Gets or sets the breakpoint being edited.
	/// </summary>
	[Reactive] public partial Breakpoint Breakpoint { get; set; }

	/// <summary>
	/// Gets the help tooltip control for the condition expression editor.
	/// </summary>
	public Control? HelpTooltip { get; } = null;

	/// <summary>
	/// Gets the window title (localized).
	/// </summary>
	public string WindowTitle { get; } = "";

	/// <summary>
	/// Gets whether the current condition expression is valid.
	/// </summary>
	[Reactive] public partial bool IsConditionValid { get; private set; }

	/// <summary>
	/// Gets whether the OK button should be enabled (all validation passed).
	/// </summary>
	[Reactive] public partial bool OkEnabled { get; private set; }

	/// <summary>
	/// Gets the maximum address display string for the current memory type.
	/// </summary>
	[Reactive] public partial string MaxAddress { get; private set; } = "";

	/// <summary>
	/// Gets whether execution breakpoints are supported for the current memory type.
	/// </summary>
	[Reactive] public partial bool CanExec { get; private set; } = false;

	/// <summary>
	/// Gets whether the CPU supports dummy operations (affects UI visibility).
	/// </summary>
	[Reactive] public partial bool HasDummyOperations { get; private set; } = false;

	/// <summary>
	/// Gets the array of memory types available for breakpoints on this CPU.
	/// </summary>
	public Enum[] AvailableMemoryTypes { get; private set; } = [];

	/// <summary>
	/// Designer-only constructor. Do not use in code.
	/// </summary>
	[Obsolete("For designer only")]
	public BreakpointEditViewModel() : this(null!) { }

	/// <summary>
	/// Initializes a new instance of the <see cref="BreakpointEditViewModel"/> class.
	/// </summary>
	/// <param name="bp">The breakpoint to edit.</param>
	/// <remarks>
	/// Sets up reactive subscriptions for validation and UI updates.
	/// </remarks>
	public BreakpointEditViewModel(Breakpoint bp) {
		Breakpoint = bp;

		if (Design.IsDesignMode) {
			return;
		}

		WindowTitle = ResourceHelper.GetViewLabel(nameof(BreakpointEditWindow), bp.Forbid ? "wndTitleForbid" : "wndTitle");

		HasDummyOperations = bp.CpuType.HasDummyOperations() && !bp.Forbid;
		HelpTooltip = ExpressionTooltipHelper.GetHelpTooltip(bp.CpuType, false);
		AvailableMemoryTypes = Enum.GetValues<MemoryType>().Where(t => {
			if (bp.Forbid && !t.SupportsExecBreakpoints()) {
				return false;
			}

			return bp.CpuType.CanAccessMemoryType(t) && t.SupportsBreakpoints() && DebugApi.GetMemorySize(t) > 0;
		}).Cast<Enum>().ToArray();
		if (!AvailableMemoryTypes.Contains(Breakpoint.MemoryType)) {
			Breakpoint.MemoryType = (MemoryType)AvailableMemoryTypes[0];
		}

		AddDisposable(this.WhenAnyValue(x => x.Breakpoint.StartAddress)
			.Buffer(2, 1)
			.Select(b => (Previous: b[0], Current: b[1]))
			.Subscribe(t => {
				if (t.Previous == Breakpoint.EndAddress) {
					Breakpoint.EndAddress = t.Current;
				}
			}
		));

		AddDisposable(this.WhenAnyValue(x => x.Breakpoint.MemoryType).Subscribe(memoryType => CanExec = memoryType.SupportsExecBreakpoints()));

		AddDisposable(this.WhenAnyValue(x => x.Breakpoint.MemoryType).Subscribe(memoryType => {
			int maxAddress = DebugApi.GetMemorySize(memoryType) - 1;
			MaxAddress = maxAddress <= 0 ? "(unavailable)" : "(Max: $" + maxAddress.ToString("X4") + ")";
		}));

		AddDisposable(this.WhenAnyValue(x => x.Breakpoint.Condition).Subscribe(condition => {
			if (!string.IsNullOrWhiteSpace(condition)) {
				EvalResultType resultType;
				DebugApi.EvaluateExpression(condition.Replace(Environment.NewLine, " "), Breakpoint.CpuType, out resultType, false);
				if (resultType == EvalResultType.Invalid) {
					IsConditionValid = false;
					return;
				}
			}

			IsConditionValid = true;
		}));

		AddDisposable(this.WhenAnyValue(
			x => x.Breakpoint.BreakOnExec,
			x => x.Breakpoint.BreakOnRead,
			x => x.Breakpoint.BreakOnWrite,
			x => x.Breakpoint.MemoryType,
			x => x.Breakpoint.StartAddress,
			x => x.Breakpoint.EndAddress,
			x => x.IsConditionValid
		).Subscribe(_ => {
			bool enabled = true;
			if (Breakpoint.Type == BreakpointTypeFlags.None || !IsConditionValid) {
				enabled = false;
			} else {
				int maxAddress = DebugApi.GetMemorySize(Breakpoint.MemoryType) - 1;
				if (Breakpoint.StartAddress > maxAddress || Breakpoint.EndAddress > maxAddress || Breakpoint.StartAddress > Breakpoint.EndAddress) {
					enabled = false;
				}
			}

			OkEnabled = enabled;
		}));
	}
}
