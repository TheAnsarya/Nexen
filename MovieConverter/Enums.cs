namespace Nexen.MovieConverter;

/// <summary>
/// Represents the format of a TAS movie file
/// </summary>
public enum MovieFormat {
	/// <summary>Unknown or unsupported format</summary>
	Unknown,

	/// <summary>Nexen native format (.nexen-movie) - ZIP-based, human-readable</summary>
	Nexen,

	/// <summary>Mesen/Mesen2 format (.mmm/.mmo)</summary>
	Mesen,

	/// <summary>Snes9x format (.smv)</summary>
	Smv,

	/// <summary>lsnes format (.lsmv) - ZIP-based</summary>
	Lsmv,

	/// <summary>FCEUX format (.fm2) - text-based, NES only</summary>
	Fm2,

	/// <summary>BizHawk format (.bk2) - ZIP-based, multi-system</summary>
	Bk2,

	/// <summary>VisualBoyAdvance format (.vbm)</summary>
	Vbm,

	/// <summary>Gens format (.gmv) - Genesis</summary>
	Gmv,

	/// <summary>Mednafen format (.mcm)</summary>
	Mcm,

	/// <summary>Mesen-S format (.msm) - SNES</summary>
	MesenS,

	/// <summary>BSNES format (.bsv)</summary>
	Bsv,

	/// <summary>DeSmuME format (.dsm) - Nintendo DS</summary>
	Dsm,

	/// <summary>Dolphin format (.dtm) - GameCube/Wii</summary>
	Dtm
}

/// <summary>
/// Target system/console type
/// </summary>
public enum SystemType {
	/// <summary>Nintendo Entertainment System (Famicom)</summary>
	Nes,

	/// <summary>Super Nintendo Entertainment System (Super Famicom)</summary>
	Snes,

	/// <summary>Game Boy (Original)</summary>
	Gb,

	/// <summary>Game Boy Color</summary>
	Gbc,

	/// <summary>Game Boy Advance</summary>
	Gba,

	/// <summary>Sega Master System</summary>
	Sms,

	/// <summary>Sega Genesis (Mega Drive)</summary>
	Genesis,

	/// <summary>PC Engine (TurboGrafx-16)</summary>
	Pce,

	/// <summary>WonderSwan</summary>
	Ws,

	/// <summary>Nintendo DS</summary>
	Nds,

	/// <summary>Sega Saturn</summary>
	Saturn,

	/// <summary>PlayStation</summary>
	Psx,

	/// <summary>Atari 2600</summary>
	A2600,

	/// <summary>Atari 7800</summary>
	A7800,

	/// <summary>Neo Geo</summary>
	NeoGeo,

	/// <summary>ColecoVision</summary>
	Coleco,

	/// <summary>MSX</summary>
	Msx,

	/// <summary>Sega Game Gear</summary>
	GameGear,

	/// <summary>Nintendo 64</summary>
	N64,

	/// <summary>Atari Lynx</summary>
	Lynx,

	/// <summary>Virtual Boy</summary>
	VirtualBoy,

	/// <summary>Other or unknown system</summary>
	Other,

	// Aliases for compatibility with various format converters

	/// <summary>Alias for Pce (PC Engine / TurboGrafx-16)</summary>
	PcEngine = Pce,

	/// <summary>Alias for Ws (WonderSwan)</summary>
	WonderSwan = Ws
}

/// <summary>
/// Video region/timing standard
/// </summary>
public enum RegionType {
	/// <summary>NTSC (60Hz, ~60.098814 fps for NES/SNES)</summary>
	NTSC,

	/// <summary>PAL (50Hz, ~50.006979 fps for NES/SNES)</summary>
	PAL,

	/// <summary>SECAM (similar to PAL timing)</summary>
	SECAM,

	/// <summary>Dendy (Russian NES clone, 50Hz but different timing)</summary>
	Dendy
}

/// <summary>
/// Controller/input device type
/// </summary>
public enum ControllerType {
	/// <summary>No controller connected</summary>
	None,

	/// <summary>Standard gamepad (varies by system)</summary>
	Gamepad,

	/// <summary>Mouse (SNES Mouse, etc.)</summary>
	Mouse,

	/// <summary>Super Scope (SNES light gun)</summary>
	SuperScope,

	/// <summary>Konami Justifier (light gun)</summary>
	Justifier,

	/// <summary>Multitap (4+ player adapter)</summary>
	Multitap,

	/// <summary>NES Four Score (4-player adapter)</summary>
	FourScore,

	/// <summary>Zapper (NES light gun)</summary>
	Zapper,

	/// <summary>Power Pad / Family Trainer</summary>
	PowerPad,

	/// <summary>Arkanoid Paddle Controller</summary>
	Paddle,

	/// <summary>SNES Multitap (5 player adapter)</summary>
	SnesMultitap,

	/// <summary>Famicom Expansion Port Device</summary>
	FamicomExpansion,

	/// <summary>Virtual Boy Controller</summary>
	VirtualBoy,

	/// <summary>Keyboard (Famicom BASIC, etc.)</summary>
	Keyboard,

	/// <summary>Mahjong Controller</summary>
	Mahjong,

	/// <summary>Party Tap / Quiz Controller</summary>
	PartyTap,

	/// <summary>Family BASIC Data Recorder</summary>
	DataRecorder,

	/// <summary>Turbo File (SNES storage device)</summary>
	TurboFile,

	/// <summary>Battle Box</summary>
	BattleBox,

	/// <summary>Barcode Reader</summary>
	BarcodeReader,

	/// <summary>Satellaview</summary>
	Satellaview
}

/// <summary>
/// Special frame commands (reset, disk insert, etc.)
/// </summary>
[Flags]
public enum FrameCommand {
	/// <summary>Normal frame, no special command</summary>
	None = 0,

	/// <summary>Soft reset (NMI/reset button)</summary>
	SoftReset = 1 << 0,

	/// <summary>Hard reset (power cycle)</summary>
	HardReset = 1 << 1,

	/// <summary>FDS: Insert disk</summary>
	FdsInsert = 1 << 2,

	/// <summary>FDS: Select disk side</summary>
	FdsSelect = 1 << 3,

	/// <summary>VS System: Insert coin</summary>
	VsInsertCoin = 1 << 4,

	/// <summary>SNES: Controller port swap</summary>
	ControllerSwap = 1 << 5,

	/// <summary>Pause emulation (for sync frames)</summary>
	Pause = 1 << 6,

	/// <summary>Frame advance</summary>
	FrameAdvance = 1 << 7,

	/// <summary>Fast forward marker</summary>
	FastForward = 1 << 8,

	/// <summary>Lag frame marker (input not polled)</summary>
	LagFrame = 1 << 9,

	/// <summary>Subframe input (multiple inputs per frame)</summary>
	SubFrame = 1 << 10,

	/// <summary>VS System: Service button</summary>
	VsServiceButton = 1 << 11,

	/// <summary>VS System: Dip switch change</summary>
	VsDipSwitch = 1 << 12,

	/// <summary>Barcode input</summary>
	BarcodeInput = 1 << 13,

	/// <summary>Power off</summary>
	PowerOff = 1 << 14,

	// Aliases for compatibility with various format converters

	/// <summary>Alias for SoftReset</summary>
	Reset = SoftReset,

	/// <summary>Alias for HardReset</summary>
	Power = HardReset,

	/// <summary>Alias for FdsSelect</summary>
	FdsSide = FdsSelect
}

/// <summary>
/// Movie start type
/// </summary>
public enum StartType {
	/// <summary>Start from power-on (cold boot)</summary>
	PowerOn,

	/// <summary>Start from soft reset</summary>
	Reset,

	/// <summary>Start from savestate</summary>
	Savestate,

	/// <summary>Start from SRAM</summary>
	Sram,

	/// <summary>Start from BIOS</summary>
	Bios
}

/// <summary>
/// Timing mode for the movie
/// </summary>
public enum TimingMode {
	/// <summary>Count all frames including lag frames</summary>
	AllFrames,

	/// <summary>Count only non-lag frames (input frames)</summary>
	InputFrames,

	/// <summary>Use CPU cycles for timing</summary>
	CpuCycles,

	/// <summary>Use emulated time</summary>
	EmulatedTime
}

/// <summary>
/// Synchronization mode for TAS playback
/// </summary>
public enum SyncMode {
	/// <summary>Standard frame-based sync</summary>
	Frame,

	/// <summary>Cycle-accurate sync</summary>
	Cycle,

	/// <summary>Sample-accurate audio sync</summary>
	Sample
}

/// <summary>
/// Type of movie marker
/// </summary>
public enum MarkerType {
	/// <summary>General bookmark/marker</summary>
	Bookmark,

	/// <summary>Subtitle/text overlay</summary>
	Subtitle,

	/// <summary>Input display</summary>
	InputDisplay,

	/// <summary>Greenzone (savestate point)</summary>
	Greenzone,

	/// <summary>Auto-save point</summary>
	AutoSave,

	/// <summary>User comment</summary>
	Comment,

	/// <summary>Chapter marker</summary>
	Chapter,

	/// <summary>Segment boundary</summary>
	Segment
}
