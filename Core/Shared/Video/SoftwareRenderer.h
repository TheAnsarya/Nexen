#include "pch.h"
#include <array>
#include "Shared/Interfaces/IRenderingDevice.h"
#include "Utilities/SimpleLock.h"

class Emulator;

class SoftwareRenderer : public IRenderingDevice {
private:
	Emulator* _emu = nullptr;

	SimpleLock _frameLock;
	SimpleLock _textureLock;

	uint32_t _frameWidth = 0;
	uint32_t _frameHeight = 0;

	std::array<std::unique_ptr<uint32_t[]>, 2> _textureBuffer;

	atomic<bool> _needSwap = false;

	void SetScreenSize(uint32_t width, uint32_t height);

public:
	SoftwareRenderer(Emulator* emu);
	~SoftwareRenderer();

	void UpdateFrame(RenderedFrame& frame) override;
	void ClearFrame() override;
	void Render(RenderSurfaceInfo& emuHud, RenderSurfaceInfo& scriptHud) override;
	void Reset() override;
	void SetExclusiveFullscreenMode(bool fullscreen, void* windowHandle) override;
};