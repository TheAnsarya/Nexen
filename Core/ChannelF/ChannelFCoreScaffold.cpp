#include "pch.h"
#include "ChannelF/ChannelFCoreScaffold.h"

ChannelFCoreScaffold::ChannelFCoreScaffold(ChannelFBiosVariant variant)
	: _variant(variant) {
}

void ChannelFCoreScaffold::Reset() {
	_masterClock = 0;
	_frameCount = 0;
	_panelButtons = 0;
	_rightController = 0;
	_leftController = 0;
	_deterministicState = 0x6d2b79f5;
}

void ChannelFCoreScaffold::RunFrame() {
	_frameCount++;
	_masterClock += CyclesPerFrame;

	// Deterministic state update used by smoke tests and microbenchmarks.
	uint32_t mix = (uint32_t)_frameCount;
	mix ^= ((uint32_t)_panelButtons << 8);
	mix ^= ((uint32_t)_rightController << 16);
	mix ^= ((uint32_t)_leftController << 24);

	_deterministicState ^= mix;
	_deterministicState ^= (_deterministicState << 13);
	_deterministicState ^= (_deterministicState >> 17);
	_deterministicState ^= (_deterministicState << 5);
}

void ChannelFCoreScaffold::SetPanelButtons(uint8_t panelButtons) {
	_panelButtons = panelButtons;
}

void ChannelFCoreScaffold::SetRightController(uint8_t value) {
	_rightController = value;
}

void ChannelFCoreScaffold::SetLeftController(uint8_t value) {
	_leftController = value;
}

void ChannelFCoreScaffold::DetectVariantFromHashes(const string& biosSha1, const string& biosMd5) {
	_variant = ChannelFBiosDatabase::DetectVariant(biosSha1, biosMd5);
}

void ChannelFCoreScaffold::Serialize(Serializer& s) {
	s.Stream(_variant, "variant");
	s.Stream(_masterClock, "masterClock");
	s.Stream(_frameCount, "frameCount");
	s.Stream(_panelButtons, "panelButtons");
	s.Stream(_rightController, "rightController");
	s.Stream(_leftController, "leftController");
	s.Stream(_deterministicState, "deterministicState");
}
