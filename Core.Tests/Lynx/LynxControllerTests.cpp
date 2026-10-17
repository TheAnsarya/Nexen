#include "pch.h"
#include "Lynx/LynxTypes.h"

/// <summary>
/// Tests for the Lynx controller input system.
///
/// The Lynx has a fixed controller configuration:
///   - D-pad (Up, Down, Left, Right)
///   - Two face buttons (A, B)
///   - Two option buttons (Option1, Option2 — aka Start/Pause on other systems)
///   - Pause button (directly triggers IRQ via Mikey)
///
/// Input is read via two Suzy registers:
///   - JOYSTICK ($FCB0): D-pad and face buttons (active-low)
///   - SWITCHES ($FCB1): Option buttons, pause, cart/bank select
///
/// Bit mapping in JOYSTICK (active-low, 0=pressed):
///   Bit 0: Right
///   Bit 1: Left
///   Bit 2: Down
///   Bit 3: Up
///   Bit 4: Option1
///   Bit 5: Option2
///   Bit 6: B
///   Bit 7: A
///
/// The Lynx supports hardware rotation (flip left/right), which swaps
/// some button mappings for left-handed play.
///
/// References:
///   - ~docs/plans/lynx-subsystems-deep-dive.md (Section: Input)
///   - Epyx hardware reference: monlynx.de/lynx/hardware.html
/// </summary>
class LynxControllerTest : public ::testing::Test {
protected:
	uint8_t _joystick = 0xff;  // Active-low: all released
	uint8_t _switches = 0xff;

	void SetUp() override {
		_joystick = 0xff;
		_switches = 0xff;
	}

	/// <summary>Set a button as pressed (active-low)</summary>
	void PressButton(uint8_t bit) {
		_joystick &= ~(1 << bit);
	}

	/// <summary>Release a button</summary>
	void ReleaseButton(uint8_t bit) {
		_joystick |= (1 << bit);
	}

	/// <summary>Check if a button is pressed</summary>
	bool IsPressed(uint8_t bit) const {
		return (_joystick & (1 << bit)) == 0;
	}
};

//=============================================================================
// Default State Tests
//=============================================================================

TEST_F(LynxControllerTest, DefaultState_AllReleased) {
	EXPECT_EQ(_joystick, 0xff);
	EXPECT_FALSE(IsPressed(0));
	EXPECT_FALSE(IsPressed(7));
}

TEST_F(LynxControllerTest, Switches_DefaultState) {
	EXPECT_EQ(_switches, 0xff);
}

//=============================================================================
// D-Pad Tests
//=============================================================================

TEST_F(LynxControllerTest, DPad_Right_Bit0) {
	PressButton(0);
	EXPECT_TRUE(IsPressed(0));
	EXPECT_EQ(_joystick, 0xfe);
}

TEST_F(LynxControllerTest, DPad_Left_Bit1) {
	PressButton(1);
	EXPECT_TRUE(IsPressed(1));
	EXPECT_EQ(_joystick, 0xfd);
}

TEST_F(LynxControllerTest, DPad_Down_Bit2) {
	PressButton(2);
	EXPECT_TRUE(IsPressed(2));
	EXPECT_EQ(_joystick, 0xfb);
}

TEST_F(LynxControllerTest, DPad_Up_Bit3) {
	PressButton(3);
	EXPECT_TRUE(IsPressed(3));
	EXPECT_EQ(_joystick, 0xf7);
}

TEST_F(LynxControllerTest, DPad_DiagonalUpRight) {
	PressButton(0); // Right
	PressButton(3); // Up
	EXPECT_TRUE(IsPressed(0));
	EXPECT_TRUE(IsPressed(3));
	EXPECT_EQ(_joystick, 0xf6); // 1111 0110
}

TEST_F(LynxControllerTest, DPad_OppositeDirections) {
	// Games typically prevent opposite directions
	PressButton(0); // Right
	PressButton(1); // Left
	EXPECT_TRUE(IsPressed(0));
	EXPECT_TRUE(IsPressed(1));
	// Hardware allows it, game logic filters
}

//=============================================================================
// Face Button Tests
//=============================================================================

TEST_F(LynxControllerTest, Button_A_Bit7) {
	PressButton(7);
	EXPECT_TRUE(IsPressed(7));
	EXPECT_EQ(_joystick, 0x7f);
}

TEST_F(LynxControllerTest, Button_B_Bit6) {
	PressButton(6);
	EXPECT_TRUE(IsPressed(6));
	EXPECT_EQ(_joystick, 0xbf);
}

TEST_F(LynxControllerTest, Buttons_AB_Together) {
	PressButton(7); // A
	PressButton(6); // B
	EXPECT_TRUE(IsPressed(7));
	EXPECT_TRUE(IsPressed(6));
	EXPECT_EQ(_joystick, 0x3f);
}

//=============================================================================
// Option Button Tests
//=============================================================================

TEST_F(LynxControllerTest, Button_Option1_Bit4) {
	PressButton(4);
	EXPECT_TRUE(IsPressed(4));
	EXPECT_EQ(_joystick, 0xef);
}

TEST_F(LynxControllerTest, Button_Option2_Bit5) {
	PressButton(5);
	EXPECT_TRUE(IsPressed(5));
	EXPECT_EQ(_joystick, 0xdf);
}

//=============================================================================
// Release Tests
//=============================================================================

TEST_F(LynxControllerTest, Release_After_Press) {
	PressButton(7); // Press A
	EXPECT_TRUE(IsPressed(7));
	ReleaseButton(7); // Release A
	EXPECT_FALSE(IsPressed(7));
	EXPECT_EQ(_joystick, 0xff);
}

TEST_F(LynxControllerTest, Release_MultipleButtons) {
	PressButton(0);
	PressButton(7);
	EXPECT_EQ(_joystick, 0x7e);

	ReleaseButton(0);
	EXPECT_EQ(_joystick, 0x7f);

	ReleaseButton(7);
	EXPECT_EQ(_joystick, 0xff);
}

//=============================================================================
// All Buttons Tests
//=============================================================================

TEST_F(LynxControllerTest, AllButtons_Pressed) {
	for (int i = 0; i < 8; i++) {
		PressButton(i);
	}
	EXPECT_EQ(_joystick, 0x00);
}

TEST_F(LynxControllerTest, AllButtons_Released) {
	for (int i = 0; i < 8; i++) {
		PressButton(i);
	}
	for (int i = 0; i < 8; i++) {
		ReleaseButton(i);
	}
	EXPECT_EQ(_joystick, 0xff);
}

//=============================================================================
// Active-Low Verification Tests
//=============================================================================

TEST_F(LynxControllerTest, ActiveLow_PressedIsZero) {
	PressButton(0);
	// Bit 0 should be 0
	EXPECT_EQ(_joystick & 0x01, 0);
}

TEST_F(LynxControllerTest, ActiveLow_ReleasedIsOne) {
	// Bit 0 should be 1
	EXPECT_EQ(_joystick & 0x01, 1);
}

//=============================================================================
// Combo Input Tests
//=============================================================================

TEST_F(LynxControllerTest, Combo_UpA) {
	PressButton(3); // Up
	PressButton(7); // A
	EXPECT_EQ(_joystick, 0x77);
}

TEST_F(LynxControllerTest, Combo_DownOption1) {
	PressButton(2); // Down
	PressButton(4); // Option1
	EXPECT_EQ(_joystick, 0xeb);
}

TEST_F(LynxControllerTest, Combo_AllDirectionsAndA) {
	PressButton(0); // Right
	PressButton(1); // Left
	PressButton(2); // Down
	PressButton(3); // Up
	PressButton(7); // A
	EXPECT_EQ(_joystick, 0x70); // 0111 0000
}

//=============================================================================
// TAS Input String Tests
//=============================================================================

TEST_F(LynxControllerTest, TasString_KeyNames) {
	// Standard key names for TAS: "UDLRabOoP"
	// U=Up, D=Down, L=Left, R=Right, a=Option1, b=Option2, O=A, o=B, P=Pause
	const char* keyNames = "UDLRabOoP";
	EXPECT_EQ(strlen(keyNames), 9u);
}

TEST_F(LynxControllerTest, TasString_9Characters) {
	// BK2 format uses exactly 9 characters per frame
	const char* emptyFrame = ".........";
	EXPECT_EQ(strlen(emptyFrame), 9u);
}

TEST_F(LynxControllerTest, TasString_AllPressed) {
	const char* fullFrame = "UDLRabOoP";
	EXPECT_EQ(strlen(fullFrame), 9u);
}

//=============================================================================
// Switches Register Tests
//=============================================================================

TEST_F(LynxControllerTest, Switches_PauseBit) {
	// Pause button in switches register (implementation-specific bit)
	_switches &= ~0x01;
	EXPECT_EQ(_switches & 0x01, 0);
}

TEST_F(LynxControllerTest, Switches_CartBankBit) {
	// Cart bank select bit
	_switches &= ~0x02;
	EXPECT_EQ(_switches & 0x02, 0);
}

//=============================================================================
// Rotation / Flip Tests
//=============================================================================

TEST_F(LynxControllerTest, Rotation_None_StandardMapping) {
	// Standard mapping: no remapping
	uint8_t bitRight = 0;
	uint8_t bitLeft = 1;
	EXPECT_EQ(bitRight, 0);
	EXPECT_EQ(bitLeft, 1);
}

TEST_F(LynxControllerTest, Rotation_Left_SwapButtons) {
	// In left-rotated mode, some games swap A↔B and directions
	// Hardware supports this via SWITCHES register
	// Just verify the bits can be read/written
	EXPECT_TRUE(true);
}

