using System;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Net.Http;
using System.Net.Http.Headers;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using StreamHash.Core;
using System.Text.Json;
using System.Text.Json.Serialization;
using System.Threading.Tasks;
using Avalonia.Threading;
using Nexen.Config;
using Nexen.Interop;
using Nexen.Utilities;
using Nexen.Windows;
using ReactiveUI.Fody.Helpers;

namespace Nexen.ViewModels;
/// <summary>
/// ViewModel for the update prompt dialog.
/// Handles checking for updates, downloading, and installing new versions.
/// </summary>
public sealed class UpdatePromptViewModel : ViewModelBase {
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
	/// Checks for available updates from GitHub Releases.
	/// </summary>
	/// <param name="silent">If true, suppresses error dialogs.</param>
	/// <returns>Update ViewModel if an update is available; otherwise null.</returns>
	public static async Task<UpdatePromptViewModel?> GetUpdateInformation(bool silent) {
		try {
			using var client = new HttpClient();
			// GitHub API requires User-Agent header
			client.DefaultRequestHeaders.UserAgent.Add(new ProductInfoHeaderValue("Nexen", EmuApi.GetNexenVersion().ToString()));

			string releaseData = await client.GetStringAsync("https://api.github.com/repos/TheAnsarya/Nexen/releases/latest");
			var release = (GitHubRelease?)JsonSerializer.Deserialize(releaseData, typeof(GitHubRelease), NexenSerializerContext.Default);

			if (release is null || string.IsNullOrEmpty(release.TagName)) {
				return null;
			}

			// Parse version from tag (e.g., "v2.0.0" -> "2.0.0")
			string versionStr = release.TagName.TrimStart('v', 'V');
			if (!Version.TryParse(versionStr, out Version? latestVersion)) {
				return null;
			}

			// Find the appropriate asset for this platform
			string platform = GetPlatformIdentifier();
			var asset = release.Assets?.FirstOrDefault(a =>
				a.Name?.Contains(platform, StringComparison.OrdinalIgnoreCase) == true);

			var updateInfo = new UpdateInfo {
				LatestVersion = latestVersion,
				ReleaseNotes = release.Body ?? "",
				Files = release.Assets?
					.Where(a => a.BrowserDownloadUrl?.StartsWith("https://github.com/TheAnsarya/Nexen/") == true)
					.Select(a => new UpdateFileInfo {
						Platform = [a.Name ?? ""],
						DownloadUrl = a.BrowserDownloadUrl ?? "",
						Hash = "" // GitHub doesn't provide SHA256 in API, hash check will be skipped
					})
					.ToArray() ?? []
			};

			UpdateFileInfo? file = asset is not null
				? new UpdateFileInfo {
					Platform = [asset.Name ?? ""],
					DownloadUrl = asset.BrowserDownloadUrl ?? "",
					Hash = ""
				}
				: null;

			return new UpdatePromptViewModel(updateInfo, file);
		} catch (Exception ex) {
			if (!silent) {
				Dispatcher.UIThread.Post(() => NexenMsgBox.ShowException(ex));
			}
			return null;
		}
	}

	/// <summary>
	/// Gets the platform identifier string for matching release assets.
	/// </summary>
	private static string GetPlatformIdentifier() {
		string platform;
		if (OperatingSystem.IsWindows()) {
			platform = "win";
		} else if (OperatingSystem.IsLinux()) {
			platform = "linux";
		} else if (OperatingSystem.IsMacOS()) {
			platform = "macos";
		} else {
			platform = "unknown";
		}

		string arch = RuntimeInformation.OSArchitecture.ToString().ToLower();
		return $"{platform}-{arch}";
	}

	public async Task<bool> UpdateNexen(UpdatePromptWindow wnd) {
		if (FileInfo is null) {
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

			using var sha256Hasher = HashFacade.CreateStreaming(HashAlgorithm.Sha256);
			memoryStream.Position = 0;
			byte[] hashBuffer = new byte[8192];
			int hashBytesRead;
			while ((hashBytesRead = memoryStream.Read(hashBuffer, 0, hashBuffer.Length)) > 0) {
				sha256Hasher.Update(hashBuffer.AsSpan(0, hashBytesRead));
			}
			string hash = sha256Hasher.FinalizeHex().ToUpperInvariant();
			// If hash is provided, verify it; otherwise skip hash check (GitHub releases don't include SHA256)
			if (!string.IsNullOrEmpty(FileInfo.Hash) && hash != FileInfo.Hash) {
				File.Delete(downloadPath);
				Dispatcher.UIThread.Post(() => NexenMsgBox.Show(wnd, "AutoUpdateInvalidFile", MessageBoxButtons.OK, MessageBoxIcon.Info, FileInfo.Hash, hash));
				return false;
			}

			// Extract the downloaded file
			memoryStream.Position = 0;
			using ZipArchive archive = new ZipArchive(memoryStream);
			foreach (var entry in archive.Entries) {
				downloadPath += Path.GetExtension(entry.Name);
				entry.ExtractToFile(downloadPath, true);
				break;
			}
		}

		return UpdateHelper.LaunchUpdate(downloadPath);
	}
}

public sealed class UpdateFileInfo {
	public string[] Platform { get; set; } = [];
	public string DownloadUrl { get; set; } = "";
	public string Hash { get; set; } = "";
}

public sealed class UpdateInfo {
	public Version LatestVersion { get; set; } = new();
	public string ReleaseNotes { get; set; } = "";
	public UpdateFileInfo[] Files { get; set; } = [];
}

/// <summary>
/// Represents a GitHub release from the API.
/// </summary>
public sealed class GitHubRelease {
	[JsonPropertyName("tag_name")]
	public string? TagName { get; set; }

	[JsonPropertyName("name")]
	public string? Name { get; set; }

	[JsonPropertyName("body")]
	public string? Body { get; set; }

	[JsonPropertyName("prerelease")]
	public bool Prerelease { get; set; }

	[JsonPropertyName("assets")]
	public GitHubAsset[]? Assets { get; set; }
}

/// <summary>
/// Represents a GitHub release asset (downloadable file).
/// </summary>
public sealed class GitHubAsset {
	[JsonPropertyName("name")]
	public string? Name { get; set; }

	[JsonPropertyName("browser_download_url")]
	public string? BrowserDownloadUrl { get; set; }

	[JsonPropertyName("size")]
	public long Size { get; set; }
}
