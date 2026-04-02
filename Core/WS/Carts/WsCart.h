#pragma once
#include "pch.h"
#include "WS/WsTypes.h"
#include "Utilities/ISerializable.h"
#include "Shared/MemoryType.h"

class Emulator;
class WsMemoryManager;
class WsEeprom;

// TODOWS RTC

class WsCart final : public ISerializable {
protected:
	WsCartState _state = {};

	Emulator* _emu = nullptr;
	WsMemoryManager* _memoryManager = nullptr;
	WsEeprom* _cartEeprom = nullptr;

	uint8_t* _prgRom = nullptr;
	uint32_t _prgRomSize = 0;

	void Map(uint32_t start, uint32_t end, MemoryType type, uint32_t offset, bool readonly);
	void Unmap(uint32_t start, uint32_t end);

public:
	WsCart();
	virtual ~WsCart() {}

	void Init(Emulator* emu, WsMemoryManager* memoryManager, WsEeprom* cartEeprom, uint8_t* prgRom, uint32_t prgRomSize, bool hasFlash);
	void RefreshMappings();

	WsCartState& GetState() { return _state; }
	WsEeprom* GetEeprom() { return _cartEeprom; }

	virtual uint8_t ReadPort(uint16_t port);
	virtual void WritePort(uint16_t port, uint8_t value);

	uint8_t FlashRead(uint32_t addr);
	void FlashWrite(uint32_t addr, uint8_t value);
	bool HasFlash() const { return _state.HasFlash; }
	bool IsFlashSoftwareId() const { return _state.FlashSoftwareId; }

	void Serialize(Serializer& s) override;
};
