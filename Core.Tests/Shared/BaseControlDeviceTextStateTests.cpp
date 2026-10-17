#include "pch.h"
#include <memory>
#include <string>
#include <string_view>
#include "Shared/BaseControlDevice.h"

using std::string;
using std::string_view;
using std::unique_ptr;
using std::make_unique;

/// <summary>
/// Tests for BaseControlDevice text state serialization round-trip.
///
/// SetTextState/GetTextState are the foundational functions for TAS movie
/// determinism. Every frame of recording and playback depends on their
/// correctness. This suite verifies:
///   - Button state round-trip for standard controllers
///   - Raw string round-trip (tape/barcode devices)
///   - Empty and fully-pressed edge cases
///   - Colon handling (multitap separator passthrough)
///   - Coordinate device GetTextState output format
/// </summary>

// Concrete test subclass: standard button controller (no coordinates, not raw)
class TestButtonController : public BaseControlDevice {
private:
	string _keyNames;
	int _buttonCount;

public:
	TestButtonController(const string& keyNames, int buttonCount)
		: BaseControlDevice(nullptr, ControllerType::None, 0, KeyMappingSet()),
		_keyNames(keyNames), _buttonCount(buttonCount) {
	}

protected:
	string GetKeyNames() override { return _keyNames; }
	uint8_t ReadRam(uint16_t) override { return 0; }
	void WriteRam(uint16_t, uint8_t) override {}
};

// Concrete test subclass: raw string device (tape, barcode)
class TestRawStringDevice : public BaseControlDevice {
public:
	TestRawStringDevice()
		: BaseControlDevice(nullptr, ControllerType::None, 0, KeyMappingSet()) {
	}

protected:
	bool IsRawString() override { return true; }
	string GetKeyNames() override { return ""; }
	uint8_t ReadRam(uint16_t) override { return 0; }
	void WriteRam(uint16_t, uint8_t) override {}
};

// Concrete test subclass: coordinate device
// We can't call SetTextState for coordinates (it uses SetCoordinates which
// requires _emu), but we can test GetTextState output by directly writing
// coordinate bytes + button state.
class TestCoordinateDevice : public BaseControlDevice {
private:
	string _keyNames;

public:
	TestCoordinateDevice(const string& keyNames)
		: BaseControlDevice(nullptr, ControllerType::None, 0, KeyMappingSet()),
		_keyNames(keyNames) {
	}

	// Directly set coordinate bytes in state (bypasses _emu check)
	void SetCoordinatesDirect(int16_t x, int16_t y) {
		auto lock = _stateLock.AcquireSafe();
		EnsureCapacity(-1);
		_state.State[0] = x & 0xff;
		_state.State[1] = (x >> 8) & 0xff;
		_state.State[2] = y & 0xff;
		_state.State[3] = (y >> 8) & 0xff;
	}

protected:
	bool HasCoordinates() override { return true; }
	string GetKeyNames() override { return _keyNames; }
	uint8_t ReadRam(uint16_t) override { return 0; }
	void WriteRam(uint16_t, uint8_t) override {}
};

//=============================================================================
// Standard Button Controller Tests
//=============================================================================

class ButtonControllerTextStateTest : public ::testing::Test {
protected:
	// NES-style: 8 buttons ABSSUDLR
	unique_ptr<TestButtonController> _nes;
	// SNES-style: 12 buttons ABXYLRSsUDLR
	unique_ptr<TestButtonController> _snes;

	void SetUp() override {
		_nes = make_unique<TestButtonController>("ABSsUDLR", 8);
		_snes = make_unique<TestButtonController>("BYSsUDLRAxXy", 12);
	}
};

TEST_F(ButtonControllerTextStateTest, AllDotsWhenNothingPressed) {
	string state = _nes->GetTextState();
	EXPECT_EQ(state, "........");
}

TEST_F(ButtonControllerTextStateTest, AllDotsWhenNothingPressed_SNES) {
	string state = _snes->GetTextState();
	EXPECT_EQ(state, "............");
}

TEST_F(ButtonControllerTextStateTest, SetTextState_AllPressed) {
	_nes->SetTextState("ABSsUDLR");
	string result = _nes->GetTextState();
	EXPECT_EQ(result, "ABSsUDLR");
}

TEST_F(ButtonControllerTextStateTest, SetTextState_AllPressed_SNES) {
	_snes->SetTextState("BYSsUDLRAxXy");
	string result = _snes->GetTextState();
	EXPECT_EQ(result, "BYSsUDLRAxXy");
}

TEST_F(ButtonControllerTextStateTest, SetTextState_NonePressed) {
	_nes->SetTextState("........");
	string result = _nes->GetTextState();
	EXPECT_EQ(result, "........");
}

TEST_F(ButtonControllerTextStateTest, RoundTrip_SomePressedNES) {
	string input = "A.S.U...";
	_nes->SetTextState(input);
	string output = _nes->GetTextState();
	EXPECT_EQ(output, "A.S.U...");
}

TEST_F(ButtonControllerTextStateTest, RoundTrip_SomePressedSNES) {
	string input = "BY..UD..A.X.";
	_snes->SetTextState(input);
	string output = _snes->GetTextState();
	EXPECT_EQ(output, "BY..UD..A.X.");
}

TEST_F(ButtonControllerTextStateTest, RoundTrip_SingleButton) {
	// Only first button pressed
	string input = "A.......";
	_nes->SetTextState(input);
	EXPECT_EQ(_nes->GetTextState(), "A.......");
}

TEST_F(ButtonControllerTextStateTest, RoundTrip_LastButton) {
	// Only last button pressed
	string input = ".......R";
	_nes->SetTextState(input);
	EXPECT_EQ(_nes->GetTextState(), ".......R");
}

TEST_F(ButtonControllerTextStateTest, EmptyString_AllUnpressed) {
	_nes->SetTextState("");
	string result = _nes->GetTextState();
	EXPECT_EQ(result, "........");
}

TEST_F(ButtonControllerTextStateTest, MultipleSetTextState_OverwritesPrevious) {
	_nes->SetTextState("ABSsUDLR");
	_nes->SetTextState("........");
	EXPECT_EQ(_nes->GetTextState(), "........");
}

TEST_F(ButtonControllerTextStateTest, MultipleSetTextState_RestoresState) {
	_nes->SetTextState("........");
	_nes->SetTextState("ABSsUDLR");
	EXPECT_EQ(_nes->GetTextState(), "ABSsUDLR");
}

TEST_F(ButtonControllerTextStateTest, ColonsIgnoredDuringSetting) {
	// Colons are used by ControllerHub as separators and are skipped
	// during SetTextState parsing
	_nes->SetTextState("A.:S.U.L.");
	// The colon is skipped, so effective chars are: A . S . U . L .
	EXPECT_EQ(_nes->GetTextState(), "A.S.U.L.");
}

TEST_F(ButtonControllerTextStateTest, SetBit_And_GetTextState) {
	_nes->SetBit(0); // A
	_nes->SetBit(4); // U
	EXPECT_EQ(_nes->GetTextState(), "A...U...");
}

TEST_F(ButtonControllerTextStateTest, ClearState_AllUnpressed) {
	_nes->SetTextState("ABSsUDLR");
	_nes->ClearState();
	EXPECT_EQ(_nes->GetTextState(), "........");
}

//=============================================================================
// Raw String Device Tests
//=============================================================================

class RawStringDeviceTextStateTest : public ::testing::Test {
protected:
	unique_ptr<TestRawStringDevice> _device;

	void SetUp() override {
		_device = make_unique<TestRawStringDevice>();
	}
};

TEST_F(RawStringDeviceTextStateTest, EmptyByDefault) {
	string state = _device->GetTextState();
	EXPECT_TRUE(state.empty());
}

TEST_F(RawStringDeviceTextStateTest, RoundTrip_SimpleString) {
	string input = "Hello World";
	_device->SetTextState(input);
	EXPECT_EQ(_device->GetTextState(), input);
}

TEST_F(RawStringDeviceTextStateTest, RoundTrip_BinaryData) {
	// Raw string stores arbitrary bytes
	string input = "ABC\x01\x02\x03XYZ";
	_device->SetTextState(input);
	EXPECT_EQ(_device->GetTextState(), input);
}

TEST_F(RawStringDeviceTextStateTest, RoundTrip_EmptyString) {
	_device->SetTextState("");
	EXPECT_TRUE(_device->GetTextState().empty());
}

TEST_F(RawStringDeviceTextStateTest, OverwritesPrevious) {
	_device->SetTextState("First");
	_device->SetTextState("Second");
	EXPECT_EQ(_device->GetTextState(), "Second");
}

TEST_F(RawStringDeviceTextStateTest, RoundTrip_LongString) {
	string input(256, 'X');
	_device->SetTextState(input);
	EXPECT_EQ(_device->GetTextState(), input);
}

//=============================================================================
// Coordinate Device Tests (GetTextState only — SetTextState needs _emu)
//=============================================================================

class CoordinateDeviceTextStateTest : public ::testing::Test {
protected:
	unique_ptr<TestCoordinateDevice> _device;

	void SetUp() override {
		_device = make_unique<TestCoordinateDevice>("TB");
	}
};

TEST_F(CoordinateDeviceTextStateTest, GetTextState_NoButtonsPressed) {
	_device->SetCoordinatesDirect(100, 200);
	string state = _device->GetTextState();
	EXPECT_EQ(state, "100 200 ..");
}

TEST_F(CoordinateDeviceTextStateTest, GetTextState_ButtonPressed) {
	_device->SetCoordinatesDirect(50, 75);
	_device->SetBit(0); // T
	string state = _device->GetTextState();
	EXPECT_EQ(state, "50 75 T.");
}

TEST_F(CoordinateDeviceTextStateTest, GetTextState_AllButtonsPressed) {
	_device->SetCoordinatesDirect(0, 0);
	_device->SetBit(0); // T
	_device->SetBit(1); // B
	string state = _device->GetTextState();
	EXPECT_EQ(state, "0 0 TB");
}

TEST_F(CoordinateDeviceTextStateTest, GetTextState_NegativeCoordinates) {
	_device->SetCoordinatesDirect(-1, -1);
	string state = _device->GetTextState();
	EXPECT_EQ(state, "-1 -1 ..");
}

TEST_F(CoordinateDeviceTextStateTest, GetTextState_LargeCoordinates) {
	_device->SetCoordinatesDirect(32767, -32768);
	string state = _device->GetTextState();
	EXPECT_EQ(state, "32767 -32768 ..");
}

//=============================================================================
// SetRawState / GetRawState round-trip
//=============================================================================

class RawStateRoundTripTest : public ::testing::Test {
protected:
	unique_ptr<TestButtonController> _device;

	void SetUp() override {
		_device = make_unique<TestButtonController>("ABCD", 4);
	}
};

TEST_F(RawStateRoundTripTest, RoundTrip_ViaSetRawState) {
	_device->SetTextState("AB.D");
	ControlDeviceState saved = _device->GetRawState();

	_device->ClearState();
	EXPECT_EQ(_device->GetTextState(), "....");

	_device->SetRawState(saved);
	EXPECT_EQ(_device->GetTextState(), "AB.D");
}

TEST_F(RawStateRoundTripTest, GetRawState_EmptyController) {
	ControlDeviceState state = _device->GetRawState();
	// Fresh controller should have some state (capacity ensured by GetTextState)
	_device->SetRawState(state);
	EXPECT_EQ(_device->GetTextState(), "....");
}

//=============================================================================
// string_view compatibility (SetTextState accepts string_view)
//=============================================================================

TEST(StringViewTextState, AcceptsStringView) {
	TestButtonController device("ABCD", 4);
	string_view sv = "A.C.";
	device.SetTextState(sv);
	EXPECT_EQ(device.GetTextState(), "A.C.");
}

TEST(StringViewTextState, AcceptsSubstring) {
	TestButtonController device("ABCD", 4);
	string full = "A.C.EXTRA";
	string_view sv = string_view(full).substr(0, 4);
	device.SetTextState(sv);
	EXPECT_EQ(device.GetTextState(), "A.C.");
}

TEST(StringViewTextState, AcceptsTemporaryString) {
	TestButtonController device("ABCD", 4);
	device.SetTextState(string("ABCD"));
	EXPECT_EQ(device.GetTextState(), "ABCD");
}
