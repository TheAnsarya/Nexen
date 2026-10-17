#include "pch.h"
#include "CrossFeedFilter.h"

void CrossFeedFilter::ApplyFilter(int16_t* stereoBuffer, size_t sampleCount, int ratio) {
	for (size_t i = 0; i < sampleCount; i++) {
		int16_t leftSample = stereoBuffer[0];
		int16_t rightSample = stereoBuffer[1];

		// Use int32_t arithmetic to prevent int16_t overflow clipping
		int32_t newLeft = (int32_t)leftSample + (int32_t)rightSample * ratio / 100;
		int32_t newRight = (int32_t)rightSample + (int32_t)leftSample * ratio / 100;
		stereoBuffer[0] = (int16_t)std::clamp(newLeft, (int32_t)INT16_MIN, (int32_t)INT16_MAX);
		stereoBuffer[1] = (int16_t)std::clamp(newRight, (int32_t)INT16_MIN, (int32_t)INT16_MAX);

		stereoBuffer += 2;
	}
}
