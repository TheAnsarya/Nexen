#include "pch.h"
#include <algorithm>
#include "Shared/Video/DebugHud.h"
#include "Shared/Video/DrawCommand.h"
#include "Shared/Video/DrawLineCommand.h"
#include "Shared/Video/DrawPixelCommand.h"
#include "Shared/Video/DrawRectangleCommand.h"
#include "Shared/Video/DrawStringCommand.h"
#include "Shared/Video/DrawScreenBufferCommand.h"

DebugHud::DebugHud() {
	_commandCount = 0;
}

DebugHud::~DebugHud() {
	_commandLock.Acquire();
	_commandLock.Release();
}

void DebugHud::ClearScreen() {
	auto lock = _commandLock.AcquireSafe();
	_commands.clear();
	_drawPixels.clear();
	_prevDrawPixels.clear();
	_dirtyIndices.clear();
}

bool DebugHud::Draw(uint32_t* argbBuffer, FrameInfo frameInfo, OverscanDimensions overscan, uint32_t frameNumber, HudScaleFactors scaleFactors, bool clearAndUpdate) {
	auto lock = _commandLock.AcquireSafe();

	bool isDirty = false;
	uint32_t bufferSize = frameInfo.Height * frameInfo.Width;

	if (clearAndUpdate) {
		// Ensure flat buffers are sized for this frame
		if ((uint32_t)_drawPixels.size() != bufferSize) {
			_drawPixels.assign(bufferSize, 0);
			_prevDrawPixels.assign(bufferSize, 0);
			_dirtyIndices.clear();
		} else {
			// Clear only previously-dirty pixels (O(k) vs O(n) memset)
			for (uint32_t idx : _dirtyIndices) {
				_drawPixels[idx] = 0;
			}
			_dirtyIndices.clear();
		}

		// Draw all commands into the flat buffer — O(1) per pixel vs O(1) amortized hash
		for (unique_ptr<DrawCommand>& command : _commands) {
			command->Draw(_drawPixels.data(), bufferSize, &_dirtyIndices, argbBuffer, frameInfo, overscan, frameNumber, scaleFactors);
		}

		// Fast diff: compare only dirty pixel positions against previous frame
		// Also need to detect pixels that were in prev but not in current
		isDirty = (memcmp(_drawPixels.data(), _prevDrawPixels.data(), bufferSize * sizeof(uint32_t)) != 0);

		if (isDirty) {
			memset(argbBuffer, 0, bufferSize * sizeof(uint32_t));
			for (uint32_t idx : _dirtyIndices) {
				argbBuffer[idx] = _drawPixels[idx];
			}
			// Swap current → previous for next frame comparison
			std::swap(_prevDrawPixels, _drawPixels);
		}
	} else {
		isDirty = true;
		for (unique_ptr<DrawCommand>& command : _commands) {
			command->Draw(nullptr, 0, nullptr, argbBuffer, frameInfo, overscan, frameNumber, scaleFactors);
		}
	}

	_commands.erase(std::remove_if(_commands.begin(), _commands.end(), [](const unique_ptr<DrawCommand>& c) { return c->Expired(); }), _commands.end());
	_commandCount = (uint32_t)_commands.size();

	return isDirty;
}

void DebugHud::DrawPixel(int x, int y, int color, int frameCount, int startFrame) {
	AddCommand(std::make_unique<DrawPixelCommand>(x, y, color, frameCount, startFrame));
}

void DebugHud::DrawLine(int x, int y, int x2, int y2, int color, int frameCount, int startFrame) {
	AddCommand(std::make_unique<DrawLineCommand>(x, y, x2, y2, color, frameCount, startFrame));
}

void DebugHud::DrawRectangle(int x, int y, int width, int height, int color, bool fill, int frameCount, int startFrame) {
	AddCommand(std::make_unique<DrawRectangleCommand>(x, y, width, height, color, fill, frameCount, startFrame));
}

void DebugHud::DrawString(int x, int y, const string& text, int color, int backColor, int frameCount, int startFrame, int maxWidth, bool overwritePixels, int fontScale) {
	AddCommand(std::make_unique<DrawStringCommand>(x, y, text, color, backColor, frameCount, startFrame, maxWidth, overwritePixels, fontScale));
}
