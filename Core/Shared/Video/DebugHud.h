#pragma once
#include "pch.h"
#include "Utilities/SimpleLock.h"
#include "Shared/SettingTypes.h"
#include "Shared/Video/DrawCommand.h"

class DebugHud {
private:
	static constexpr size_t MaxCommandCount = 500000;
	vector<unique_ptr<DrawCommand>> _commands;
	atomic<uint32_t> _commandCount;
	SimpleLock _commandLock;

	// Flat pixel buffer (width*height) replaces unordered_map for O(1) indexed access.
	// Value 0 = no pixel drawn (transparent), nonzero = drawn ARGB color.
	vector<uint32_t> _drawPixels;
	vector<uint32_t> _prevDrawPixels;
	// Dirty pixel indices for fast diff/clear without scanning the full buffer
	vector<uint32_t> _dirtyIndices;

public:
	DebugHud();
	~DebugHud();

	[[nodiscard]] bool HasCommands() { return _commandCount > 0; }

	bool Draw(uint32_t* argbBuffer, FrameInfo frameInfo, OverscanDimensions overscan, uint32_t frameNumber, HudScaleFactors scaleFactors, bool clearAndUpdate = false);
	void ClearScreen();

	void DrawPixel(int x, int y, int color, int frameCount, int startFrame = -1);
	void DrawLine(int x, int y, int x2, int y2, int color, int frameCount, int startFrame = -1);
	void DrawRectangle(int x, int y, int width, int height, int color, bool fill, int frameCount, int startFrame = -1);
	void DrawString(int x, int y, const string& text, int color, int backColor, int frameCount, int startFrame = -1, int maxWidth = 0, bool overwritePixels = false, int fontScale = 1);

	__forceinline void AddCommand(unique_ptr<DrawCommand> cmd) {
		auto lock = _commandLock.AcquireSafe();
		if (_commands.size() < DebugHud::MaxCommandCount) {
			_commands.push_back(std::move(cmd));
			_commandCount++;
		}
	}
};
