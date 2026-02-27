#pragma once
#include "pch.h"
#include "Core/Debugger/DebugTypes.h"
#include "Core/Debugger/Debugger.h"
#include "Core/Debugger/DebugUtilities.h"
#include "Core/Shared/EmulatorLock.h"
#include "Core/Shared/Interfaces/IConsole.h"
#include "Core/Shared/LightweightCdlRecorder.h"
#include "Core/Shared/Audio/AudioPlayerTypes.h"
#include "Utilities/Timer.h"
#include "Utilities/safe_ptr.h"
#include "Utilities/SimpleLock.h"
#include "Utilities/VirtualFile.h"

class Debugger;
class DebugHud;
class SoundMixer;
class VideoRenderer;
class VideoDecoder;
class NotificationManager;
class EmuSettings;
class SaveStateManager;
class RewindManager;
class BatteryManager;
class CheatManager;
class MovieManager;
class HistoryViewer;
class FrameLimiter;
class DebugStats;
class BaseControlManager;
class VirtualFile;
class BaseVideoFilter;
class ShortcutKeyHandler;
class SystemActionManager;
class AudioPlayerHud;
class GameServer;
class GameClient;

class IInputRecorder;
class IInputProvider;

struct RomInfo;
struct TimingInfo;

enum class MemoryOperationType;
enum class MemoryType;
enum class EventType;
enum class ConsoleRegion;
enum class ConsoleType;
enum class HashType;
enum class TapeRecorderAction;

/// <summary>Memory region information for debugger access</summary>
struct ConsoleMemoryInfo {
	void* Memory;  ///< Pointer to memory region
	uint32_t Size; ///< Size in bytes
};

/// <summary>
/// Central emulator coordinator - manages all emulation subsystems.
/// Owns console, debugger, audio/video, save states, cheats, and networking.
/// </summary>
/// <remarks>
/// Architecture:
/// - **IConsole**: Platform-specific emulation (NES, SNES, GB, etc.)
/// - **SoundMixer/VideoRenderer**: A/V output
/// - **Debugger**: Full-featured debugger (breakpoints, watches, etc.)
/// - **SaveStateManager/RewindManager**: Save states and rewind
/// - **CheatManager**: Game Genie, Action Replay, etc.
/// - **MovieManager**: TAS recording/playback
/// - **BatteryManager**: Save RAM persistence
/// - **GameServer/Client**: Netplay
///
/// Threading model:
/// - Emulation thread (_emuThread): Runs console->RunFrame() in loop
/// - UI thread: Calls methods via EmulatorLock (thread-safe)
/// - Debugger: Can pause emulation thread for inspection
///
/// Lifecycle:
/// 1. Initialize() - Create subsystems
/// 2. LoadRom() - Load game, create console
/// 3. Run() - Start emulation thread
/// 4. Stop() - Stop emulation, destroy console
/// 5. Release() - Cleanup subsystems
///
/// Key methods:
/// - LoadRom: Detect console type, create IConsole, init subsystems
/// - Run: Emulation loop (RunFrame, process audio/video, handle debugger)
/// - Pause/Resume: Thread synchronization
/// - AcquireLock: RAII lock for UI thread safety
///
/// Performance:
/// - ProcessMemoryRead/Write templates: Inline debugger hooks (zero cost when debugging disabled)
/// - Run-ahead support: Speculative execution for input lag reduction
/// - Frame limiting: Precise timing via FrameLimiter
/// </remarks>
class Emulator {
private:
	friend class DebuggerRequest;
	friend class EmulatorLock;

	// Subsystems (unique_ptr = owned, shared_ptr = shared, safe_ptr = thread-safe)
	unique_ptr<thread> _emuThread;              ///< Emulation worker thread
	unique_ptr<AudioPlayerHud> _audioPlayerHud; ///< NSF/SPC player HUD
	safe_ptr<IConsole> _console;                ///< Active console (NES/SNES/GB/etc.)

	shared_ptr<ShortcutKeyHandler> _shortcutKeyHandler;   ///< Keyboard shortcuts
	safe_ptr<Debugger> _debugger;                         ///< Debugger (optional, created on demand)
	unique_ptr<LightweightCdlRecorder> _cdlRecorder;      ///< Lightweight CDL recorder (no debugger overhead)
	shared_ptr<SystemActionManager> _systemActionManager; ///< System action queue

	const unique_ptr<EmuSettings> _settings;                    ///< Global settings
	const unique_ptr<DebugHud> _debugHud;                       ///< Debug overlay (FPS, lag, etc.)
	const unique_ptr<DebugHud> _scriptHud;                      ///< Lua script overlay
	const unique_ptr<NotificationManager> _notificationManager; ///< Event broadcast
	const unique_ptr<BatteryManager> _batteryManager;           ///< Save RAM persistence
	const unique_ptr<SoundMixer> _soundMixer;                   ///< Audio mixing/output
	const unique_ptr<VideoRenderer> _videoRenderer;             ///< Video output/filters
	const unique_ptr<VideoDecoder> _videoDecoder;               ///< Frame decoding
	const unique_ptr<SaveStateManager> _saveStateManager;       ///< Save state management
	const unique_ptr<CheatManager> _cheatManager;               ///< Cheat code support
	const unique_ptr<MovieManager> _movieManager;               ///< TAS recording/playback
	const unique_ptr<HistoryViewer> _historyViewer;

	const shared_ptr<GameServer> _gameServer;
	const shared_ptr<GameClient> _gameClient;
	const shared_ptr<RewindManager> _rewindManager;

	thread_local static thread::id _currentThreadId;
	thread::id _emulationThreadId;

	atomic<uint32_t> _lockCounter;
	SimpleLock _runLock;
	SimpleLock _loadLock;

	SimpleLock _debuggerLock;
	atomic<bool> _stopFlag;
	atomic<bool> _paused;
	atomic<bool> _pauseOnNextFrame;
	atomic<bool> _threadPaused;

	atomic<int> _debugRequestCount;
	atomic<int> _blockDebuggerRequestCount;

	atomic<bool> _isRunAheadFrame;
	bool _frameRunning = false;

	RomInfo _rom;
	ConsoleType _consoleType = {};

	ConsoleMemoryInfo _consoleMemory[DebugUtilities::GetMemoryTypeCount()] = {};

	unique_ptr<DebugStats> _stats;
	unique_ptr<FrameLimiter> _frameLimiter;
	Timer _lastFrameTimer;
	double _frameDelay = 0;

	uint32_t _autoSaveStateFrameCounter = 0;
	int32_t _stopCode = 0;
	bool _stopRequested = false;

	void WaitForLock();
	void WaitForPauseEnd();

	void ProcessAutoSaveState();
	bool ProcessSystemActions();
	void RunFrameWithRunAhead();

	void BlockDebuggerRequests();
	void ResetDebugger(bool startDebugger = false);

	double GetFrameDelay();

	void TryLoadRom(VirtualFile& romFile, LoadRomResult& result, unique_ptr<IConsole>& console, bool useFileSignature);
	template <typename T>
	void TryLoadRom(VirtualFile& romFile, LoadRomResult& result, unique_ptr<IConsole>& console, bool useFileSignature);

	void InitConsole(unique_ptr<IConsole>& newConsole, ConsoleMemoryInfo originalConsoleMemory[], bool preserveRom);

	bool InternalLoadRom(VirtualFile romFile, VirtualFile patchFile, bool stopRom = true, bool forPowerCycle = false);

public:
	Emulator();
	~Emulator();

	void Initialize(bool enableShortcuts = true);
	void Release();

	void Run();
	void Stop(bool sendNotification, bool preventRecentGameSave = false, bool saveBattery = true);

	/// <summary>Called at end of each emulated frame</summary>
	void ProcessEndOfFrame();

	/// <summary>Reset console (soft reset)</summary>
	void Reset();

	/// <summary>Reload current ROM (for settings changes)</summary>
	void ReloadRom(bool forPowerCycle);

	/// <summary>Power cycle console (hard reset)</summary>
	void PowerCycle();

	/// <summary>Request pause after current frame completes</summary>
	void PauseOnNextFrame();

	/// <summary>Pause emulation</summary>
	void Pause();

	/// <summary>Resume emulation</summary>
	void Resume();

	/// <summary>Check if emulation paused</summary>
	bool IsPaused();

	/// <summary>
	/// Prepare for pause (clear audio buffer, etc.).
	/// </summary>
	/// <param name="clearAudioBuffer">Clear audio buffer if true</param>
	void OnBeforePause(bool clearAudioBuffer);

	/// <summary>
	/// Called before sending a frame to the video renderer.
	/// Handles frame-related events and notifications.
	/// </summary>
	void OnBeforeSendFrame();

	/// <summary>
	/// Load ROM file.
	/// </summary>
	/// <param name="romFile">ROM file to load</param>
	/// <param name="patchFile">Optional patch file (IPS/BPS/UPS)</param>
	/// <param name="stopRom">Stop current ROM if true</param>
	/// <param name="forPowerCycle">Loading for power cycle if true</param>
	/// <returns>True if ROM loaded successfully</returns>
	bool LoadRom(VirtualFile romFile, VirtualFile patchFile, bool stopRom = true, bool forPowerCycle = false);

	/// <summary>Get loaded ROM information</summary>
	RomInfo& GetRomInfo() { return _rom; }

	/// <summary>Get ROM hash (SHA1 or SHA1 for cheat code lookup)</summary>
	string GetHash(HashType type);

	/// <summary>Get ROM CRC32 checksum</summary>
	uint32_t GetCrc32();

	/// <summary>Get current PPU frame info</summary>
	PpuFrameInfo GetPpuFrame();

	/// <summary>Get console region (NTSC/PAL/Dendy)</summary>
	ConsoleRegion GetRegion();

	/// <summary>Get active console (thread-safe shared_ptr)</summary>
	shared_ptr<IConsole> GetConsole();

	/// <summary>Get active console (unsafe raw pointer - faster, use with EmulatorLock)</summary>
	IConsole* GetConsoleUnsafe();

	/// <summary>Get active console type</summary>
	ConsoleType GetConsoleType();

	/// <summary>Get list of CPU types for active console</summary>
	vector<CpuType> GetCpuTypes();

	/// <summary>Get master clock cycle count</summary>
	uint64_t GetMasterClock();

	/// <summary>Get master clock rate in Hz</summary>
	uint32_t GetMasterClockRate();

	/// <summary>
	/// Acquire RAII emulator lock for safe state access.
	/// </summary>
	/// <param name="allowDebuggerLock">Allow debugger to break during lock if true</param>
	/// <returns>EmulatorLock RAII object</returns>
	EmulatorLock AcquireLock(bool allowDebuggerLock = true);

	/// <summary>Acquire emulator lock (use AcquireLock() for RAII instead)</summary>
	void Lock();

	/// <summary>Release emulator lock (use AcquireLock() for RAII instead)</summary>
	void Unlock();

	/// <summary>Check if emulation thread currently paused</summary>
	bool IsThreadPaused();

	/// <summary>Check if debugger requests blocked</summary>
	[[nodiscard]] bool IsDebuggerBlocked() { return _blockDebuggerRequestCount > 0; }

	/// <summary>
	/// Suspend debugger temporarily.
	/// </summary>
	/// <param name="release">Release suspension if true, suspend if false</param>
	void SuspendDebugger(bool release);

	/// <summary>
	/// Serialize emulator state to stream (save state).
	/// </summary>
	/// <param name="out">Output stream</param>
	/// <param name="includeSettings">Include settings in save state if true</param>
	/// <param name="compressionLevel">zlib compression level (0-9, 1=default)</param>
	void Serialize(ostream& out, bool includeSettings, int compressionLevel = 1);

	/// <summary>
	/// Deserialize emulator state from stream (load state).
	/// </summary>
	/// <param name="in">Input stream</param>
	/// <param name="fileFormatVersion">Save state file format version</param>
	/// <param name="includeSettings">Load settings from save state if true</param>
	/// <param name="consoleType">Expected console type (for validation)</param>
	/// <param name="sendNotification">Send StateLoaded notification if true</param>
	/// <returns>Deserialization result</returns>
	DeserializeResult Deserialize(istream& in, uint32_t fileFormatVersion, bool includeSettings, optional<ConsoleType> consoleType = std::nullopt, bool sendNotification = true);

	// Subsystem accessors (getters return raw pointers for performance)

	/// <summary>Get sound mixer</summary>
	SoundMixer* GetSoundMixer() { return _soundMixer.get(); }

	/// <summary>Get video renderer</summary>
	VideoRenderer* GetVideoRenderer() { return _videoRenderer.get(); }

	/// <summary>Get video decoder</summary>
	VideoDecoder* GetVideoDecoder() { return _videoDecoder.get(); }

	/// <summary>Get shortcut key handler</summary>
	ShortcutKeyHandler* GetShortcutKeyHandler() { return _shortcutKeyHandler.get(); }

	/// <summary>Get notification manager</summary>
	NotificationManager* GetNotificationManager() { return _notificationManager.get(); }

	/// <summary>Get settings</summary>
	EmuSettings* GetSettings() { return _settings.get(); }

	/// <summary>Get save state manager</summary>
	SaveStateManager* GetSaveStateManager() { return _saveStateManager.get(); }

	/// <summary>Get rewind manager</summary>
	RewindManager* GetRewindManager() { return _rewindManager.get(); }

	/// <summary>Get debug HUD overlay</summary>
	DebugHud* GetDebugHud() { return _debugHud.get(); }

	/// <summary>Get Lua script HUD overlay</summary>
	DebugHud* GetScriptHud() { return _scriptHud.get(); }

	/// <summary>Get battery manager</summary>
	BatteryManager* GetBatteryManager() { return _batteryManager.get(); }

	/// <summary>Get cheat manager</summary>
	/// <summary>Get cheat manager</summary>
	CheatManager* GetCheatManager() { return _cheatManager.get(); }

	/// <summary>Get movie manager (TAS recording/playback)</summary>
	MovieManager* GetMovieManager() { return _movieManager.get(); }

	/// <summary>Get history viewer</summary>
	HistoryViewer* GetHistoryViewer() { return _historyViewer.get(); }

	/// <summary>Get netplay server</summary>
	GameServer* GetGameServer() { return _gameServer.get(); }

	/// <summary>Get netplay client</summary>
	GameClient* GetGameClient() { return _gameClient.get(); }

	/// <summary>Get system action manager</summary>
	shared_ptr<SystemActionManager> GetSystemActionManager() { return _systemActionManager; }

	/// <summary>
	/// Get active video filter.
	/// </summary>
	/// <param name="getDefaultFilter">Get default filter if true, active filter if false</param>
	BaseVideoFilter* GetVideoFilter(bool getDefaultFilter = false);

	/// <summary>Get screen rotation override (for Game Boy orientation)</summary>
	void GetScreenRotationOverride(uint32_t& rotation);

	/// <summary>Input barcode data (for Famicom Barcode Battler)</summary>
	void InputBarcode(uint64_t barcode, uint32_t digitCount);

	/// <summary>Process tape recorder action (for 8-bit computers)</summary>
	void ProcessTapeRecorderAction(TapeRecorderAction action, string filename);

	/// <summary>
	/// Check if keyboard shortcut allowed.
	/// </summary>
	/// <returns>Shortcut state (Enabled/Disabled/Hidden)</returns>
	ShortcutState IsShortcutAllowed(EmulatorShortcut shortcut, uint32_t shortcutParam);

	/// <summary>Check if keyboard connected to console (for on-screen keyboard)</summary>
	bool IsKeyboardConnected();

	/// <summary>Initialize debugger subsystem</summary>
	void InitDebugger();

	/// <summary>Stop and destroy debugger</summary>
	void StopDebugger();

	/// <summary>
	/// Get debugger request (RAII lock for debugger access).
	/// </summary>
	/// <param name="autoInit">Auto-initialize debugger if not running</param>
	DebuggerRequest GetDebugger(bool autoInit = false);

	/// <summary>Check if debugger active</summary>
	[[nodiscard]] bool IsDebugging() { return !!_debugger; }

	/// <summary>Get debugger instance (unsafe - use GetDebugger() for RAII instead)</summary>
	Debugger* InternalGetDebugger() { return _debugger.get(); }

	// Lightweight CDL recording (zero-overhead alternative to full debugger for CDL)

	/// <summary>Start lightweight CDL recording without creating the full debugger.</summary>
	void StartLightweightCdl();

	/// <summary>Stop lightweight CDL recording.</summary>
	void StopLightweightCdl();

	/// <summary>Check if lightweight CDL recording is active.</summary>
	[[nodiscard]] bool IsLightweightCdlActive() { return !!_cdlRecorder; }

	/// <summary>Get the lightweight CDL recorder (may be null).</summary>
	LightweightCdlRecorder* GetCdlRecorder() { return _cdlRecorder.get(); }

	/// <summary>Get emulation thread ID</summary>
	thread::id GetEmulationThreadId() { return _emulationThreadId; }

	/// <summary>Check if current thread is emulation thread</summary>
	bool IsEmulationThread();

	/// <summary>Get emulator stop code (reason for stopping)</summary>
	[[nodiscard]] int32_t GetStopCode() { return _stopCode; }

	/// <summary>Set emulator stop code</summary>
	void SetStopCode(int32_t stopCode);

	/// <summary>
	/// Register memory region for debugger access.
	/// </summary>
	/// <param name="type">Memory type (RAM, PRG ROM, CHR ROM, etc.)</param>
	/// <param name="memory">Pointer to memory region</param>
	/// <param name="size">Size in bytes</param>
	void RegisterMemory(MemoryType type, void* memory, uint32_t size);

	/// <summary>Get registered memory region</summary>
	ConsoleMemoryInfo GetMemory(MemoryType type);

	/// <summary>Get audio player track info (for NSF/SPC/etc.)</summary>
	AudioTrackInfo GetAudioTrackInfo();

	/// <summary>Process audio player action (play/pause/next/prev track)</summary>
	void ProcessAudioPlayerAction(AudioPlayerActionParams p);

	/// <summary>Get audio player HUD</summary>
	AudioPlayerHud* GetAudioPlayerHud() { return _audioPlayerHud.get(); }

	/// <summary>Check if emulator running (ROM loaded)</summary>
	[[nodiscard]] bool IsRunning() { return _console != nullptr; }

	/// <summary>Check if currently executing run-ahead frame</summary>
	[[nodiscard]] bool IsRunAheadFrame() { return _isRunAheadFrame; }

	/// <summary>Get timing info for CPU type</summary>
	TimingInfo GetTimingInfo(CpuType cpuType);

	/// <summary>Get current frame number</summary>
	uint32_t GetFrameCount();

	/// <summary>Get lag frame counter</summary>
	uint32_t GetLagCounter();

	/// <summary>Reset lag frame counter</summary>
	void ResetLagCounter();

	/// <summary>Check if controller type connected</summary>
	bool HasControlDevice(ControllerType type);

	/// <summary>Register input recorder (for movies, netplay)</summary>
	void RegisterInputRecorder(IInputRecorder* recorder);

	/// <summary>Unregister input recorder</summary>
	void UnregisterInputRecorder(IInputRecorder* recorder);

	/// <summary>Register input provider (for movies, rewind)</summary>
	void RegisterInputProvider(IInputProvider* provider);

	/// <summary>Unregister input provider</summary>
	void UnregisterInputProvider(IInputProvider* provider);

	/// <summary>Get current FPS (frames per second)</summary>
	/// <summary>Get current FPS (frames per second)</summary>
	double GetFps();

	// Debugger hooks - templated for zero-cost abstraction when debugger disabled
	// These are __forceinline and check if(_debugger) before calling, so they compile to nothing when not debugging

	/// <summary>
	/// Process CPU instruction for debugger (breakpoints, step, etc.).
	/// </summary>
	/// <remarks>
	/// Zero-cost when debugger not active - inlined and optimized away.
	/// </remarks>
	template <CpuType type>
	__forceinline void ProcessInstruction() {
		if (_debugger) {
			_debugger->ProcessInstruction<type>();
		} else if (_cdlRecorder) {
			_cdlRecorder->RecordInstruction();
		}
	}

	/// <summary>
	/// Process memory read for debugger (watchpoints, read breakpoints).
	/// When only lightweight CDL is active, records code/data marking without debugger overhead.
	/// </summary>
	/// <typeparam name="type">CPU type</typeparam>
	/// <typeparam name="accessWidth">Access width in bytes (1/2/4)</typeparam>
	/// <typeparam name="flags">Memory access flags</typeparam>
	template <CpuType type, uint8_t accessWidth = 1, MemoryAccessFlags flags = MemoryAccessFlags::None, typename T>
	__forceinline void ProcessMemoryRead(uint32_t addr, T& value, MemoryOperationType opType) {
		if (_debugger) {
			_debugger->ProcessMemoryRead<type, accessWidth, flags>(addr, value, opType);
		} else if (_cdlRecorder) {
			_cdlRecorder->RecordRead(addr, DebugUtilities::GetCpuMemoryType(type), opType);
		}
	}

	/// <summary>
	/// Process memory write for debugger (watchpoints, write breakpoints, freeze).
	/// </summary>
	/// <returns>True if write allowed, false if frozen by debugger</returns>
	template <CpuType type, uint8_t accessWidth = 1, MemoryAccessFlags flags = MemoryAccessFlags::None, typename T>
	__forceinline bool ProcessMemoryWrite(uint32_t addr, T& value, MemoryOperationType opType) {
		if (_debugger) {
			return _debugger->ProcessMemoryWrite<type, accessWidth, flags>(addr, value, opType);
		}
		return true;
	}

	/// <summary>
	/// Process generic memory access for event tracking.
	/// </summary>
	template <CpuType cpuType, MemoryType memType, MemoryOperationType opType, typename T>
	__forceinline void ProcessMemoryAccess(uint32_t addr, T value) {
		if (_debugger) {
			_debugger->ProcessMemoryAccess<cpuType, memType, opType, T>(addr, value);
		}
	}

	/// <summary>
	/// Process idle CPU cycle for debugger.
	/// </summary>
	template <CpuType type>
	__forceinline void ProcessIdleCycle() {
		if (_debugger) {
			_debugger->ProcessIdleCycle<type>();
		}
	}

	/// <summary>
	/// Process halted CPU state for debugger.
	/// </summary>
	template <CpuType type>
	__forceinline void ProcessHaltedCpu() {
		if (_debugger) {
			_debugger->ProcessHaltedCpu<type>();
		}
	}

	/// <summary>
	/// Process PPU memory read for debugger.
	/// </summary>
	template <CpuType type, typename T>
	__forceinline void ProcessPpuRead(uint32_t addr, T& value, MemoryType memoryType, MemoryOperationType opType = MemoryOperationType::Read) {
		if (_debugger) {
			_debugger->ProcessPpuRead<type>(addr, value, memoryType, opType);
		}
	}

	/// <summary>
	/// Process PPU memory write for debugger.
	/// </summary>
	template <CpuType type, typename T>
	__forceinline void ProcessPpuWrite(uint32_t addr, T& value, MemoryType memoryType) {
		if (_debugger) {
			_debugger->ProcessPpuWrite<type>(addr, value, memoryType);
		}
	}

	/// <summary>
	/// Process PPU cycle for debugger (scanline tracking).
	/// </summary>
	template <CpuType type>
	__forceinline void ProcessPpuCycle() {
		if (_debugger) {
			_debugger->ProcessPpuCycle<type>();
		}
	}

	/// <summary>
	/// Process CPU interrupt for debugger (NMI/IRQ tracking).
	/// </summary>
	template <CpuType type>
	void ProcessInterrupt(uint32_t originalPc, uint32_t currentPc, bool forNmi) {
		if (_debugger) {
			_debugger->ProcessInterrupt<type>(originalPc, currentPc, forNmi);
		}
	}

	/// <summary>
	/// Log debug message to debugger.
	/// </summary>
	__forceinline void DebugLog(string log) {
		if (_debugger) {
			_debugger->Log(log);
		}
	}

	/// <summary>
	/// Process emulator event for debugger (frame end, reset, etc.).
	/// </summary>
	void ProcessEvent(EventType type, std::optional<CpuType> cpuType = std::nullopt);

	/// <summary>
	/// Add debug event to event viewer.
	/// </summary>
	template <CpuType cpuType>
	void AddDebugEvent(DebugEventType evtType);

	/// <summary>
	/// Break into debugger if debugging active.
	/// </summary>
	void BreakIfDebugging(CpuType sourceCpu, BreakSource source);
};

enum class HashType {
	Sha1,
	Sha1Cheat
};
