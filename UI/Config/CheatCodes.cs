using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Text;
using System.Text.Json;
using Nexen.Interop;
using Nexen.Utilities;
using Nexen.ViewModels;
using ReactiveUI.SourceGenerators;

namespace Nexen.Config;
public sealed partial class CheatCodes {
	private static string FilePath { get { return Path.Combine(ConfigManager.CheatFolder, EmuApi.GetRomInfo().GetRomName() + ".json"); } }

	public List<CheatCode> Cheats { get; set; } = [];

	public static CheatCodes LoadCheatCodes() {
		return Deserialize(CheatCodes.FilePath);
	}

	private static CheatCodes Deserialize(string path) {
		CheatCodes cheats = new CheatCodes();

		if (File.Exists(path)) {
			try {
				cheats = (CheatCodes?)JsonSerializer.Deserialize(File.ReadAllText(path), typeof(CheatCodes), NexenSerializerContext.Default) ?? new CheatCodes();
			} catch (Exception ex) {
				Debug.WriteLine($"CheatCodes.Deserialize failed for '{path}': {ex.Message}");
			}
		}

		return cheats;
	}

	public void Save() {
		try {
			if (Cheats.Count > 0) {
				FileHelper.WriteAllText(CheatCodes.FilePath, JsonSerializer.Serialize(this, typeof(CheatCodes), NexenSerializerContext.Default));
			} else {
				if (File.Exists(CheatCodes.FilePath)) {
					File.Delete(CheatCodes.FilePath);
				}
			}
		} catch (Exception ex) {
			Debug.WriteLine($"CheatCodes.Save failed: {ex.Message}");
		}
	}

	public static void ApplyCheats() {
		if (ConfigManager.Config.Cheats.DisableAllCheats) {
			EmuApi.ClearCheats();
		} else {
			CheatCodes.ApplyCheats(LoadCheatCodes().Cheats);
		}
	}

	public static void ApplyCheats(IEnumerable<CheatCode> cheats) {
		List<InteropCheatCode> encodedCheats = [];
		foreach (CheatCode cheat in cheats) {
			if (cheat.Enabled) {
				encodedCheats.AddRange(cheat.ToInteropCheats());
			}
		}

		EmuApi.SetCheats(encodedCheats.ToArray(), (UInt32)encodedCheats.Count);
	}
}

public sealed partial class CheatCode : ViewModelBase {
	[Reactive] public partial string Description { get; set; } = "";
	[Reactive] public partial CheatType Type { get; set; }
	[Reactive] public partial bool Enabled { get; set; } = true;
	[Reactive] public partial string Codes { get; set; } = "";

	public List<InteropCheatCode> ToInteropCheats() {
		List<InteropCheatCode> encodedCheats = [];
		foreach (string code in Codes.Split('\n', StringSplitOptions.TrimEntries | StringSplitOptions.RemoveEmptyEntries)) {
			encodedCheats.Add(new InteropCheatCode(Type, code));
		}

		return encodedCheats;
	}

	public CheatCode Clone() {
		return new() {
			Description = Description,
			Type = Type,
			Enabled = Enabled,
			Codes = Codes,
		};
	}

	public void CopyFrom(CheatCode copy) {
		Description = copy.Description;
		Type = copy.Type;
		Enabled = copy.Enabled;
		Codes = String.Join(Environment.NewLine, copy.Codes.Split('\n', StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries));
	}
}
