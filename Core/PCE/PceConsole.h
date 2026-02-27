#pragma once
#include "pch.h"
#include "Shared/Interfaces/IConsole.h"

class PceCpu;
class PceVdc;
class PceVpc;
class PceVce;
class PcePsg;
class PceTimer;
class PceCdRom;
class PceMemoryManager;
class PceControlManager;
class IPceMapper;
class Emulator;
struct PceVideoState;
struct HesFileData;
struct DiscInfo;
enum class PceConsoleType;

/// <summary>
/// PC Engine / TurboGrafx-16 console emulator.
/// Implements the complete PCE hardware including SuperGrafx and CD-ROM².
/// </summary>
/// <remarks>
/// **System Variants:**
/// - **PC Engine / TurboGrafx-16**: Standard HuCard-based console
/// - **SuperGrafx**: Enhanced version with dual VDC chips
/// - **PC Engine CD-ROM²**: CD-ROM add-on with ADPCM audio
/// - **PC Engine Duo**: Integrated CD-ROM system
///
/// **Hardware Components:**
/// - **CPU**: HuC6280 (65C02 variant) @ 7.16 MHz
///   - Integrated PSG, timer, and memory mapper
///   - 8KB zero-page accessible via MMU
/// - **VDC**: HuC6270 - Video Display Controller
///   - 64KB VRAM, 64 sprites, 2 background layers (via BAT)
/// - **VCE**: HuC6260 - Video Color Encoder
///   - 512-color palette, composite/RGB output
/// - **PSG**: 6-channel wavetable synthesizer (in CPU)
///
/// **SuperGrafx Enhancements:**
/// - Dual VDC chips (VDC1 + VDC2)
/// - VPC (Video Priority Controller) for layer composition
/// - 128KB VRAM total (64KB per VDC)
///
/// **CD-ROM² Features:**
/// - 64KB RAM + 2KB battery-backed RAM
/// - ADPCM audio playback
/// - Red Book CD audio support
/// - System Card/Arcade Card for expanded memory
/// </remarks>
class PceConsole final : public IConsole {
private:
	Emulator* _emu;

	unique_ptr<PceCpu> _cpu;
	unique_ptr<PceVdc> _vdc;
	unique_ptr<PceVdc> _vdc2;
	unique_ptr<PceVpc> _vpc;
	unique_ptr<PceVce> _vce;
	unique_ptr<PcePsg> _psg;
	unique_ptr<PceTimer> _timer;
	unique_ptr<PceMemoryManager> _memoryManager;
	unique_ptr<PceControlManager> _controlManager;
	unique_ptr<PceCdRom> _cdrom;
	unique_ptr<IPceMapper> _mapper;
	unique_ptr<HesFileData> _hesData;
	RomFormat _romFormat = RomFormat::Pce;

	static bool IsPopulousCard(uint32_t crc32);
	static bool IsSuperGrafxCard(uint32_t crc32);

	bool LoadHesFile(VirtualFile& hesFile);
	bool LoadFirmware(DiscInfo& disc, vector<uint8_t>& romData);

public:
	PceConsole(Emulator* emu);
	virtual ~PceConsole();

	static vector<string> GetSupportedExtensions() { return {".pce", ".cue", ".sgx", ".hes"}; }
	static vector<string> GetSupportedSignatures() { return {"HESM"}; }

	void Serialize(Serializer& s) override;

	void InitializeRam(void* data, uint32_t length);

	void Reset() override;

	LoadRomResult LoadRom(VirtualFile& romFile) override;

	void RunFrame() override;

	void ProcessEndOfFrame();

	void SaveBattery() override;

	BaseControlManager* GetControlManager() override;
	ConsoleRegion GetRegion() override;
	ConsoleType GetConsoleType() override;
	vector<CpuType> GetCpuTypes() override;

	PceCpu* GetCpu();
	PceVdc* GetVdc();
	PceVce* GetVce();
	PceVpc* GetVpc();
	PcePsg* GetPsg();
	PceMemoryManager* GetMemoryManager();

	[[nodiscard]] bool IsSuperGrafx() { return _vdc2 != nullptr; }

	uint64_t GetMasterClock() override;
	uint32_t GetMasterClockRate() override;
	double GetFps() override;

	BaseVideoFilter* GetVideoFilter(bool getDefaultFilter) override;

	PpuFrameInfo GetPpuFrame() override;
	RomFormat GetRomFormat() override;

	void InitHesPlayback(uint8_t selectedTrack);
	AudioTrackInfo GetAudioTrackInfo() override;
	void ProcessAudioPlayerAction(AudioPlayerActionParams p) override;

	AddressInfo GetAbsoluteAddress(AddressInfo& relAddress) override;
	AddressInfo GetPcAbsoluteAddress() override;
	AddressInfo GetRelativeAddress(AddressInfo& absAddress, CpuType cpuType) override;

	PceVideoState GetVideoState();
	void SetVideoState(PceVideoState& state);
	void GetConsoleState(BaseState& state, ConsoleType consoleType) override;
};
