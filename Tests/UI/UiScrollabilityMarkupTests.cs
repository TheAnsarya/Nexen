using System.IO;
using System.Text.RegularExpressions;
using Xunit;

namespace Nexen.Tests.UI;

/// <summary>
/// Guards against accidental removal of explicit scroll containers from dense UI surfaces.
/// These are lightweight markup tests until broader UI automation coverage lands.
/// </summary>
public sealed class UiScrollabilityMarkupTests {
	[Theory]
	[InlineData("UI/Debugger/Windows/DebuggerConfigWindow.axaml")]
	[InlineData("UI/Windows/SetupWizardWindow.axaml")]
	public void DenseUiSurface_ContainsScrollViewer(string relativePath) {
		string repoRoot = GetRepositoryRoot();
		string fullPath = Path.Combine(repoRoot, relativePath.Replace('/', Path.DirectorySeparatorChar));

		Assert.True(File.Exists(fullPath), $"Expected markup file to exist: {fullPath}");

		string markup = File.ReadAllText(fullPath);
		Assert.Contains("<ScrollViewer", markup);
	}

	[Fact]
	public void SetupWizardWindow_IsResizableAndHasMinimumSize() {
		string repoRoot = GetRepositoryRoot();
		string fullPath = Path.Combine(repoRoot, "UI", "Windows", "SetupWizardWindow.axaml");

		Assert.True(File.Exists(fullPath), $"Expected markup file to exist: {fullPath}");

		string markup = File.ReadAllText(fullPath);
		Assert.Contains("CanResize=\"True\"", markup);
		Assert.Contains("MinWidth=\"460\"", markup);
		Assert.Contains("MinHeight=\"560\"", markup);
		int minHeightTouchTargetCount = Regex.Matches(markup, "MinHeight=\"36\"").Count;
		Assert.True(minHeightTouchTargetCount >= 3, $"Expected at least 3 touch-target controls with MinHeight=36, found {minHeightTouchTargetCount}.");
	}

	[Fact]
	public void ConfigWindow_IsResizableAndUsesLargerTouchTargets() {
		string repoRoot = GetRepositoryRoot();
		string fullPath = Path.Combine(repoRoot, "UI", "Windows", "ConfigWindow.axaml");

		Assert.True(File.Exists(fullPath), $"Expected markup file to exist: {fullPath}");

		string markup = File.ReadAllText(fullPath);
		Assert.Contains("CanResize=\"True\"", markup);
		Assert.Contains("MinWidth=\"700\"", markup);
		Assert.Contains("MinHeight=\"560\"", markup);
		Assert.Contains("MinHeight=\"36\"", markup);
		Assert.Contains("Name=\"SettingsTabControl\"", markup);
		Assert.Contains("Background=\"{DynamicResource SettingsTabStripBackgroundBrush}\"", markup);
		Assert.DoesNotContain("Background=\"#20888888\"", markup);
	}

	[Fact]
	public void DebuggerConfigWindow_IsResizableAndUsesLargerTouchTargets() {
		string repoRoot = GetRepositoryRoot();
		string fullPath = Path.Combine(repoRoot, "UI", "Debugger", "Windows", "DebuggerConfigWindow.axaml");

		Assert.True(File.Exists(fullPath), $"Expected markup file to exist: {fullPath}");

		string markup = File.ReadAllText(fullPath);
		Assert.Contains("CanResize=\"True\"", markup);
		Assert.Contains("MinWidth=\"700\"", markup);
		Assert.Contains("MinHeight=\"560\"", markup);
		Assert.Contains("MinHeight=\"36\"", markup);
		Assert.Contains("Name=\"SettingsTabControl\"", markup);
		Assert.Contains("Background=\"{DynamicResource SettingsTabStripBackgroundBrush}\"", markup);
		Assert.DoesNotContain("Background=\"#20888888\"", markup);
	}

	[Fact]
	public void ThemeStyles_DefineSettingsTabStripBackgroundToken() {
		string repoRoot = GetRepositoryRoot();
		string lightPath = Path.Combine(repoRoot, "UI", "Styles", "NexenStyles.Light.xaml");
		string darkPath = Path.Combine(repoRoot, "UI", "Styles", "NexenStyles.Dark.xaml");

		Assert.True(File.Exists(lightPath), $"Expected style file to exist: {lightPath}");
		Assert.True(File.Exists(darkPath), $"Expected style file to exist: {darkPath}");

		string lightMarkup = File.ReadAllText(lightPath);
		string darkMarkup = File.ReadAllText(darkPath);

		Assert.Contains("SettingsTabStripBackgroundColor", lightMarkup);
		Assert.Contains("SettingsTabStripBackgroundBrush", lightMarkup);
		Assert.Contains("SettingsTabStripBackgroundColor", darkMarkup);
		Assert.Contains("SettingsTabStripBackgroundBrush", darkMarkup);
	}

	[Fact]
	public void ConfigWindow_WiresResizePersistence() {
		string repoRoot = GetRepositoryRoot();
		string fullPath = Path.Combine(repoRoot, "UI", "Windows", "ConfigWindow.axaml.cs");

		Assert.True(File.Exists(fullPath), $"Expected code-behind file to exist: {fullPath}");

		string source = File.ReadAllText(fullPath);
		Assert.Contains("ConfigManager.Config.ConfigWindow.LoadWindowSettings(this);", source);
		Assert.Contains("ConfigManager.Config.ConfigWindow.SaveWindowSettings(this);", source);
		Assert.Contains("CompactTabBreakpointWidth = 700", source);
		Assert.Contains("TabStripPlacement = ClientSize.Width < CompactTabBreakpointWidth ? Avalonia.Controls.Dock.Top : Avalonia.Controls.Dock.Left;", source);
	}

	[Fact]
	public void DebuggerConfigWindow_WiresResizePersistence() {
		string repoRoot = GetRepositoryRoot();
		string fullPath = Path.Combine(repoRoot, "UI", "Debugger", "Windows", "DebuggerConfigWindow.axaml.cs");

		Assert.True(File.Exists(fullPath), $"Expected code-behind file to exist: {fullPath}");

		string source = File.ReadAllText(fullPath);
		Assert.Contains("ConfigManager.Config.Debug.DebuggerConfigWindow.LoadWindowSettings(this);", source);
		Assert.Contains("ConfigManager.Config.Debug.DebuggerConfigWindow.SaveWindowSettings(this);", source);
		Assert.Contains("CompactTabBreakpointWidth = 900", source);
		Assert.Contains("TabStripPlacement = ClientSize.Width < CompactTabBreakpointWidth ? Avalonia.Controls.Dock.Top : Avalonia.Controls.Dock.Left;", source);
	}

	[Fact]
	public void DebuggerConfigWindow_IntegrationTabContainsClarificationHints() {
		string repoRoot = GetRepositoryRoot();
		string fullPath = Path.Combine(repoRoot, "UI", "Debugger", "Windows", "DebuggerConfigWindow.axaml");

		Assert.True(File.Exists(fullPath), $"Expected markup file to exist: {fullPath}");

		string markup = File.ReadAllText(fullPath);
		Assert.Contains("{l:Translate lblIntegrationDefaultsHint}", markup);
		Assert.Contains("{l:Translate lblAutoExportHint}", markup);
		Assert.Contains("{l:Translate lblPansyLifecycleHint}", markup);
		Assert.Contains("{l:Translate lblPansyCompressionHint}", markup);
	}

	[Fact]
	public void ConfigWindow_DataTemplatesUseExplicitScrollViewers() {
		string repoRoot = GetRepositoryRoot();
		string fullPath = Path.Combine(repoRoot, "UI", "Windows", "ConfigWindow.axaml");

		Assert.True(File.Exists(fullPath), $"Expected markup file to exist: {fullPath}");

		string markup = File.ReadAllText(fullPath);
		int scrollViewerCount = Regex.Matches(markup, "<ScrollViewer AllowAutoHide=\"False\" HorizontalScrollBarVisibility=\"Auto\" Padding=\"0 0 2 0\">").Count;
		Assert.True(scrollViewerCount >= 16, $"Expected at least 16 standardized ScrollViewer wrappers, found {scrollViewerCount}.");
	}

	[Fact]
	public void SettingsWindows_TabHeadersKeepIconCoverage() {
		string repoRoot = GetRepositoryRoot();
		string configPath = Path.Combine(repoRoot, "UI", "Windows", "ConfigWindow.axaml");
		string debuggerPath = Path.Combine(repoRoot, "UI", "Debugger", "Windows", "DebuggerConfigWindow.axaml");

		Assert.True(File.Exists(configPath), $"Expected markup file to exist: {configPath}");
		Assert.True(File.Exists(debuggerPath), $"Expected markup file to exist: {debuggerPath}");

		string configMarkup = File.ReadAllText(configPath);
		string debuggerMarkup = File.ReadAllText(debuggerPath);

		int configHeaderCount = Regex.Matches(configMarkup, "<TabItem.Header>").Count;
		int configHeaderImages = Regex.Matches(configMarkup, "<TabItem.Header>[\\s\\S]*?<Image ").Count;
		int debuggerHeaderCount = Regex.Matches(debuggerMarkup, "<TabItem.Header>").Count;
		int debuggerHeaderImages = Regex.Matches(debuggerMarkup, "<TabItem.Header>[\\s\\S]*?<Image ").Count;

		Assert.True(configHeaderCount >= 18, $"Expected at least 18 ConfigWindow tab headers, found {configHeaderCount}.");
		Assert.True(configHeaderImages >= 16, $"Expected at least 16 ConfigWindow iconized headers, found {configHeaderImages}.");
		Assert.True(debuggerHeaderCount >= 7, $"Expected at least 7 DebuggerConfigWindow tab headers, found {debuggerHeaderCount}.");
		Assert.True(debuggerHeaderImages >= 5, $"Expected at least 5 DebuggerConfigWindow iconized headers, found {debuggerHeaderImages}.");

		Assert.DoesNotContain("<TabItem.Header>\r\n\t\t\t\t\t<TextBlock", configMarkup);
		Assert.DoesNotContain("<TabItem.Header>\r\n\t\t\t\t\t<TextBlock", debuggerMarkup);
	}

	[Fact]
	public void PreferencesView_ContainsThemeSelector() {
		string repoRoot = GetRepositoryRoot();
		string fullPath = Path.Combine(repoRoot, "UI", "Views", "PreferencesConfigView.axaml");

		Assert.True(File.Exists(fullPath), $"Expected markup file to exist: {fullPath}");

		string markup = File.ReadAllText(fullPath);
		Assert.Contains("Text=\"{l:Translate lblTheme}\"", markup);
		Assert.Contains("SelectedItem=\"{Binding Config.Theme}\"", markup);
	}

	private static string GetRepositoryRoot() {
		string? current = Path.GetFullPath(Path.Combine(AppContext.BaseDirectory, "..", "..", "..", ".."));

		while (!string.IsNullOrEmpty(current)) {
			if (File.Exists(Path.Combine(current, "Nexen.sln"))) {
				return current;
			}

			string? parent = Directory.GetParent(current)?.FullName;
			if (string.Equals(parent, current, System.StringComparison.OrdinalIgnoreCase)) {
				break;
			}
			current = parent;
		}

		throw new DirectoryNotFoundException("Could not locate Nexen repository root.");
	}
}
