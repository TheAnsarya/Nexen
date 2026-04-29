using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
using System.Threading.Tasks;
using Avalonia;
using Avalonia.Controls;
using Nexen.Config;
using Nexen.Interop;
using Nexen.Localization;
using Nexen.Windows;

namespace Nexen.Utilities;
public static class FirmwareHelper {
	private static FirmwareFiles GetKnownFirmwaresOrFallback(MissingFirmwareMessage msg, string filename) {
		try {
			return msg.Firmware.GetFirmwareInfo();
		} catch {
			List<FirmwareFileInfo> candidates = new();
			if (msg.Size > 0) {
				candidates.Add(new FirmwareFileInfo((int)msg.Size));
			}

			if (msg.AltSize > 0) {
				candidates.Add(new FirmwareFileInfo((int)msg.AltSize));
			}

			if (candidates.Count == 0) {
				candidates.Add(new FirmwareFileInfo(0));
			}

			FirmwareFiles fallback = new(filename);
			fallback.AddRange(candidates);
			return fallback;
		}
	}

	public static string GetFileHash(string filename) {
		var hashes = RomHashService.ComputeFileHashes(filename);
		return hashes.Sha256;
	}

	/// <summary>
	/// Scans the firmware folder for any file that matches one of the expected firmware hashes.
	/// Returns the path to the matching file, or null if no match is found.
	/// </summary>
	private static string? FindMatchingFirmwareInFolder(FirmwareFiles knownFirmwares) {
		string firmwareFolder = ConfigManager.FirmwareFolder;
		if (!Directory.Exists(firmwareFolder)) {
			return null;
		}

		foreach (string file in Directory.EnumerateFiles(firmwareFolder)) {
			try {
				long fileSize = new FileInfo(file).Length;
				string fileName = Path.GetFileName(file);
				foreach (FirmwareFileInfo knownFirmware in knownFirmwares) {
					if (knownFirmware.Size == fileSize) {
						if (knownFirmware.Hashes.Length == 0) {
							if (Array.Exists(knownFirmwares.Names, name => string.Equals(name, fileName, StringComparison.OrdinalIgnoreCase))) {
								return file;
							}
							continue;
						}

						string hash = GetFileHash(file);
						if (Array.IndexOf(knownFirmware.Hashes, hash) >= 0) {
							return file;
						}
					}
				}
			} catch (Exception ex) {
				// Skip files we can't read
				Debug.WriteLine($"FirmwareHelper: Skipping file '{file}': {ex.Message}");
			}
		}

		return null;
	}

	public static async Task RequestFirmwareFile(MissingFirmwareMessage msg) {
		string filename = Marshal.PtrToStringUTF8(msg.Filename) ?? "";
		Window? wnd = ApplicationHelper.GetMainWindow();

		// Try to auto-find a matching firmware file in the firmware folder
		FirmwareFiles knownFirmwares = GetKnownFirmwaresOrFallback(msg, filename);
		string? autoMatch = FindMatchingFirmwareInFolder(knownFirmwares);
		if (autoMatch is not null) {
			string destination = Path.Combine(ConfigManager.FirmwareFolder, knownFirmwares.Names[0]);
			if (autoMatch != destination) {
				File.Copy(autoMatch, destination, true);
			}
			return;
		}

		if (await NexenMsgBox.Show(wnd, "FirmwareNotFound", MessageBoxButtons.OKCancel, MessageBoxIcon.Question, ResourceHelper.GetEnumText(msg.Firmware), filename, msg.Size.ToString()) == DialogResult.OK) {
			while (true) {
				string? selectedFile = await FileDialogHelper.OpenFile(null, wnd, FileDialogHelper.FirmwareExt);
				if (selectedFile is not null) {
					try {
						if (await SelectFirmwareFile(msg.Firmware, selectedFile, ApplicationHelper.GetMainWindow())) {
							break;
						}
					} catch (Exception ex) {
						await NexenMsgBox.ShowException(ex);
					}
				} else {
					break;
				}
			}
		}
	}

	public static async Task<bool> SelectFirmwareFile(FirmwareType type, string selectedFile, Visual? wnd) {
		FirmwareFiles knownFirmwares;
		try {
			knownFirmwares = type.GetFirmwareInfo();
		} catch {
			knownFirmwares = new(Path.GetFileName(selectedFile)) {
				new((int)new FileInfo(selectedFile).Length)
			};
		}

		long fileSize = new FileInfo(selectedFile).Length;

		bool foundSizeMatch = false;
		foreach (FirmwareFileInfo knownFirmware in knownFirmwares) {
			if (knownFirmware.Size == fileSize) {
				foundSizeMatch = true;
			}
		}

		if (!foundSizeMatch) {
			await NexenMsgBox.Show(wnd, "FirmwareFileWrongSize", MessageBoxButtons.OK, MessageBoxIcon.Error, knownFirmwares[0].Size.ToString());
			return false;
		}

		string fileHash = GetFileHash(selectedFile);
		bool hashMatches = false;
		bool hasKnownHashes = false;
		foreach (FirmwareFileInfo knownFirmware in knownFirmwares) {
			if (knownFirmware.Hashes.Length > 0) {
				hasKnownHashes = true;
				if (Array.IndexOf(knownFirmware.Hashes, fileHash) >= 0) {
					hashMatches = true;
					break;
				}
			}
		}

		if (hasKnownHashes && !hashMatches) {
			if (await NexenMsgBox.Show(wnd, "FirmwareMismatch", MessageBoxButtons.OKCancel, MessageBoxIcon.Warning, ResourceHelper.GetEnumText(type), knownFirmwares[0].Hashes[0], fileHash) != DialogResult.OK) {
				//Files don't match and user cancelled the action, retry
				return false;
			}
		}

		string destination = Path.Combine(ConfigManager.FirmwareFolder, knownFirmwares.Names[0]);
		if (selectedFile != destination) {
			if (File.Exists(destination)) {
				if (await NexenMsgBox.Show(wnd, "OverwriteFirmware", MessageBoxButtons.OKCancel, MessageBoxIcon.Warning, destination) != DialogResult.OK) {
					//Don't overwrite the existing file
					return true;
				}
			}

			File.Copy(selectedFile, destination, true);
		}

		return true;
	}
}
