using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Reactive.Linq;
using System.Text;
using System.Threading.Tasks;
using Avalonia.Controls;
using Nexen.Config;
using Nexen.Debugger.Utilities;
using Nexen.Interop;
using Nexen.Localization;
using Nexen.Utilities;
using Nexen.ViewModels;
using ReactiveUI;
using ReactiveUI.Fody.Helpers;

namespace Nexen.Debugger.ViewModels;
/// <summary>
/// ViewModel for the memory tools display options panel.
/// Controls display preferences for the hex editor including column widths and platform-specific visualization options.
/// </summary>
public class MemoryToolsDisplayOptionsViewModel : DisposableViewModel {
	/// <summary>
	/// Gets the hex editor configuration containing display settings.
	/// </summary>
	public HexEditorConfig Config { get; }

	/// <summary>
	/// Gets the parent memory tools view model.
	/// </summary>
	public MemoryToolsViewModel MemoryTools { get; }

	/// <summary>
	/// Gets the available column width options for the hex editor display.
	/// </summary>
	public int[] AvailableWidths => new int[] { 4, 8, 16, 32, 48, 64, 80, 96, 112, 128 };

	/// <summary>
	/// Gets or sets whether the "Show Frozen Addresses" option should be visible.
	/// Only applicable for memory types that support address freezing.
	/// </summary>
	[Reactive] public bool ShowFrozenAddressesOption { get; set; }

	/// <summary>
	/// Gets or sets whether the "Show NES PCM Data" visualization option should be visible.
	/// Only applicable when viewing NES memory types.
	/// </summary>
	[Reactive] public bool ShowNesPcmDataOption { get; set; }

	/// <summary>
	/// Gets or sets whether the "Show NES Drawn CHR ROM" option should be visible.
	/// Only applicable for NES games with CHR ROM (not CHR RAM).
	/// </summary>
	[Reactive] public bool ShowNesDrawnChrRomOption { get; set; }

	/// <summary>
	/// Designer-only constructor. Do not use in production code.
	/// </summary>
	[Obsolete("For designer only")]
	public MemoryToolsDisplayOptionsViewModel() : this(new()) { }

	/// <summary>
	/// Initializes a new instance of the <see cref="MemoryToolsDisplayOptionsViewModel"/> class.
	/// </summary>
	/// <param name="memoryTools">The parent memory tools view model.</param>
	public MemoryToolsDisplayOptionsViewModel(MemoryToolsViewModel memoryTools) {
		Config = memoryTools.Config;
		MemoryTools = memoryTools;

		if (Design.IsDesignMode) {
			return;
		}

		AddDisposable(this.WhenAnyValue(x => x.Config.MemoryType).Subscribe(x => UpdateAvailableOptions()));
	}

	/// <summary>
	/// Updates the visibility of platform-specific display options based on the current memory type.
	/// </summary>
	/// <remarks>
	/// Called automatically when <see cref="HexEditorConfig.MemoryType"/> changes.
	/// Enables/disables options that only apply to certain memory types or platforms.
	/// </remarks>
	public void UpdateAvailableOptions() {
		ShowFrozenAddressesOption = Config.MemoryType.SupportsFreezeAddress();
		ShowNesPcmDataOption = Config.MemoryType.ToCpuType() == CpuType.Nes;
		ShowNesDrawnChrRomOption = Config.MemoryType.ToCpuType() == CpuType.Nes && DebugApi.GetMemorySize(MemoryType.NesChrRom) > 0;
	}
}
