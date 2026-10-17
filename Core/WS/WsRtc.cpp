#include "pch.h"
#include "WS/WsRtc.h"
#include "Utilities/Serializer.h"
#include "Shared/Emulator.h"
#include "Shared/BatteryManager.h"

WsRtc::WsRtc(Emulator* emu) {
	_emu = emu;

	_state.Month = 1;
	_state.Day = 1;

	_lastUpdateTime = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

uint8_t WsRtc::GetDataLength() {
	uint8_t reg = (_command >> 1) & 0x07;
	switch (reg) {
		case 0: return 1; // Status
		case 1: return 7; // DateTime
		case 2: return 2; // Alarm (INT)
		default: return 0;
	}
}

void WsRtc::PrepareReadData() {
	UpdateTime();

	uint8_t reg = (_command >> 1) & 0x07;
	memset(_dataBuffer, 0, sizeof(_dataBuffer));

	switch (reg) {
		case 0:
			// Status register
			_dataBuffer[0] = 0;
			break;

		case 1:
			// DateTime: Year/Month/Day/DoW/Hour/Minute/Second (BCD)
			_dataBuffer[0] = _state.Year;
			_dataBuffer[1] = _state.Month;
			_dataBuffer[2] = _state.Day;
			_dataBuffer[3] = _state.DoW;
			_dataBuffer[4] = _state.Hour;
			_dataBuffer[5] = _state.Minute;
			_dataBuffer[6] = _state.Second;
			break;

		case 2:
			// Alarm (not commonly used, return zeros)
			_dataBuffer[0] = 0;
			_dataBuffer[1] = 0;
			break;
	}
}

void WsRtc::WriteData() {
	UpdateTime();

	uint8_t reg = (_command >> 1) & 0x07;

	switch (reg) {
		case 0:
			// Status register write (no-op for now)
			break;

		case 1:
			// DateTime write
			_state.Year = _dataBuffer[0];
			_state.Month = _dataBuffer[1];
			_state.Day = _dataBuffer[2];
			_state.DoW = _dataBuffer[3];
			_state.Hour = _dataBuffer[4];
			_state.Minute = _dataBuffer[5];
			_state.Second = _dataBuffer[6];
			_lastUpdateTime = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
			break;

		case 2:
			// Alarm write (not commonly used)
			break;
	}
}

void WsRtc::ProcessCommand() {
	_reading = _command & 0x01;
	_dataSize = GetDataLength();
	_dataIndex = 0;
	_dataBitCounter = 0;

	if (_reading) {
		PrepareReadData();
	} else {
		memset(_dataBuffer, 0, sizeof(_dataBuffer));
	}
}

void WsRtc::UpdateTime() {
	uint64_t currentTime = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	uint32_t elapsedSeconds = (uint32_t)(currentTime - _lastUpdateTime);
	if (elapsedSeconds <= 0) {
		return;
	}

	std::tm tm = {};
	tm.tm_sec = (_state.Second & 0x0f) + ((_state.Second >> 4) * 10);
	tm.tm_min = (_state.Minute & 0x0f) + ((_state.Minute >> 4) * 10);
	int hour = _state.Hour & 0x3f;
	tm.tm_hour = (hour & 0x0f) + ((hour >> 4) * 10);
	if (tm.tm_hour < 12 && (_state.Hour & 0x80)) {
		tm.tm_hour += 12;
	}
	tm.tm_mday = (_state.Day & 0x0f) + ((_state.Day >> 4) * 10);

	int month = _state.Month - 1;
	tm.tm_mon = (month & 0x0f) + ((month >> 4) * 10);
	tm.tm_year = 100 + (_state.Year & 0x0f) + ((_state.Year >> 4) * 10);

	std::time_t tt = mktime(&tm);
	if (tt == -1) {
		_lastUpdateTime = currentTime;
		return;
	}

	int8_t dowGap = 0;
	if (tm.tm_wday != _state.DoW) {
		dowGap = (int8_t)tm.tm_wday - (int8_t)_state.DoW;
	}

	std::chrono::system_clock::time_point timePoint = std::chrono::system_clock::from_time_t(tt);
	timePoint += std::chrono::seconds((uint32_t)elapsedSeconds);

	std::time_t newTime = system_clock::to_time_t(timePoint);
	std::tm newTm;
#if (defined(WIN32) || defined(_WIN32) || defined(__WIN32__))
	localtime_s(&newTm, &newTime);
#else
	localtime_r(&newTime, &newTm);
#endif

	_state.Second = (newTm.tm_sec % 10) + ((newTm.tm_sec / 10) << 4);
	_state.Minute = (newTm.tm_min % 10) + ((newTm.tm_min / 10) << 4);
	_state.Hour = (newTm.tm_hour % 10) + ((newTm.tm_hour / 10) << 4);
	if (newTm.tm_hour >= 12) {
		_state.Hour |= 0x80;
	}
	_state.Day = (newTm.tm_mday % 10) + ((newTm.tm_mday / 10) << 4);

	month = newTm.tm_mon + 1;
	_state.Month = (month % 10) + ((month / 10) << 4);

	int year = newTm.tm_year - 100;
	_state.Year = (year % 10) + ((year / 10) << 4);

	int dow = newTm.tm_wday - dowGap;
	_state.DoW = dow < 0 ? (dow + 7) : (dow % 7);

	_lastUpdateTime = currentTime;
}

uint8_t WsRtc::Read() {
	return _bitOut;
}

void WsRtc::Write(uint8_t value) {
	uint8_t data = value & 0x01;
	uint8_t clk = (value >> 1) & 0x01;
	bool chipSelect = (value >> 4) & 0x01;

	if (!chipSelect) {
		if (_chipSelect && !_reading && _dataIndex > 0) {
			// CS went low after write — commit written data
			WriteData();
		}
		_chipSelect = false;
		_command = 0;
		_bitCounter = 0;
		_dataSize = 0;
		_dataIndex = 0;
		_dataBitCounter = 0;
		_reading = false;
		_bitOut = 0;
		_clk = 0;
		return;
	}

	if (!_chipSelect && chipSelect) {
		// CS transition low→high: begin new transaction
		_chipSelect = true;
		_command = 0;
		_bitCounter = 0;
		_dataSize = 0;
		_dataIndex = 0;
		_dataBitCounter = 0;
		_reading = false;
		_bitOut = 0;
		_clk = clk;
		return;
	}

	// Rising edge of clock
	if (clk && !_clk) {
		if (_bitCounter < 4) {
			// Command phase: receive 4 bits MSB-first
			_command = (_command << 1) | data;
			_bitCounter++;

			if (_bitCounter == 4) {
				ProcessCommand();
			}
		} else if (_dataIndex < _dataSize) {
			if (_reading) {
				// Read: output data bit MSB-first
				_bitOut = (_dataBuffer[_dataIndex] >> (7 - _dataBitCounter)) & 0x01;
				_dataBitCounter++;
				if (_dataBitCounter >= 8) {
					_dataBitCounter = 0;
					_dataIndex++;
				}
			} else {
				// Write: receive data bit MSB-first
				_dataBuffer[_dataIndex] = (_dataBuffer[_dataIndex] << 1) | data;
				_dataBitCounter++;
				if (_dataBitCounter >= 8) {
					_dataBitCounter = 0;
					_dataIndex++;
				}
			}
		}
	}

	_clk = clk;
}

void WsRtc::LoadBattery() {
	vector<uint8_t> rtcData = _emu->GetBatteryManager()->LoadBattery(".rtc");

	if (rtcData.size() == sizeof(_state) + sizeof(uint64_t)) {
		_state.Year = rtcData[0];
		_state.Month = rtcData[1];
		_state.Day = rtcData[2];
		_state.DoW = rtcData[3];
		_state.Hour = rtcData[4];
		_state.Minute = rtcData[5];
		_state.Second = rtcData[6];

		uint64_t time = 0;
		for (uint32_t i = 0; i < sizeof(uint64_t); i++) {
			time <<= 8;
			time |= rtcData[sizeof(_state) + i];
		}
		_lastUpdateTime = time;
	} else {
		_lastUpdateTime = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	}
}

void WsRtc::SaveBattery() {
	vector<uint8_t> rtcData;

	rtcData.push_back(_state.Year);
	rtcData.push_back(_state.Month);
	rtcData.push_back(_state.Day);
	rtcData.push_back(_state.DoW);
	rtcData.push_back(_state.Hour);
	rtcData.push_back(_state.Minute);
	rtcData.push_back(_state.Second);

	rtcData.resize(sizeof(_state) + sizeof(uint64_t), 0);

	uint64_t time = _lastUpdateTime;
	for (uint32_t i = 0; i < sizeof(uint64_t); i++) {
		rtcData[sizeof(_state) + i] = (time >> 56) & 0xff;
		time <<= 8;
	}

	_emu->GetBatteryManager()->SaveBattery(".rtc", std::span<const uint8_t>(rtcData));
}

void WsRtc::Serialize(Serializer& s) {
	SV(_command);
	SV(_bitCounter);
	SV(_clk);
	SV(_chipSelect);
	SVArray(_dataBuffer, 7);
	SV(_dataSize);
	SV(_dataIndex);
	SV(_dataBitCounter);
	SV(_reading);
	SV(_bitOut);
	SV(_lastUpdateTime);

	SV(_state.Year);
	SV(_state.Month);
	SV(_state.Day);
	SV(_state.DoW);
	SV(_state.Hour);
	SV(_state.Minute);
	SV(_state.Second);
}
