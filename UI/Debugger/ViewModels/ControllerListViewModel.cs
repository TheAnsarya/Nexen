using System;
using System.Collections.Generic;
using Avalonia.Controls;
using Nexen.Interop;
using Nexen.ViewModels;
using ReactiveUI.SourceGenerators;

namespace Nexen.Debugger.ViewModels; 
/// <summary>
/// ViewModel for managing a list of controller input overrides in the debugger.
/// </summary>
/// <remarks>
/// <para>
/// This ViewModel provides a container for multiple <see cref="ControllerInputViewModel"/> instances,
/// one for each available controller port that supports input overrides.
/// </para>
/// <para>
/// Input overrides allow the debugger to inject specific button states into the emulation,
/// which is useful for testing game behavior with specific inputs or for automation.
/// </para>
/// </remarks>
public sealed partial class ControllerListViewModel : ViewModelBase {
	/// <summary>
	/// Gets or sets the list of controller input ViewModels.
	/// </summary>
	/// <remarks>
	/// Each item represents one controller port that supports input overrides.
	/// The list is populated automatically based on which controller ports are available.
	/// </remarks>
	[Reactive] public partial List<ControllerInputViewModel> Controllers { get; set; } = new();

	/// <summary>
	/// Designer-only constructor. Do not use in code.
	/// </summary>
	[Obsolete("For designer only")]
	public ControllerListViewModel() : this(ConsoleType.Snes) { }

	/// <summary>
	/// Initializes a new instance of the <see cref="ControllerListViewModel"/> class.
	/// </summary>
	/// <param name="consoleType">The console type, which determines controller layout and button availability.</param>
	/// <remarks>
	/// Queries the debug API for available input override ports and creates
	/// a <see cref="ControllerInputViewModel"/> for each one.
	/// </remarks>
	public ControllerListViewModel(ConsoleType consoleType) {
		if (Design.IsDesignMode) {
			return;
		}

		// Create a ViewModel for each controller port that supports input overrides
		foreach (int index in DebugApi.GetAvailableInputOverrides()) {
			Controllers.Add(new ControllerInputViewModel(consoleType, index));
		}
	}

	/// <summary>
	/// Applies input overrides for all controllers to the emulation.
	/// </summary>
	/// <remarks>
	/// Iterates through all controller ViewModels and calls their individual
	/// <see cref="ControllerInputViewModel.SetInputOverrides"/> methods to
	/// inject the current button states into the emulation.
	/// </remarks>
	public void SetInputOverrides() {
		foreach (ControllerInputViewModel controller in Controllers) {
			controller.SetInputOverrides();
		}
	}
}
