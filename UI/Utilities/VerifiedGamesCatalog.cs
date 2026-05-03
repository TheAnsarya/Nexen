using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text.Json;

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
	private static readonly Dictionary<string, string[]> _systemKeywords = new(StringComparer.OrdinalIgnoreCase) {
		["genesis"] = ["genesis", "mega drive", "megadrive", "sega md", "md"],
		["nes"] = ["nes", "famicom"],
		["snes"] = ["snes", "super nintendo", "super famicom"],
		["gb"] = ["game boy", "gb"],
		["gba"] = ["game boy advance", "gba"],
		["sms"] = ["master system", "sms"],
		["pce"] = ["pc engine", "turbografx", "tg16", "pce"],
		["ws"] = ["wonderswan", "ws"],
		["lynx"] = ["atari lynx", "lynx"]
	};

	private static readonly Dictionary<string, string[]> _systemExtensions = new(StringComparer.OrdinalIgnoreCase) {
		["genesis"] = [".md", ".bin", ".gen", ".smd"],
		["nes"] = [".nes", ".fds", ".nsf"],
		["snes"] = [".sfc", ".smc"],
		["gb"] = [".gb", ".gbc"],
		["gba"] = [".gba"],
		["sms"] = [".sms"],
		["pce"] = [".pce"],
		["ws"] = [".ws", ".wsc"],
		["lynx"] = [".lnx"]
	};

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
		Dictionary<string, VerifiedGameEntry> entries = new(StringComparer.OrdinalIgnoreCase);

		LoadSeedlistEntries(system, entries);
		LoadSeedListsManifestEntries(system, entries);

		return entries
			.Values
			.OrderByDescending(e => e.RomExists)
			.ThenBy(e => e.Name, StringComparer.OrdinalIgnoreCase)
			.ToList();
	}

	private static void LoadSeedlistEntries(VerifiedSystemInfo system, Dictionary<string, VerifiedGameEntry> entries) {
		string? seedlistsFolder = ResolveSeedlistsFolder();
		if (seedlistsFolder is null) {
			Log.Warn("[VerifiedGamesCatalog] Seedlists folder not found");
			return;
		}

		string path = Path.Combine(seedlistsFolder, system.SeedlistFileName);
		if (!File.Exists(path)) {
			Log.Warn($"[VerifiedGamesCatalog] Missing seedlist for system '{system.Id}': {path}");
			return;
		}

		foreach (string line in File.ReadLines(path)) {
			string trimmed = line.Trim();
			if (trimmed.Length == 0 || trimmed.StartsWith("#")) {
				continue;
			}

			string[] parts = trimmed.Split('|', 2, StringSplitOptions.TrimEntries);
			string name = parts[0];
			string romPath = parts.Length > 1 ? ExpandPath(parts[1]) : string.Empty;

			UpsertEntry(entries, name, romPath);
		}
	}

	private static void LoadSeedListsManifestEntries(VerifiedSystemInfo system, Dictionary<string, VerifiedGameEntry> entries) {
		try {
			foreach ((string name, string romPath) in ReadSeedListsManifestEntries(system)) {
				UpsertEntry(entries, name, romPath);
			}
		} catch (Exception ex) {
			Log.Warn($"[VerifiedGamesCatalog] Failed to read SeedLists manifests: {ex.Message}");
		}
	}

	private static void UpsertEntry(Dictionary<string, VerifiedGameEntry> entries, string name, string romPath) {
		if (string.IsNullOrWhiteSpace(name)) {
			return;
		}

		string resolvedPath = string.IsNullOrWhiteSpace(romPath) ? string.Empty : ExpandPath(romPath);
		if (!entries.TryGetValue(name, out VerifiedGameEntry? existing)) {
			entries[name] = new VerifiedGameEntry {
				Name = name,
				RomPath = resolvedPath
			};
			return;
		}

		if (!existing.RomExists && !string.IsNullOrWhiteSpace(resolvedPath)) {
			entries[name] = new VerifiedGameEntry {
				Name = name,
				RomPath = resolvedPath
			};
		}
	}

	private static IEnumerable<(string Name, string RomPath)> ReadSeedListsManifestEntries(VerifiedSystemInfo system) {
		string root = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "SeedLists", "dats");
		if (!Directory.Exists(root)) {
			yield break;
		}

		string[] manifests = Directory.GetFiles(root, "latest-sync-manifest.json", SearchOption.AllDirectories);
		if (manifests.Length == 0) {
			yield break;
		}

		foreach (string manifestPath in manifests) {
			using JsonDocument document = JsonDocument.Parse(File.ReadAllText(manifestPath));
			if (!document.RootElement.TryGetProperty("sources", out JsonElement sources) || sources.ValueKind != JsonValueKind.Array) {
				continue;
			}

			foreach (JsonElement source in sources.EnumerateArray()) {
				string name = source.TryGetProperty("name", out JsonElement nameElement) ? nameElement.GetString() ?? string.Empty : string.Empty;
				string sourceSystem = source.TryGetProperty("system", out JsonElement systemElement) ? systemElement.GetString() ?? string.Empty : string.Empty;
				string identifier = source.TryGetProperty("identifier", out JsonElement idElement) ? idElement.GetString() ?? string.Empty : string.Empty;

				if (!MatchesSystem(system.Id, sourceSystem, name, identifier)) {
					continue;
				}

				string romPath = ResolvePathFromIdentifier(identifier, system.Id);
				if (string.IsNullOrWhiteSpace(romPath)) {
					romPath = TryResolveReferenceRomPath(system.Id, name);
				}

				yield return (name, romPath);
			}
		}
	}

	private static bool MatchesSystem(string systemId, string sourceSystem, string name, string identifier) {
		if (!_systemKeywords.TryGetValue(systemId, out string[]? keywords) || keywords.Length == 0) {
			return false;
		}

		string haystack = $"{sourceSystem} {name} {identifier}";
		return keywords.Any(keyword => haystack.Contains(keyword, StringComparison.OrdinalIgnoreCase));
	}

	private static string ResolvePathFromIdentifier(string identifier, string systemId) {
		if (string.IsNullOrWhiteSpace(identifier) || !identifier.StartsWith("local::", StringComparison.OrdinalIgnoreCase)) {
			return string.Empty;
		}

		string candidate = identifier.Substring("local::".Length);
		if (string.IsNullOrWhiteSpace(candidate)) {
			return string.Empty;
		}

		string extension = Path.GetExtension(candidate);
		if (!_systemExtensions.TryGetValue(systemId, out string[]? extensions) || !extensions.Contains(extension, StringComparer.OrdinalIgnoreCase)) {
			return string.Empty;
		}

		return candidate;
	}

	private static string TryResolveReferenceRomPath(string systemId, string name) {
		if (!_systemExtensions.TryGetValue(systemId, out string[]? extensions) || extensions.Length == 0) {
			return string.Empty;
		}

		string root = Path.Combine("C:\\~reference-roms", systemId);
		if (!Directory.Exists(root)) {
			return string.Empty;
		}

		foreach (string extension in extensions) {
			string candidate = Path.Combine(root, $"{name}{extension}");
			if (File.Exists(candidate)) {
				return candidate;
			}
		}

		return string.Empty;
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
