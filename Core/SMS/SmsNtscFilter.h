#pragma once
#include "pch.h"
#include <memory>
#include "Shared/Video/BaseVideoFilter.h"
#include "Utilities/NTSC/sms_ntsc.h"
#include "Utilities/NTSC/snes_ntsc.h"

class Emulator;
class SmsConsole;

class SmsNtscFilter : public BaseVideoFilter {
private:
	unique_ptr<sms_ntsc_setup_t> _ntscSetup;
	unique_ptr<sms_ntsc_t> _ntscData;

	unique_ptr<snes_ntsc_setup_t> _snesNtscSetup;
	unique_ptr<snes_ntsc_t> _snesNtscData;

	std::unique_ptr<uint32_t[]> _ntscBuffer;
	SmsConsole* _console = nullptr;

protected:
	void OnBeforeApplyFilter() override;
	FrameInfo GetFrameInfo() override;

public:
	SmsNtscFilter(Emulator* emu, SmsConsole* console);
	virtual ~SmsNtscFilter();

	OverscanDimensions GetOverscan() override;
	HudScaleFactors GetScaleFactor() override;

	void ApplyFilter(uint16_t* ppuOutputBuffer) override;
};