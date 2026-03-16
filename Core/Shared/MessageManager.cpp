#include "pch.h"
#include "Shared/MessageManager.h"

std::unordered_map<string, string> MessageManager::_enResources = {
    {"Cheats",                        "Cheats"                                                                                                },
    {"Debug",                         "Debug"                                                                                                 },
    {"EmulationSpeed",                "Emulation Speed"                                                                                       },
    {"ClockRate",                     "Clock Rate"                                                                                            },
    {"Error",                         "Error"                                                                                                 },
    {"GameInfo",                      "Game Info"                                                                                             },
    {"GameLoaded",                    "Game loaded"                                                                                           },
    {"Input",                         "Input"                                                                                                 },
    {"Patch",                         "Patch"                                                                                                 },
    {"Movies",                        "Movies"                                                                                                },
    {"NetPlay",                       "Net Play"                                                                                              },
    {"Overclock",                     "Overclock"                                                                                             },
    {"Region",                        "Region"                                                                                                },
    {"SaveStates",                    "Save States"                                                                                           },
    {"ScreenshotSaved",               "Screenshot Saved"                                                                                      },
    {"SoundRecorder",                 "Sound Recorder"                                                                                        },
    {"Test",                          "Test"                                                                                                  },
    {"VideoRecorder",                 "Video Recorder"                                                                                        },

    {"ApplyingPatch",                 "Applying patch: %1"                                                                                    },
    {"PatchFailed",                   "Failed to apply patch: %1"                                                                             },
    {"CheatApplied",                  "1 cheat applied."                                                                                      },
    {"CheatsApplied",                 "%1 cheats applied."                                                                                    },
    {"CheatsDisabled",                "All cheats disabled."                                                                                  },
    {"CoinInsertedSlot",              "Coin inserted (slot %1)"                                                                               },
    {"ConnectedToServer",             "Connected to server."                                                                                  },
    {"ConnectionLost",                "Connection to server lost."                                                                            },
    {"CouldNotConnect",               "Could not connect to the server."                                                                      },
    {"CouldNotInitializeAudioSystem", "Could not initialize audio system"                                                                     },
    {"CouldNotFindRom",               "Could not find matching game ROM. (%1)"                                                                },
    {"CouldNotWriteToFile",           "Could not write to file: %1"                                                                           },
    {"CouldNotLoadFile",              "Could not load file: %1"                                                                               },
    {"EmulationMaximumSpeed",         "Maximum speed"                                                                                         },
    {"EmulationSpeedPercent",         "%1%"                                                                                                   },
    {"FdsDiskInserted",               "Disk %1 Side %2 inserted."                                                                             },
    {"Frame",                         "Frame"                                                                                                 },
    {"GameCrash",                     "Game has crashed (%1)"                                                                                 },
    {"KeyboardModeDisabled",          "Keyboard mode disabled."                                                                               },
    {"KeyboardModeEnabled",           "Keyboard connected - shortcut keys disabled."                                                          },
    {"Lag",                           "Lag"                                                                                                   },
    {"Mapper",                        "Mapper: %1, SubMapper: %2"                                                                             },
    {"MovieEnded",                    "Movie ended."                                                                                          },
    {"MovieStopped",                  "Movie stopped."                                                                                        },
    {"MovieInvalid",                  "Invalid movie file."                                                                                   },
    {"MovieMissingRom",               "Missing ROM required (%1) to play movie."                                                              },
    {"MovieNewerVersion",             "Cannot load movies created by a more recent version of Nexen. Please download the latest version."     },
    {"MovieIncompatibleVersion",      "This movie is incompatible with this version of Nexen."                                                },
    {"MovieIncorrectConsole",         "This movie was recorded on another console (%1) and can't be loaded."                                  },
    {"MoviePlaying",                  "Playing movie: %1"                                                                                     },
    {"MovieRecordingTo",              "Recording to: %1"                                                                                      },
    {"MovieSaved",                    "Movie saved to file: %1"                                                                               },
    {"NetplayVersionMismatch",        "Netplay client is not running the same version of Nexen and has been disconnected."                    },
    {"NetplayNotAllowed",             "This action is not allowed while connected to a server."                                               },
    {"OverclockEnabled",              "Overclocking enabled."                                                                                 },
    {"OverclockDisabled",             "Overclocking disabled."                                                                                },
    {"PrgSizeWarning",                "PRG size is smaller than 32kb"                                                                         },
    {"SaveStateEmpty",                "Slot is empty."                                                                                        },
    {"SaveStateIncompatibleVersion",  "Save state is incompatible with this version of Nexen."                                                },
    {"SaveStateInvalidFile",          "Invalid save state file."                                                                              },
    {"SaveStateWrongSystem",          "Error: State cannot be loaded (wrong console type)"                                                    },
    {"SaveStateLoaded",               "State #%1 loaded."                                                                                     },
    {"SaveStateLoadedFile",           "State loaded: %1"                                                                                      },
    {"SaveStateSavedFile",            "State saved: %1"                                                                                       },
    {"SaveStateMissingRom",           "Missing ROM required (%1) to load save state."                                                         },
    {"SaveStateNewerVersion",         "Cannot load save states created by a more recent version of Nexen. Please download the latest version."},
    {"SaveStateSaved",                "State #%1 saved."                                                                                      },
    {"SaveStateSavedTime",            "State saved at %1"                                                                                     },
    {"SaveStateSlotSelected",         "Slot #%1 selected."                                                                                    },
    {"ScanlineTimingWarning",         "PPU timing has been changed."                                                                          },
    {"ServerStarted",                 "Server started (Port: %1)"                                                                             },
    {"ServerStopped",                 "Server stopped"                                                                                        },
    {"SoundRecorderStarted",          "Recording to: %1"                                                                                      },
    {"SoundRecorderStopped",          "Recording saved to: %1"                                                                                },
    {"TestFileSavedTo",               "Test file saved to: %1"                                                                                },
    {"UnexpectedError",               "Unexpected error: %1"                                                                                  },
    {"UnsupportedMapper",             "Unsupported mapper (%1), cannot load game."                                                            },
    {"VideoRecorderStarted",          "Recording to: %1"                                                                                      },
    {"VideoRecorderStopped",          "Recording saved to: %1"                                                                                },
};

std::deque<string> MessageManager::_log;
SimpleLock MessageManager::_logLock;
SimpleLock MessageManager::_messageLock;
bool MessageManager::_osdEnabled = true;
bool MessageManager::_outputToStdout = false;
IMessageManager* MessageManager::_messageManager = nullptr;

void MessageManager::RegisterMessageManager(IMessageManager* messageManager) {
	auto lock = _messageLock.AcquireSafe();
	if (MessageManager::_messageManager == nullptr) {
		MessageManager::_messageManager = messageManager;
	}
}

void MessageManager::UnregisterMessageManager(IMessageManager* messageManager) {
	auto lock = _messageLock.AcquireSafe();
	if (MessageManager::_messageManager == messageManager) {
		MessageManager::_messageManager = nullptr;
	}
}

void MessageManager::SetOptions(bool osdEnabled, bool outputToStdout) {
	_osdEnabled = osdEnabled;
	_outputToStdout = outputToStdout;
}

string MessageManager::Localize(const string& key) {
	const auto* resources = &_enResources;
	/*switch(EmulationSettings::GetDisplayLanguage()) {
	    case Language::English: resources = &_enResources; break;
	    case Language::French: resources = &_frResources; break;
	    case Language::Japanese: resources = &_jaResources; break;
	    case Language::Russian: resources = &_ruResources; break;
	    case Language::Spanish: resources = &_esResources; break;
	    case Language::Ukrainian: resources = &_ukResources; break;
	    case Language::Portuguese: resources = &_ptResources; break;
	    case Language::Catalan: resources = &_caResources; break;
	    case Language::Chinese: resources = &_zhResources; break;
	}*/
	if (resources) {
		auto it = resources->find(key);
		if (it != resources->end()) {
			return it->second;
		} /* else if(EmulationSettings::GetDisplayLanguage() != Language::English) {
		     //Fallback on English if resource key is missing another language
		     resources = &_enResources;
		     auto it2 = resources->find(key);
		     if (it2 != resources->end()) {
		         return it2->second;
		     }
		 }*/
	}

	return string(key);
}

void MessageManager::DisplayMessage(const string& title, const string& message, const string& param1, const string& param2) {
	if (MessageManager::_messageManager) {
		auto lock = _messageLock.AcquireSafe();
		if (!MessageManager::_messageManager) {
			return;
		}

		string localTitle = Localize(title);
		string localMessage = Localize(message);

		size_t startPos = localMessage.find("%1");
		if (startPos != std::string::npos) {
			localMessage.replace(startPos, 2, param1);
		}

		startPos = localMessage.find("%2");
		if (startPos != std::string::npos) {
			localMessage.replace(startPos, 2, param2);
		}

		if (_osdEnabled) {
			MessageManager::_messageManager->DisplayMessage(localTitle, localMessage);
		} else {
			MessageManager::Log("[" + localTitle + "] " + localMessage);
		}
	}
}

void MessageManager::Log(const string& message) {
	auto lock = _logLock.AcquireSafe();
	if (message.empty()) {
		_log.push_back("------------------------------------------------------");
	} else {
		_log.push_back(message);
	}
	if (_log.size() > 1000) {
		_log.pop_front();
	}

	if (_outputToStdout) {
		std::cout << (message.empty() ? _log.back() : message) << '\n';
	}
}

void MessageManager::Log(string&& message) {
	auto lock = _logLock.AcquireSafe();
	if (message.empty()) {
		_log.push_back("------------------------------------------------------");
	} else {
		_log.push_back(std::move(message));
	}
	if (_log.size() > 1000) {
		_log.pop_front();
	}

	if (_outputToStdout) {
		std::cout << _log.back() << '\n';
	}
}

void MessageManager::ClearLog() {
	auto lock = _logLock.AcquireSafe();
	_log.clear();
}

string MessageManager::GetLog() {
	auto lock = _logLock.AcquireSafe();
	string result;
	size_t totalSize = 0;
	for (const string& msg : _log) {
		totalSize += msg.size() + 1;
	}
	result.reserve(totalSize);
	for (const string& msg : _log) {
		result.append(msg);
		result += '\n';
	}
	return result;
}
