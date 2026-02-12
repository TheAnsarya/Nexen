#pragma once
#include "pch.h"
#include "orfanidis_eq.h"
#include <array>
#include <span>

class Equalizer {
private:
	unique_ptr<orfanidis_eq::freq_grid> _eqFrequencyGrid;
	unique_ptr<orfanidis_eq::eq1> _equalizerLeft;
	unique_ptr<orfanidis_eq::eq1> _equalizerRight;

	uint32_t _prevSampleRate = 0;
	std::array<double, 20> _prevEqualizerGains{};

public:
	void ApplyEqualizer(uint32_t sampleCount, int16_t* samples);
	void UpdateEqualizers(std::span<const double, 20> bandGains, uint32_t sampleRate);
};