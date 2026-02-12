using System;
using System.Collections.Generic;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using Avalonia.Media.Imaging;
using Nexen.Config.Shortcuts;
using Nexen.Localization;
using Nexen.Utilities;

namespace Nexen.Interop;
public sealed class EmuApi {
	public const string DllName = "NexenCore.dll";
	private const string DllPath = EmuApi.DllName;

	[DllImport(DllPath)][return: MarshalAs(UnmanagedType.I1)] public static extern bool TestDll();
	[DllImport(DllPath)] public static extern void InitDll();

	[DllImport(DllPath, EntryPoint = "GetNexenVersion")] private static extern UInt32 GetNexenVersionWrapper();
	public static Version GetNexenVersion() {
		UInt32 version = GetNexenVersionWrapper();
		UInt32 revision = version & 0xFF;
		UInt32 minor = (version >> 8) & 0xFF;
		UInt32 major = (version >> 16) & 0xFFFF;
		return new Version((int)major, (int)minor, (int)revision);
	}

	[DllImport(DllPath, EntryPoint = "GetNexenBuildDate")] private static extern IntPtr GetNexenBuildDateWrapper();
	public static string GetNexenBuildDate() {
		return Utf8Utilities.PtrToStringUtf8(GetNexenBuildDateWrapper());
	}

	[DllImport(DllPath)] public static extern IntPtr RegisterNotificationCallback(NotificationListener.NotificationCallback callback);
	[DllImport(DllPath)] public static extern void UnregisterNotificationCallback(IntPtr notificationListener);

	[DllImport(DllPath)] public static extern void InitializeEmu([MarshalAs(UnmanagedType.LPUTF8Str)] string homeFolder, IntPtr windowHandle, IntPtr dxViewerHandle, [MarshalAs(UnmanagedType.I1)] bool useSoftwareRenderer, [MarshalAs(UnmanagedType.I1)] bool noAudio, [MarshalAs(UnmanagedType.I1)] bool noVideo, [MarshalAs(UnmanagedType.I1)] bool noInput);

	[DllImport(DllPath)] public static extern void Release();

	[DllImport(DllPath)][return: MarshalAs(UnmanagedType.I1)] public static extern bool IsRunning();
	[DllImport(DllPath)] public static extern void Stop();
	[DllImport(DllPath)] public static extern Int32 GetStopCode();

	[DllImport(DllPath)] public static extern void Pause();
	[DllImport(DllPath)] public static extern void Resume();
	[DllImport(DllPath)][return: MarshalAs(UnmanagedType.I1)] public static extern bool IsPaused();

	[DllImport(DllPath)] public static extern void TakeScreenshot();

	[DllImport(DllPath)] public static extern void ProcessAudioPlayerAction(AudioPlayerActionParams p);

	[DllImport(DllPath)]
	[return: MarshalAs(UnmanagedType.I1)]
	public static extern bool LoadRom(
		[MarshalAs(UnmanagedType.LPUTF8Str)] string filepath,
		[MarshalAs(UnmanagedType.LPUTF8Str)] string? patchFile = null
	);

	[DllImport(DllPath, EntryPoint = "GetRomInfo")] private static extern void GetRomInfoWrapper(out InteropRomInfo romInfo);
	public static RomInfo GetRomInfo() {
		InteropRomInfo info;
		EmuApi.GetRomInfoWrapper(out info);
		return new RomInfo(info);
	}

	[DllImport(DllPath)] public static extern void LoadRecentGame([MarshalAs(UnmanagedType.LPUTF8Str)] string filepath, [MarshalAs(UnmanagedType.I1)] bool resetGame);

	[DllImport(DllPath)] public static extern void AddKnownGameFolder([MarshalAs(UnmanagedType.LPUTF8Str)] string folder);

	[DllImport(DllPath)] public static extern void SetExclusiveFullscreenMode([MarshalAs(UnmanagedType.I1)] bool fullscreen, IntPtr windowHandle);

	[DllImport(DllPath)] public static extern TimingInfo GetTimingInfo(CpuType cpuType);

	[DllImport(DllPath)] public static extern double GetAspectRatio();
	[DllImport(DllPath)] public static extern FrameInfo GetBaseScreenSize();
	[DllImport(DllPath)] public static extern Int32 GetGameMemorySize(MemoryType type);

	[DllImport(DllPath)] public static extern void SetRendererSize(UInt32 width, UInt32 height);

	[DllImport(DllPath)] public static extern void ExecuteShortcut(ExecuteShortcutParams p);
	[DllImport(DllPath)][return: MarshalAs(UnmanagedType.I1)] public static extern bool IsShortcutAllowed(EmulatorShortcut shortcut, UInt32 shortcutParam = 0);

	[DllImport(DllPath, EntryPoint = "GetLog")] private static extern void GetLogWrapper(IntPtr outLog, Int32 maxLength);
	public static string GetLog() { return Utf8Utilities.CallStringApi(GetLogWrapper, 100000); }

	[DllImport(DllPath)] public static extern void WriteLogEntry([MarshalAs(UnmanagedType.LPUTF8Str)] string message);
	[DllImport(DllPath)] public static extern void DisplayMessage([MarshalAs(UnmanagedType.LPUTF8Str)] string title, [MarshalAs(UnmanagedType.LPUTF8Str)] string message, [MarshalAs(UnmanagedType.LPUTF8Str)] string? param1 = null);

	[DllImport(DllPath, EntryPoint = "GetRomHash")] private static extern void GetRomHashWrapper(HashType hashType, IntPtr outLog, Int32 maxLength);
	public static string GetRomHash(HashType hashType) {
		return Utf8Utilities.CallStringApi((IntPtr outLog, Int32 maxLength) => GetRomHashWrapper(hashType, outLog, maxLength), 1000000);
	}

	[DllImport(DllPath)] public static extern IntPtr GetArchiveRomList([MarshalAs(UnmanagedType.LPUTF8Str)] string filename, IntPtr outFileList, Int32 maxLength);

	[DllImport(DllPath)] public static extern void SaveState(UInt32 stateIndex);
	[DllImport(DllPath)] public static extern void LoadState(UInt32 stateIndex);
	[DllImport(DllPath)] public static extern void SaveStateFile([MarshalAs(UnmanagedType.LPUTF8Str)] string filepath);
	[DllImport(DllPath)] public static extern void LoadStateFile([MarshalAs(UnmanagedType.LPUTF8Str)] string filepath);

	// ========== Timestamped Save State API ==========

	[DllImport(DllPath, EntryPoint = "SaveTimestampedState")]
	private static extern void SaveTimestampedStateWrapper(IntPtr outFilepath, Int32 maxLength);

	/// <summary>
	/// Save a new timestamped save state.
	/// Creates a save state with datetime-based filename in the ROM's subdirectory.
	/// </summary>
	/// <returns>Full path to the saved file, or empty string on failure</returns>
	public static string SaveTimestampedState() {
		return Utf8Utilities.CallStringApi(SaveTimestampedStateWrapper, 2048);
	}

	[DllImport(DllPath, EntryPoint = "GetSaveStateList")]
	private static extern UInt32 GetSaveStateListWrapper([Out] InteropSaveStateInfo[] outInfoArray, UInt32 maxCount);

	/// <summary>
	/// Get list of all save states for the current ROM.
	/// Returns saves sorted by timestamp (newest first).
	/// </summary>
	/// <param name="maxCount">Maximum number of saves to return</param>
	/// <returns>Array of SaveStateInfo structs</returns>
	public static SaveStateInfo[] GetSaveStateList(int maxCount = 1000) {
		InteropSaveStateInfo[] interopArray = new InteropSaveStateInfo[maxCount];
		UInt32 count = GetSaveStateListWrapper(interopArray, (UInt32)maxCount);

		SaveStateInfo[] result = new SaveStateInfo[count];
		for (int i = 0; i < count; i++) {
			result[i] = new SaveStateInfo(interopArray[i]);
		}

		return result;
	}

	[DllImport(DllPath)] public static extern UInt32 GetSaveStateCount();

	[DllImport(DllPath)]
	[return: MarshalAs(UnmanagedType.I1)]
	public static extern bool DeleteSaveState([MarshalAs(UnmanagedType.LPUTF8Str)] string filepath);

	// ========== Recent Play Queue API ==========

	[DllImport(DllPath, EntryPoint = "SaveRecentPlayState")]
	private static extern void SaveRecentPlayStateWrapper(IntPtr outFilepath, Int32 maxLength);

	/// <summary>
	/// Save a Recent Play checkpoint (rotating slots 1-12).
	/// </summary>
	/// <returns>Full path to saved file, or empty string on failure</returns>
	public static string SaveRecentPlayState() {
		return Utf8Utilities.CallStringApi(SaveRecentPlayStateWrapper, 2048);
	}

	[DllImport(DllPath)]
	[return: MarshalAs(UnmanagedType.I1)]
	public static extern bool ShouldSaveRecentPlay();

	[DllImport(DllPath)]
	public static extern void ResetRecentPlayTimer();

	[DllImport(DllPath, EntryPoint = "GetRecentPlayStates")]
	private static extern UInt32 GetRecentPlayStatesWrapper([Out] InteropSaveStateInfo[] outInfoArray, UInt32 maxCount);

	/// <summary>
	/// Get Recent Play saves only, sorted newest first.
	/// </summary>
	public static SaveStateInfo[] GetRecentPlayStates(int maxCount = 12) {
		InteropSaveStateInfo[] interopArray = new InteropSaveStateInfo[maxCount];
		UInt32 count = GetRecentPlayStatesWrapper(interopArray, (UInt32)maxCount);

		SaveStateInfo[] result = new SaveStateInfo[count];
		for (int i = 0; i < count; i++) {
			result[i] = new SaveStateInfo(interopArray[i]);
		}

		return result;
	}

	// ========== Designated Save API ==========

	[DllImport(DllPath)]
	public static extern void SetDesignatedSave([MarshalAs(UnmanagedType.LPUTF8Str)] string filepath);

	[DllImport(DllPath, EntryPoint = "GetDesignatedSave")]
	private static extern void GetDesignatedSaveWrapper(IntPtr outFilepath, Int32 maxLength);

	/// <summary>
	/// Get the current designated save path for quick loading (F4).
	/// </summary>
	/// <returns>Path to designated save, or empty if none set</returns>
	public static string GetDesignatedSave() {
		return Utf8Utilities.CallStringApi(GetDesignatedSaveWrapper, 2048);
	}

	[DllImport(DllPath)]
	[return: MarshalAs(UnmanagedType.I1)]
	public static extern bool LoadDesignatedState();

	[DllImport(DllPath)]
	[return: MarshalAs(UnmanagedType.I1)]
	public static extern bool HasDesignatedSave();

	[DllImport(DllPath)]
	public static extern void ClearDesignatedSave();

	// ========== Per-ROM Save State Directory ==========

	/// <summary>
	/// Set the per-ROM save state directory for the C++ core.
	/// Called on ROM load to redirect save states to the GameDataManager folder.
	/// </summary>
	/// <param name="path">Full path to per-ROM save state directory, or empty to use default.</param>
	[DllImport(DllPath)]
	public static extern void SetPerRomSaveStateDirectory([MarshalAs(UnmanagedType.LPUTF8Str)] string path);

	// ========== Per-ROM Battery Save Directory ==========

	/// <summary>
	/// Set the per-ROM battery save directory for the C++ core.
	/// Called on ROM load to redirect battery saves (.sav, .srm, etc.) to per-game folders.
	/// </summary>
	/// <param name="path">Full path to per-ROM save directory, or empty to use default.</param>
	[DllImport(DllPath)]
	public static extern void SetPerRomSaveDirectory([MarshalAs(UnmanagedType.LPUTF8Str)] string path);

	[DllImport(DllPath, EntryPoint = "GetSaveStatePreview")] private static extern Int32 GetSaveStatePreviewWrapper([MarshalAs(UnmanagedType.LPUTF8Str)] string saveStatePath, [Out] byte[] imgData);
	/// <summary>
	/// Gets save state preview bitmap using ArrayPool to avoid ~1MB allocation per call.
	/// </summary>
	public static Bitmap? GetSaveStatePreview(string saveStatePath) {
		if (File.Exists(saveStatePath)) {
			byte[] buffer = System.Buffers.ArrayPool<byte>.Shared.Rent(512 * 478 * 4);
			try {
				Int32 size = EmuApi.GetSaveStatePreviewWrapper(saveStatePath, buffer);
				if (size > 0) {
					using MemoryStream stream = new MemoryStream(buffer, 0, size);
					return new Bitmap(stream);
				}
			} finally {
				System.Buffers.ArrayPool<byte>.Shared.Return(buffer);
			}
		}

		return null;
	}

	[DllImport(DllPath)][return: MarshalAs(UnmanagedType.I1)] public static extern bool GetConvertedCheat([In] InteropCheatCode input, ref InteropInternalCheatCode output);
	[DllImport(DllPath)] public static extern void SetCheats([In] InteropCheatCode[] cheats, UInt32 cheatCount);
	[DllImport(DllPath)] public static extern void ClearCheats();

	[DllImport(DllPath)] public static extern void InputBarcode(UInt64 barcode, UInt32 digitCount);
	[DllImport(DllPath)] public static extern void ProcessTapeRecorderAction(TapeRecorderAction action, [MarshalAs(UnmanagedType.LPUTF8Str)] string filename = "");
}

public struct TimingInfo {
	public double Fps;
	public UInt64 MasterClock;
	public UInt32 MasterClockRate;
	public UInt32 FrameCount;

	public UInt32 ScanlineCount;
	public Int32 FirstScanline;
	public UInt32 CycleCount;
}

public struct FrameInfo {
	public UInt32 Width;
	public UInt32 Height;
}

public enum RomFormat {
	Unknown,

	Sfc,
	Spc,

	Gb,
	Gbs,

	iNes,
	Unif,
	Fds,
	VsSystem,
	VsDualSystem,
	Nsf,
	StudyBox,

	Pce,
	PceCdRom,
	PceHes,

	Sms,
	GameGear,
	Sg,
	ColecoVision,

	Gba,

	Ws
}

public enum ConsoleType {
	Snes = 0,
	Gameboy = 1,
	Nes = 2,
	PcEngine = 3,
	Sms = 4,
	Gba = 5,
	Ws = 6,
}

public struct InteropDipSwitchInfo {
	public UInt32 DatabaseId;
	public UInt32 DipSwitchCount;
}

public struct InteropRomInfo {
	[MarshalAs(UnmanagedType.ByValArray, SizeConst = 2000)]
	public byte[] RomPath;
	[MarshalAs(UnmanagedType.ByValArray, SizeConst = 2000)]
	public byte[] PatchPath;

	public RomFormat Format;
	public ConsoleType ConsoleType;
	public InteropDipSwitchInfo DipSwitches;

	[MarshalAs(UnmanagedType.ByValArray, SizeConst = 5)]
	public CpuType[] CpuTypes;
	public UInt32 CpuTypeCount;
}

public sealed class RomInfo {
	public string RomPath = "";
	public string PatchPath = "";
	public RomFormat Format = RomFormat.Unknown;
	public ConsoleType ConsoleType = ConsoleType.Snes;
	public InteropDipSwitchInfo DipSwitches;
	public HashSet<CpuType> CpuTypes = [];

	public RomInfo() { }

	public RomInfo(InteropRomInfo romInfo) {
		RomPath = (ResourcePath)Utf8Utilities.GetStringFromArray(romInfo.RomPath);
		PatchPath = (ResourcePath)Utf8Utilities.GetStringFromArray(romInfo.PatchPath);
		Format = romInfo.Format;
		ConsoleType = romInfo.ConsoleType;
		DipSwitches = romInfo.DipSwitches;

		for (int i = 0; i < romInfo.CpuTypeCount; i++) {
			CpuTypes.Add(romInfo.CpuTypes[i]);
		}
	}

	public string GetRomName() {
		return Path.GetFileNameWithoutExtension(((ResourcePath)RomPath).FileName);
	}
}

public enum FirmwareType {
	DSP1,
	DSP1B,
	DSP2,
	DSP3,
	DSP4,
	ST010,
	ST011,
	ST018,
	Satellaview,
	SufamiTurbo,
	Gameboy,
	GameboyColor,
	GameboyAdvance,
	Sgb1GameboyCpu,
	Sgb2GameboyCpu,
	SGB1,
	SGB2,
	FDS,
	StudyBox,
	PceSuperCd,
	PceGamesExpress,
	ColecoVision,
	WonderSwan,
	WonderSwanColor,
	SwanCrystal,
	Ymf288AdpcmRom,
	SmsBootRom,
	GgBootRom
}

public struct MissingFirmwareMessage {
	public IntPtr Filename;
	public FirmwareType Firmware;
	public UInt32 Size;
	public UInt32 AltSize;
}

public struct SufamiTurboFilePromptMessage {
	[MarshalAs(UnmanagedType.ByValArray, SizeConst = 5000)]
	public byte[] Filename;
}

public struct ExecuteShortcutParams {
	public EmulatorShortcut Shortcut;
	public UInt32 Param;
	public IntPtr ParamPtr;
};

public enum AudioPlayerAction {
	NextTrack,
	PrevTrack,
	SelectTrack,
}

public struct AudioPlayerActionParams {
	public AudioPlayerAction Action;
	public UInt32 TrackNumber;
}

public enum TapeRecorderAction {
	Play,
	StartRecord,
	StopRecord
}

public enum CheatType : byte {
	NesGameGenie = 0,
	NesProActionRocky,
	NesCustom,
	GbGameGenie,
	GbGameShark,
	SnesGameGenie,
	SnesProActionReplay,
	PceRaw,
	PceAddress,
	SmsProActionReplay,
	SmsGameGenie
}

public static class CheatTypeExtensions {
	public static CpuType ToCpuType(this CheatType cheatType) {
		return cheatType switch {
			CheatType.NesGameGenie or CheatType.NesProActionRocky or CheatType.NesCustom => CpuType.Nes,
			CheatType.SnesGameGenie or CheatType.SnesProActionReplay => CpuType.Snes,
			CheatType.GbGameGenie or CheatType.GbGameShark => CpuType.Gameboy,
			CheatType.PceRaw or CheatType.PceAddress => CpuType.Pce,
			CheatType.SmsGameGenie or CheatType.SmsProActionReplay => CpuType.Sms,
			_ => throw new NotImplementedException("unsupported cheat type")
		};
	}
}

public struct InteropCheatCode {
	public CheatType Type;

	[MarshalAs(UnmanagedType.ByValArray, SizeConst = 16)]
	public byte[] Code;

	public InteropCheatCode(CheatType type, string code) {
		Type = type;

		Code = new byte[16];
		byte[] codeBytes = Encoding.UTF8.GetBytes(code);
		Array.Copy(codeBytes, Code, Math.Min(codeBytes.Length, 15));
	}
}

public struct InteropInternalCheatCode {
	public MemoryType MemType;
	public UInt32 Address;
	public Int16 Compare;
	public byte Value;
	public CheatType Type;
	public CpuType Cpu;
	[MarshalAs(UnmanagedType.I1)] public bool IsRamCode;
	[MarshalAs(UnmanagedType.I1)] public bool IsAbsoluteAddress;
}

public enum HashType {
	Sha1,
	Sha1Cheat
}

public struct SoftwareRendererSurface {
	public IntPtr FrameBuffer;
	public UInt32 Width;
	public UInt32 Height;
	[MarshalAs(UnmanagedType.I1)] public bool IsDirty;
}

public struct SoftwareRendererFrame {
	public SoftwareRendererSurface Frame;
	public SoftwareRendererSurface EmuHud;
	public SoftwareRendererSurface ScriptHud;
}

// ========== Timestamped Save State Types ==========

/// <summary>
/// Origin category for save state files.
/// Determines the colored badge shown in UI and the save state's purpose.
/// </summary>
/// <remarks>
/// Badge colors:
/// - Auto (0): Blue - Background periodic saves (every 20-30 min)
/// - Save (1): Green - User-initiated saves (Quick Save with Ctrl+S)
/// - Recent (2): Red - Recent play checkpoints (automatic 5-min interval queue)
/// - Lua (3): Yellow - Script-created saves
/// </remarks>
public enum SaveStateOrigin : byte {
	/// <summary>Auto-save (blue badge) - periodic background saves</summary>
	Auto = 0,
	/// <summary>User save (green badge) - Quick Save (Ctrl+S)</summary>
	Save = 1,
	/// <summary>Recent play (red badge) - 5-min interval queue</summary>
	Recent = 2,
	/// <summary>Lua script (yellow badge) - script-created saves</summary>
	Lua = 3
}

/// <summary>
/// Interop struct for marshaling save state info from native code.
/// </summary>
[StructLayout(LayoutKind.Sequential)]
public struct InteropSaveStateInfo {
	[MarshalAs(UnmanagedType.ByValArray, SizeConst = 2000)]
	public byte[] Filepath;
	[MarshalAs(UnmanagedType.ByValArray, SizeConst = 256)]
	public byte[] RomName;
	public Int64 Timestamp;
	public UInt32 FileSize;
	public SaveStateOrigin Origin;
}

/// <summary>
/// Managed save state information.
/// </summary>
public sealed class SaveStateInfo {
	/// <summary>Full path to save state file</summary>
	public string Filepath { get; set; } = "";

	/// <summary>ROM name this save is for</summary>
	public string RomName { get; set; } = "";

	/// <summary>DateTime when save was created</summary>
	public DateTime Timestamp { get; set; }

	/// <summary>File size in bytes</summary>
	public uint FileSize { get; set; }

	/// <summary>Origin category (Auto/Save/Recent/Lua)</summary>
	public SaveStateOrigin Origin { get; set; }

	public SaveStateInfo() { }

	public SaveStateInfo(InteropSaveStateInfo interop) {
		Filepath = Utf8Utilities.GetStringFromArray(interop.Filepath);
		RomName = Utf8Utilities.GetStringFromArray(interop.RomName);
		Timestamp = DateTimeOffset.FromUnixTimeSeconds(interop.Timestamp).LocalDateTime;
		FileSize = interop.FileSize;
		Origin = interop.Origin;
	}

	/// <summary>
	/// Get a friendly display string for the timestamp.
	/// Always includes both date and time.
	/// Examples: "Today 1/31/2026 2:30 PM", "Yesterday 1/30/2026 5:45 PM", "1/25/2026 3:00 PM"
	/// </summary>
	public string GetFriendlyTimestamp() {
		DateTime now = DateTime.Now;
		DateTime date = Timestamp.Date;

		string dateStr = Timestamp.ToString("M/d/yyyy");
		string timeStr = Timestamp.ToString("h:mm tt");

		if (date == now.Date) {
			return $"Today {dateStr} {timeStr}";
		} else if (date == now.Date.AddDays(-1)) {
			return $"Yesterday {dateStr} {timeStr}";
		} else {
			return $"{dateStr} {timeStr}";
		}
	}

	/// <summary>
	/// Get the badge color for this save state's origin.
	/// </summary>
	/// <returns>Hex color code (without #)</returns>
	public string GetBadgeColor() => Origin switch {
		SaveStateOrigin.Auto => "4a90d9",    // Blue
		SaveStateOrigin.Save => "5cb85c",    // Green
		SaveStateOrigin.Recent => "d9534f",  // Red
		SaveStateOrigin.Lua => "f0ad4e",     // Yellow
		_ => "777777"                         // Gray fallback
	};

	/// <summary>
	/// Get the badge label text for this save state's origin.
	/// </summary>
	public string GetBadgeLabel() => Origin switch {
		SaveStateOrigin.Auto => "Auto",
		SaveStateOrigin.Save => "Save",
		SaveStateOrigin.Recent => "Recent",
		SaveStateOrigin.Lua => "Lua",
		_ => "Unknown"
	};
}
