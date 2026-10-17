#pragma once
#include "SDL.h"
#include "Core/Shared/Audio/BaseSoundManager.h"
#include <atomic>

class Emulator;

class SdlSoundManager : public BaseSoundManager
{
public:
	SdlSoundManager(Emulator* emu);
	~SdlSoundManager();

	void PlayBuffer(int16_t *soundBuffer, uint32_t bufferSize, uint32_t sampleRate, bool isStereo);
	void Pause();
	void Stop();

	void ProcessEndOfFrame();

	string GetAvailableDevices();
	void SetAudioDevice(const string& deviceName);

private:
	vector<string> GetAvailableDeviceInfo();
	bool InitializeAudio(uint32_t sampleRate, bool isStereo);
	void Release();

	static void FillAudioBuffer(void *userData, uint8_t *stream, int len);

	void ReadFromBuffer(uint8_t* output, uint32_t len);
	void WriteToBuffer(uint8_t* output, uint32_t len);

private:
	Emulator* _emu;
	SDL_AudioDeviceID _audioDeviceID;
	string _deviceName;
	bool _needReset = false;

	uint16_t _previousLatency = 0;

	uint8_t* _buffer = nullptr;
	std::atomic<uint32_t> _writePosition{0};
	std::atomic<uint32_t> _readPosition{0};
};
