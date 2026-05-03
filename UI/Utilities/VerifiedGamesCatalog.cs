using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;

namespace Nexen.Utilities;

public sealed class VerifiedSystemInfo {
	public required string Id { get; init; }
	public required string Name { get; init; }
	public required string IconPath { get; init; }
	public required string SeedlistFileName { get; init; }
}

public sealed class VerifiedGameEntry {
	public required string Name { get; init; }
	public required string RomPath { get; init; }
	public bool RomExists => !string.IsNullOrWhiteSpace(RomPath) && File.Exists(RomPath);
}

public static class VerifiedGamesCatalog {
	private static readonly List<VerifiedSystemInfo> _systems = [
		new() { Id = "genesis", Name = "Genesis / Mega Drive", IconPath = "/Assets/Drive.svg", SeedlistFileName = "genesis.txt" },
		new() { Id = "nes", Name = "Nintendo Entertainment System", IconPath = "/Assets/NesIcon.svg", SeedlistFileName = "nes.txt" },
		new() { Id = "snes", Name = "Super Nintendo", IconPath = "/Assets/SnesIcon.svg", SeedlistFileName = "snes.txt" },
		new() { Id = "gb", Name = "Game Boy", IconPath = "/Assets/GameboyIcon.svg", SeedlistFileName = "gb.txt" },
		new() { Id = "gba", Name = "Game Boy Advance", IconPath = "/Assets/GbaIcon.svg", SeedlistFileName = "gba.txt" },
		new() { Id = "sms", Name = "Sega Master System", IconPath = "/Assets/SmsIcon.svg", SeedlistFileName = "sms.txt" },
		new() { Id = "pce", Name = "PC Engine", IconPath = "/Assets/PceIcon.svg", SeedlistFileName = "pce.txt" },
		new() { Id = "ws", Name = "WonderSwan", IconPath = "/Assets/WsIcon.svg", SeedlistFileName = "ws.txt" },
		new() { Id = "lynx", Name = "Atari Lynx", IconPath = "/Assets/LynxIcon.svg", SeedlistFileName = "lynx.txt" }
	];

	public static IReadOnlyList<VerifiedSystemInfo> GetSystems() {
		return _systems;
	}

	public static List<VerifiedGameEntry> LoadGames(VerifiedSystemInfo system) {
		string? seedlistsFolder = ResolveSeedlistsFolder();
		if (seedlistsFolder is null) {
			Log.Warn("[VerifiedGamesCatalog] Seedlists folder not found");
			return [];
		}

		string path = Path.Combine(seedlistsFolder, system.SeedlistFileName);
		if (!File.Exists(path)) {
			Log.Warn($"[VerifiedGamesCatalog] Missing seedlist for system '{system.Id}': {path}");
			return [];
		}

		List<VerifiedGameEntry> entries = new();
		foreach (string line in File.ReadLines(path)) {
			string trimmed = line.Trim();
			if (trimmed.Length == 0 || trimmed.StartsWith("#")) {
				continue;
			}

			string[] parts = trimmed.Split('|', 2, StringSplitOptions.TrimEntries);
			if (parts.Length == 1) {
				entries.Add(new VerifiedGameEntry {
					Name = parts[0],
					RomPath = string.Empty
				});
				continue;
			}

			entries.Add(new VerifiedGameEntry {
				Name = parts[0],
				RomPath = ExpandPath(parts[1])
			});
		}

		return entries;
	}

	private static string ExpandPath(string rawPath) {
		return Environment.ExpandEnvironmentVariables(rawPath.Trim());
	}

	private static string? ResolveSeedlistsFolder() {
		List<string> candidates = [
			Path.Combine(AppContext.BaseDirectory, "seedlists"),
			Path.Combine(Program.OriginalFolder, "seedlists")
		];

		string? exeFolder = Path.GetDirectoryName(Program.ExePath);
		if (!string.IsNullOrWhiteSpace(exeFolder)) {
			candidates.Add(Path.Combine(exeFolder, "seedlists"));
		}

		DirectoryInfo? current = new DirectoryInfo(AppContext.BaseDirectory);
		for (int i = 0; i < 8 && current is not null; i++) {
			candidates.Add(Path.Combine(current.FullName, "seedlists"));
			current = current.Parent;
		}

		foreach (string candidate in candidates.Distinct(StringComparer.OrdinalIgnoreCase)) {
			if (Directory.Exists(candidate)) {
				Log.Info($"[VerifiedGamesCatalog] Using seedlists folder: {candidate}");
				return candidate;
			}
		}

		return null;
	}
}
