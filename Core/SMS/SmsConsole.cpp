#include "pch.h"
#include <ranges>
#include "SMS/SmsConsole.h"
#include "SMS/SmsCpu.h"
#include "SMS/SmsControlManager.h"
#include "SMS/SmsVdp.h"
#include "SMS/SmsPsg.h"
#include "SMS/SmsFmAudio.h"
#include "SMS/SmsMemoryManager.h"
#include "SMS/SmsDefaultVideoFilter.h"
#include "SMS/SmsNtscFilter.h"
#include "SMS/SmsTypes.h"
#include "SMS/Carts/SmsSegaCart.h"
#include "SMS/Carts/SmsCodemasterCart.h"
#include "SMS/Carts/SmsMsxCart.h"
#include "SMS/Carts/SmsKoreanCart.h"
#include "SMS/Carts/SmsNemesisCart.h"
#include "Shared/Emulator.h"
#include "Shared/CheatManager.h"
#include "Shared/FirmwareHelper.h"
#include "Utilities/Serializer.h"
#include "Utilities/StringUtilities.h"
#include "Utilities/CRC32.h"

// Initialize Sega Master System / Game Gear console emulator
SmsConsole::SmsConsole(Emulator* emu) {
	_emu = emu;
}

SmsConsole::~SmsConsole() {
}

// Load ROM file and initialize all SMS/GG/SG/CV hardware components
LoadRomResult SmsConsole::LoadRom(VirtualFile& romFile) {
	vector<uint8_t> romData;
	(void)romFile.ReadFile(romData);

	if (romData.size() >= 0x100) {
		if ((romData.size() % 0x400) == 0x200) {
			// ROM has 512-byte copier header, discard it
			romData.erase(romData.begin(), romData.begin() + 0x200);
		}

		_filename = romFile.GetFileName();

		// Determine console model based on file extension
		string ext = romFile.GetFileExtension();
		bool isGameGear = ext == ".gg";
		if (isGameGear) {
			_romFormat = RomFormat::GameGear;
			_model = SmsModel::GameGear;
		} else if (ext == ".col") {
			_romFormat = RomFormat::ColecoVision;
			_model = SmsModel::ColecoVision;
		} else if (ext == ".sg") {
			_romFormat = RomFormat::Sg;
			_model = SmsModel::Sg;
		} else {
			_romFormat = RomFormat::Sms;
			_model = SmsModel::Sms;
		}

		// Create all hardware components
		_vdp = std::make_unique<SmsVdp>();                             // Video Display Processor (TMS9918 derivative)
		_memoryManager = std::make_unique<SmsMemoryManager>();         // Memory mapper
		_cpu = std::make_unique<SmsCpu>();                             // Zilog Z80 CPU
		_psg = std::make_unique<SmsPsg>(_emu, this);                   // Programmable Sound Generator (SN76489)
		_fmAudio = std::make_unique<SmsFmAudio>(_emu, this);           // YM2413 FM sound (SMS only)
		_controlManager = std::make_unique<SmsControlManager>(_emu, this, _vdp.get());  // Input controller

		InitCart(romData);

		vector<uint8_t> biosRom;
		if (_model == SmsModel::ColecoVision) {
			FirmwareHelper::LoadColecoVisionBios(_emu, biosRom);
		} else {
			FirmwareHelper::LoadSmsBios(_emu, biosRom, isGameGear);
		}

		_memoryManager->Init(_emu, this, romData, biosRom, _vdp.get(), _controlManager.get(), _cart.get(), _psg.get(), _fmAudio.get());
		_vdp->Init(_emu, this, _cpu.get(), _controlManager.get(), _memoryManager.get());
		_cpu->Init(_emu, this, _memoryManager.get());

		UpdateRegion(true);

		return LoadRomResult::Success;
	}

	return LoadRomResult::Failure;
}

void SmsConsole::InitCart(vector<uint8_t>& romData) {
	bool isCodemasterMapper = false;
	if (romData.size() > 0x8000) {
		// Codemaster games have a checksum+complement at 7FE6-7FE9
		uint32_t checksum = romData[0x7FE6] | (romData[0x7FE7] << 8);
		uint32_t complement = romData[0x7FE8] | (romData[0x7FE9] << 8);
		isCodemasterMapper = (complement + checksum) == 0x10000;
	}

	if (isCodemasterMapper) {
		_cart = std::make_unique<SmsCodemasterCart>(_memoryManager.get());
	} else {
		uint32_t crc = CRC32::GetCRC(romData);
		switch (crc) {
			case 0x77EFE84A: // Cyborg Z
			case 0x06965ED9: // F-1 Spirit - The way to Formula-1
			case 0xF89AF3CC: // Knightmare II: The Maze of Galious
			case 0x0A77FA5E: // Nemesis 2
			case 0x445525E2: // Penguin Adventure
			case 0x83F0EEDE: // Street Master
			case 0x9195C34C: // Super Boy 3
			case 0xA05258F5: // Wonsiin
				_cart = std::make_unique<SmsMsxCart>(_memoryManager.get());
				break;

			case 0xE316C06D: // Nemesis
				_cart = std::make_unique<SmsNemesisCart>(_memoryManager.get(), (uint32_t)romData.size());
				break;

			case 0x89B79E77: // Dallyeora Pigu-Wang
			case 0x97D03541: // Samgukji  3
			case 0x929222C4: // Jang Pung II
			case 0x18FB98A3: // Jang Pung 3
				_cart = std::make_unique<SmsKoreanCart>(_memoryManager.get());
				break;

				// TODOSMS
				// case 0x192949D5: //Janggun-ui Adeul

			default:
				if (romData.size() <= 0xC000) {
					// No mapper (up to 48kb of prg rom)
					_cart = std::make_unique<SmsCart>(_memoryManager.get());
				} else {
					_cart = std::make_unique<SmsSegaCart>(_memoryManager.get());
				}
				break;
		}
	}

	uint32_t power = (uint32_t)std::log2(romData.size());
	if (romData.size() > ((uint64_t)1 << power)) {
		// If size isn't a power of 2, pad the end of the ROM to the next power of 2
		uint32_t newSize = 1 << (power + 1);
		romData.insert(romData.end(), newSize - romData.size(), 0);
	}
}

void SmsConsole::Reset() {
	// SMS/GG has no physical reset button, behave like power cycle
	_emu->ReloadRom(true);
}

void SmsConsole::RunFrame() {
	UpdateRegion(false);

	uint32_t frame = _vdp->GetFrameCount();
	while (frame == _vdp->GetFrameCount()) {
		_cpu->Exec();
	}

	_psg->Run();
	_psg->PlayQueuedAudio();
}

void SmsConsole::ProcessEndOfFrame() {
	_controlManager->UpdateControlDevices();
	_controlManager->UpdateInputState();
}

void SmsConsole::UpdateRegion(bool forceUpdate) {
	ConsoleRegion region;
	switch (_model) {
		default:
		case SmsModel::Sms:
			region = _emu->GetSettings()->GetSmsConfig().Region;
			break;
		case SmsModel::GameGear:
			region = _emu->GetSettings()->GetSmsConfig().GameGearRegion;
			break;
		case SmsModel::ColecoVision:
			region = _emu->GetSettings()->GetCvConfig().Region;
			break;
	}

	if (region == ConsoleRegion::Auto) {
		string filename = StringUtilities::ToLower(_filename);
		if (filename.contains("(europe)") || filename.contains("(e)")) {
			region = ConsoleRegion::Pal;
		} else {
			if (_model == SmsModel::GameGear && (filename.contains("(japan)") || filename.contains("(j)"))) {
				region = ConsoleRegion::NtscJapan;
			} else {
				region = ConsoleRegion::Ntsc;
			}
		}
	}

	if (_region != region || forceUpdate) {
		_region = region;
		_vdp->SetRegion(_model == SmsModel::GameGear ? ConsoleRegion::Ntsc : _region);
		_psg->SetRegion(_model == SmsModel::GameGear ? ConsoleRegion::Ntsc : _region);
	}
}

void SmsConsole::SaveBattery() {
	_memoryManager->SaveBattery();
}

bool SmsConsole::IsPsgAudioMuted() {
	return _fmAudio->IsPsgAudioMuted();
}

bool SmsConsole::HasBios() {
	return _memoryManager->HasBios();
}

BaseControlManager* SmsConsole::GetControlManager() {
	return _controlManager.get();
}

SmsRevision SmsConsole::GetRevision() {
	if (_model == SmsModel::Sg || _model == SmsModel::ColecoVision) {
		return SmsRevision::Sms1;
	} else if (_model == SmsModel::GameGear) {
		return SmsRevision::Sms2;
	}
	return _emu->GetSettings()->GetSmsConfig().Revision;
}

ConsoleRegion SmsConsole::GetRegion() {
	return _region;
}

ConsoleType SmsConsole::GetConsoleType() {
	return ConsoleType::Sms;
}

vector<CpuType> SmsConsole::GetCpuTypes() {
	return {CpuType::Sms};
}

uint64_t SmsConsole::GetMasterClock() {
	return _cpu->GetState().CycleCount;
}

uint32_t SmsConsole::GetMasterClockRate() {
	return (_model != SmsModel::GameGear && _region == ConsoleRegion::Pal) ? 3546895 : 3579545;
}

double SmsConsole::GetFps() {
	return (_model != SmsModel::GameGear && _region == ConsoleRegion::Pal) ? 49.701460 : 59.9227434;
}

BaseVideoFilter* SmsConsole::GetVideoFilter(bool getDefaultFilter) {
	if (getDefaultFilter) {
		return new SmsDefaultVideoFilter(_emu, this);
	}

	VideoFilterType filterType = _emu->GetSettings()->GetVideoConfig().VideoFilter;

	switch (filterType) {
		case VideoFilterType::NtscBisqwit:
		case VideoFilterType::NtscBlargg:
			return new SmsNtscFilter(_emu, this);

		default:
			return new SmsDefaultVideoFilter(_emu, this);
	}
}

PpuFrameInfo SmsConsole::GetPpuFrame() {
	PpuFrameInfo frame = {};
	frame.FirstScanline = 0;
	frame.FrameCount = _vdp->GetFrameCount();
	frame.Width = 256;
	frame.Height = 240;
	frame.ScanlineCount = _region == ConsoleRegion::Ntsc ? 262 : 313;
	frame.CycleCount = 342;
	frame.FrameBufferSize = 256 * 240 * sizeof(uint16_t);
	frame.FrameBuffer = (uint8_t*)_vdp->GetScreenBuffer(false);
	return frame;
}

RomFormat SmsConsole::GetRomFormat() {
	return _romFormat;
}

AudioTrackInfo SmsConsole::GetAudioTrackInfo() {
	SmsPsgState& psg = _psg->GetState();

	int activeChannels = 0;
	string channelInfo;

	for (int i = 0; i < 3; i++) {
		if (psg.Tone[i].Volume < 15) {
			activeChannels++;
			channelInfo += "T" + std::to_string(i + 1) + "(f=" + std::to_string(psg.Tone[i].ReloadValue) + ",v=" + std::to_string(15 - psg.Tone[i].Volume) + ") ";
		}
	}

	if (psg.Noise.Volume < 15) {
		activeChannels++;
		uint8_t mode = (psg.Noise.Control >> 2) & 1;
		uint8_t rate = psg.Noise.Control & 3;
		channelInfo += string("N(") + (mode ? "white" : "periodic") + ",r=" + std::to_string(rate) + ",v=" + std::to_string(15 - psg.Noise.Volume) + ") ";
	}

	if (_fmAudio && _fmAudio->IsPsgAudioMuted()) {
		channelInfo += "[FM active] ";
	}

	string modelName;
	switch (_model) {
		case SmsModel::GameGear: modelName = "Game Gear"; break;
		case SmsModel::Sg: modelName = "SG-1000"; break;
		case SmsModel::ColecoVision: modelName = "ColecoVision"; break;
		default: modelName = "Master System"; break;
	}

	AudioTrackInfo info = {};
	info.GameTitle = modelName;
	info.SongTitle = std::to_string(activeChannels) + " active channel" + (activeChannels != 1 ? "s" : "");
	info.Comment = channelInfo.empty() ? "silent" : channelInfo;
	info.Position = _vdp->GetFrameCount() / GetFps();
	info.Length = 0;
	info.FadeLength = 0;
	info.TrackNumber = 1;
	info.TrackCount = 1;
	return info;
}

void SmsConsole::ProcessAudioPlayerAction(AudioPlayerActionParams p) {
	// SMS has no multi-track audio format; no-op
}

void SmsConsole::RefreshRamCheats() {
	// Used to refresh pro action replay codes when vertical blank IRQ is triggered
	for (InternalCheatCode& code : _emu->GetCheatManager()->GetRamRefreshCheats(CpuType::Sms)) {
		_memoryManager->DebugWrite(code.Address, code.Value);
	}
}

AddressInfo SmsConsole::GetAbsoluteAddress(uint32_t relAddress) {
	return _memoryManager->GetAbsoluteAddress(relAddress);
}

AddressInfo SmsConsole::GetAbsoluteAddress(AddressInfo& relAddress) {
	return GetAbsoluteAddress(relAddress.Address);
}

AddressInfo SmsConsole::GetPcAbsoluteAddress() {
	return _memoryManager->GetAbsoluteAddress(_cpu->GetState().PC);
}

AddressInfo SmsConsole::GetRelativeAddress(AddressInfo& absAddress, CpuType cpuType) {
	return {_memoryManager->GetRelativeAddress(absAddress), MemoryType::SmsMemory};
}

SmsState SmsConsole::GetState() {
	SmsState state;
	state.Cpu = _cpu->GetState();
	state.Vdp = _vdp->GetState();
	state.Psg = _psg->GetState();
	state.MemoryManager = _memoryManager->GetState();
	state.ControlManager = _controlManager->GetState();
	return state;
}

void SmsConsole::GetConsoleState(BaseState& state, ConsoleType consoleType) {
	(SmsState&)state = GetState();
}

void SmsConsole::InitializeRam(void* data, uint32_t length) {
	EmuSettings* settings = _emu->GetSettings();
	RamState ramState = _model == SmsModel::ColecoVision ? settings->GetCvConfig().RamPowerOnState : settings->GetSmsConfig().RamPowerOnState;
	settings->InitializeRam(ramState, data, length);
}

void SmsConsole::Serialize(Serializer& s) {
	SV(_cpu);
	SV(_vdp);
	SV(_controlManager);
	SV(_cart); // Process cart before memory manager to ensure mappings are updated properly
	SV(_memoryManager);
	SV(_psg);
	SV(_fmAudio);
}
