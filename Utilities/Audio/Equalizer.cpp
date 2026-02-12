#include "pch.h"
#include "Equalizer.h"
#include "orfanidis_eq.h"

void Equalizer::ApplyEqualizer(uint32_t sampleCount, int16_t* samples) {
	double outL, outR;
	for (uint32_t i = 0; i < sampleCount; i++) {
		double inL = samples[i * 2];
		double inR = samples[i * 2 + 1];

		_equalizerLeft->sbs_process(&inL, &outL);
		_equalizerRight->sbs_process(&inR, &outR);

		samples[i * 2] = (int16_t)std::max(std::min(outL, 32767.0), -32768.0);
		samples[i * 2 + 1] = (int16_t)std::max(std::min(outR, 32767.0), -32768.0);
	}
}

void Equalizer::UpdateEqualizers(std::span<const double, 20> bandGains, uint32_t sampleRate) {
	if (_prevSampleRate != sampleRate || !std::equal(bandGains.begin(), bandGains.end(), _prevEqualizerGains.begin())) {
		// Pre-computed band frequencies with boundary values included
		// Original: {40, 56, 80, 113, 160, 225, 320, 450, 600, 750, 1000, 2000, 3000, 4000, 5000, 6000, 7000, 10000, 12500, 13000}
		// Boundary[0] = 40 - (56 - 40) = 24
		// Boundary[21] = 13000 + (13000 - 12500) = 13500
		static constexpr std::array<double, 22> bands = {
			24, 40, 56, 80, 113, 160, 225, 320, 450, 600, 750, 1000,
			2000, 3000, 4000, 5000, 6000, 7000, 10000, 12500, 13000, 13500
		};

		_eqFrequencyGrid.reset(new orfanidis_eq::freq_grid());
		for (size_t i = 1; i < bands.size() - 1; i++) {
			_eqFrequencyGrid->add_band((bands[i] + bands[i - 1]) / 2, bands[i], (bands[i + 1] + bands[i]) / 2);
		}

		_equalizerLeft.reset(new orfanidis_eq::eq1(_eqFrequencyGrid.get(), orfanidis_eq::filter_type::butterworth));
		_equalizerRight.reset(new orfanidis_eq::eq1(_eqFrequencyGrid.get(), orfanidis_eq::filter_type::butterworth));
		_equalizerLeft->set_sample_rate(sampleRate);
		_equalizerRight->set_sample_rate(sampleRate);

		for (unsigned int i = 0; i < _eqFrequencyGrid->get_number_of_bands(); i++) {
			_equalizerLeft->change_band_gain_db(i, bandGains[i]);
			_equalizerRight->change_band_gain_db(i, bandGains[i]);
		}

		_prevSampleRate = sampleRate;
		std::copy(bandGains.begin(), bandGains.end(), _prevEqualizerGains.begin());
	}
}
