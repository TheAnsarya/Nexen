#include "Common.h"
#include "Core/Shared/RecordedRomTest.h"
#include "Core/Shared/Emulator.h"
#include "Core/Shared/EmuSettings.h"

extern unique_ptr<Emulator> _emu;
shared_ptr<RecordedRomTest> _recordedRomTest;

extern "C" {
DllExport RomTestResult __stdcall RunRecordedTest(char* filename, bool inBackground) {
	if (inBackground) {
		unique_ptr<Emulator> emu(new Emulator());
		emu->Initialize(false);
		emu->GetSettings()->SetFlag(EmulationFlags::TestMode);
		shared_ptr<RecordedRomTest> romTest(new RecordedRomTest(emu.get(), true));
		RomTestResult result = romTest->Run(filename);
		emu->Release();
		return result;
	} else {
		shared_ptr<RecordedRomTest> romTest(new RecordedRomTest(_emu.get(), false));
		return romTest->Run(filename);
	}
}

DllExport uint64_t __stdcall RunTest(char* filename, uint32_t address, MemoryType memType, uint32_t frameCount, int32_t earlyExitByte) {
	unique_ptr<Emulator> emu(new Emulator());
	emu->Initialize();
	emu->GetSettings()->SetFlag(EmulationFlags::TestMode);
	emu->GetSettings()->GetGameboyConfig().Model = GameboyModel::Gameboy;
	emu->GetSettings()->GetGameboyConfig().RamPowerOnState = RamState::AllZeros;
	(void)emu->LoadRom((VirtualFile)filename, VirtualFile());
	emu->GetSettings()->SetFlag(EmulationFlags::MaximumSpeed);

	bool checkEarlyExit = (earlyExitByte >= 0);
	uint32_t maxFrames = (frameCount > 0) ? frameCount : 500;

	while(emu->GetFrameCount() < maxFrames) {
		std::this_thread::sleep_for(std::chrono::duration<int, std::milli>(10));

		if(checkEarlyExit && emu->GetFrameCount() > 0) {
			ConsoleMemoryInfo earlyMemInfo = emu->GetMemory(memType);
			uint8_t* earlyBuffer = (uint8_t*)earlyMemInfo.Memory;
			if(address < earlyMemInfo.Size && earlyBuffer[address] != (uint8_t)earlyExitByte) {
				break;
			}
		}
	}

	ConsoleMemoryInfo memInfo = emu->GetMemory(memType);
	uint8_t* memBuffer = (uint8_t*)memInfo.Memory;
	uint64_t result = memBuffer[address];
	for (int i = 1; i < 8; i++) {
		if (address + i < memInfo.Size) {
			result |= ((uint64_t)memBuffer[address + i] << (8 * i));
		} else {
			break;
		}
	}

	emu->Stop(false);
	emu->Release();

	return result;
}

DllExport void __stdcall RomTestRecord(char* filename, bool reset) {
	_recordedRomTest = std::make_unique<RecordedRomTest>(_emu.get(), false);
	_recordedRomTest->Record(filename, reset);
}

DllExport void __stdcall RomTestStop() {
	if (_recordedRomTest) {
		_recordedRomTest->Stop();
		_recordedRomTest.reset();
	}
}

DllExport bool __stdcall RomTestRecording() {
	return _recordedRomTest != nullptr;
}
}
