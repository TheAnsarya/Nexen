#pragma once
#include "pch.h"
#include "Utilities/ISerializable.h"

class Emulator;

struct WsRtcState {
	uint8_t Year;
	uint8_t Month;
	uint8_t Day;
	uint8_t DoW;
	uint8_t Hour;
	uint8_t Minute;
	uint8_t Second;
};

class WsRtc final : public ISerializable {
private:
	Emulator* _emu = nullptr;

	WsRtcState _state = {};
	uint64_t _lastUpdateTime = 0;

	uint8_t _command = 0;
	uint8_t _bitCounter = 0;
	uint8_t _clk = 0;
	bool _chipSelect = false;

	uint8_t _dataBuffer[7] = {};
	uint8_t _dataSize = 0;
	uint8_t _dataIndex = 0;
	uint8_t _dataBitCounter = 0;
	bool _reading = false;

	uint8_t _bitOut = 0;

	void ProcessCommand();
	void UpdateTime();

	uint8_t GetDataLength();
	void PrepareReadData();
	void WriteData();

public:
	WsRtc(Emulator* emu);

	void LoadBattery();
	void SaveBattery();

	uint8_t Read();
	void Write(uint8_t value);

	void Serialize(Serializer& s) override;
};
