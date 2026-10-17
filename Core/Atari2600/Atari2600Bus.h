#pragma once
#include "pch.h"

class Atari2600Riot;
class Atari2600Tia;
class Atari2600Mapper;

class Atari2600Bus {
private:
	Atari2600Riot* _riot = nullptr;
	Atari2600Tia* _tia = nullptr;
	Atari2600Mapper* _mapper = nullptr;

public:
	void Attach(Atari2600Riot* riot, Atari2600Tia* tia, Atari2600Mapper* mapper) {
		_riot = riot;
		_tia = tia;
		_mapper = mapper;
	}

	uint8_t Read(uint16_t addr);
	void Write(uint16_t addr, uint8_t value);
};
