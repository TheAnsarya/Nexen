#pragma once
#include "pch.h"
#include <array>
#include "Shared/Video/DrawCommand.h"

class DrawStringCommand : public DrawCommand {
private:
	int _x, _y, _color, _backColor;
	int _maxWidth = 0;
	int _fontScale = 1;
	string _text;

	// Taken from FCEUX's LUA code
	static constexpr int _tabSpace = 4;

	/// Sorted array of Japanese font glyph data (6880 entries, binary search lookup).
	/// Keys are 3-byte UTF-8 codes packed into int, values are 12-byte glyph bitmaps.
	static const std::array<std::pair<int, char const*>, 6880> _jpFont;

	static constexpr uint8_t _font[792] = {
	    6,
	    0,
	    0,
	    0,
	    0,
	    0,
	    0,
	    0, // 0x20 - Spacebar
	    2,
	    128,
	    128,
	    128,
	    128,
	    128,
	    0,
	    128,
	    5,
	    80,
	    80,
	    80,
	    0,
	    0,
	    0,
	    0,
	    6,
	    80,
	    80,
	    248,
	    80,
	    248,
	    80,
	    80,
	    6,
	    32,
	    120,
	    160,
	    112,
	    40,
	    240,
	    32,
	    6,
	    64,
	    168,
	    80,
	    32,
	    80,
	    168,
	    16,
	    6,
	    96,
	    144,
	    160,
	    64,
	    168,
	    144,
	    104,
	    3,
	    64,
	    64,
	    0,
	    0,
	    0,
	    0,
	    0,
	    4,
	    32,
	    64,
	    64,
	    64,
	    64,
	    64,
	    32,
	    4,
	    128,
	    64,
	    64,
	    64,
	    64,
	    64,
	    128,
	    6,
	    0,
	    80,
	    32,
	    248,
	    32,
	    80,
	    0,
	    6,
	    0,
	    32,
	    32,
	    248,
	    32,
	    32,
	    0,
	    3,
	    0,
	    0,
	    0,
	    0,
	    0,
	    64,
	    128,
	    5,
	    0,
	    0,
	    0,
	    240,
	    0,
	    0,
	    0,
	    3,
	    0,
	    0,
	    0,
	    0,
	    0,
	    0,
	    64,
	    5,
	    16,
	    16,
	    32,
	    32,
	    32,
	    64,
	    64,
	    6,
	    112,
	    136,
	    136,
	    136,
	    136,
	    136,
	    112, // 0x30 - 0
	    6,
	    32,
	    96,
	    32,
	    32,
	    32,
	    32,
	    32,
	    6,
	    112,
	    136,
	    8,
	    48,
	    64,
	    128,
	    248,
	    6,
	    112,
	    136,
	    8,
	    48,
	    8,
	    136,
	    112,
	    6,
	    16,
	    48,
	    80,
	    144,
	    248,
	    16,
	    16,
	    6,
	    248,
	    128,
	    128,
	    240,
	    8,
	    8,
	    240,
	    6,
	    48,
	    64,
	    128,
	    240,
	    136,
	    136,
	    112,
	    6,
	    248,
	    8,
	    16,
	    16,
	    32,
	    32,
	    32,
	    6,
	    112,
	    136,
	    136,
	    112,
	    136,
	    136,
	    112,
	    6,
	    112,
	    136,
	    136,
	    120,
	    8,
	    16,
	    96,
	    3,
	    0,
	    0,
	    64,
	    0,
	    0,
	    64,
	    0,
	    3,
	    0,
	    0,
	    64,
	    0,
	    0,
	    64,
	    128,
	    4,
	    0,
	    32,
	    64,
	    128,
	    64,
	    32,
	    0,
	    5,
	    0,
	    0,
	    240,
	    0,
	    240,
	    0,
	    0,
	    4,
	    0,
	    128,
	    64,
	    32,
	    64,
	    128,
	    0,
	    6,
	    112,
	    136,
	    8,
	    16,
	    32,
	    0,
	    32, // 0x3F - ?
	    6,
	    112,
	    136,
	    136,
	    184,
	    176,
	    128,
	    112, // 0x40 - @
	    6,
	    112,
	    136,
	    136,
	    248,
	    136,
	    136,
	    136, // 0x41 - A
	    6,
	    240,
	    136,
	    136,
	    240,
	    136,
	    136,
	    240,
	    6,
	    112,
	    136,
	    128,
	    128,
	    128,
	    136,
	    112,
	    6,
	    224,
	    144,
	    136,
	    136,
	    136,
	    144,
	    224,
	    6,
	    248,
	    128,
	    128,
	    240,
	    128,
	    128,
	    248,
	    6,
	    248,
	    128,
	    128,
	    240,
	    128,
	    128,
	    128,
	    6,
	    112,
	    136,
	    128,
	    184,
	    136,
	    136,
	    120,
	    6,
	    136,
	    136,
	    136,
	    248,
	    136,
	    136,
	    136,
	    4,
	    224,
	    64,
	    64,
	    64,
	    64,
	    64,
	    224,
	    6,
	    8,
	    8,
	    8,
	    8,
	    8,
	    136,
	    112,
	    6,
	    136,
	    144,
	    160,
	    192,
	    160,
	    144,
	    136,
	    6,
	    128,
	    128,
	    128,
	    128,
	    128,
	    128,
	    248,
	    6,
	    136,
	    216,
	    168,
	    168,
	    136,
	    136,
	    136,
	    6,
	    136,
	    136,
	    200,
	    168,
	    152,
	    136,
	    136,
	    7,
	    48,
	    72,
	    132,
	    132,
	    132,
	    72,
	    48,
	    6,
	    240,
	    136,
	    136,
	    240,
	    128,
	    128,
	    128,
	    6,
	    112,
	    136,
	    136,
	    136,
	    168,
	    144,
	    104,
	    6,
	    240,
	    136,
	    136,
	    240,
	    144,
	    136,
	    136,
	    6,
	    112,
	    136,
	    128,
	    112,
	    8,
	    136,
	    112,
	    6,
	    248,
	    32,
	    32,
	    32,
	    32,
	    32,
	    32,
	    6,
	    136,
	    136,
	    136,
	    136,
	    136,
	    136,
	    112,
	    6,
	    136,
	    136,
	    136,
	    80,
	    80,
	    32,
	    32,
	    6,
	    136,
	    136,
	    136,
	    136,
	    168,
	    168,
	    80,
	    6,
	    136,
	    136,
	    80,
	    32,
	    80,
	    136,
	    136,
	    6,
	    136,
	    136,
	    80,
	    32,
	    32,
	    32,
	    32,
	    6,
	    248,
	    8,
	    16,
	    32,
	    64,
	    128,
	    248,
	    3,
	    192,
	    128,
	    128,
	    128,
	    128,
	    128,
	    192,
	    5,
	    64,
	    64,
	    32,
	    32,
	    32,
	    16,
	    16,
	    3,
	    192,
	    64,
	    64,
	    64,
	    64,
	    64,
	    192,
	    4,
	    64,
	    160,
	    0,
	    0,
	    0,
	    0,
	    0,
	    6,
	    0,
	    0,
	    0,
	    0,
	    0,
	    0,
	    248,
	    3,
	    128,
	    64,
	    0,
	    0,
	    0,
	    0,
	    0,
	    5,
	    0,
	    0,
	    96,
	    16,
	    112,
	    144,
	    112, // 0x61 - a
	    5,
	    128,
	    128,
	    224,
	    144,
	    144,
	    144,
	    224,
	    5,
	    0,
	    0,
	    112,
	    128,
	    128,
	    128,
	    112,
	    5,
	    16,
	    16,
	    112,
	    144,
	    144,
	    144,
	    112,
	    5,
	    0,
	    0,
	    96,
	    144,
	    240,
	    128,
	    112,
	    5,
	    48,
	    64,
	    224,
	    64,
	    64,
	    64,
	    64,
	    5,
	    0,
	    112,
	    144,
	    144,
	    112,
	    16,
	    224,
	    5,
	    128,
	    128,
	    224,
	    144,
	    144,
	    144,
	    144,
	    2,
	    128,
	    0,
	    128,
	    128,
	    128,
	    128,
	    128,
	    4,
	    32,
	    0,
	    32,
	    32,
	    32,
	    32,
	    192,
	    5,
	    128,
	    128,
	    144,
	    160,
	    192,
	    160,
	    144,
	    2,
	    128,
	    128,
	    128,
	    128,
	    128,
	    128,
	    128,
	    6,
	    0,
	    0,
	    208,
	    168,
	    168,
	    168,
	    168,
	    5,
	    0,
	    0,
	    224,
	    144,
	    144,
	    144,
	    144,
	    5,
	    0,
	    0,
	    96,
	    144,
	    144,
	    144,
	    96,
	    5,
	    0,
	    224,
	    144,
	    144,
	    224,
	    128,
	    128,
	    5,
	    0,
	    112,
	    144,
	    144,
	    112,
	    16,
	    16,
	    5,
	    0,
	    0,
	    176,
	    192,
	    128,
	    128,
	    128,
	    5,
	    0,
	    0,
	    112,
	    128,
	    96,
	    16,
	    224,
	    4,
	    64,
	    64,
	    224,
	    64,
	    64,
	    64,
	    32,
	    5,
	    0,
	    0,
	    144,
	    144,
	    144,
	    144,
	    112,
	    5,
	    0,
	    0,
	    144,
	    144,
	    144,
	    160,
	    192,
	    6,
	    0,
	    0,
	    136,
	    136,
	    168,
	    168,
	    80,
	    5,
	    0,
	    0,
	    144,
	    144,
	    96,
	    144,
	    144,
	    5,
	    0,
	    144,
	    144,
	    144,
	    112,
	    16,
	    96,
	    5,
	    0,
	    0,
	    240,
	    32,
	    64,
	    128,
	    240,
	    4,
	    32,
	    64,
	    64,
	    128,
	    64,
	    64,
	    32,
	    3,
	    64,
	    64,
	    64,
	    64,
	    64,
	    64,
	    64,
	    4,
	    128,
	    64,
	    64,
	    32,
	    64,
	    64,
	    128,
	    6,
	    0,
	    104,
	    176,
	    0,
	    0,
	    0,
	    0,
	    5,
	    0,
	    0,
	    113,
	    80,
	    113,
	    0,
	    0,
	};

	static int GetCharNumber(char ch) {
		if (ch < 32) {
			return 0;
		}

		ch -= 32;
		return (ch < 0 || ch > 94) ? 95 : ch;
	}

	static int GetCharWidth(char ch) {
		return _font[GetCharNumber(ch) * 8];
	}

protected:
	void InternalDraw() {
		int scale = std::max(1, _fontScale);
		int startX = (int)(_x * _xScale / std::floor(_xScale));
		int lineWidth = 0;
		int x = startX;
		int y = _y;
		int baseLineHeight = 9;
		int lineHeight = baseLineHeight * scale;

		auto newLine = [&lineWidth, &x, &y, &lineHeight, startX, scale, baseLineHeight]() {
			lineWidth = 0;
			x = startX;
			y += lineHeight;
			lineHeight = baseLineHeight * scale;
		};

		for (int i = 0; i < _text.size(); i++) {
			unsigned char c = _text[i];
			if (c == '\n') {
				newLine();
			} else if (c == '\t') {
				int tabWidth = (_tabSpace - (((x - startX) / (8 * scale)) % _tabSpace)) * 8 * scale;
				x += tabWidth;
				lineWidth += tabWidth;
				if (_maxWidth > 0 && lineWidth > _maxWidth) {
					newLine();
				}
			} else if (c == 0x20) {
				// Space (ignore spaces at the start of a new line, when text wrapping is enabled)
				int spaceWidth = 6 * scale;
				if (lineWidth > 0 || _maxWidth == 0) {
					if (_backColor & 0xFF000000) {
						// Draw bg color for spaces (when bg color is set)
						for (int row = 0; row < lineHeight; row++) {
							for (int column = 0; column < spaceWidth; column++) {
								DrawPixel(x + column, y + row - scale, _backColor);
							}
						}
					}

					lineWidth += spaceWidth;
					x += spaceWidth;
				}
			} else if (c >= 0x80) {
				// 8x12 UTF-8 font for Japanese
				int code = (uint8_t)c;
				if (i + 2 < _text.size()) {
					code |= ((uint8_t)_text[i + 1]) << 8;
					code |= ((uint8_t)_text[i + 2]) << 16;

					auto res = std::lower_bound(
						_jpFont.begin(), _jpFont.end(), code,
						[](const auto& pair, int k) { return pair.first < k; }
					);
					if (res != _jpFont.end() && res->first == code) {
						int charWidth = 8 * scale;
						lineWidth += charWidth;
						if (_maxWidth > 0 && lineWidth > _maxWidth) {
							newLine();
							lineWidth += charWidth;
						}

						uint8_t* charDef = (uint8_t*)res->second;

						for (int row = 0; row < 12; row++) {
							uint8_t rowData = charDef[row];
							for (int column = 0; column < 8; column++) {
								int drawFg = (rowData >> (7 - column)) & 0x01;
								int color = drawFg ? _color : _backColor;
								for (int sy = 0; sy < scale; sy++) {
									for (int sx = 0; sx < scale; sx++) {
										DrawPixel(x + column * scale + sx, y + row * scale + sy - 2 * scale, color);
									}
								}
							}
						}
						i += 2;
						x += 8 * scale;
						lineHeight = 12 * scale;
					}
				}
			} else {
				// Variable size font for standard ASCII
				int ch = GetCharNumber(c);
				int width = GetCharWidth(c);
				int scaledWidth = width * scale;

				lineWidth += scaledWidth;
				if (_maxWidth > 0 && lineWidth > _maxWidth) {
					newLine();
					lineWidth += scaledWidth;
				}

				int rowOffset = (c == 'y' || c == 'g' || c == 'p' || c == 'q') ? 1 : 0;
				for (int row = 0; row < 8; row++) {
					uint8_t rowData = ((row == 7 && rowOffset == 0) || (row == 0 && rowOffset == 1)) ? 0 : _font[ch * 8 + 1 + row - rowOffset];
					for (int col = 0; col < width; col++) {
						int drawFg = (rowData >> (7 - col)) & 0x01;
						int color = drawFg ? _color : _backColor;
						for (int sy = 0; sy < scale; sy++) {
							for (int sx = 0; sx < scale; sx++) {
								DrawPixel(x + col * scale + sx, y + row * scale + sy, color);
							}
						}
					}
				}
				for (int col = 0; col < width; col++) {
					for (int sx = 0; sx < scale; sx++) {
						DrawPixel(x + col * scale + sx, y - scale, _backColor);
					}
				}
				x += scaledWidth;
			}
		}
	}

public:
	DrawStringCommand(int x, int y, string text, int color, int backColor, int frameCount, int startFrame, int maxWidth = 0, bool overwritePixels = false, int fontScale = 1) : DrawCommand(startFrame, frameCount, true), _x(x), _y(y), _color(color), _backColor(backColor), _maxWidth(maxWidth), _fontScale(std::max(1, fontScale)), _text(std::move(text)) {
		// Invert alpha byte - 0 = opaque, 255 = transparent (this way, no need to specifiy alpha channel all the time)
		_overwritePixels = overwritePixels;
		_color = (~color & 0xFF000000) | (color & 0xFFFFFF);
		_backColor = (~backColor & 0xFF000000) | (backColor & 0xFFFFFF);
	}

	static TextSize MeasureString(string& text, uint32_t maxWidth = 0, int fontScale = 1) {
		int scale = std::max(1, fontScale);
		uint32_t maxX = 0;
		uint32_t x = 0;
		uint32_t y = 0;
		uint32_t lineHeight = 9 * scale;

		auto newLine = [&]() {
			maxX = std::max(x, maxX);
			x = 0;
			y += lineHeight;
			lineHeight = 9 * scale;
		};

		for (int i = 0; i < text.size(); i++) {
			unsigned char c = text[i];
			if (c == '\n') {
				maxX = std::max(x, maxX);
				x = 0;
				y += 9 * scale;
			} else if (c == '\t') {
				x += (_tabSpace - (((x / (8 * scale)) % _tabSpace))) * 8 * scale;
			} else if (c == 0x20) {
				// Space (ignore spaces at the start of a new line, when text wrapping is enabled)
				uint32_t spaceWidth = 6 * scale;
				if (x > 0 || maxWidth == 0) {
					if (maxWidth > 0 && x + spaceWidth > maxWidth) {
						newLine();
					}
					x += spaceWidth;
				}
			} else if (c >= 0x80) {
				// 8x12 UTF-8 font for Japanese
				int code = (uint8_t)c;
				if (i + 2 < text.size()) {
					code |= ((uint8_t)text[i + 1]) << 8;
					code |= ((uint8_t)text[i + 2]) << 16;
					auto res = std::lower_bound(
						_jpFont.begin(), _jpFont.end(), code,
						[](const auto& pair, int k) { return pair.first < k; }
					);
					if (res != _jpFont.end() && res->first == code) {
						uint32_t charWidth = 8 * scale;
						if (maxWidth > 0 && x + charWidth > maxWidth) {
							newLine();
						}

						i += 2;
						x += charWidth;
						lineHeight = 12 * scale;
					}
				}
			} else {
				// Variable size font for standard ASCII
				uint32_t charWidth = GetCharWidth(c) * scale;
				if (maxWidth > 0 && x + charWidth > maxWidth) {
					newLine();
				}
				x += charWidth;
			}
		}

		TextSize size;
		size.X = std::max(x, maxX);
		size.Y = y + lineHeight;
		return size;
	}
};
