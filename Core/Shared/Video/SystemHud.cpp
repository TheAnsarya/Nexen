#include "pch.h"
#include "Shared/Video/SystemHud.h"
#include "Shared/Video/DebugHud.h"
#include "Shared/Movies/MovieManager.h"
#include "Shared/MessageManager.h"
#include "Shared/BaseControlManager.h"
#include "Shared/Video/DrawStringCommand.h"
#include "Shared/Interfaces/IMessageManager.h"

SystemHud::SystemHud(Emulator* emu) {
	_emu = emu;
	MessageManager::RegisterMessageManager(this);
}

SystemHud::~SystemHud() {
	MessageManager::UnregisterMessageManager(this);
}

std::pair<int, int> SystemHud::GetBottomLeftBoxPosition(uint32_t screenWidth, uint32_t screenHeight, int boxWidth, int boxHeight, int leftMargin, int bottomMargin) {
	int x = std::max(0, leftMargin);
	if (x + boxWidth > (int)screenWidth) {
		x = std::max(0, (int)screenWidth - boxWidth);
	}

	int y = (int)screenHeight - boxHeight - std::max(0, bottomMargin);
	y = std::max(0, y);

	return { x, y };
}

void SystemHud::Draw(DebugHud* hud, uint32_t width, uint32_t height) const {
	DrawCounters(hud, width);
	DrawMessages(hud, width, height);

	// Draw lag frame visual indicator if enabled
	PreferencesConfig cfg = _emu->GetSettings()->GetPreferences();
	if(cfg.ShowLagFrameIndicator) {
		DrawLagFrameIndicator(hud, width, height);
	}

	if (_emu->IsRunning()) {
		EmuSettings* settings = _emu->GetSettings();
		bool showMovieIcons = settings->GetPreferences().ShowMovieIcons;
		int xOffset = 0;
		if (_emu->IsPaused()) {
			DrawPauseIcon(hud);
		} else if (showMovieIcons && _emu->GetMovieManager()->Playing()) {
			DrawPlayIcon(hud);
			xOffset += 12;
		} else if (showMovieIcons && _emu->GetMovieManager()->Recording()) {
			DrawRecordIcon(hud);
			xOffset += 12;
			// Show R/W indicator for TAS mode
			DrawTasReadWriteIndicator(hud, xOffset);
		}

		bool showTurboRewindIcons = settings->GetPreferences().ShowTurboRewindIcons;
		if (!_emu->IsPaused() && showTurboRewindIcons) {
			if (settings->CheckFlag(EmulationFlags::Rewind)) {
				DrawTurboRewindIcon(hud, true, xOffset);
			} else if (settings->CheckFlag(EmulationFlags::Turbo)) {
				DrawTurboRewindIcon(hud, false, xOffset);
			}
		}
	}
}

void SystemHud::DrawMessage(DebugHud* hud, MessageInfo& msg, uint32_t screenWidth, uint32_t screenHeight, int& lastHeight) const {
	int fontScale = GetFontScale(screenHeight);

	// Save state notifications use centered box display
	if (msg.GetTitle() == MessageManager::Localize("SaveStates") && msg.GetMessage().find('\n') != string::npos) {
		DrawSaveStateNotification(hud, msg, screenWidth, screenHeight);
		return;
	}

	// Action notifications (recording, screenshot, export) use centered box display
	if (IsActionNotification(msg.GetTitle())) {
		DrawActionNotification(hud, msg, screenWidth, screenHeight);
		return;
	}

	// Get opacity for fade in/out effect
	uint8_t opacity = (uint8_t)(msg.GetOpacity() * 255);
	int textLeftMargin = 4 * fontScale;

	string text = "[" + msg.GetTitle() + "] " + msg.GetMessage();

	int maxWidth = screenWidth - textLeftMargin;
	TextSize size = DrawStringCommand::MeasureString(text, maxWidth, fontScale);
	lastHeight += size.Y;
	DrawString(hud, screenWidth, text, textLeftMargin, screenHeight - lastHeight, opacity, fontScale);
}

int SystemHud::GetFontScale(uint32_t screenHeight) {
	// Scale font based on HUD buffer height relative to base console height (240px)
	// At 1x (240), scale=1. At 480+, scale=2. At 720+, scale=3. Capped at 4.
	if (screenHeight >= 960) return 4;
	if (screenHeight >= 720) return 3;
	if (screenHeight >= 480) return 2;
	return 1;
}

void SystemHud::DrawString(DebugHud* hud, uint32_t screenWidth, const string& text, int x, int y, uint8_t opacity, int fontScale) const {
	int maxWidth = screenWidth - x;
	opacity = 255 - opacity;
	int shadowOffset = std::max(1, fontScale);
	for (int i = -shadowOffset; i <= shadowOffset; i += shadowOffset) {
		for (int j = -shadowOffset; j <= shadowOffset; j += shadowOffset) {
			hud->DrawString(x + i, y + j, text, 0 | (opacity << 24), 0xFF000000, 1, -1, maxWidth, true, fontScale);
		}
	}
	hud->DrawString(x, y, text, 0xFFFFFF | (opacity << 24), 0xFF000000, 1, -1, maxWidth, true, fontScale);
}

void SystemHud::DrawColoredString(DebugHud* hud, uint32_t screenWidth, const string& text, int x, int y, uint32_t color, uint8_t opacity, int fontScale) const {
	int maxWidth = screenWidth - x;
	uint8_t fadeOp = 255 - opacity;
	int shadowOffset = std::max(1, fontScale);
	for (int i = -shadowOffset; i <= shadowOffset; i += shadowOffset) {
		for (int j = -shadowOffset; j <= shadowOffset; j += shadowOffset) {
			hud->DrawString(x + i, y + j, text, (int)(fadeOp << 24), (int)0xFF000000, 1, -1, maxWidth, true, fontScale);
		}
	}
	hud->DrawString(x, y, text, (int)((color & 0xFFFFFF) | (fadeOp << 24)), (int)0xFF000000, 1, -1, maxWidth, true, fontScale);
}

uint32_t SystemHud::GetSaveStateBadgeColor(const string& badge) {
	if (badge.find("Auto") != string::npos) return 0x5DADE2;    // Blue
	if (badge.find("Slot") != string::npos) return 0xAF7AC5;    // Purple
	if (badge.find("Recent") != string::npos) return 0xEC7063;  // Red
	if (badge.find("Lua") != string::npos) return 0xF7DC6F;     // Yellow
	if (badge.find('#') != string::npos) return 0x85C1E9;       // Light blue
	return 0x58D68D; // Green (Save)
}

void SystemHud::DrawSaveStateNotification(DebugHud* hud, MessageInfo& msg, uint32_t screenWidth, uint32_t screenHeight) const {
	uint8_t opacity = (uint8_t)(msg.GetOpacity() * 255);
	uint8_t fadeOp = 255 - opacity;
	int fontScale = GetFontScale(screenHeight);

	string message = msg.GetMessage();

	// Parse "badge\ndate\ntime"
	size_t firstNl = message.find('\n');
	size_t secondNl = (firstNl != string::npos) ? message.find('\n', firstNl + 1) : string::npos;

	string badge, dateStr, timeStr;
	if (firstNl != string::npos && secondNl != string::npos) {
		badge = message.substr(0, firstNl);
		dateStr = message.substr(firstNl + 1, secondNl - firstNl - 1);
		timeStr = message.substr(secondNl + 1);
	} else if (firstNl != string::npos) {
		badge = message.substr(0, firstNl);
		dateStr = message.substr(firstNl + 1);
	} else {
		badge = message;
	}

	// Measure text widths (with font scale)
	TextSize badgeSize = DrawStringCommand::MeasureString(badge, 0, fontScale);
	TextSize dateSize = !dateStr.empty() ? DrawStringCommand::MeasureString(dateStr, 0, fontScale) : TextSize{0, 0};
	TextSize timeSize = !timeStr.empty() ? DrawStringCommand::MeasureString(timeStr, 0, fontScale) : TextSize{0, 0};

	int padding = 6 * fontScale;
	int lineSpacing = 11 * fontScale;
	int maxTextWidth = (int)badgeSize.X;
	int numLines = 1;

	if (!dateStr.empty()) {
		maxTextWidth = std::max(maxTextWidth, (int)dateSize.X);
		numLines++;
	}
	if (!timeStr.empty()) {
		maxTextWidth = std::max(maxTextWidth, (int)timeSize.X);
		numLines++;
	}

	int boxWidth = maxTextWidth + padding * 2;
	int boxHeight = numLines * lineSpacing + padding * 2 - 2 * fontScale;

	int leftMargin = NotificationLeftMargin * fontScale;
	int bottomMargin = NotificationBottomMargin * fontScale;
	auto position = GetBottomLeftBoxPosition(screenWidth, screenHeight, boxWidth, boxHeight, leftMargin, bottomMargin);
	int boxX = position.first;
	int boxY = position.second;

	// Semi-transparent background box
	uint8_t boxAlpha = (uint8_t)std::min((int)0x40 + (int)fadeOp, 255);
	uint32_t bgColor = (uint32_t)boxAlpha << 24;
	hud->DrawRectangle(boxX, boxY, boxWidth, boxHeight, bgColor, true, 1);

	// Subtle border
	uint8_t borderAlpha = (uint8_t)std::min((int)0x20 + (int)fadeOp, 255);
	uint32_t borderColor = ((uint32_t)borderAlpha << 24) | 0x666666;
	hud->DrawRectangle(boxX - 1, boxY - 1, boxWidth + 2, boxHeight + 2, borderColor, false, 1);

	int yPos = boxY + padding;

	// Badge line (colored, centered)
	uint32_t badgeColor = GetSaveStateBadgeColor(badge);
	int badgeX = boxX + (boxWidth - (int)badgeSize.X) / 2;
	DrawColoredString(hud, screenWidth, badge, badgeX, yPos, badgeColor, opacity, fontScale);
	yPos += lineSpacing;

	// Date (white, centered)
	if (!dateStr.empty()) {
		int dateX = boxX + (boxWidth - (int)dateSize.X) / 2;
		DrawString(hud, screenWidth, dateStr, dateX, yPos, opacity, fontScale);
		yPos += lineSpacing;
	}

	// Time (white, centered)
	if (!timeStr.empty()) {
		int timeX = boxX + (boxWidth - (int)timeSize.X) / 2;
		DrawString(hud, screenWidth, timeStr, timeX, yPos, opacity, fontScale);
	}
}

bool SystemHud::IsActionNotification(const string& title) {
	return title == MessageManager::Localize("SoundRecorder") ||
		title == MessageManager::Localize("VideoRecorder") ||
		title == MessageManager::Localize("ScreenshotSaved") ||
		title == "Tools";
}

void SystemHud::DrawActionNotification(DebugHud* hud, MessageInfo& msg, uint32_t screenWidth, uint32_t screenHeight) const {
	uint8_t opacity = (uint8_t)(msg.GetOpacity() * 255);
	uint8_t fadeOp = 255 - opacity;
	int fontScale = GetFontScale(screenHeight);

	// Show message if non-empty, otherwise show title
	string text = msg.GetMessage().empty() ? msg.GetTitle() : msg.GetMessage();

	TextSize textSize = DrawStringCommand::MeasureString(text, 0, fontScale);

	int padding = 6 * fontScale;
	int boxWidth = (int)textSize.X + padding * 2;
	int boxHeight = 11 * fontScale + padding * 2 - 2 * fontScale;

	int leftMargin = NotificationLeftMargin * fontScale;
	int bottomMargin = NotificationBottomMargin * fontScale;
	auto position = GetBottomLeftBoxPosition(screenWidth, screenHeight, boxWidth, boxHeight, leftMargin, bottomMargin);
	int boxX = position.first;
	int boxY = position.second;

	// Semi-transparent background box
	uint8_t boxAlpha = (uint8_t)std::min((int)0x40 + (int)fadeOp, 255);
	uint32_t bgColor = (uint32_t)boxAlpha << 24;
	hud->DrawRectangle(boxX, boxY, boxWidth, boxHeight, bgColor, true, 1);

	// Subtle border
	uint8_t borderAlpha = (uint8_t)std::min((int)0x20 + (int)fadeOp, 255);
	uint32_t borderColor = ((uint32_t)borderAlpha << 24) | 0x666666;
	hud->DrawRectangle(boxX - 1, boxY - 1, boxWidth + 2, boxHeight + 2, borderColor, false, 1);

	// Centered text
	int textX = boxX + (boxWidth - (int)textSize.X) / 2;
	int textY = boxY + padding;
	DrawString(hud, screenWidth, text, textX, textY, opacity, fontScale);
}

void SystemHud::ShowFpsCounter(DebugHud* hud, uint32_t screenWidth, int lineNumber) const {
	int yPos = 10 + 10 * lineNumber;

	string fpsString = string("FPS: ") + std::to_string(_currentFPS); // +" / " + std::to_string(_currentRenderedFPS);
	uint32_t length = DrawStringCommand::MeasureString(fpsString).X;
	DrawString(hud, screenWidth, fpsString, screenWidth - 8 - length, yPos);
}

void SystemHud::ShowGameTimer(DebugHud* hud, uint32_t screenWidth, int lineNumber) const {
	int yPos = 10 + 10 * lineNumber;
	uint32_t frameCount = _emu->GetFrameCount();
	double frameRate = _emu->GetFps();
	uint32_t seconds = (uint32_t)(frameCount / frameRate) % 60;
	uint32_t minutes = (uint32_t)(frameCount / frameRate / 60) % 60;
	uint32_t hours = (uint32_t)(frameCount / frameRate / 3600);

	char buf[9]; // "HH:MM:SS\0"
	snprintf(buf, sizeof(buf), "%02u:%02u:%02u", hours, minutes, seconds);

	string text(buf);
	uint32_t length = DrawStringCommand::MeasureString(text).X;
	DrawString(hud, screenWidth, text, screenWidth - 8 - length, yPos);
}

void SystemHud::ShowFrameCounter(DebugHud* hud, uint32_t screenWidth, int lineNumber) const {
	int yPos = 10 + 10 * lineNumber;
	uint32_t frameCount = _emu->GetFrameCount();

	string frameCounter = MessageManager::Localize("Frame") + ": " + std::to_string(frameCount);
	uint32_t length = DrawStringCommand::MeasureString(frameCounter).X;
	DrawString(hud, screenWidth, frameCounter, screenWidth - 8 - length, yPos);
}

void SystemHud::ShowLagCounter(DebugHud* hud, uint32_t screenWidth, int lineNumber) const {
	int yPos = 10 + 10 * lineNumber;
	uint32_t count = _emu->GetLagCounter();

	string lagCounter = MessageManager::Localize("Lag") + ": " + std::to_string(count);
	uint32_t length = DrawStringCommand::MeasureString(lagCounter).X;
	DrawString(hud, screenWidth, lagCounter, screenWidth - 8 - length, yPos);
}

void SystemHud::DrawCounters(DebugHud* hud, uint32_t screenWidth) const {
	int lineNumber = 0;
	const auto& cfg = _emu->GetSettings()->GetPreferences();

	if (_emu->IsRunning()) {
		if (cfg.ShowFps) {
			ShowFpsCounter(hud, screenWidth, lineNumber++);
		}
		if (cfg.ShowGameTimer) {
			ShowGameTimer(hud, screenWidth, lineNumber++);
		}
		if (cfg.ShowFrameCounter) {
			ShowFrameCounter(hud, screenWidth, lineNumber++);
		}
		if (cfg.ShowLagCounter) {
			ShowLagCounter(hud, screenWidth, lineNumber++);
		}
		if(cfg.ShowRerecordCounter) {
			ShowRerecordCounter(hud, screenWidth, lineNumber++);
		}
	}
}

void SystemHud::DisplayMessage(const string& title, const string& message) {
	auto lock = _msgLock.AcquireSafe();
	_messages.push_front(std::make_unique<MessageInfo>(title, message, 3000));
}

void SystemHud::DrawMessages(DebugHud* hud, uint32_t screenWidth, uint32_t screenHeight) const {
	int counter = 0;
	int lastHeight = 3;
	for (const auto& msg : _messages) {
		if (counter < 4) {
			DrawMessage(hud, *msg.get(), screenWidth, screenHeight, lastHeight);
		} else {
			break;
		}
		counter++;
	}
}

void SystemHud::DrawBar(DebugHud* hud, int x, int y, int width, int height) const {
	hud->DrawRectangle(x, y, width, height, 0xFFFFFF, true, 1);
	hud->DrawLine(x, y + 1, x + width, y + 1, 0x4FBECE, 1);
	hud->DrawLine(x + 1, y, x + 1, y + height, 0x4FBECE, 1);

	hud->DrawLine(x + width - 1, y, x + width - 1, y + height, 0xCC9E22, 1);
	hud->DrawLine(x, y + height - 1, x + width, y + height - 1, 0xCC9E22, 1);

	hud->DrawLine(x, y, x + width, y, 0x303030, 1);
	hud->DrawLine(x, y, x, y + height, 0x303030, 1);

	hud->DrawLine(x + width, y, x + width, y + height, 0x303030, 1);
	hud->DrawLine(x, y + height, x + width, y + height, 0x303030, 1);
}

void SystemHud::DrawPauseIcon(DebugHud* hud) const {
	DrawBar(hud, 10, 7, 5, 12);
	DrawBar(hud, 17, 7, 5, 12);
}

void SystemHud::DrawPlayIcon(DebugHud* hud) const {
	int x = 12;
	int y = 12;
	int width = 5;
	int height = 8;
	int borderColor = 0x00000;
	int color = 0xFFFFFF;

	for (int i = 0; i < width; i++) {
		int left = x + i * 2;
		int top = y + i;
		hud->DrawLine(left, top - 1, left, y + height - i + 1, borderColor, 1);
		hud->DrawLine(left + 1, top - 1, left + 1, y + height - i + 1, borderColor, 1);

		if (i > 0) {
			hud->DrawLine(left, top, left, y + height - i, color, 1);
		}

		if (i < width - 1) {
			hud->DrawLine(left + 1, top, left + 1, y + height - i, color, 1);
		}
	}
}

void SystemHud::DrawRecordIcon(DebugHud* hud) const {
	int x = 12;
	int y = 11;
	int borderColor = 0x00000;
	int color = 0xFF0000;

	hud->DrawRectangle(x + 3, y, 4, 10, borderColor, true, 1);
	hud->DrawRectangle(x, y + 3, 10, 4, borderColor, true, 1);
	hud->DrawRectangle(x + 2, y + 1, 6, 8, borderColor, true, 1);
	hud->DrawRectangle(x + 1, y + 2, 8, 6, borderColor, true, 1);

	hud->DrawRectangle(x + 3, y + 1, 4, 8, color, true, 1);
	hud->DrawRectangle(x + 2, y + 2, 6, 6, color, true, 1);
	hud->DrawRectangle(x + 1, y + 3, 8, 4, color, true, 1);
}

void SystemHud::DrawTasReadWriteIndicator(DebugHud* hud, int xOffset) const {
	int x = 24 + xOffset;
	hud->DrawString(x, 11, "R/W", 0xE0E0E0, 0xFF000000, 1);
}

void SystemHud::ShowRerecordCounter(DebugHud* hud, uint32_t screenWidth, int lineNumber) const {
	int yPos = 10 + 10 * lineNumber;
	string rerecordText = MessageManager::Localize("Rerecord") + ": 0";
	uint32_t length = DrawStringCommand::MeasureString(rerecordText).X;
	DrawString(hud, screenWidth, rerecordText, screenWidth - 8 - length, yPos);
}

void SystemHud::DrawLagFrameIndicator(DebugHud* hud, uint32_t screenWidth, uint32_t screenHeight) const {
	if (_emu->GetLagCounter() == 0) {
		return;
	}

	int x = (int)screenWidth - 12;
	int y = (int)screenHeight - 12;
	hud->DrawRectangle(x, y, 6, 6, 0xFFFF00, true, 1);
}

void SystemHud::DrawTurboRewindIcon(DebugHud* hud, bool forRewind, int xOffset) const {
	int x = 12 + xOffset;
	int y = 12;
	int width = 3;
	int height = 8;

	int frameId = (int)(_animationTimer.GetElapsedMS() / 75) % 16;
	if (frameId >= 8) {
		frameId = (~frameId & 0x07);
	}

	static constexpr uint32_t rewindColors[8] = {0xFF8080, 0xFF9080, 0xFFA080, 0xFFB080, 0xFFC080, 0xFFD080, 0xFFE080, 0xFFF080};
	static constexpr uint32_t turboColors[8] = {0x80FF80, 0x90FF80, 0xA0FF80, 0xB0FF80, 0xC0FF80, 0xD0FF80, 0xE0FF80, 0xF0FF80};

	int color;
	if (forRewind) {
		color = rewindColors[frameId];
		x += 5;
	} else {
		color = turboColors[frameId];
	}

	int borderColor = 0x333333;
	int sign = forRewind ? -1 : 1;

	for (int j = 0; j < 2; j++) {
		for (int i = 0; i < width; i++) {
			int left = x + i * sign * 2;
			int top = y + i * 2;
			hud->DrawLine(left, top - 2, left, y + height - i * 2 + 2, borderColor, 1);
			hud->DrawLine(left + 1 * sign, top - 1, left + 1 * sign, y + height - i * 2 + 1, borderColor, 1);

			if (i > 0) {
				hud->DrawLine(left, top - 1, left, y + height + 1 - i * 2, color, 1);
			}

			if (i < width - 1) {
				hud->DrawLine(left + 1 * sign, top, left + 1 * sign, y + height - i * 2, color, 1);
			}
		}

		x += 6;
	}
}

void SystemHud::UpdateHud() {
	{
		auto lock = _msgLock.AcquireSafe();
		_messages.remove_if([](unique_ptr<MessageInfo>& msg) { return msg->IsExpired(); });
	}

	if (_emu->IsRunning()) {
		if (_fpsTimer.GetElapsedMS() > 1000) {
			// Update fps every sec
			uint32_t frameCount = _emu->GetFrameCount();
			if (_lastFrameCount > frameCount) {
				_currentFPS = 0;
			} else {
				_currentFPS = (int)(std::round((double)(frameCount - _lastFrameCount) / (_fpsTimer.GetElapsedMS() / 1000)));
				_currentRenderedFPS = (int)(std::round((double)(_renderedFrameCount - _lastRenderedFrameCount) / (_fpsTimer.GetElapsedMS() / 1000)));
			}
			_lastFrameCount = frameCount;
			_lastRenderedFrameCount = _renderedFrameCount;
			_fpsTimer.Reset();
		}

		if (_currentFPS > 5000) {
			_currentFPS = 0;
		}
		if (_currentRenderedFPS > 5000) {
			_currentRenderedFPS = 0;
		}

		_renderedFrameCount++;
	}
}
