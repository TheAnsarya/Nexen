using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reactive.Linq;
using Avalonia.Controls;
using Nexen.Config;
using Nexen.Controls;
using Nexen.Utilities;
using ReactiveUI;
using ReactiveUI.SourceGenerators;

namespace Nexen.ViewModels; 
public sealed partial class OtherConsolesConfigViewModel : DisposableViewModel {
	[Reactive] public partial CvConfig CvConfig { get; set; }
	[Reactive] public partial CvConfig CvOriginalConfig { get; set; }
	[Reactive] public partial OtherConsolesConfigTab SelectedTab { get; set; } = 0;

	public CvInputConfigViewModel CvInput { get; private set; }

	public Enum[] AvailableRegionsCv => new Enum[] {
		ConsoleRegion.Auto,
		ConsoleRegion.Ntsc,
		ConsoleRegion.Pal
	};

	public OtherConsolesConfigViewModel() {
		CvConfig = ConfigManager.Config.Cv;
		CvOriginalConfig = CvConfig.Clone();
		CvInput = new CvInputConfigViewModel(CvConfig);

		if (Design.IsDesignMode) {
			return;
		}

		AddDisposable(CvInput);
		AddDisposable(ReactiveHelper.RegisterRecursiveObserver(CvConfig, (s, e) => CvConfig.ApplyConfig()));
	}
}

public enum OtherConsolesConfigTab {
	ColecoVision
}
