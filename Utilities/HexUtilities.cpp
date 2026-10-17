#include "pch.h"
#include "HexUtilities.h"

constexpr const char* _hexCharCache[256] = {
    "00", "01", "02", "03", "04", "05", "06", "07", "08", "09", "0A", "0B", "0C", "0D", "0E", "0F",
    "10", "11", "12", "13", "14", "15", "16", "17", "18", "19", "1A", "1B", "1C", "1D", "1E", "1F",
    "20", "21", "22", "23", "24", "25", "26", "27", "28", "29", "2A", "2B", "2C", "2D", "2E", "2F",
    "30", "31", "32", "33", "34", "35", "36", "37", "38", "39", "3A", "3B", "3C", "3D", "3E", "3F",
    "40", "41", "42", "43", "44", "45", "46", "47", "48", "49", "4A", "4B", "4C", "4D", "4E", "4F",
    "50", "51", "52", "53", "54", "55", "56", "57", "58", "59", "5A", "5B", "5C", "5D", "5E", "5F",
    "60", "61", "62", "63", "64", "65", "66", "67", "68", "69", "6A", "6B", "6C", "6D", "6E", "6F",
    "70", "71", "72", "73", "74", "75", "76", "77", "78", "79", "7A", "7B", "7C", "7D", "7E", "7F",
    "80", "81", "82", "83", "84", "85", "86", "87", "88", "89", "8A", "8B", "8C", "8D", "8E", "8F",
    "90", "91", "92", "93", "94", "95", "96", "97", "98", "99", "9A", "9B", "9C", "9D", "9E", "9F",
    "A0", "A1", "A2", "A3", "A4", "A5", "A6", "A7", "A8", "A9", "AA", "AB", "AC", "AD", "AE", "AF",
    "B0", "B1", "B2", "B3", "B4", "B5", "B6", "B7", "B8", "B9", "BA", "BB", "BC", "BD", "BE", "BF",
    "C0", "C1", "C2", "C3", "C4", "C5", "C6", "C7", "C8", "C9", "CA", "CB", "CC", "CD", "CE", "CF",
    "D0", "D1", "D2", "D3", "D4", "D5", "D6", "D7", "D8", "D9", "DA", "DB", "DC", "DD", "DE", "DF",
    "E0", "E1", "E2", "E3", "E4", "E5", "E6", "E7", "E8", "E9", "EA", "EB", "EC", "ED", "EE", "EF",
    "F0", "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "FA", "FB", "FC", "FD", "FE", "FF"};

constexpr const char* _hexSingleChar[256] = {
    "0",
    "1",
    "2",
    "3",
    "4",
    "5",
    "6",
    "7",
    "8",
    "9",
    "A",
    "B",
    "C",
    "D",
    "E",
    "F",
};

// Constexpr nibble lookup table for branchless hex parsing (char → 0-15, -1 for invalid)
constexpr int8_t _hexNibbleLut[256] = {
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 0x00-0x0F
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 0x10-0x1F
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 0x20-0x2F
	 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,-1,-1,-1,-1,-1,-1, // 0x30-0x3F ('0'-'9')
	-1,10,11,12,13,14,15,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 0x40-0x4F ('A'-'F')
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 0x50-0x5F
	-1,10,11,12,13,14,15,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 0x60-0x6F ('a'-'f')
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 0x70-0x7F
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 0x80-0x8F
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 0x90-0x9F
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 0xA0-0xAF
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 0xB0-0xBF
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 0xC0-0xCF
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 0xD0-0xDF
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 0xE0-0xEF
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1  // 0xF0-0xFF
};

int HexUtilities::FromHex(string_view hex) {
	int value = 0;
	for (char c : hex) {
		int8_t nibble = _hexNibbleLut[static_cast<uint8_t>(c)];
		if (nibble >= 0) {
			value = (value << 4) | nibble;
		}
	}
	return value;
}

string HexUtilities::ToHex(uint8_t value) {
	return string(_hexCharCache[value], 2);
}

const char* HexUtilities::ToHexChar(uint8_t value) {
	return _hexCharCache[value];
}

string HexUtilities::ToHex(uint16_t value) {
	// Direct buffer construction avoids intermediate string temporaries
	const char* hi = _hexCharCache[value >> 8];
	const char* lo = _hexCharCache[value & 0xFF];
	char buf[4] = { hi[0], hi[1], lo[0], lo[1] };
	return string(buf, 4);
}

string HexUtilities::ToHex(int32_t value, bool fullSize) {
	return HexUtilities::ToHex((uint32_t)value, fullSize);
}

string HexUtilities::ToHex20(uint32_t value) {
	const char* hi = _hexCharCache[(value >> 12) & 0xFF];
	const char* mid = _hexCharCache[(value >> 4) & 0xFF];
	char buf[5] = { hi[0], hi[1], mid[0], mid[1], _hexSingleChar[value & 0xF][0] };
	return string(buf, 5);
}

string HexUtilities::ToHex24(int32_t value) {
	const char* b2 = _hexCharCache[(value >> 16) & 0xFF];
	const char* b1 = _hexCharCache[(value >> 8) & 0xFF];
	const char* b0 = _hexCharCache[value & 0xFF];
	char buf[6] = { b2[0], b2[1], b1[0], b1[1], b0[0], b0[1] };
	return string(buf, 6);
}

string HexUtilities::ToHex32(uint32_t value) {
	const char* b3 = _hexCharCache[value >> 24];
	const char* b2 = _hexCharCache[(value >> 16) & 0xFF];
	const char* b1 = _hexCharCache[(value >> 8) & 0xFF];
	const char* b0 = _hexCharCache[value & 0xFF];
	char buf[8] = { b3[0], b3[1], b2[0], b2[1], b1[0], b1[1], b0[0], b0[1] };
	return string(buf, 8);
}

string HexUtilities::ToHex(uint32_t value, bool fullSize) {
	if (fullSize || value > 0xFFFFFF) {
		return ToHex32(value);
	} else if (value <= 0xFF) {
		return ToHex((uint8_t)value);
	} else if (value <= 0xFFFF) {
		return ToHex((uint16_t)value);
	} else {
		return ToHex24((int32_t)value);
	}
}

string HexUtilities::ToHex(uint64_t value) {
	char buf[16];
	for (int i = 0; i < 8; i++) {
		const char* pair = _hexCharCache[(value >> (56 - i * 8)) & 0xFF];
		buf[i * 2] = pair[0];
		buf[i * 2 + 1] = pair[1];
	}
	return string(buf, 16);
}

string HexUtilities::ToHex(vector<uint8_t>& data, char delimiter) {
	string result;
	result.reserve(data.size() * 2);
	for (uint8_t value : data) {
		result += HexUtilities::ToHex(value);
		if (delimiter) {
			result += delimiter;
		}
	}
	return result;
}
