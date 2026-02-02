using System;
using System.Collections.Generic;
using System.IO;
using System.Threading.Tasks;
using Avalonia.Threading;
using Nexen.Config;
using Nexen.Utilities;
using Nexen.ViewModels;
using ReactiveUI;
using ReactiveUI.Fody.Helpers;

namespace Nexen.ViewModels;
/// <summary>
/// ViewModel for the storage folder selection dialog.
/// Handles migrating configuration between portable and user profile storage.
/// </summary>
public class SelectStorageFolderViewModel : ViewModelBase {
	/// <summary>Gets or sets whether to store in the user profile folder.</summary>
	[Reactive] public bool StoreInUserProfile { get; set; }

	/// <summary>Gets or sets whether a file copy operation is in progress.</summary>
	[Reactive] public bool IsCopying { get; set; }

	/// <summary>Gets or sets the copy operation progress percentage.</summary>
	[Reactive] public int CopyProgress { get; set; }

	/// <summary>Gets the destination folder path based on current selection.</summary>
	[Reactive] public string DestinationFolder { get; private set; } = "";

	/// <summary>
	/// Initializes a new instance of the <see cref="SelectStorageFolderViewModel"/> class.
	/// </summary>
	public SelectStorageFolderViewModel() {
		this.WhenAnyValue(x => x.StoreInUserProfile).Subscribe(x => DestinationFolder = x ? ConfigManager.DefaultDocumentsFolder : ConfigManager.DefaultPortableFolder);
	}

	/// <summary>
	/// Recursively builds lists of files to copy.
	/// </summary>
	private void GetFilesToCopy(string source, string target, List<string> sourceFiles, List<string> targetFiles) {
		if (!Directory.Exists(target)) {
			Directory.CreateDirectory(target);
		}

		foreach (string file in Directory.GetFiles(source)) {
			string ext = Path.GetExtension(file).ToLower();
			if (ext is not ".exe" and not ".dll" and not "") {
				sourceFiles.Add(file);
				targetFiles.Add(Path.Combine(target, Path.GetFileName(file)));
			}
		}

		foreach (string subdir in Directory.GetDirectories(source)) {
			GetFilesToCopy(subdir, Path.Combine(target, Path.GetFileName(subdir)), sourceFiles, targetFiles);
		}
	}

	private Task<bool> CopyFiles(string source, string target) {
		return Task.Run(() => {
			IsCopying = true;
			try {
				List<string> sourceFiles = new();
				List<string> targetFiles = new();
				GetFilesToCopy(source, target, sourceFiles, targetFiles);
				int fileCount = sourceFiles.Count;
				int filesCopied = 0;
				for (int i = 0; i < sourceFiles.Count; i++) {
					try {
						File.Copy(sourceFiles[i], targetFiles[i], true);
					} catch (Exception ex) {
						Dispatcher.UIThread.Post(() => NexenMsgBox.ShowException(ex));
						return false;
					}

					filesCopied++;
					CopyProgress = filesCopied * 100 / fileCount;
				}

				return true;
			} finally {
				IsCopying = false;
			}
		});
	}

	public async Task<bool> MigrateData() {
		string source = ConfigManager.HomeFolder;
		string target = StoreInUserProfile ? ConfigManager.DefaultDocumentsFolder : ConfigManager.DefaultPortableFolder;

		if (string.Compare(source.Trim(), target.Trim(), StringComparison.OrdinalIgnoreCase) == 0) {
			return false;
		}

		ConfigManager.Config.Save();

		if (await CopyFiles(source, target)) {
			try {
				if (File.Exists(Path.Combine(source, "settings.backup.json"))) {
					File.Delete(Path.Combine(source, "settings.backup.json"));
				}

				File.Move(Path.Combine(source, "settings.json"), Path.Combine(source, "settings.backup.json"));
			} catch (Exception ex) {
				Dispatcher.UIThread.Post(() => NexenMsgBox.ShowException(ex));
			}

			ConfigManager.ResetHomeFolder();
			return true;
		} else {
			return false;
		}
	}
}
