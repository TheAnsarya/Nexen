#include "pch.h"
#include "Atari2600/Atari2600Bus.h"
#include "Atari2600/Atari2600Riot.h"
#include "Atari2600/Atari2600Tia.h"
#include "Atari2600/Atari2600Mapper.h"

uint8_t Atari2600Bus::Read(uint16_t addr) {
	addr &= 0x1FFF;
	if ((addr & 0x1000) == 0x1000 && _mapper) {
		return _mapper->Read(addr);
	}
	if ((addr & 0x1080) == 0x0080 && _riot) {
		return _riot->ReadRegister(addr);
	}
	if ((addr & 0x1080) == 0x0000 && _tia) {
		return _tia->ReadRegister(addr);
	}
	return 0;
}

void Atari2600Bus::Write(uint16_t addr, uint8_t value) {
	addr &= 0x1FFF;
	if (_mapper) {
		_mapper->Write(addr, value);
	}

	if ((addr & 0x1080) == 0x0080 && _riot) {
		_riot->WriteRegister(addr, value);
		return;
	}
	if ((addr & 0x1080) == 0x0000 && _tia) {
		_tia->WriteRegister(addr, value);
	}
}
