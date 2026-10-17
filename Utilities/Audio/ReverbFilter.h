#pragma once
#include "pch.h"
#include <vector>

/// <summary>
/// Single reverb delay line using a fixed-size circular buffer.
/// Replaces std::deque per-sample push/pop with O(1) ring buffer
/// operations for cache-friendly contiguous memory access.
/// </summary>
class ReverbDelay {
private:
	std::vector<int16_t> _ringBuffer;
	uint32_t _writePos = 0;
	uint32_t _readPos = 0;
	uint32_t _count = 0;
	uint32_t _capacity = 0;
	uint32_t _delay = 0;
	double _decay = 0;

public:
	void SetParameters(double delay, double decay, int32_t sampleRate) {
		uint32_t delaySampleCount = (uint32_t)(delay / 1000 * sampleRate);
		if (delaySampleCount != _delay || decay != _decay) {
			_delay = delaySampleCount;
			_decay = decay;
			// Pre-allocate ring buffer with generous capacity
			_capacity = _delay + 8192;
			_ringBuffer.assign(_capacity, 0);
			_writePos = 0;
			_readPos = 0;
			_count = 0;
		}
	}

	void Reset() {
		_writePos = 0;
		_readPos = 0;
		_count = 0;
		if (_capacity > 0) {
			std::fill(_ringBuffer.begin(), _ringBuffer.end(), 0);
		}
	}

	void AddSamples(int16_t* buffer, size_t sampleCount) {
		for (size_t i = 0; i < sampleCount; i++) {
			if (_count < _capacity) {
				_ringBuffer[_writePos] = buffer[i * 2];
				_writePos = (_writePos + 1) % _capacity;
				_count++;
			}
		}
	}

	void ApplyReverb(int16_t* buffer, size_t sampleCount) {
		if (_count > _delay) {
			size_t samplesToInsert = std::min<size_t>(_count - _delay, sampleCount);

			for (size_t j = sampleCount - samplesToInsert; j < sampleCount; j++) {
				buffer[j * 2] += (int16_t)((double)_ringBuffer[_readPos] * _decay);
				_readPos = (_readPos + 1) % _capacity;
				_count--;
			}
		}
	}
};

class ReverbFilter {
private:
	ReverbDelay _delay[10];

public:
	void ResetFilter();
	void ApplyFilter(int16_t* stereoBuffer, size_t sampleCount, uint32_t sampleRate, double reverbStrength, double reverbDelay);
};
