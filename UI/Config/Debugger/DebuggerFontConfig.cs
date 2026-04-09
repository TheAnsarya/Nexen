using Avalonia;
using Avalonia.Media;
using Nexen.Interop;
using ReactiveUI.SourceGenerators;

namespace Nexen.Config; 
public sealed partial class DebuggerFontConfig : BaseConfig<DebuggerFontConfig> {
	[Reactive] public partial FontConfig DisassemblyFont { get; set; } = new() { FontFamily = "Consolas", FontSize = 14 };
	[Reactive] public partial FontConfig MemoryViewerFont { get; set; } = new() { FontFamily = "Consolas", FontSize = 14 };
	[Reactive] public partial FontConfig AssemblerFont { get; set; } = new() { FontFamily = "Consolas", FontSize = 14 };
	[Reactive] public partial FontConfig ScriptWindowFont { get; set; } = new() { FontFamily = "Consolas", FontSize = 14 };
	[Reactive] public partial FontConfig OtherMonoFont { get; set; } = new() { FontFamily = "Consolas", FontSize = 12 };

	public void ApplyConfig() {
		if (Application.Current is Application app) {
			string disFont = Configuration.GetValidFontFamily(DisassemblyFont.FontFamily, true);
			string memViewerFont = Configuration.GetValidFontFamily(MemoryViewerFont.FontFamily, true);
			string assemblerFont = Configuration.GetValidFontFamily(AssemblerFont.FontFamily, true);
			string scriptFont = Configuration.GetValidFontFamily(ScriptWindowFont.FontFamily, true);
			string otherFont = Configuration.GetValidFontFamily(OtherMonoFont.FontFamily, true);

			if ((app.Resources["NexenDisassemblyFont"] as FontFamily)?.Name != disFont) {
				app.Resources["NexenDisassemblyFont"] = new FontFamily(disFont);
			}

			if ((app.Resources["NexenMemoryViewerFont"] as FontFamily)?.Name != memViewerFont) {
				app.Resources["NexenMemoryViewerFont"] = new FontFamily(memViewerFont);
			}

			if ((app.Resources["NexenAssemblerFont"] as FontFamily)?.Name != assemblerFont) {
				app.Resources["NexenAssemblerFont"] = new FontFamily(assemblerFont);
			}

			if ((app.Resources["NexenScriptWindowFont"] as FontFamily)?.Name != scriptFont) {
				app.Resources["NexenScriptWindowFont"] = new FontFamily(scriptFont);
			}

			if ((app.Resources["NexenMonospaceFont"] as FontFamily)?.Name != otherFont) {
				app.Resources["NexenMonospaceFont"] = new FontFamily(otherFont);
			}

			if ((double?)app.Resources["NexenDisassemblyFontSize"] != DisassemblyFont.FontSize) {
				app.Resources["NexenDisassemblyFontSize"] = (double)DisassemblyFont.FontSize;
			}

			if ((double?)app.Resources["NexenMemoryViewerFontSize"] != MemoryViewerFont.FontSize) {
				app.Resources["NexenMemoryViewerFontSize"] = (double)MemoryViewerFont.FontSize;
			}

			if ((double?)app.Resources["NexenAssemblerFontSize"] != AssemblerFont.FontSize) {
				app.Resources["NexenAssemblerFontSize"] = (double)AssemblerFont.FontSize;
			}

			if ((double?)app.Resources["NexenScriptWindowFontSize"] != ScriptWindowFont.FontSize) {
				app.Resources["NexenScriptWindowFontSize"] = (double)ScriptWindowFont.FontSize;
			}

			if ((double?)app.Resources["NexenMonospaceFontSize"] != OtherMonoFont.FontSize) {
				app.Resources["NexenMonospaceFontSize"] = (double)OtherMonoFont.FontSize;
			}
		}
	}
}
