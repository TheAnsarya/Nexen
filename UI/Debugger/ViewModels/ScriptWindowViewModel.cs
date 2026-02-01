using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Reactive.Linq;
using System.Reflection;
using System.Text;
using System.Threading.Tasks;
using Avalonia.Controls;
using Avalonia.Threading;
using Nexen.Config;
using Nexen.Debugger.Utilities;
using Nexen.Debugger.Windows;
using Nexen.Interop;
using Nexen.Localization;
using Nexen.Utilities;
using Nexen.ViewModels;
using Nexen.Windows;
using ReactiveUI;
using ReactiveUI.Fody.Helpers;

namespace Nexen.Debugger.ViewModels {
	/// <summary>
	/// ViewModel for the Lua script editor window.
	/// Provides functionality for editing, running, and managing Lua scripts for emulator automation.
	/// </summary>
	public class ScriptWindowViewModel : ViewModelBase {
		/// <summary>
		/// Gets the configuration settings for the script window.
		/// </summary>
		public ScriptWindowConfig Config { get; } = ConfigManager.Config.Debug.ScriptWindow;

		/// <summary>
		/// Gets or sets the Lua script source code.
		/// </summary>
		[Reactive] public string Code { get; set; } = "";

		/// <summary>
		/// Gets or sets the file path of the currently loaded script, or empty for unsaved scripts.
		/// </summary>
		[Reactive] public string FilePath { get; set; } = "";

		/// <summary>
		/// Gets or sets the ID of the currently running script, or -1 if no script is running.
		/// </summary>
		[Reactive] public int ScriptId { get; set; } = -1;

		/// <summary>
		/// Gets or sets the script execution log output.
		/// </summary>
		[Reactive] public string Log { get; set; } = "";

		/// <summary>
		/// Gets or sets the display name for the script.
		/// </summary>
		[Reactive] public string ScriptName { get; set; } = "";

		/// <summary>
		/// Gets the window title including the script name.
		/// </summary>
		[ObservableAsProperty] public string WindowTitle { get; } = "";

		/// <summary>Original script text for detecting unsaved changes.</summary>
		private string _originalText = "";

		/// <summary>Reference to the parent script window.</summary>
		private ScriptWindow? _wnd = null;

		/// <summary>Watches for external file changes to auto-reload scripts.</summary>
		private FileSystemWatcher _fileWatcher = new();

		/// <summary>Context menu action for recent scripts submenu.</summary>
		private ContextMenuAction _recentScriptsAction = new();

		/// <summary>
		/// Gets or sets the menu actions for the File menu.
		/// </summary>
		[Reactive] public List<ContextMenuAction> FileMenuActions { get; private set; } = new();

		/// <summary>
		/// Gets or sets the menu actions for the Script menu.
		/// </summary>
		[Reactive] public List<ContextMenuAction> ScriptMenuActions { get; private set; } = new();

		/// <summary>
		/// Gets or sets the menu actions for the Help menu.
		/// </summary>
		[Reactive] public List<ContextMenuAction> HelpMenuActions { get; private set; } = new();

		/// <summary>
		/// Gets or sets the toolbar button actions.
		/// </summary>
		[Reactive] public List<ContextMenuAction> ToolbarActions { get; private set; } = new();

		/// <summary>
		/// Designer-only constructor. Do not use in production code.
		/// </summary>
		[Obsolete("For designer only")]
		public ScriptWindowViewModel() : this(null) { }

		/// <summary>
		/// Initializes a new instance of the <see cref="ScriptWindowViewModel"/> class.
		/// </summary>
		/// <param name="behavior">The startup behavior, or null to use config default.</param>
		public ScriptWindowViewModel(ScriptStartupBehavior? behavior) {
			this.WhenAnyValue(x => x.ScriptName).Select(x => {
				string wndTitle = ResourceHelper.GetViewLabel(nameof(ScriptWindow), "wndTitle");
				if (!string.IsNullOrWhiteSpace(x)) {
					return wndTitle + " - " + x;
				}

				return wndTitle;
			}).ToPropertyEx(this, x => x.WindowTitle);

			switch (behavior ?? Config.ScriptStartupBehavior) {
				case ScriptStartupBehavior.ShowBlankWindow: break;
				case ScriptStartupBehavior.ShowTutorial: LoadScriptFromResource("Nexen.Debugger.Utilities.LuaScripts.Example.lua"); break;
				case ScriptStartupBehavior.LoadLastScript:
					if (Config.RecentScripts.Count > 0) {
						LoadScript(Config.RecentScripts[0]);
					}

					break;
			}
		}

		/// <summary>
		/// Initializes menu actions and toolbar for the script window.
		/// </summary>
		/// <param name="wnd">The parent script window.</param>
		public void InitActions(ScriptWindow wnd) {
			_wnd = wnd;

			ScriptMenuActions = GetScriptMenuActions();
			ToolbarActions = GetToolbarActions();

			FileMenuActions = GetSharedFileActions();
			FileMenuActions.AddRange(new List<ContextMenuAction>() {
				new ContextMenuAction() {
					ActionType = ActionType.SaveAs,
					OnClick = async () => await SaveAs(Path.GetFileName(FilePath))
				},
				new ContextMenuSeparator(),
				new ContextMenuAction() {
					ActionType = ActionType.BuiltInScripts,
					SubActions = GetBuiltInScriptActions()
				},
				new ContextMenuAction() {
					ActionType = ActionType.RecentScripts,
					IsEnabled = () => ConfigManager.Config.Debug.ScriptWindow.RecentScripts.Count > 0,
					SubActions = new() {
						GetRecentMenuItem(0),
						GetRecentMenuItem(1),
						GetRecentMenuItem(2),
						GetRecentMenuItem(3),
						GetRecentMenuItem(4),
						GetRecentMenuItem(5),
						GetRecentMenuItem(6),
						GetRecentMenuItem(7),
						GetRecentMenuItem(8),
						GetRecentMenuItem(9)
					}
				},
				new ContextMenuSeparator(),
				new ContextMenuAction() {
					ActionType = ActionType.Exit,
					OnClick = () => _wnd?.Close()
				}
			});

			HelpMenuActions = new() {
				new ContextMenuAction() {
					ActionType = ActionType.HelpApiReference,
					OnClick = () => {
						string tmpDoc = Path.Combine(ConfigManager.HomeFolder, "NexenLuaApiDoc.html");
						if(FileHelper.WriteAllText(tmpDoc, CodeCompletionHelper.GenerateHtmlDocumentation())) {
							ApplicationHelper.OpenBrowser(tmpDoc);
						}
					}
				}
			};

			DebugShortcutManager.RegisterActions(_wnd, ScriptMenuActions);
			DebugShortcutManager.RegisterActions(_wnd, FileMenuActions);
		}

		/// <summary>
		/// Creates a menu action for a recent script entry at the specified index.
		/// </summary>
		/// <param name="index">The index in the recent scripts list.</param>
		/// <returns>A menu action that loads the recent script when clicked.</returns>
		private MainMenuAction GetRecentMenuItem(int index) {
			return new MainMenuAction() {
				ActionType = ActionType.Custom,
				DynamicText = () => index < ConfigManager.Config.Debug.ScriptWindow.RecentScripts.Count ? ConfigManager.Config.Debug.ScriptWindow.RecentScripts[index] : "",
				IsVisible = () => index < ConfigManager.Config.Debug.ScriptWindow.RecentScripts.Count,
				OnClick = () => {
					if (index < ConfigManager.Config.Debug.ScriptWindow.RecentScripts.Count) {
						LoadScript(ConfigManager.Config.Debug.ScriptWindow.RecentScripts[index]);
					}
				}
			};
		}

		/// <summary>
		/// Gets the shared file menu actions (New, Open, Save) used in both menu and toolbar.
		/// </summary>
		/// <returns>List of file-related context menu actions.</returns>
		private List<ContextMenuAction> GetSharedFileActions() {
			return new() {
				new ContextMenuAction() {
					ActionType = ActionType.NewScript,
					Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.ScriptWindow_NewScript),
					OnClick = () => new ScriptWindow(new ScriptWindowViewModel(ScriptStartupBehavior.ShowBlankWindow)).Show()			   },
				new ContextMenuAction() {
					ActionType = ActionType.Open,
					Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.ScriptWindow_OpenScript),
					OnClick = () => OpenScript()
				},
				new ContextMenuAction() {
					ActionType = ActionType.Save,
					Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.ScriptWindow_SaveScript),
					OnClick = async () => await SaveScript()
				}
			};
		}

		/// <summary>
		/// Gets the toolbar actions combining file actions, script actions, and built-in scripts menu.
		/// </summary>
		/// <returns>List of toolbar actions.</returns>
		private List<ContextMenuAction> GetToolbarActions() {
			List<ContextMenuAction> actions = GetSharedFileActions();
			actions.Add(new ContextMenuSeparator());
			actions.AddRange(GetScriptMenuActions());
			actions.Add(new ContextMenuSeparator());
			actions.Add(new ContextMenuAction() {
				ActionType = ActionType.BuiltInScripts,
				AlwaysShowLabel = true,
				SubActions = GetBuiltInScriptActions()
			});
			return actions;
		}

		/// <summary>
		/// Gets menu actions for loading built-in example Lua scripts from embedded resources.
		/// </summary>
		/// <returns>List of actions for each embedded Lua script.</returns>
		private List<object> GetBuiltInScriptActions() {
			List<object> actions = new();
			Assembly assembly = Assembly.GetExecutingAssembly();
			foreach (string name in assembly.GetManifestResourceNames()) {
				if (Path.GetExtension(name).ToLower() == ".lua") {
					string scriptName = name.Substring(name.LastIndexOf('.', name.Length - 5) + 1);

					actions.Add(new ContextMenuAction() {
						ActionType = ActionType.Custom,
						CustomText = scriptName,
						OnClick = () => LoadScriptFromResource(name)
					});
				}
			}

			actions.Sort((a, b) => ((ContextMenuAction)a).Name.CompareTo(((ContextMenuAction)b).Name));
			return actions;
		}

		/// <summary>
		/// Loads a Lua script from an embedded assembly resource.
		/// </summary>
		/// <param name="resName">The full resource name (e.g., "Nexen.Debugger.Utilities.LuaScripts.Example.lua").</param>
		private void LoadScriptFromResource(string resName) {
			Assembly assembly = Assembly.GetExecutingAssembly();
			string scriptName = resName.Substring(resName.LastIndexOf('.', resName.Length - 5) + 1);
			using Stream? stream = assembly.GetManifestResourceStream(resName);
			if (stream != null) {
				using StreamReader sr = new StreamReader(stream);
				LoadScriptFromString(sr.ReadToEnd());
				ScriptName = scriptName;
				FilePath = "";
			}
		}

		/// <summary>
		/// Executes the current script in the emulator.
		/// Optionally saves the script first if configured to do so.
		/// </summary>
		public async void RunScript() {
			if (Config.SaveScriptBeforeRun && !string.IsNullOrWhiteSpace(FilePath)) {
				await SaveScript();
			}

			string path = (Path.GetDirectoryName(FilePath) ?? Program.OriginalFolder) + Path.DirectorySeparatorChar;
			UpdateScriptId(DebugApi.LoadScript(ScriptName.Length == 0 ? "DefaultName" : ScriptName, path, Code, ScriptId));
		}

		/// <summary>
		/// Updates the script ID on the UI thread.
		/// </summary>
		/// <param name="scriptId">The new script ID from the emulator core.</param>
		private void UpdateScriptId(int scriptId) {
			if (Dispatcher.UIThread.CheckAccess()) {
				ScriptId = scriptId;
			} else {
				Dispatcher.UIThread.Post(() => ScriptId = scriptId);
			}
		}

		/// <summary>
		/// Gets the Script menu actions (Run, Stop, Settings).
		/// </summary>
		/// <returns>List of script-related menu actions.</returns>
		private List<ContextMenuAction> GetScriptMenuActions() {
			return new() {
				new ContextMenuAction() {
					ActionType = ActionType.RunScript,
					Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.ScriptWindow_RunScript),
					OnClick = RunScript
				},
				new ContextMenuAction() {
					ActionType = ActionType.StopScript,
					Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.ScriptWindow_StopScript),
					IsEnabled = () => ScriptId >= 0,
					OnClick = StopScript
				},
				new ContextMenuSeparator(),
				new ContextMenuAction() {
					ActionType = ActionType.OpenDebugSettings,
					OnClick = () => DebuggerConfigWindow.Open(DebugConfigWindowTab.ScriptWindow, _wnd)
				}
			};
		}

		/// <summary>
		/// Stops the currently running script.
		/// </summary>
		public void StopScript() {
			DebugApi.RemoveScript(ScriptId);
			UpdateScriptId(-1);
		}

		/// <summary>
		/// Stops and restarts the current script (useful for applying code changes).
		/// </summary>
		public void RestartScript() {
			DebugApi.RemoveScript(ScriptId);
			string path = (Path.GetDirectoryName(FilePath) ?? Program.OriginalFolder) + Path.DirectorySeparatorChar;
			UpdateScriptId(DebugApi.LoadScript(ScriptName.Length == 0 ? "DefaultName" : ScriptName, path, Code, -1));
		}

		/// <summary>
		/// Gets the initial folder for file dialogs (from most recent script path).
		/// </summary>
		private string? InitialFolder {
			get {
				if (ConfigManager.Config.Debug.ScriptWindow.RecentScripts.Count > 0) {
					return Path.GetDirectoryName(ConfigManager.Config.Debug.ScriptWindow.RecentScripts[0]);
				}

				return null;
			}
		}

		/// <summary>
		/// Opens a file dialog to select and load a Lua script file.
		/// </summary>
		private async void OpenScript() {
			if (!await SavePrompt()) {
				return;
			}

			string? filename = await FileDialogHelper.OpenFile(InitialFolder, _wnd, FileDialogHelper.LuaExt);
			if (filename != null) {
				LoadScript(filename);
			}
		}

		/// <summary>
		/// Adds a script file to the recent scripts list.
		/// </summary>
		/// <param name="filename">The full path to the script file.</param>
		private void AddRecentScript(string filename) {
			ConfigManager.Config.Debug.ScriptWindow.AddRecentScript(filename);
		}

		/// <summary>
		/// Loads script content from a string and optionally auto-runs it.
		/// </summary>
		/// <param name="scriptContent">The Lua script source code.</param>
		private void LoadScriptFromString(string scriptContent) {
			Code = scriptContent;
			_originalText = Code;

			if (Config.AutoStartScriptOnLoad) {
				RunScript();
			}
		}

		/// <summary>
		/// Loads a script from a file path.
		/// </summary>
		/// <param name="filename">The full path to the Lua script file.</param>
		public void LoadScript(string filename) {
			if (File.Exists(filename)) {
				string? code = FileHelper.ReadAllText(filename);
				if (code != null) {
					AddRecentScript(filename);
					SetFilePath(filename);
					LoadScriptFromString(code);
				}
			}
		}

		/// <summary>
		/// Sets the file path and configures file watching for auto-reload.
		/// </summary>
		/// <param name="filename">The full path to the script file.</param>
		private void SetFilePath(string filename) {
			FilePath = filename;
			ScriptName = Path.GetFileName(filename);

			_fileWatcher.EnableRaisingEvents = false;

			_fileWatcher = new(Path.GetDirectoryName(FilePath) ?? "", Path.GetFileName(FilePath));
			_fileWatcher.Changed += (s, e) => {
				if (Config.AutoReloadScriptWhenFileChanges) {
					System.Threading.Thread.Sleep(100);
					Dispatcher.UIThread.Post(() => LoadScript(FilePath));
				}
			};
			_fileWatcher.EnableRaisingEvents = true;
		}

		/// <summary>
		/// Prompts the user to save unsaved changes before proceeding.
		/// </summary>
		/// <returns><c>true</c> if the operation can proceed, <c>false</c> if cancelled.</returns>
		public async Task<bool> SavePrompt() {
			if (_originalText != Code) {
				DialogResult result = await NexenMsgBox.Show(_wnd, "ScriptSaveConfirmation", MessageBoxButtons.YesNoCancel, MessageBoxIcon.Warning);
				if (result == DialogResult.Yes) {
					return await SaveScript();
				} else if (result == DialogResult.Cancel) {
					return false;
				}
			}

			return true;
		}

		/// <summary>
		/// Saves the current script to its file path, or prompts for Save As if unsaved.
		/// </summary>
		/// <returns><c>true</c> if save succeeded, <c>false</c> otherwise.</returns>
		private async Task<bool> SaveScript() {
			if (!string.IsNullOrWhiteSpace(FilePath)) {
				if (_originalText != Code) {
					if (FileHelper.WriteAllText(FilePath, Code, Encoding.UTF8)) {
						_originalText = Code;
					} else {
						return false;
					}
				}

				return true;
			} else {
				return await SaveAs("NewScript.lua");
			}
		}

		/// <summary>
		/// Prompts the user for a file name and saves the script.
		/// </summary>
		/// <param name="newName">The default file name suggestion.</param>
		/// <returns><c>true</c> if save succeeded, <c>false</c> if cancelled or failed.</returns>
		private async Task<bool> SaveAs(string newName) {
			string? filename = await FileDialogHelper.SaveFile(InitialFolder, newName, _wnd, FileDialogHelper.LuaExt);
			if (filename != null) {
				if (FileHelper.WriteAllText(filename, Code, Encoding.UTF8)) {
					AddRecentScript(filename);
					SetFilePath(filename);
					return true;
				}
			}

			return false;
		}
	}
}
