#include "pch.h"
#include <memory>
#include <string>
#include <string_view>
#include <deque>
#include "Shared/BaseControlDevice.h"
#include "Shared/ControllerHub.h"
#include "Shared/ControlDeviceState.h"
#include "Shared/RewindData.h"
#include "Shared/SettingTypes.h"
#include "Utilities/StringUtilities.h"

using std::string;
using std::string_view;
using std::unique_ptr;
using std::make_unique;
using std::shared_ptr;
using std::make_shared;
using std::deque;

// ===== Test Helpers =====

// Reusable mock button controller (same as BaseControlDeviceTextStateTests)
class MockButtonController : public BaseControlDevice {
private:
	string _keyNames;
	int _buttonCount;

public:
	MockButtonController(const string& keyNames, int buttonCount)
		: BaseControlDevice(nullptr, ControllerType::None, 0, KeyMappingSet()),
		_keyNames(keyNames), _buttonCount(buttonCount) {
	}

protected:
	string GetKeyNames() override { return _keyNames; }
	uint8_t ReadRam(uint16_t) override { return 0; }
	void WriteRam(uint16_t, uint8_t) override {}
};

// Testable ControllerHub subclass that accepts pre-built test controllers
template <int HubPortCount>
class TestableControllerHub : public ControllerHub<HubPortCount> {
public:
	TestableControllerHub() : ControllerHub<HubPortCount>(nullptr, ControllerType::None, 0, _emptyConfigs) {
	}

	void SetPort(int index, shared_ptr<BaseControlDevice> controller) {
		this->_ports[index] = std::move(controller);
	}

protected:
	uint8_t ReadRam(uint16_t) override { return 0; }

private:
	static inline ControllerConfig _emptyConfigs[HubPortCount] = {};
};

class ConfiguredHub2Port : public ControllerHub<2> {
public:
	ConfiguredHub2Port(ControllerConfig controllers[2]) : ControllerHub<2>(nullptr, ControllerType::None, 0, controllers) {
	}

	uint8_t ReadRam(uint16_t) override {
		return 0;
	}
};

// ===== ControllerHub GetTextState Consistency Tests =====
// Verify that GetTextState() and GetTextState(string&) produce identical output

class ControllerHubTextStateTest : public ::testing::Test {
protected:
	unique_ptr<TestableControllerHub<4>> _hub;

	void SetUp() override {
		_hub = make_unique<TestableControllerHub<4>>();
	}
};

TEST_F(ControllerHubTextStateTest, EmptyHub_BothVersionsMatch) {
	string fromReturn = _hub->GetTextState();
	string fromBuffer;
	_hub->GetTextState(fromBuffer);
	EXPECT_EQ(fromReturn, fromBuffer);
}

TEST_F(ControllerHubTextStateTest, SinglePort_BothVersionsMatch) {
	auto ctrl = make_shared<MockButtonController>("ABSsUDLR", 8);
	ctrl->SetTextState("A..s...R");
	_hub->SetPort(0, ctrl);

	string fromReturn = _hub->GetTextState();
	string fromBuffer;
	_hub->GetTextState(fromBuffer);
	EXPECT_EQ(fromReturn, fromBuffer);
	EXPECT_TRUE(fromReturn.find("A..s...R") != string::npos);
}

TEST_F(ControllerHubTextStateTest, AllPorts_BothVersionsMatch) {
	auto ctrl0 = make_shared<MockButtonController>("ABSsUDLR", 8);
	auto ctrl1 = make_shared<MockButtonController>("ABSsUDLR", 8);
	auto ctrl2 = make_shared<MockButtonController>("ABSsUDLR", 8);
	auto ctrl3 = make_shared<MockButtonController>("ABSsUDLR", 8);
	ctrl0->SetTextState("AB......");
	ctrl1->SetTextState("..Ss....");
	ctrl2->SetTextState("....UDLR");
	ctrl3->SetTextState("........");
	_hub->SetPort(0, ctrl0);
	_hub->SetPort(1, ctrl1);
	_hub->SetPort(2, ctrl2);
	_hub->SetPort(3, ctrl3);

	string fromReturn = _hub->GetTextState();
	string fromBuffer;
	_hub->GetTextState(fromBuffer);
	EXPECT_EQ(fromReturn, fromBuffer);
	// Should be colon-separated
	EXPECT_EQ(StringUtilities::CountSegments(fromReturn, ':'), 4u);
}

TEST_F(ControllerHubTextStateTest, MixedPorts_NullAndActive) {
	auto ctrl0 = make_shared<MockButtonController>("ABSsUDLR", 8);
	ctrl0->SetTextState("ABCD....");
	_hub->SetPort(0, ctrl0);
	// Ports 1-3 are null

	string fromReturn = _hub->GetTextState();
	string fromBuffer;
	_hub->GetTextState(fromBuffer);
	EXPECT_EQ(fromReturn, fromBuffer);
}

TEST_F(ControllerHubTextStateTest, RoundTrip_SetTextStateGetTextState) {
	auto ctrl0 = make_shared<MockButtonController>("ABSsUDLR", 8);
	auto ctrl1 = make_shared<MockButtonController>("ABSsUDLR", 8);
	_hub->SetPort(0, ctrl0);
	_hub->SetPort(1, ctrl1);

	_hub->SetTextState("AB..UDLR:..Ss....");

	string result;
	_hub->GetTextState(result);
	// Hub has 4 ports, ports 2,3 are null so produce empty segments
	EXPECT_EQ(StringUtilities::GetNthSegmentView(result, ':', 0), "AB..UDLR");
	EXPECT_EQ(StringUtilities::GetNthSegmentView(result, ':', 1), "..Ss....");
}

TEST_F(ControllerHubTextStateTest, BufferReuse_NoReallocation) {
	auto ctrl0 = make_shared<MockButtonController>("ABSsUDLR", 8);
	ctrl0->SetTextState("AB......");
	_hub->SetPort(0, ctrl0);

	string buffer;
	buffer.reserve(256);
	const char* origData = buffer.data();

	// Multiple calls should reuse the buffer capacity
	for (int i = 0; i < 10; i++) {
		_hub->GetTextState(buffer);
	}
	// Buffer should not have reallocated (data pointer unchanged)
	EXPECT_EQ(origData, buffer.data());
}

TEST(ControllerHubCreationTests, GenesisControllerSubPortIsDetectedByType) {
	ControllerConfig controllers[2] = {};
	controllers[0].Type = ControllerType::GenesisController;
	controllers[1].Type = ControllerType::None;

	ConfiguredHub2Port hub(controllers);
	EXPECT_TRUE(hub.HasControllerType(ControllerType::GenesisController));
}

// ===== RewindData InputLog Tests =====

class RewindDataTest : public ::testing::Test {};

TEST_F(RewindDataTest, DefaultValues) {
	RewindData rd;
	EXPECT_EQ(rd.FrameCount, 0);
	EXPECT_FALSE(rd.EndOfSegment);
	EXPECT_FALSE(rd.IsFullState);
	EXPECT_EQ(rd.GetStateSize(), 0u);
}

TEST_F(RewindDataTest, InputLogs_EmptyByDefault) {
	RewindData rd;
	for (int i = 0; i < BaseControlDevice::PortCount; i++) {
		EXPECT_TRUE(rd.InputLogs[i].empty());
	}
}

TEST_F(RewindDataTest, InputLogs_PushBack) {
	RewindData rd;
	ControlDeviceState state;
	state.State = {0x01, 0x02};
	rd.InputLogs[0].push_back(state);

	EXPECT_EQ(rd.InputLogs[0].size(), 1u);
	EXPECT_EQ(rd.InputLogs[0][0].State[0], 0x01);
	EXPECT_EQ(rd.InputLogs[0][0].State[1], 0x02);
}

TEST_F(RewindDataTest, InputLogs_MultipleFrames) {
	RewindData rd;
	for (int frame = 0; frame < 30; frame++) {
		ControlDeviceState state;
		state.State = {(uint8_t)frame};
		rd.InputLogs[0].push_back(state);
	}
	EXPECT_EQ(rd.InputLogs[0].size(), 30u);
	EXPECT_EQ(rd.InputLogs[0][0].State[0], 0x00);
	EXPECT_EQ(rd.InputLogs[0][29].State[0], 0x1d);
}

TEST_F(RewindDataTest, InputLogs_MultiplePorts) {
	RewindData rd;
	for (int port = 0; port < 2; port++) {
		ControlDeviceState state;
		state.State = {(uint8_t)(port * 10)};
		rd.InputLogs[port].push_back(state);
	}
	EXPECT_EQ(rd.InputLogs[0].size(), 1u);
	EXPECT_EQ(rd.InputLogs[1].size(), 1u);
	EXPECT_EQ(rd.InputLogs[0][0].State[0], 0x00);
	EXPECT_EQ(rd.InputLogs[1][0].State[0], 0x0a);
}

TEST_F(RewindDataTest, SegmentMarkers) {
	RewindData rd;
	rd.FrameCount = 30;
	rd.EndOfSegment = true;
	rd.IsFullState = true;

	EXPECT_EQ(rd.FrameCount, 30);
	EXPECT_TRUE(rd.EndOfSegment);
	EXPECT_TRUE(rd.IsFullState);
}

// ===== Movie Input Format Parsing Tests =====
// Test the string segmentation used by NexenMovie::SetInput

class MovieInputFormatTest : public ::testing::Test {};

TEST_F(MovieInputFormatTest, SingleDevice_ParseSegment) {
	// NES single controller: |AB..UDLR
	string_view frameLine = "AB..UDLR";
	EXPECT_EQ(StringUtilities::CountSegments(frameLine, '|'), 1u);
	EXPECT_EQ(StringUtilities::GetNthSegmentView(frameLine, '|', 0), "AB..UDLR");
}

TEST_F(MovieInputFormatTest, TwoDevices_ParseSegments) {
	// Two NES controllers: AB..UDLR|..Ss....
	string_view frameLine = "AB..UDLR|..Ss....";
	EXPECT_EQ(StringUtilities::CountSegments(frameLine, '|'), 2u);
	EXPECT_EQ(StringUtilities::GetNthSegmentView(frameLine, '|', 0), "AB..UDLR");
	EXPECT_EQ(StringUtilities::GetNthSegmentView(frameLine, '|', 1), "..Ss....");
}

TEST_F(MovieInputFormatTest, FourDevices_Multitap) {
	// Four controllers via multitap
	string_view frameLine = "A.......|B.......|.B......|........";
	EXPECT_EQ(StringUtilities::CountSegments(frameLine, '|'), 4u);
	EXPECT_EQ(StringUtilities::GetNthSegmentView(frameLine, '|', 0), "A.......");
	EXPECT_EQ(StringUtilities::GetNthSegmentView(frameLine, '|', 3), "........");
}

TEST_F(MovieInputFormatTest, EmptyDevices) {
	// Two devices, both idle
	string_view frameLine = "........|........";
	EXPECT_EQ(StringUtilities::CountSegments(frameLine, '|'), 2u);
	EXPECT_EQ(StringUtilities::GetNthSegmentView(frameLine, '|', 0), "........");
}

TEST_F(MovieInputFormatTest, OutOfBoundsIndex_ReturnsEmpty) {
	string_view frameLine = "AB..UDLR|..Ss....";
	EXPECT_EQ(StringUtilities::GetNthSegmentView(frameLine, '|', 5), "");
}

TEST_F(MovieInputFormatTest, SNESController_12Buttons) {
	// SNES has 12 buttons: ABXYLRSsUDLR
	string_view frameLine = "ABXYLRSsUDLR|............";
	EXPECT_EQ(StringUtilities::CountSegments(frameLine, '|'), 2u);
	EXPECT_EQ(StringUtilities::GetNthSegmentView(frameLine, '|', 0), "ABXYLRSsUDLR");
	EXPECT_EQ(StringUtilities::GetNthSegmentView(frameLine, '|', 1), "............");
}

// ===== Movie Text State Round-Trip Tests =====
// Verify SetTextState/GetTextState consistency for controller state

class MovieTextStateRoundTripTest : public ::testing::Test {
protected:
	unique_ptr<MockButtonController> _nes;
	unique_ptr<MockButtonController> _snes;

	void SetUp() override {
		_nes = make_unique<MockButtonController>("ABSsUDLR", 8);
		_snes = make_unique<MockButtonController>("ABXYLRSsUDLR", 12);
	}
};

TEST_F(MovieTextStateRoundTripTest, NES_AllButtonsPressed) {
	_nes->SetTextState("ABSsUDLR");
	string result;
	_nes->GetTextState(result);
	EXPECT_EQ(result, "ABSsUDLR");
}

TEST_F(MovieTextStateRoundTripTest, NES_NoButtonsPressed) {
	_nes->SetTextState("........");
	string result;
	_nes->GetTextState(result);
	EXPECT_EQ(result, "........");
}

TEST_F(MovieTextStateRoundTripTest, NES_BufferOverload_MatchesStringReturn) {
	_nes->SetTextState("A..s..L.");
	string fromReturn = _nes->GetTextState();
	string fromBuffer;
	_nes->GetTextState(fromBuffer);
	EXPECT_EQ(fromReturn, fromBuffer);
}

TEST_F(MovieTextStateRoundTripTest, SNES_AllButtonsPressed) {
	_snes->SetTextState("ABXYLRSsUDLR");
	string result;
	_snes->GetTextState(result);
	EXPECT_EQ(result, "ABXYLRSsUDLR");
}

TEST_F(MovieTextStateRoundTripTest, SNES_BufferOverload_MatchesStringReturn) {
	_snes->SetTextState("AB..L..s.D.R");
	string fromReturn = _snes->GetTextState();
	string fromBuffer;
	_snes->GetTextState(fromBuffer);
	EXPECT_EQ(fromReturn, fromBuffer);
}

TEST_F(MovieTextStateRoundTripTest, MultipleFrames_BufferReuse) {
	string buffer;
	buffer.reserve(64);

	// Simulate 100 frames of recording
	const char* states[] = {"AB......", "..Ss....", "....UDLR", "........", "ABSsUDLR"};
	for (int frame = 0; frame < 100; frame++) {
		_nes->SetTextState(states[frame % 5]);
		_nes->GetTextState(buffer);
		string expected = states[frame % 5];
		EXPECT_EQ(buffer, expected) << "Frame " << frame;
	}
}
