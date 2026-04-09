using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Avalonia;
using Avalonia.Media;
using Nexen.Interop;
using ReactiveUI.SourceGenerators;

namespace Nexen.Config; 
public sealed partial class ScriptWindowConfig : BaseWindowConfig<ScriptWindowConfig> {
	private const int MaxRecentScripts = 10;

	[Reactive] public partial List<string> RecentScripts { get; set; } = [];

	[Reactive] public partial int Zoom { get; set; } = 100;

	[Reactive] public partial double LogWindowHeight { get; set; } = 100;

	[Reactive] public partial ScriptStartupBehavior ScriptStartupBehavior { get; set; } = ScriptStartupBehavior.ShowTutorial;
	[Reactive] public partial bool SaveScriptBeforeRun { get; set; } = true;
	[Reactive] public partial bool AutoStartScriptOnLoad { get; set; } = true;
	[Reactive] public partial bool AutoReloadScriptWhenFileChanges { get; set; } = true;
	[Reactive] public partial bool AutoRestartScriptAfterPowerCycle { get; set; } = true;

	[Reactive] public partial bool AllowIoOsAccess { get; set; } = false;
	[Reactive] public partial bool AllowNetworkAccess { get; set; } = false;

	[Reactive] public partial bool ShowLineNumbers { get; set; } = false;

	[Reactive] public partial UInt32 ScriptTimeout { get; set; } = 1;

	public void AddRecentScript(string scriptFile) {
		string? existingItem = RecentScripts.Where((file) => file == scriptFile).FirstOrDefault();
		if (existingItem is not null) {
			RecentScripts.Remove(existingItem);
		}

		RecentScripts.Insert(0, scriptFile);
		if (RecentScripts.Count > ScriptWindowConfig.MaxRecentScripts) {
			RecentScripts.RemoveAt(ScriptWindowConfig.MaxRecentScripts);
		}
	}
}

public enum ScriptStartupBehavior {
	ShowTutorial = 0,
	ShowBlankWindow = 1,
	LoadLastScript = 2
}
