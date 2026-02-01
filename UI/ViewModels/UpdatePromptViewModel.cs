using System;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Net.Http;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Security.Cryptography;
using System.Text.Json;
using System.Threading.Tasks;
using Avalonia.Threading;
using Nexen.Config;
using Nexen.Interop;
using Nexen.Utilities;
using Nexen.Windows;
using ReactiveUI.Fody.Helpers;

namespace Nexen.ViewModels {
	/// <summary>
	/// ViewModel for the update prompt dialog.
	/// Handles checking for updates, downloading, and installing new versions.
	/// </summary>
	public class UpdatePromptViewModel : ViewModelBase {
		/// <summary>Gets the latest available version.</summary>
		public Version LatestVersion { get; }

		/// <summary>Gets the currently installed version.</summary>
		public Version InstalledVersion { get; }

		/// <summary>Gets the changelog/release notes for the update.</summary>
		public string Changelog { get; }

		/// <summary>Gets or sets whether an update is currently in progress.</summary>
		[Reactive] public bool IsUpdating { get; internal set; }

		/// <summary>Gets or sets the download progress percentage (0-100).</summary>
		[Reactive] public int Progress { get; internal set; }

		/// <summary>Gets the file information for the platform-specific update.</summary>
		public UpdateFileInfo? FileInfo { get; }

		/// <summary>
		/// Initializes a new instance of the <see cref="UpdatePromptViewModel"/> class.
		/// </summary>
		/// <param name="updateInfo">The update information from the server.</param>
		/// <param name="file">The platform-specific file to download.</param>
		public UpdatePromptViewModel(UpdateInfo updateInfo, UpdateFileInfo? file) {
			LatestVersion = updateInfo.LatestVersion;
			Changelog = updateInfo.ReleaseNotes;
			InstalledVersion = EmuApi.GetNexenVersion();

			FileInfo = file;
		}

		/// <summary>
		/// Checks for available updates from the server.
		/// </summary>
		/// <param name="silent">If true, suppresses error dialogs.</param>
		/// <returns>Update ViewModel if an update is available; otherwise null.</returns>
		public static async Task<UpdatePromptViewModel?> GetUpdateInformation(bool silent) {
			UpdateInfo? updateInfo = null;
			try {
				using (var client = new HttpClient()) {
					string updateData = await client.GetStringAsync("https://www.Nexen.ca/Services/v2/latestversion.json");
					updateInfo = (UpdateInfo?)JsonSerializer.Deserialize(updateData, typeof(UpdateInfo), NexenSerializerContext.Default);

					if (
						updateInfo == null ||
						updateInfo.Files == null ||
						updateInfo.Files.Where(f => f.DownloadUrl == null || (!f.DownloadUrl.StartsWith("https://www.Nexen.ca/") && !f.DownloadUrl.StartsWith("https://github.com/SourNexen/"))).Count() > 0
					) {
						return null;
					}
				}
			} catch (Exception ex) {
				if (!silent) {
					Dispatcher.UIThread.Post(() => NexenMsgBox.ShowException(ex));
				}
			}

			if (updateInfo != null) {
				string platform;
				if (OperatingSystem.IsWindows()) {
					platform = OperatingSystem.IsWindowsVersionAtLeast(10) ? "win" : "win7";
				} else if (OperatingSystem.IsLinux()) {
					platform = "linux";
				} else if (OperatingSystem.IsMacOS()) {
					platform = "macos";
				} else {
					return null;
				}

				platform += "-" + RuntimeInformation.OSArchitecture.ToString().ToLower();
				platform += RuntimeFeature.IsDynamicCodeSupported ? "-jit" : "-aot";

				if (OperatingSystem.IsLinux() && Program.ExePath.ToLower().EndsWith("appimage")) {
					platform += "-appimage";
				}

				UpdateFileInfo? file = updateInfo.Files.Where(f => f.Platform.Contains(platform)).FirstOrDefault();
				return updateInfo != null ? new UpdatePromptViewModel(updateInfo, file) : null;
			}

			return null;
		}

		public async Task<bool> UpdateNexen(UpdatePromptWindow wnd) {
			if (FileInfo == null) {
				return false;
			}

			string downloadPath = Path.Combine(ConfigManager.BackupFolder, "Nexen." + LatestVersion.ToString(3));

			using (var client = new HttpClient()) {
				HttpResponseMessage response = await client.GetAsync(FileInfo.DownloadUrl, HttpCompletionOption.ResponseHeadersRead);
				response.EnsureSuccessStatusCode();

				using Stream contentStream = await response.Content.ReadAsStreamAsync();
				using MemoryStream memoryStream = new MemoryStream();
				long? length = response.Content.Headers.ContentLength;
				if (length is null or 0) {
					return false;
				}

				byte[] buffer = new byte[0x10000];
				while (true) {
					int byteCount = await contentStream.ReadAsync(buffer, 0, buffer.Length);
					if (byteCount == 0) {
						break;
					} else {
						await memoryStream.WriteAsync(buffer, 0, byteCount);
						Dispatcher.UIThread.Post(() => Progress = (int)((double)memoryStream.Length / length * 100));
					}
				}

				using SHA256 sha256 = SHA256.Create();
				memoryStream.Position = 0;
				string hash = BitConverter.ToString(sha256.ComputeHash(memoryStream)).Replace("-", "");
				if (hash == FileInfo.Hash) {
					using ZipArchive archive = new ZipArchive(memoryStream);
					foreach (var entry in archive.Entries) {
						downloadPath += Path.GetExtension(entry.Name);
						entry.ExtractToFile(downloadPath, true);
						break;
					}
				} else {
					File.Delete(downloadPath);
					Dispatcher.UIThread.Post(() => NexenMsgBox.Show(wnd, "AutoUpdateInvalidFile", MessageBoxButtons.OK, MessageBoxIcon.Info, FileInfo.Hash, hash));
					return false;
				}
			}

			return UpdateHelper.LaunchUpdate(downloadPath);
		}
	}

	public class UpdateFileInfo {
		public string[] Platform { get; set; } = Array.Empty<string>();
		public string DownloadUrl { get; set; } = "";
		public string Hash { get; set; } = "";
	}

	public class UpdateInfo {
		public Version LatestVersion { get; set; } = new();
		public string ReleaseNotes { get; set; } = "";
		public UpdateFileInfo[] Files { get; set; } = Array.Empty<UpdateFileInfo>();
	}
}
