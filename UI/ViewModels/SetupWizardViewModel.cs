using System;
using System.Diagnostics;
using System.IO;
using Avalonia.Controls;
using Nexen.Config;
using Nexen.Utilities;
using Nexen.Windows;
using ReactiveUI;
using ReactiveUI.Fody.Helpers;

namespace Nexen.ViewModels {
	/// <summary>
	/// ViewModel for the first-run setup wizard.
	/// Handles initial configuration of storage location, controller mappings, and shortcuts.
	/// </summary>
	public class SetupWizardViewModel : ViewModelBase {
		/// <summary>Gets or sets whether to store configuration in the user profile folder.</summary>
		[Reactive] public bool StoreInUserProfile { get; set; } = true;

		/// <summary>Gets or sets whether to enable default Xbox controller mappings.</summary>
		[Reactive] public bool EnableXboxMappings { get; set; } = true;

		/// <summary>Gets or sets whether to enable default PlayStation controller mappings.</summary>
		[Reactive] public bool EnablePsMappings { get; set; }

		/// <summary>Gets or sets whether to enable WASD keyboard mappings.</summary>
		[Reactive] public bool EnableWasdMappings { get; set; }

		/// <summary>Gets or sets whether to enable arrow key mappings.</summary>
		[Reactive] public bool EnableArrowMappings { get; set; } = true;

		/// <summary>Gets or sets the install/configuration folder location.</summary>
		[Reactive] public string InstallLocation { get; set; }

		/// <summary>Gets or sets whether to create a desktop shortcut.</summary>
		[Reactive] public bool CreateShortcut { get; set; } = true;

		/// <summary>Gets or sets whether to enable automatic update checks.</summary>
		[Reactive] public bool CheckForUpdates { get; set; } = true;

		/// <summary>Gets whether the current platform is macOS.</summary>
		[Reactive] public bool IsOsx { get; set; } = OperatingSystem.IsMacOS();

		/// <summary>
		/// Initializes a new instance of the <see cref="SetupWizardViewModel"/> class.
		/// </summary>
		public SetupWizardViewModel() {
			InstallLocation = ConfigManager.DefaultDocumentsFolder;

			this.WhenAnyValue(x => x.StoreInUserProfile).Subscribe(x => InstallLocation = StoreInUserProfile ? ConfigManager.DefaultDocumentsFolder : ConfigManager.DefaultPortableFolder);

			this.WhenAnyValue(x => x.EnableWasdMappings).Subscribe(x => {
				if (x) {
					EnableArrowMappings = false;
				}
			});

			this.WhenAnyValue(x => x.EnableArrowMappings).Subscribe(x => {
				if (x) {
					EnableWasdMappings = false;
				}
			});
		}

		/// <summary>
		/// Confirms the setup choices and initializes configuration.
		/// </summary>
		/// <param name="parent">The parent window for error dialogs.</param>
		/// <returns>True if setup completed successfully.</returns>
		public bool Confirm(Window parent) {
			string targetFolder = StoreInUserProfile ? ConfigManager.DefaultDocumentsFolder : ConfigManager.DefaultPortableFolder;
			string testFile = Path.Combine(targetFolder, "test.txt");
			try {
				if (!Directory.Exists(targetFolder)) {
					Directory.CreateDirectory(targetFolder);
				}

				File.WriteAllText(testFile, "test");
				File.Delete(testFile);
				InitializeConfig();
				if (CreateShortcut) {
					CreateShortcutFile();
				}

				return true;
			} catch (Exception ex) {
				NexenMsgBox.Show(parent, "CannotWriteToFolder", MessageBoxButtons.OK, MessageBoxIcon.Error, ex.ToString());
			}

			return false;
		}

		private void InitializeConfig() {
			ConfigManager.CreateConfig(!StoreInUserProfile);
			DefaultKeyMappingType mappingType = DefaultKeyMappingType.None;
			if (EnableXboxMappings) {
				mappingType |= DefaultKeyMappingType.Xbox;
			}

			if (EnablePsMappings) {
				mappingType |= DefaultKeyMappingType.Ps4;
			}

			if (EnableWasdMappings) {
				mappingType |= DefaultKeyMappingType.WasdKeys;
			}

			if (EnableArrowMappings) {
				mappingType |= DefaultKeyMappingType.ArrowKeys;
			}

			ConfigManager.Config.DefaultKeyMappings = mappingType;
			ConfigManager.Config.Preferences.AutomaticallyCheckForUpdates = CheckForUpdates;
			ConfigManager.Config.Save();
		}

		private void CreateShortcutFile() {
			if (OperatingSystem.IsMacOS()) {
				//TODO OSX
				return;
			}

			if (OperatingSystem.IsWindows()) {
				string linkPath = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.DesktopDirectory), "Nexen.url");
				FileHelper.WriteAllText(linkPath,
					"[InternetShortcut]" + Environment.NewLine +
					"URL=file:///" + Program.ExePath + Environment.NewLine +
					"IconIndex=0" + Environment.NewLine +
					"IconFile=" + Program.ExePath.Replace('\\', '/') + Environment.NewLine
				);
			} else {
				string shortcutFile = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.Desktop), "nexen.desktop");
				FileAssociationHelper.CreateLinuxShortcutFile(shortcutFile);
				Process.Start("chmod", "744 " + shortcutFile);
			}
		}
	}
}
