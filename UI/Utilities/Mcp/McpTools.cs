using Nexen.Debugger;
using Nexen.Interop;
using System;
using System.Collections.Generic;
using System.Text.Json.Nodes;

namespace Nexen.Utilities.Mcp;

/// <summary>
/// MCP tool implementations. Each tool maps an arguments JsonNode to a
/// result JsonNode. Tools throw McpException for protocol-level failures
/// (bad args); uncaught exceptions are caught in McpServer and surfaced as
/// isError tool results so the caller sees the failure as output.
///
/// Tools are intentionally small and composable — the caller orchestrates
/// multi-step workflows, not the tool layer. If a workflow needs 4 calls,
/// that's fine; the session is persistent and round-trips are cheap.
/// </summary>
internal static class McpTools
{
	private static string GetMcpCaptureFolder()
	{
		string? envDir = Environment.GetEnvironmentVariable("NEXEN_MCP_CAPTURE_DIR");
		string dir = string.IsNullOrWhiteSpace(envDir)
			? Nexen.Config.ConfigManager.ScreenshotFolder
			: envDir;
		System.IO.Directory.CreateDirectory(dir);
		return dir;
	}

	public static readonly IReadOnlyList<McpToolDesc> Descriptions = new List<McpToolDesc> {
		new("ping",
			"Echo back. Use to verify the MCP session is alive.",
			null),

		new("get_state",
			"Snapshot of emulator state: isRunning, isPaused. Emulation state "
				+ "reads are only consistent if the emulator is paused — use pause "
				+ "before a sequence of read_memory calls to get a coherent snapshot.",
			null),


		new("reset_diag",
			"Return native SNES and SA-1 reset counters. Use this to distinguish "
				+ "a coprocessor reset from an instruction-level branch or mode change.",
			null),

		new("get_cpu_state",
			"Get a CPU's register state: pc, k(bank), a, x, y, sp, d, dbr, ps(flags), "
				+ "emulationMode, stopState, cycleCount. cpuType='Sa1' reads the SA-1 "
				+ "coprocessor (use to debug SA-1 bring-up). Pause first for a coherent read.",
			BuildGetCpuStateSchema()),

		new("set_cpu_state",
			"Set a CPU's architectural registers (pc,k,a,x,y,sp,d,dbr,ps,cycleCount,"
				+ "emulationMode); only provided fields change. cpuType='Sa1' or 'Snes'. "
				+ "Used with write_memory to transplant an execution state. Pause first.",
			BuildSetCpuStateSchema()),

		new("pause",
			"Pause emulation. All read_* tools become race-free while paused. "
				+ "Follow with resume or run_frames to advance.",
			null),

		new("resume",
			"Resume emulation at full speed. Reads are valid but may see a moving "
				+ "target across multiple calls.",
			null),

		new("step_cpu",
			"Execute an exact number of debugger CPU steps, then pause. This is the "
				+ "instruction-boundary primitive for deterministic CPU bring-up; unlike "
				+ "run_frames it does not advance through a frame-level race.",
			BuildStepCpuSchema()),

		new("run_frames",
			"Advance emulation by exactly N frames, then pause again. Deterministic: "
				+ "polls EmuApi.GetFrameCount() until the counter has advanced by N "
				+ "(no wall-clock racing). Returns startFrame, endFrame, framesAdvanced, "
				+ "and a timedOut flag if the 2x wall-clock safety cap fires (indicates "
				+ "a wedged emulator, not a normal slowdown).",
			BuildRunFramesSchema()),

		new("read_memory",
			"Read N bytes from a memory region. Returns hex-encoded bytes. "
				+ "Pause first for a race-free read.",
			BuildReadMemorySchema()),

		new("write_memory",
			"Write N bytes (hex-encoded) to a memory region. Use for WRAM pokes "
				+ "(room transitions, actor position seeds, etc). Pause the emulator "
				+ "first if you need the write to take effect on a specific frame.",
			BuildWriteMemorySchema()),

		new("take_screenshot",
			"Capture the current PPU frame as a PNG. Returns path (default) or "
				+ "base64-encoded bytes, plus width, height, and unique_colors of the image. "
				+ "Pause first or accept mid-render.",
			BuildScreenshotSchema()),

		new("save_state",
			"Save emulator state to a named slot or file path. Pair with load_state "
				+ "to checkpoint expensive boots (skip intros permanently).",
			BuildSaveLoadSchema()),

		new("load_state",
			"Load emulator state from a named slot or file path.",
			BuildSaveLoadSchema()),

		new("set_input",
			"Inject controller state for the next N frames. Buttons are bit-OR "
				+ "of: a=1,b=2,select=4,start=8,up=16,down=32,left=64,right=128 and "
				+ "SNES-only x=256,l=512,r=1024,y=2048. Use when scripts need to "
				+ "drive past the title screen or trigger script-driven transitions "
				+ "the way the real controller does.",
			BuildSetInputSchema()),

		new("get_ppu_state",
			"Return PPU register snapshot: forced-blank flag, brightness, BG mode, "
				+ "main/sub screen layer enable masks (bit0=BG1..bit4=OBJ), per-layer "
				+ "scroll + tile/tilemap addrs, window config. Essential for diagnosing "
				+ "'black scanlines' / 'missing layer' bugs without eyeball + guesswork.",
			null),

		new("add_exec_hook",
			"Fire on CPU instruction execution at address (or in [address..endAddress]). "
				+ "Returns a handle; remove_hook to detach. Per-fire the server sends "
				+ "notifications/mesen/hookFired with handle, address, value, frame. "
				+ "Optional matchValue+matchValueMask filters server-side so high-volume "
				+ "PCs don't flood the socket.",
			BuildAddHookSchema(includeValueMatch: true)),

		new("add_read_hook",
			"Fire on CPU memory reads at address (or [address..endAddress]). The "
				+ "value field of the notification is the byte read. matchValue/Mask "
				+ "supports the common 'fire when value == X' pattern.",
			BuildAddHookSchema(includeValueMatch: true)),

		new("add_write_hook",
			"Fire on CPU memory writes. value = byte written. matchValue/Mask same as "
				+ "the other hook tools.",
			BuildAddHookSchema(includeValueMatch: true)),

		new("remove_hook",
			"Detach a previously-registered hook by handle.",
			BuildRemoveHookSchema()),

		new("list_hooks",
			"List currently-registered hooks. Snapshot only; hooks can fire between "
				+ "the list call and the response.",
			null),

		new("hook_diag",
			"Diagnostic counters: total calls into the MCP hook hot-path + total "
				+ "address-range matches since reset. Lets you confirm the hot-path "
				+ "is alive even when no hook address has fired yet.",
			null),

		new("lookup_symbol",
			"Resolve symbol names from a WLA-DX-format sym file. Pass the file path "
				+ "(typically build/SuperMonkeyIsland.sym for SNES projects) and a "
				+ "regex pattern. Returns matches with their bank:offset + 24-bit ROM "
				+ "address ($C0|offset for HiROM bank 0). Caches the parsed file.",
			BuildLookupSymbolSchema()),

		new("disassemble",
			"Return N disassembled instructions starting at address. Wraps Nexen's "
				+ "GetDisassemblyOutput. Useful for one-shot 'what's at this PC' rather "
				+ "than dragging out a hex dump.",
			BuildDisassembleSchema()),

		new("run_until",
			"Resume emulation until any of (a) up to N frames pass, or (b) a hook fires. "
				+ "Returns the trigger reason and (if a hook fired) which handle. "
				+ "Atomic 'pause -> set hook -> resume -> wait -> pause' so callers "
				+ "don't have to roll the state machine themselves.",
			BuildRunUntilSchema()),

		new("crop_screenshot",
			"Capture a screenshot, then crop a region (x, y, w, h). Returns path or "
				+ "inline base64 like take_screenshot. For UI region checks where "
				+ "the full frame is wasted bytes.",
			BuildCropScreenshotSchema()),

		new("render_tilemap",
			"Render a BG layer's full tilemap as a PNG using the current PPU config "
				+ "(chr base, palette, flip bits read live from the layer's PPU state). "
				+ "Unlike take_screenshot which gives you the visible 256x224 viewport, "
				+ "this dumps the whole tilemap so you can see what's loaded outside the "
				+ "camera (e.g. NT1 cols 32-39 in a HiROM viewport-load bug). NOTE: if a "
				+ "DoubleWidth tilemap shows NT1 (cols 32-63) as a solid color, that "
				+ "usually means the GAME hasn't populated NT1 yet — many scroll engines "
				+ "fill nametables lazily as the camera moves. Verify with read_memory "
				+ "at the layer's tilemapAddress + 0x800 (returned in result) before "
				+ "blaming the tool. Returns path or base64.",
			BuildRenderTilemapSchema()),

		new("render_tile_sheet",
			"Render a region of VRAM as a tile sheet PNG using the chosen palette + "
				+ "BPP. Like Nexen's Tile Viewer but as one MCP call. Use to spot "
				+ "uninitialized tiles or verify what just got DMA'd into a chr base. "
				+ "Defaults to BG1 chr base (VRAM $0000), 256 tiles, 4bpp, palette 0.",
			BuildRenderTileSheetSchema()),

		new("render_oam",
			"Render OAM as a PNG. mode='positioned' draws the 128 sprites at their "
				+ "current screen X/Y on a transparent canvas (the same view as Nexen's "
				+ "Sprite Viewer 'Sprite Outlines'). mode='sheet' lays the 128 sprites "
				+ "out in a grid with their tile pixels for tile-data inspection.",
			BuildRenderOamSchema()),

		new("render_palette",
			"Render the current CGRAM (256 colors) as a PNG. mode='grid' = 16x16 "
				+ "swatch grid (one row per palette, default). mode='strip' = 1x256 "
				+ "horizontal strip. Optional highlight=N outlines that CGRAM index "
				+ "in red. Useful for color-cycling debug, BG/sprite palette boundary "
				+ "checks, and verifying palette restore after a room load.",
			BuildRenderPaletteSchema()),

		new("render_filmstrip",
			"Capture N screenshots, M frames apart, composed into a single PNG. "
				+ "Layout = horizontal (default) or grid (columns specified). Each "
				+ "frame labelled with its frame counter. Pair with run_until or "
				+ "set_input to capture animation cycles. Useful for spotting "
				+ "off-by-one transitions and motion glitches without scrubbing "
				+ "individual screenshots.",
			BuildFilmstripSchema()),

		new("memory_diff",
			"Snapshot one or more memory regions, advance N frames, snapshot again, "
				+ "return the bytes that changed. Eliminates the read -> run_frames "
				+ "-> read -> Python-side diff round-trips. Returns a sorted list of "
				+ "{address, oldValue, newValue, region} entries plus per-region "
				+ "summary counts.",
			BuildMemoryDiffSchema()),

		new("symbolic_dump",
			"Resolve every byte/word/long in a memory range to its nearest symbol. "
				+ "Reads the .sym file (cached, mtime-checked) and for each unit in "
				+ "[address..address+length) returns the closest symbol name plus "
				+ "the offset distance. Lets you ask 'what's living at $7EF000..$7EF200' "
				+ "without scrolling through the symbol file by hand.",
			BuildSymbolicDumpSchema()),

		new("lookup_pansy",
			"Open a TheAnsarya/pansy v1.0 metadata file and return its SYMBOLS, "
				+ "COMMENTS, and MEMORY_REGIONS sections (decompressed). Pansy is a "
				+ "cross-tool format that carries richer metadata than WLA-DX .sym: "
				+ "named regions, per-address comments, symbol types (label/constant/"
				+ "enum/struct/...). Optional regex filter narrows the symbol/comment "
				+ "lists by name/text. Pair with lookup_symbol when both files exist. "
				+ "Cached mtime-checked.",
			BuildLookupPansySchema()),

		new("record_movie",
			"Start recording a Nexen movie (.mmo) to disk. Use 'from'='CurrentState' "
				+ "to record from the current emulator state (default — pair with "
				+ "save_state for reproducible boots), or 'StartWithoutSaveData' / "
				+ "'StartWithSaveData' to record from power-on. Stops automatically "
				+ "when stop_movie is called or the emulator resets. Errors if a "
				+ "movie is already recording or playing.",
			BuildRecordMovieSchema()),

		new("play_movie",
			"Start playing back a Nexen movie (.mmo). The emulator advances input "
				+ "frames from the file. Pair with run_frames or run_until to drive "
				+ "to the assertion point, then read state. Errors if a movie is "
				+ "already recording or playing.",
			BuildPlayMovieSchema()),

		new("stop_movie",
			"Stop the currently recording or playing movie. No-op if nothing is "
				+ "active. Always safe to call as a teardown step.",
			null),

		new("movie_state",
			"Return whether a movie is currently recording or playing. Useful for "
				+ "polling after a long-running playback to detect EOF.",
			null),

		new("audio_fingerprint",
			"Hash + summarize a previously recorded WAV file (typically captured "
				+ "via record_audio). Returns SHA-256 of the raw sample data, sample "
				+ "rate, channel count, total samples, duration, peak amplitude, and "
				+ "per-second RMS levels. The fingerprint hash is suitable for "
				+ "regression testing ('did the audio change?') and the RMS timeline "
				+ "for spotting silent/clipped segments without rendering a waveform.",
			BuildAudioFingerprintSchema()),

		new("audio_waveform_png",
			"Render a recorded WAV file's amplitude envelope as a PNG. Each pixel "
				+ "column shows the min/max sample range in that time bucket. Useful "
				+ "for spotting song-section boundaries, silence, and clipping at a "
				+ "glance without external tooling. Color: white-on-black single-trace.",
			BuildAudioWaveformSchema()),

		new("trace_log",
			"Return the last N CPU instructions executed, each with PC, opcode bytes, "
				+ "disassembly, and register state (A, X, Y, SP, D, DB, P). Reads from "
				+ "Nexen's TraceLogger ring buffer (30000 entries deep). Far cheaper than "
				+ "setting an exec hook + Lua callback when you just want 'what was the "
				+ "CPU doing right before it stopped here?'. Auto-enables the trace "
				+ "logger on first call.",
			BuildTraceLogSchema()),

		new("watch_addresses",
			"Watch a list of memory addresses for changes over N frames. Bypasses the "
				+ "broken WRAM write-hook path on SA-1 by polling at end-of-frame via "
				+ "Nexen's PpuFrameDone notification. Returns a timeline of every change "
				+ "(frame, addr, oldValue, newValue, name). Pair with snesWorkRam offsets "
				+ "for $7E/$7F WRAM tracking.",
			BuildWatchAddressesSchema()),

		new("save_state_slot",
			"Save emulator state to numbered slot (0..9). Cheap checkpoint without "
				+ "managing file paths. Pair with load_state_slot to fast-forward past "
				+ "expensive boots.",
			BuildSlotSchema()),

		new("load_state_slot",
			"Load emulator state from numbered slot.",
			BuildSlotSchema()),

		new("read_dma_state",
			"Snapshot the 8 DMA/HDMA channel registers ($4300-$437F). Returns each "
				+ "channel's control / target / source / count / table addr. Critical "
				+ "for diagnosing per-scanline register effects (BG layer enables, "
				+ "scroll registers, palette ramps).",
			null),

		new("add_frame_hook",
			"Fire a notification once per emulator frame. Replaces the per-frame "
				+ "polling loop most diagnostic Lua scripts open with. The notification "
				+ "carries the frame number; use it to pace your client without sleeping.",
			BuildAddFrameHookSchema()),

		new("reset_emulator",
			"Soft-reset the emulator (equivalent to the SNES reset button). State "
				+ "wiped; the ROM stays loaded. Use to start each MCP-driven test "
				+ "from a known initial state without respawning Nexen.",
			null),

		new("record_audio",
			"Start recording emulator audio to a WAV file. Pair with stop_audio. "
				+ "Captures the same sample stream the user would hear, so client-side "
				+ "FFT analysis ('which voices are active', 'is the song playing the "
				+ "right pitch') becomes possible without ears.",
			BuildRecordAudioSchema()),

		new("stop_audio",
			"Stop the active audio recording.",
			null),

		new("get_audio_state",
			"Snapshot SPC700 + S-DSP register state. SPC700 fields = CPU registers "
				+ "(PC, A/X/Y, SP, flags). DSP fields include the 128-byte register "
				+ "block — voice key-on/off, volume L/R per voice, pitch, ADSR envelope, "
				+ "etc. Lets you ask 'what is the audio engine doing right now' rather "
				+ "than 'what does the song sound like'.",
			null),
	};

	private static JsonNode BuildRunFramesSchema()
	{
		var required = new JsonArray();
		required.Add(JsonValue.Create("count"));
		return new JsonObject {
			["type"] = "object",
			["properties"] = new JsonObject {
				["count"] = new JsonObject {
					["type"] = "integer",
					["description"] = "Number of frames to advance (1..1000000)",
				},
			},
			["required"] = required,
		};
	}

	private static JsonNode BuildStepCpuSchema()
	{
		var required = new JsonArray();
		required.Add(JsonValue.Create("cpuType"));
		return new JsonObject {
			["type"] = "object",
			["properties"] = new JsonObject {
				["cpuType"] = new JsonObject { ["type"] = "string", ["description"] = "CPU type, e.g. 'Sa1' or 'Snes'" },
				["count"] = new JsonObject { ["type"] = "integer", ["description"] = "Instruction count (1..1000, default 1)" },
				["stepType"] = new JsonObject { ["type"] = "string", ["description"] = "Debugger step type: Step, StepOver, or StepOut (default Step)" },
			},
			["required"] = required,
		};
	}

	private static JsonNode BuildWriteMemorySchema()
	{
		var required = new JsonArray();
		required.Add(JsonValue.Create("memoryType"));
		required.Add(JsonValue.Create("address"));
		required.Add(JsonValue.Create("hex"));
		return new JsonObject {
			["type"] = "object",
			["properties"] = new JsonObject {
				["memoryType"] = new JsonObject { ["type"] = "string" },
				["address"] = new JsonObject { ["type"] = "integer" },
				["hex"] = new JsonObject {
					["type"] = "string",
					["description"] = "Hex-encoded bytes, no separators (e.g. 'deadbeef')",
				},
			},
			["required"] = required,
		};
	}

	private static JsonNode BuildScreenshotSchema()
	{
		return new JsonObject {
			["type"] = "object",
			["properties"] = new JsonObject {
				["format"] = new JsonObject {
					["type"] = "string",
					["description"] = "'path' (default; writes PNG to Screenshots folder, returns path) or 'base64' (inline PNG bytes)",
				},
			},
		};
	}

	private static JsonNode BuildLookupSymbolSchema()
	{
		var required = new JsonArray();
		required.Add(JsonValue.Create("symFile"));
		required.Add(JsonValue.Create("pattern"));
		return new JsonObject {
			["type"] = "object",
			["properties"] = new JsonObject {
				["symFile"] = new JsonObject {
					["type"] = "string",
					["description"] = "Absolute path to a WLA-DX .sym file",
				},
				["pattern"] = new JsonObject {
					["type"] = "string",
					["description"] = "Regex pattern matched against symbol names. Use '^name$' for exact match.",
				},
				["maxResults"] = new JsonObject {
					["type"] = "integer",
					["description"] = "Cap on results returned (default 64)",
				},
			},
			["required"] = required,
		};
	}

	private static JsonNode BuildGetCpuStateSchema()
	{
		return new JsonObject {
			["type"] = "object",
			["properties"] = new JsonObject {
				["cpuType"] = new JsonObject {
					["type"] = "string",
					["description"] = "CPU type: 'Snes' (default) or 'Sa1'",
				},
			},
			["required"] = new JsonArray(),
		};
	}

	private static JsonNode BuildSetCpuStateSchema()
	{
		JsonObject Num(string d) => new JsonObject { ["type"] = "integer", ["description"] = d };
		return new JsonObject {
			["type"] = "object",
			["properties"] = new JsonObject {
				["cpuType"] = new JsonObject { ["type"] = "string", ["description"] = "CPU type: 'Snes' (default) or 'Sa1'" },
				["pc"] = Num("program counter (16-bit)"),
				["k"] = Num("program bank (8-bit)"),
				["a"] = Num("accumulator (16-bit)"),
				["x"] = Num("X (16-bit)"), ["y"] = Num("Y (16-bit)"),
				["sp"] = Num("stack pointer (16-bit)"), ["d"] = Num("direct page (16-bit)"),
				["dbr"] = Num("data bank (8-bit)"), ["ps"] = Num("processor flags (8-bit)"),
				["cycleCount"] = Num("cycle count (64-bit)"),
				["emulationMode"] = new JsonObject { ["type"] = "boolean", ["description"] = "65816 emulation mode flag" },
			},
			["required"] = new JsonArray(),
		};
	}

	private static JsonNode BuildDisassembleSchema()
	{
		var required = new JsonArray();
		required.Add(JsonValue.Create("address"));
		return new JsonObject {
			["type"] = "object",
			["properties"] = new JsonObject {
				["address"] = new JsonObject {
					["type"] = "integer",
					["description"] = "Start address (CPU bus, e.g. 0xC08000)",
				},
				["count"] = new JsonObject {
					["type"] = "integer",
					["description"] = "Number of lines (default 16)",
				},
				["cpuType"] = new JsonObject {
					["type"] = "string",
					["description"] = "CPU type (default 'Snes')",
				},
			},
			["required"] = required,
		};
	}

	private static JsonNode BuildRecordAudioSchema()
	{
		var required = new JsonArray();
		required.Add(JsonValue.Create("path"));
		return new JsonObject {
			["type"] = "object",
			["properties"] = new JsonObject {
				["path"] = new JsonObject {
					["type"] = "string",
					["description"] = "Output .wav path. Existing files are overwritten.",
				},
			},
			["required"] = required,
		};
	}

	private static JsonNode BuildAddFrameHookSchema()
	{
		return new JsonObject {
			["type"] = "object",
			["properties"] = new JsonObject {
				["everyN"] = new JsonObject {
					["type"] = "integer",
					["description"] = "Fire only every N-th frame (default 1, every frame). Higher values reduce notification volume.",
				},
				["cpuType"] = new JsonObject {
					["type"] = "string",
					["description"] = "CPU type the frame is associated with (default 'Snes'). Mostly cosmetic — frames are global.",
				},
			},
		};
	}

	private static JsonNode BuildCropScreenshotSchema()
	{
		var required = new JsonArray();
		required.Add(JsonValue.Create("x"));
		required.Add(JsonValue.Create("y"));
		required.Add(JsonValue.Create("width"));
		required.Add(JsonValue.Create("height"));
		return new JsonObject {
			["type"] = "object",
			["properties"] = new JsonObject {
				["x"] = new JsonObject { ["type"] = "integer" },
				["y"] = new JsonObject { ["type"] = "integer" },
				["width"] = new JsonObject { ["type"] = "integer" },
				["height"] = new JsonObject { ["type"] = "integer" },
				["format"] = new JsonObject { ["type"] = "string", ["description"] = "'path' or 'base64'" },
			},
			["required"] = required,
		};
	}

	private static JsonNode BuildRenderTilemapSchema()
	{
		return new JsonObject {
			["type"] = "object",
			["properties"] = new JsonObject {
				["layer"] = new JsonObject {
					["type"] = "integer",
					["description"] = "1..4 = BG1..BG4, 5 = Main composite, 6 = Sub composite. Default 1.",
				},
				["scale"] = new JsonObject {
					["type"] = "integer",
					["description"] = "Nearest-neighbor upscale factor 1..8. Default 1.",
				},
				["format"] = new JsonObject {
					["type"] = "string",
					["description"] = "'path' (default) or 'base64'.",
				},
			},
		};
	}

	private static JsonNode BuildRenderTileSheetSchema()
	{
		return new JsonObject {
			["type"] = "object",
			["properties"] = new JsonObject {
				["address"] = new JsonObject {
					["type"] = "integer",
					["description"] = "VRAM byte address to start at. Default 0 (BG1 chr base).",
				},
				["count"] = new JsonObject {
					["type"] = "integer",
					["description"] = "Number of tiles to render (1..1024). Default 256.",
				},
				["bpp"] = new JsonObject {
					["type"] = "integer",
					["description"] = "Bits per pixel: 2, 4, or 8. Default 4 (most SNES BG/OBJ tiles).",
				},
				["palette"] = new JsonObject {
					["type"] = "integer",
					["description"] = "CGRAM palette row index 0..15 (4bpp) or 0..7 (2bpp). Default 0.",
				},
				["columns"] = new JsonObject {
					["type"] = "integer",
					["description"] = "Tiles per row in the output. Default 16.",
				},
				["scale"] = new JsonObject {
					["type"] = "integer",
					["description"] = "Nearest-neighbor upscale 1..8. Default 1.",
				},
				["format"] = new JsonObject {
					["type"] = "string",
					["description"] = "'path' (default) or 'base64'.",
				},
			},
		};
	}

	private static JsonNode BuildRenderOamSchema()
	{
		return new JsonObject {
			["type"] = "object",
			["properties"] = new JsonObject {
				["mode"] = new JsonObject {
					["type"] = "string",
					["description"] = "'positioned' (default) or 'sheet'.",
				},
				["scale"] = new JsonObject {
					["type"] = "integer",
					["description"] = "Nearest-neighbor upscale 1..8. Default 1.",
				},
				["format"] = new JsonObject {
					["type"] = "string",
					["description"] = "'path' (default) or 'base64'.",
				},
			},
		};
	}

	private static JsonNode BuildRenderPaletteSchema()
	{
		return new JsonObject {
			["type"] = "object",
			["properties"] = new JsonObject {
				["mode"] = new JsonObject {
					["type"] = "string",
					["description"] = "'grid' (16x16, default) or 'strip' (1x256).",
				},
				["swatch"] = new JsonObject {
					["type"] = "integer",
					["description"] = "Pixel size of one color swatch (4..64). Default 16.",
				},
				["highlight"] = new JsonObject {
					["type"] = "integer",
					["description"] = "Optional CGRAM index 0..255 to outline in red.",
				},
				["format"] = new JsonObject {
					["type"] = "string",
					["description"] = "'path' (default) or 'base64'.",
				},
			},
		};
	}

	private static JsonNode BuildFilmstripSchema()
	{
		var required = new JsonArray();
		required.Add(JsonValue.Create("count"));
		return new JsonObject {
			["type"] = "object",
			["required"] = required,
			["properties"] = new JsonObject {
				["count"] = new JsonObject {
					["type"] = "integer",
					["description"] = "Number of screenshots to capture (2..64).",
				},
				["frameStep"] = new JsonObject {
					["type"] = "integer",
					["description"] = "Frames to advance between shots. Default 1.",
				},
				["columns"] = new JsonObject {
					["type"] = "integer",
					["description"] = "Grid width. 0 (default) = single horizontal strip.",
				},
				["scale"] = new JsonObject {
					["type"] = "integer",
					["description"] = "Per-frame integer upscale 1..4. Default 1.",
				},
				["label"] = new JsonObject {
					["type"] = "boolean",
					["description"] = "Burn frame number into each cell (default true).",
				},
				["format"] = new JsonObject {
					["type"] = "string",
					["description"] = "'path' (default) or 'base64'.",
				},
			},
		};
	}

	private static JsonNode BuildMemoryDiffSchema()
	{
		var required = new JsonArray();
		required.Add(JsonValue.Create("regions"));
		required.Add(JsonValue.Create("frames"));
		return new JsonObject {
			["type"] = "object",
			["required"] = required,
			["properties"] = new JsonObject {
				["regions"] = new JsonObject {
					["type"] = "array",
					["description"] = "List of {memoryType, address, length} regions to snapshot.",
					["items"] = new JsonObject {
						["type"] = "object",
						["properties"] = new JsonObject {
							["memoryType"] = new JsonObject { ["type"] = "string" },
							["address"]    = new JsonObject { ["type"] = "integer" },
							["length"]     = new JsonObject { ["type"] = "integer" },
						},
					},
				},
				["frames"] = new JsonObject {
					["type"] = "integer",
					["description"] = "Frames to advance between snapshots (1..1_000_000).",
				},
				["maxChanges"] = new JsonObject {
					["type"] = "integer",
					["description"] = "Cap result size per region. Default 256.",
				},
			},
		};
	}

	private static JsonNode BuildAudioFingerprintSchema()
	{
		var required = new JsonArray();
		required.Add(JsonValue.Create("path"));
		return new JsonObject {
			["type"] = "object",
			["required"] = required,
			["properties"] = new JsonObject {
				["path"] = new JsonObject {
					["type"] = "string",
					["description"] = "Path to a 16-bit PCM WAV file (the format Nexen's WaveRecord emits).",
				},
			},
		};
	}

	private static JsonNode BuildAudioWaveformSchema()
	{
		var required = new JsonArray();
		required.Add(JsonValue.Create("path"));
		return new JsonObject {
			["type"] = "object",
			["required"] = required,
			["properties"] = new JsonObject {
				["path"] = new JsonObject {
					["type"] = "string",
					["description"] = "Path to a 16-bit PCM WAV file.",
				},
				["outputPath"] = new JsonObject {
					["type"] = "string",
					["description"] = "Optional output PNG path. Default: alongside the WAV with .waveform.png suffix.",
				},
				["width"] = new JsonObject {
					["type"] = "integer",
					["description"] = "Output PNG width in pixels (64..4096). Default 1024.",
				},
				["height"] = new JsonObject {
					["type"] = "integer",
					["description"] = "Output PNG height in pixels (32..1024). Default 256.",
				},
				["format"] = new JsonObject {
					["type"] = "string",
					["description"] = "'path' (default) or 'base64'.",
				},
			},
		};
	}

	private static JsonNode BuildRecordMovieSchema()
	{
		var required = new JsonArray();
		required.Add(JsonValue.Create("path"));
		return new JsonObject {
			["type"] = "object",
			["required"] = required,
			["properties"] = new JsonObject {
				["path"] = new JsonObject {
					["type"] = "string",
					["description"] = "Output movie path (.mmo).",
				},
				["author"] = new JsonObject {
					["type"] = "string",
					["description"] = "Author string baked into the file. Default 'mesen-mcp'.",
				},
				["description"] = new JsonObject {
					["type"] = "string",
					["description"] = "Description string. Default empty.",
				},
				["from"] = new JsonObject {
					["type"] = "string",
					["description"] = "'CurrentState' (default), 'StartWithoutSaveData', or 'StartWithSaveData'.",
				},
			},
		};
	}

	private static JsonNode BuildPlayMovieSchema()
	{
		var required = new JsonArray();
		required.Add(JsonValue.Create("path"));
		return new JsonObject {
			["type"] = "object",
			["required"] = required,
			["properties"] = new JsonObject {
				["path"] = new JsonObject {
					["type"] = "string",
					["description"] = "Path to the movie file (.mmo).",
				},
			},
		};
	}

	private static JsonNode BuildLookupPansySchema()
	{
		var required = new JsonArray();
		required.Add(JsonValue.Create("pansyFile"));
		return new JsonObject {
			["type"] = "object",
			["required"] = required,
			["properties"] = new JsonObject {
				["pansyFile"] = new JsonObject {
					["type"] = "string",
					["description"] = "Absolute path to a .pansy metadata file.",
				},
				["pattern"] = new JsonObject {
					["type"] = "string",
					["description"] = "Optional regex filter on symbol names + comment text + region names. "
						+ "Defaults to '' which returns everything (capped by maxResults).",
				},
				["maxResults"] = new JsonObject {
					["type"] = "integer",
					["description"] = "Cap symbols / comments / regions returned (each list capped separately). "
						+ "Default 256.",
				},
				["sectionsOnly"] = new JsonObject {
					["type"] = "boolean",
					["description"] = "If true, return just the section table summary (no symbols/comments/regions). "
						+ "Useful for inspecting unfamiliar Pansy files. Default false.",
				},
			},
		};
	}

	private static JsonNode BuildSymbolicDumpSchema()
	{
		var required = new JsonArray();
		required.Add(JsonValue.Create("symFile"));
		required.Add(JsonValue.Create("address"));
		required.Add(JsonValue.Create("length"));
		return new JsonObject {
			["type"] = "object",
			["required"] = required,
			["properties"] = new JsonObject {
				["symFile"] = new JsonObject {
					["type"] = "string",
					["description"] = "Absolute path to a WLA-DX .sym file (same as lookup_symbol).",
				},
				["address"] = new JsonObject {
					["type"] = "integer",
					["description"] = "Start of address range to resolve (24-bit CPU/WRAM address).",
				},
				["length"] = new JsonObject {
					["type"] = "integer",
					["description"] = "Number of units to resolve.",
				},
				["unit"] = new JsonObject {
					["type"] = "string",
					["description"] = "'byte' (default), 'word' (2B stride), or 'long' (4B stride).",
				},
				["maxDistance"] = new JsonObject {
					["type"] = "integer",
					["description"] = "Skip a row if the nearest symbol is more than N bytes away. Default 64.",
				},
			},
		};
	}

	private static JsonNode BuildTraceLogSchema()
	{
		return new JsonObject {
			["type"] = "object",
			["properties"] = new JsonObject {
				["count"] = new JsonObject {
					["type"] = "integer",
					["description"] = "Number of recent rows to return (1..1000). Default 32.",
				},
				["cpuType"] = new JsonObject {
					["type"] = "string",
					["description"] = "'Snes' (default) or 'Sa1'. Picks which CPU's trace to read.",
				},
			},
		};
	}

	private static JsonNode BuildWatchAddressesSchema()
	{
		var required = new JsonArray();
		required.Add(JsonValue.Create("addresses"));
		return new JsonObject {
			["type"] = "object",
			["properties"] = new JsonObject {
				["addresses"] = new JsonObject {
					["type"] = "array",
					["description"] = "List of {memoryType, address, name?}. memoryType strings as in read_memory.",
				},
				["maxFrames"] = new JsonObject {
					["type"] = "integer",
					["description"] = "Frame budget (1..10000). Default 600.",
				},
				["maxEvents"] = new JsonObject {
					["type"] = "integer",
					["description"] = "Cap output size (1..4096). Default 256. Stops early if hit.",
				},
			},
			["required"] = required,
		};
	}

	private static JsonNode BuildSlotSchema()
	{
		var required = new JsonArray();
		required.Add(JsonValue.Create("slot"));
		return new JsonObject {
			["type"] = "object",
			["properties"] = new JsonObject {
				["slot"] = new JsonObject {
					["type"] = "integer",
					["description"] = "Slot index 0..9",
				},
			},
			["required"] = required,
		};
	}

	private static JsonNode BuildRunUntilSchema()
	{
		return new JsonObject {
			["type"] = "object",
			["properties"] = new JsonObject {
				["maxFrames"] = new JsonObject {
					["type"] = "integer",
					["description"] = "Frame budget; emulation pauses after this many frames if no hook fires (default 600).",
				},
				["hookHandle"] = new JsonObject {
					["type"] = "integer",
					["description"] = "An existing hook handle to wait on. If 0/missing, run for maxFrames and return.",
				},
			},
		};
	}

	private static JsonNode BuildAddHookSchema(bool includeValueMatch)
	{
		var required = new JsonArray();
		required.Add(JsonValue.Create("address"));
		var props = new JsonObject {
			["address"] = new JsonObject {
				["type"] = "integer",
				["description"] = "Start address (inclusive)",
			},
			["endAddress"] = new JsonObject {
				["type"] = "integer",
				["description"] = "End address (inclusive). Defaults to address for a single-byte/PC hook.",
			},
			["cpuType"] = new JsonObject {
				["type"] = "string",
				["description"] = "CPU type (default 'Snes'; other valid: 'Sa1', 'Spc', 'Gameboy', etc).",
			},
		};
		if(includeValueMatch) {
			props["matchValue"] = new JsonObject {
				["type"] = "integer",
				["description"] = "Optional value match. Hook only fires when (observed & matchValueMask) == (matchValue & matchValueMask).",
			};
			props["matchValueMask"] = new JsonObject {
				["type"] = "integer",
				["description"] = "Mask for matchValue. 0 disables value matching (fire on every hit). 0xFF for byte-exact.",
			};
			props["xValue"] = new JsonObject {
				["type"] = "integer",
				["description"] = "Optional X-register match for execution hooks.",
			};
			props["xMask"] = new JsonObject {
				["type"] = "integer",
				["description"] = "Mask for xValue; 0 disables X matching. Use 0xFFFF for exact X.",
			};
		}
		return new JsonObject {
			["type"] = "object",
			["properties"] = props,
			["required"] = required,
		};
	}

	private static JsonNode BuildRemoveHookSchema()
	{
		var required = new JsonArray();
		required.Add(JsonValue.Create("handle"));
		return new JsonObject {
			["type"] = "object",
			["properties"] = new JsonObject {
				["handle"] = new JsonObject {
					["type"] = "integer",
					["description"] = "Handle returned from add_exec_hook.",
				},
			},
			["required"] = required,
		};
	}

	private static JsonNode BuildSetInputSchema()
	{
		var required = new JsonArray();
		required.Add(JsonValue.Create("port"));
		required.Add(JsonValue.Create("buttons"));
		return new JsonObject {
			["type"] = "object",
			["properties"] = new JsonObject {
				["port"] = new JsonObject {
					["type"] = "integer",
					["description"] = "Controller port (0 = player 1)",
				},
				["buttons"] = new JsonObject {
					["type"] = "integer",
					["description"] = "Bitmask of buttons: A=1 B=2 Select=4 Start=8 Up=16 Down=32 Left=64 Right=128 X=256 L=512 R=1024 Y=2048",
				},
				["frames"] = new JsonObject {
					["type"] = "integer",
					["description"] = "Auto-advance mode: hold the input this many frames then auto-release. Required unless hold=true.",
				},
				["hold"] = new JsonObject {
					["type"] = "boolean",
					["description"] = "TAS-style hold: set the override and return immediately without advancing or clearing. The override persists across subsequent run_frames until released with set_input(buttons=0, hold=true). Use to observe manual $4016 serial reads while a button is held.",
				},
			},
			["required"] = required,
		};
	}

	private static JsonNode BuildSaveLoadSchema()
	{
		var required = new JsonArray();
		required.Add(JsonValue.Create("path"));
		return new JsonObject {
			["type"] = "object",
			["properties"] = new JsonObject {
				["path"] = new JsonObject {
					["type"] = "string",
					["description"] = "File path for the save-state .mss file",
				},
			},
			["required"] = required,
		};
	}

	// Build the read_memory JSON schema imperatively. JsonArray's collection-
	// initializer overload takes JsonNode params and ends up invoking the
	// generic Add<T> path at runtime, which trips IL2026 and throws under
	// trimming/AOT. Using explicit JsonValue.Create calls sidesteps the whole
	// dynamic-codegen question.
	private static JsonNode BuildReadMemorySchema()
	{
		var required = new JsonArray();
		required.Add(JsonValue.Create("memoryType"));
		required.Add(JsonValue.Create("address"));
		required.Add(JsonValue.Create("length"));

		return new JsonObject {
			["type"] = "object",
			["properties"] = new JsonObject {
				["memoryType"] = new JsonObject {
					["type"] = "string",
					["description"] = "One of: snesMemory (CPU bus), snesWorkRam, snesVideoRam, snesCgRam, snesSpriteRam, snesPrgRom, sa1Memory",
				},
				["address"] = new JsonObject {
					["type"] = "integer",
					["description"] = "Start address",
				},
				["length"] = new JsonObject {
					["type"] = "integer",
					["description"] = "Number of bytes to read (max 65536)",
				},
			},
			["required"] = required,
		};
	}

	public static JsonNode Ping(JsonNode? args)
	{
		return new JsonObject {
			["pong"] = true,
			["echo"] = args?.DeepClone(),
		};
	}

	public static JsonNode GetState(JsonNode? args)
	{
		return new JsonObject {
			["isRunning"] = EmuApi.IsRunning(),
			["isPaused"] = EmuApi.IsPaused(),
			["frameCount"] = EmuApi.GetFrameCount(),
		};
	}

	public static JsonNode ResetDiag(JsonNode? args)
	{
		DebugApi.McpGetResetCounts(out ulong snesResets, out ulong sa1Resets);
		return new JsonObject {
			["snesResets"] = snesResets,
			["sa1Resets"] = sa1Resets,
			["frameCount"] = EmuApi.GetFrameCount(),
		};
	}

	public static JsonNode GetCpuState(JsonNode? args)
	{
		string cpuStr = args?["cpuType"]?.GetValue<string>() ?? "Snes";
		if(!Enum.TryParse<CpuType>(cpuStr, ignoreCase: true, out var cpu)) {
			throw new McpException(-32602, "unknown cpuType: " + cpuStr);
		}
		var s = DebugApi.GetCpuState<SnesCpuState>(cpu);
		return new JsonObject {
			["cpuType"] = cpu.ToString(),
			["pc"] = s.PC,
			["k"] = s.K,
			["a"] = s.A,
			["x"] = s.X,
			["y"] = s.Y,
			["sp"] = s.SP,
			["d"] = s.D,
			["dbr"] = s.DBR,
			["ps"] = (byte)s.PS,
			["emulationMode"] = s.EmulationMode,
			["stopState"] = s.StopState.ToString(),
			["cycleCount"] = s.CycleCount,
		};
	}

	// Set a CPU's architectural registers. Reads the current state, overwrites any
	// provided fields, writes it back via DebugApi.SetCpuState. Used to transplant a
	// CPU's execution point (e.g. seed a gameplay state into the SA-1). Pause first.
	public static JsonNode SetCpuState(JsonNode? args)
	{
		string cpuStr = args?["cpuType"]?.GetValue<string>() ?? "Snes";
		if(!Enum.TryParse<CpuType>(cpuStr, ignoreCase: true, out var cpu)) {
			throw new McpException(-32602, "unknown cpuType: " + cpuStr);
		}
		var s = DebugApi.GetCpuState<SnesCpuState>(cpu);
		long? G(string k) => args?[k] != null ? args[k]!.GetValue<long>() : (long?)null;
		if(G("pc") is long pc) s.PC = (UInt16)pc;
		if(G("k") is long k) s.K = (byte)k;
		if(G("a") is long a) s.A = (UInt16)a;
		if(G("x") is long x) s.X = (UInt16)x;
		if(G("y") is long y) s.Y = (UInt16)y;
		if(G("sp") is long sp) s.SP = (UInt16)sp;
		if(G("d") is long d) s.D = (UInt16)d;
		if(G("dbr") is long dbr) s.DBR = (byte)dbr;
		if(G("ps") is long ps) s.PS = (SnesCpuFlags)(byte)ps;
		if(G("cycleCount") is long cc) s.CycleCount = (UInt64)cc;
		if(args?["emulationMode"] != null) s.EmulationMode = args["emulationMode"]!.GetValue<bool>();
		DebugApi.SetCpuState(s, cpu);
		return new JsonObject {
			["ok"] = true, ["cpuType"] = cpu.ToString(), ["pc"] = s.PC,
		};
	}

	public static JsonNode Pause(JsonNode? args)
	{
		EmuApi.Pause();
		// EmuApi.Pause queues the request; the emulator only actually halts
		// on the next frame boundary. Busy-wait briefly so the next read
		// sees a consistent state — callers would otherwise need to poll
		// IsPaused themselves after every pause.
		for(int i = 0; i < 200; i++) {
			if(EmuApi.IsPaused()) break;
			System.Threading.Thread.Sleep(5);
		}
		return new JsonObject { ["paused"] = EmuApi.IsPaused() };
	}

	public static JsonNode Resume(JsonNode? args)
	{
		EmuApi.Resume();
		return new JsonObject { ["paused"] = false };
	}

	public static JsonNode StepCpu(JsonNode? args)
	{
		args ??= new JsonObject();
		string cpuStr = args["cpuType"]?.GetValue<string>() ?? throw new McpException(-32602, "missing arg: cpuType");
		if(!Enum.TryParse<CpuType>(cpuStr, ignoreCase: true, out var cpu)) {
			throw new McpException(-32602, "unknown cpuType: " + cpuStr);
		}
		int count = (int)((args["count"]?.GetValue<long>()) ?? 1);
		if(count < 1 || count > 1000) throw new McpException(-32602, "count out of range (1..1000)");
		string stepStr = args["stepType"]?.GetValue<string>() ?? "Step";
		if(!Enum.TryParse<StepType>(stepStr, ignoreCase: true, out var stepType)) {
			throw new McpException(-32602, "unknown stepType: " + stepStr);
		}

		Pause(null);
		DebugApi.Step(cpu, count, stepType);
		for(int i = 0; i < 200; i++) {
			if(EmuApi.IsPaused()) break;
			System.Threading.Thread.Sleep(5);
		}
		var progress = DebugApi.GetInstructionProgress(cpu);
		return new JsonObject {
			["cpuType"] = cpu.ToString(),
			["count"] = count,
			["stepType"] = stepType.ToString(),
			["paused"] = EmuApi.IsPaused(),
			["currentCycle"] = progress.CurrentCycle,
			["lastOpCode"] = progress.LastOpCode,
			["lastMemoryAddress"] = progress.LastMemOperation.Address,
			["lastMemoryValue"] = progress.LastMemOperation.Value,
			["lastMemoryType"] = progress.LastMemOperation.Type.ToString(),
		};
	}

	public static JsonNode RunFrames(JsonNode? args)
	{
		if(args == null) {
			throw new McpException(-32602, "run_frames requires arguments");
		}
		long count = args["count"]?.GetValue<long>() ?? throw new McpException(-32602, "missing arg: count");
		if(count <= 0 || count > 1_000_000) {
			throw new McpException(-32602, "count out of range (1..1000000)");
		}

		// Frame-exact stepping: poll EmuApi.GetFrameCount() until exactly
		// `count` frames have elapsed. Replaces the old sleep-and-hope path
		// (which raced under max-speed and over-ran under emulation slowdown).
		// We still cap real wall-clock at 2x estimated runtime + 200ms slack
		// so a wedged emulator doesn't hang the MCP loop forever.
		uint startFrame = EmuApi.GetFrameCount();
		uint targetFrame = unchecked(startFrame + (uint)count);
		EmuApi.Resume();
		var sw = System.Diagnostics.Stopwatch.StartNew();
		// Black Tiger's interpreter can require substantially longer than one
		// real-time frame at max-speed.  A short cap leaves the emulator
		// running after the request times out; the next request then resumes
		// an in-flight frame and produces alternating 0/1-frame results.
		long maxMs = (long)(count * 2000.0) + 1000;
		while(sw.ElapsedMilliseconds < maxMs) {
			uint nowFrame = EmuApi.GetFrameCount();
			// Compare via wraparound-safe subtraction; a u32 frame counter
			// won't realistically wrap during a single MCP call (≈2.3 years
			// at 60FPS) but doing this right costs nothing.
			if(unchecked(nowFrame - startFrame) >= (uint)count) break;
			System.Threading.Thread.Sleep(2);
		}
		EmuApi.Pause();
		for(int i = 0; i < 200; i++) {
			if(EmuApi.IsPaused()) break;
			System.Threading.Thread.Sleep(5);
		}

		uint endFrame = EmuApi.GetFrameCount();
		long actualAdvanced = unchecked(endFrame - startFrame);
		return new JsonObject {
			["requested"] = count,
			["startFrame"] = startFrame,
			["endFrame"] = endFrame,
			["framesAdvanced"] = actualAdvanced,
			["isPaused"] = EmuApi.IsPaused(),
			["timedOut"] = sw.ElapsedMilliseconds >= maxMs && actualAdvanced < count,
		};
	}

	public static JsonNode WriteMemory(JsonNode? args)
	{
		if(args == null) {
			throw new McpException(-32602, "write_memory requires arguments");
		}
		string memTypeStr = RequireString(args, "memoryType");
		uint address = RequireUInt(args, "address");
		string hex = RequireString(args, "hex");
		byte[] bytes;
		try {
			bytes = Convert.FromHexString(hex);
		} catch {
			throw new McpException(-32602, "hex is not a valid hex string");
		}

		MemoryType memType = ParseMemoryType(memTypeStr);
		for(uint i = 0; i < bytes.Length; i++) {
			DebugApi.SetMemoryValue(memType, address + i, bytes[i]);
		}

		return new JsonObject {
			["memoryType"] = memTypeStr,
			["address"] = address,
			["length"] = bytes.Length,
		};
	}

	public static JsonNode TakeScreenshot(JsonNode? args)
	{
		string format = (args?["format"]?.GetValue<string>() ?? "path").ToLowerInvariant();

		// Use the debugger's composited main-screen view for headless SNES
		// sessions; the standard screenshot queue depends on a UI renderer.
		if(EmuApi.GetRomInfo().ConsoleType == ConsoleType.Snes) {
			return RenderSnesScreenCapture(format);
		}

		// EmuApi.TakeScreenshot writes to the configured Screenshots folder
		// using a timestamped filename. We capture the newest file that
		// appears as our return path. Simple, avoids framebuffer plumbing.
		string dir = Nexen.Config.ConfigManager.ScreenshotFolder;
		System.IO.Directory.CreateDirectory(dir);
		HashSet<string> before;
		try {
			before = new HashSet<string>(System.IO.Directory.GetFiles(dir));
		} catch {
			before = new HashSet<string>();
		}

		EmuApi.TakeScreenshot();

		// TakeScreenshot is async-queued; give it up to 2s to land.
		string? path = null;
		for(int i = 0; i < 40; i++) {
			System.Threading.Thread.Sleep(50);
			try {
				foreach(string f in System.IO.Directory.GetFiles(dir, "*.png")) {
					if(!before.Contains(f)) {
						path = f;
						break;
					}
				}
			} catch { }
			if(path != null) break;
		}
		if(path == null) {
			throw new McpException(-32603, "TakeScreenshot did not produce a file within 2s");
		}

		// Decode the PNG to read its dimensions and count unique colors.
		using var bmp = SkiaSharp.SKBitmap.Decode(path);
		int width = bmp?.Width ?? 0;
		int height = bmp?.Height ?? 0;
		int uniqueColors = 0;
		if(bmp != null) {
			var seen = new HashSet<uint>();
			SkiaSharp.SKColor[] pixels = bmp.Pixels;
			foreach(SkiaSharp.SKColor c in pixels) {
				seen.Add((uint)((c.Alpha << 24) | (c.Red << 16) | (c.Green << 8) | c.Blue));
			}
			uniqueColors = seen.Count;
		}

		var result = new JsonObject {
			["path"] = path,
			["width"] = width,
			["height"] = height,
			["unique_colors"] = uniqueColors,
		};

		if(format == "base64") {
			byte[] bytes = System.IO.File.ReadAllBytes(path);
			result["base64"] = Convert.ToBase64String(bytes);
			result["bytes"] = bytes.Length;
		}
		return result;
	}

	private static JsonObject RenderSnesScreenCapture(string format)
	{
		BaseState ppuState = DebugApi.GetPpuState(CpuType.Snes);
		BaseState ppuToolsState = DebugApi.GetPpuToolsState(CpuType.Snes);
		byte[] vram = DebugApi.GetMemoryState(MemoryType.SnesVideoRam);
		DebugPaletteInfo paletteInfo = DebugApi.GetPaletteInfo(CpuType.Snes);
		UInt32[] rgbPalette = paletteInfo.GetRgbPalette();

		// Layer 4 is the debugger's SNES Main screen view. It is populated from
		// the PPU's already-composited main-screen buffer, including sprite
		// priority and window effects, so do not overlay a second sprite preview.
		var tilemapOptions = new GetTilemapOptions {
			Layer = 4,
		};
		FrameInfo size = DebugApi.GetTilemapSize(CpuType.Snes, tilemapOptions, ppuState);
		if(size.Width == 0 || size.Height == 0) {
			throw new McpException(-32603, "SNES screenshot renderer could not resolve the main screen view");
		}

		int w = (int)size.Width;
		int h = (int)size.Height;
		var pixels = new uint[w * h];
		unsafe {
			fixed(uint* pixelPtr = pixels) {
				DebugApi.GetTilemap(
					CpuType.Snes, tilemapOptions, ppuState, ppuToolsState,
					vram, rgbPalette, (IntPtr)pixelPtr);
			}
		}

		JsonObject result = RenderArgbToPng(pixels, w, h, 1, format, "screenshot_snes_mcp");
		result["unique_colors"] = CountUniqueColors(pixels);
		result["source"] = "snes_debugger_renderer";
		result["source_view"] = "main";
		return result;
	}

	private static int CountUniqueColors(uint[] pixels)
	{
		var seen = new HashSet<uint>();
		foreach(uint p in pixels) {
			seen.Add(p | 0xFF000000);
		}
		return seen.Count;
	}

	public static JsonNode CropScreenshot(JsonNode? args)
	{
		if(args == null) throw new McpException(-32602, "crop_screenshot requires arguments");
		int x = (int)RequireUInt(args, "x");
		int y = (int)RequireUInt(args, "y");
		int w = (int)RequireUInt(args, "width");
		int h = (int)RequireUInt(args, "height");
		if(w <= 0 || h <= 0) throw new McpException(-32602, "width and height must be > 0");
		string format = (args["format"]?.GetValue<string>() ?? "path").ToLowerInvariant();

		// Reuse the regular screenshot path so we get the same async-flush
		// behaviour, then crop the resulting PNG.
		var raw = (JsonObject)TakeScreenshot(new JsonObject { ["format"] = "path" });
		string srcPath = raw["path"]!.GetValue<string>();

		using var src = SkiaSharp.SKBitmap.Decode(srcPath);
		if(src == null) {
			throw new McpException(-32603, "failed to decode screenshot: " + srcPath);
		}
		// Clamp the crop rect to the image bounds rather than throwing —
		// callers usually know the SNES is 256x224 but small mistakes are
		// cheap to fix on the way out.
		int clampW = Math.Min(w, src.Width - x);
		int clampH = Math.Min(h, src.Height - y);
		if(clampW <= 0 || clampH <= 0) {
			throw new McpException(-32602, $"crop region ({x},{y},{w}x{h}) is outside the {src.Width}x{src.Height} screenshot");
		}

		using var dst = new SkiaSharp.SKBitmap(clampW, clampH);
		using(var canvas = new SkiaSharp.SKCanvas(dst)) {
			canvas.DrawBitmap(src,
				new SkiaSharp.SKRect(x, y, x + clampW, y + clampH),
				new SkiaSharp.SKRect(0, 0, clampW, clampH));
		}

		string cropPath = System.IO.Path.ChangeExtension(srcPath, ".crop.png");
		using(var stream = System.IO.File.OpenWrite(cropPath))
		using(var img = SkiaSharp.SKImage.FromBitmap(dst))
		using(var data = img.Encode(SkiaSharp.SKEncodedImageFormat.Png, 100)) {
			data.SaveTo(stream);
		}

		var result = new JsonObject {
			["path"] = cropPath,
			["width"] = clampW,
			["height"] = clampH,
		};
		if(format == "base64") {
			byte[] bytes = System.IO.File.ReadAllBytes(cropPath);
			result["base64"] = Convert.ToBase64String(bytes);
			result["bytes"] = bytes.Length;
		}
		return result;
	}

	public static JsonNode RenderTilemap(JsonNode? args)
	{
		// SNES layer index is 0-based internally. User-facing the tool accepts:
		//   1..4  → BG1..BG4 (mapped to internal 0..3)
		//   5     → "Main" composite (Nexen tab; Layer=4)
		//   6     → "Sub" composite (Layer=5)
		// This matches the dropdown ordering in Nexen's TilemapViewer for SNES.
		int layer = (int)((args?["layer"]?.GetValue<long>()) ?? 1) - 1;
		if(layer < 0 || layer > 5) {
			throw new McpException(-32602, "layer must be 1..6 (1..4=BG, 5=Main, 6=Sub)");
		}
		int scale = (int)((args?["scale"]?.GetValue<long>()) ?? 1);
		if(scale < 1 || scale > 8) {
			throw new McpException(-32602, "scale must be 1..8");
		}
		string format = (args?["format"]?.GetValue<string>() ?? "path").ToLowerInvariant();

		// Pull the same inputs Nexen's own TilemapViewerViewModel uses.
		BaseState ppuState = DebugApi.GetPpuState(CpuType.Snes);
		BaseState ppuToolsState = DebugApi.GetPpuToolsState(CpuType.Snes);
		byte[] vram = DebugApi.GetMemoryState(MemoryType.SnesVideoRam);
		DebugPaletteInfo paletteInfo = DebugApi.GetPaletteInfo(CpuType.Snes);
		UInt32[] rgbPalette = paletteInfo.GetRgbPalette();

		var options = new GetTilemapOptions {
			Layer = (byte)layer,
		};
		FrameInfo size = DebugApi.GetTilemapSize(CpuType.Snes, options, ppuState);
		if(size.Width == 0 || size.Height == 0) {
			throw new McpException(-32603,
				$"layer {layer + 1} not currently rendering (BG mode mismatch?). "
				+ "Check get_ppu_state for the active BG mode.");
		}

		int w = (int)size.Width;
		int h = (int)size.Height;
		int pixelCount = w * h;

		// Allocate ARGB framebuffer that GetTilemap will fill.
		var pixels = new uint[pixelCount];
		DebugTilemapInfo info;
		unsafe {
			fixed(uint* pixelPtr = pixels) {
				info = DebugApi.GetTilemap(
					CpuType.Snes, options, ppuState, ppuToolsState,
					vram, rgbPalette, (IntPtr)pixelPtr);
			}
		}

		// Encode to PNG via Skia. ARGB → BGRA byte order for SKBitmap.
		using var bmp = new SkiaSharp.SKBitmap(new SkiaSharp.SKImageInfo(
			w, h, SkiaSharp.SKColorType.Bgra8888, SkiaSharp.SKAlphaType.Opaque));
		unsafe {
			byte* dst = (byte*)bmp.GetPixels().ToPointer();
			for(int i = 0; i < pixelCount; i++) {
				uint p = pixels[i];
				// GetTilemap output is 0xAARRGGBB; SKBitmap Bgra8888 wants
				// B,G,R,A in memory order.
				dst[i * 4 + 0] = (byte)(p & 0xFF);          // B
				dst[i * 4 + 1] = (byte)((p >> 8) & 0xFF);   // G
				dst[i * 4 + 2] = (byte)((p >> 16) & 0xFF);  // R
				dst[i * 4 + 3] = 0xFF;                       // A (force opaque)
			}
		}

		// Optional integer upscale.
		SkiaSharp.SKBitmap final = bmp;
		if(scale > 1) {
			final = new SkiaSharp.SKBitmap(new SkiaSharp.SKImageInfo(
				w * scale, h * scale, SkiaSharp.SKColorType.Bgra8888, SkiaSharp.SKAlphaType.Opaque));
			using var canvas = new SkiaSharp.SKCanvas(final);
			canvas.DrawBitmap(bmp, new SkiaSharp.SKRect(0, 0, w * scale, h * scale),
				new SkiaSharp.SKPaint { FilterQuality = SkiaSharp.SKFilterQuality.None });
		}

		// Save next to the screenshot folder so screenshots + tilemap dumps
		// land in the same directory for easy comparison.
		string dir = GetMcpCaptureFolder();
		string path = System.IO.Path.Combine(dir,
			$"tilemap_L{layer + 1}_{DateTime.Now:yyyyMMdd_HHmmss_fff}.png");
		try { System.IO.Directory.CreateDirectory(dir); } catch { }
		using(var stream = System.IO.File.OpenWrite(path))
		using(var img = SkiaSharp.SKImage.FromBitmap(final))
		using(var data = img.Encode(SkiaSharp.SKEncodedImageFormat.Png, 100)) {
			data.SaveTo(stream);
		}
		if(scale > 1) {
			final.Dispose();
		}

		var result = new JsonObject {
			["path"] = path,
			["layer"] = layer + 1,
			["width"] = w * scale,
			["height"] = h * scale,
			["sourceWidth"] = w,
			["sourceHeight"] = h,
			["scale"] = scale,
			["bpp"] = info.Bpp,
			["tileWidth"] = (int)info.TileWidth,
			["tileHeight"] = (int)info.TileHeight,
			["tilemapAddress"] = info.TilemapAddress,
			["tilesetAddress"] = info.TilesetAddress,
		};
		if(format == "base64") {
			byte[] bytes = System.IO.File.ReadAllBytes(path);
			result["base64"] = Convert.ToBase64String(bytes);
			result["bytes"] = bytes.Length;
		}
		return result;
	}

	// Render an ARGB pixel buffer to a PNG and return a result object with
	// path + dimensions (and optional base64). Centralises the SkiaSharp
	// boilerplate that render_tilemap / render_tile_sheet / render_oam share.
	private static JsonObject RenderArgbToPng(uint[] pixels, int w, int h, int scale, string format, string filenamePrefix)
	{
		using var bmp = new SkiaSharp.SKBitmap(new SkiaSharp.SKImageInfo(
			w, h, SkiaSharp.SKColorType.Bgra8888, SkiaSharp.SKAlphaType.Premul));
		unsafe {
			byte* dst = (byte*)bmp.GetPixels().ToPointer();
			int pixelCount = w * h;
			for(int i = 0; i < pixelCount; i++) {
				uint p = pixels[i];
				dst[i * 4 + 0] = (byte)(p & 0xFF);          // B
				dst[i * 4 + 1] = (byte)((p >> 8) & 0xFF);   // G
				dst[i * 4 + 2] = (byte)((p >> 16) & 0xFF);  // R
				dst[i * 4 + 3] = (byte)((p >> 24) & 0xFF);  // A
			}
		}

		SkiaSharp.SKBitmap final = bmp;
		bool finalOwned = false;
		if(scale > 1) {
			final = new SkiaSharp.SKBitmap(new SkiaSharp.SKImageInfo(
				w * scale, h * scale, SkiaSharp.SKColorType.Bgra8888, SkiaSharp.SKAlphaType.Premul));
			finalOwned = true;
			using var canvas = new SkiaSharp.SKCanvas(final);
			canvas.DrawBitmap(bmp, new SkiaSharp.SKRect(0, 0, w * scale, h * scale),
				new SkiaSharp.SKPaint { FilterQuality = SkiaSharp.SKFilterQuality.None });
		}

		string dir = GetMcpCaptureFolder();
		string path = System.IO.Path.Combine(dir,
			$"{filenamePrefix}_{DateTime.Now:yyyyMMdd_HHmmss_fff}.png");
		try { System.IO.Directory.CreateDirectory(dir); } catch { }
		using(var stream = System.IO.File.OpenWrite(path))
		using(var img = SkiaSharp.SKImage.FromBitmap(final))
		using(var data = img.Encode(SkiaSharp.SKEncodedImageFormat.Png, 100)) {
			data.SaveTo(stream);
		}
		if(finalOwned) {
			final.Dispose();
		}

		var result = new JsonObject {
			["path"] = path,
			["width"] = w * scale,
			["height"] = h * scale,
			["sourceWidth"] = w,
			["sourceHeight"] = h,
			["scale"] = scale,
		};
		if(format == "base64") {
			byte[] bytes = System.IO.File.ReadAllBytes(path);
			result["base64"] = Convert.ToBase64String(bytes);
			result["bytes"] = bytes.Length;
		}
		return result;
	}

	public static JsonNode RenderTileSheet(JsonNode? args)
	{
		int address = (int)((args?["address"]?.GetValue<long>()) ?? 0);
		int count = (int)((args?["count"]?.GetValue<long>()) ?? 256);
		int bpp = (int)((args?["bpp"]?.GetValue<long>()) ?? 4);
		int paletteIdx = (int)((args?["palette"]?.GetValue<long>()) ?? 0);
		int columns = (int)((args?["columns"]?.GetValue<long>()) ?? 16);
		int scale = (int)((args?["scale"]?.GetValue<long>()) ?? 1);
		string format = (args?["format"]?.GetValue<string>() ?? "path").ToLowerInvariant();

		if(count < 1 || count > 1024) {
			throw new McpException(-32602, "count must be 1..1024");
		}
		if(columns < 1 || columns > 64) {
			throw new McpException(-32602, "columns must be 1..64");
		}
		if(scale < 1 || scale > 8) {
			throw new McpException(-32602, "scale must be 1..8");
		}
		TileFormat tileFormat = bpp switch {
			2 => TileFormat.Bpp2,
			4 => TileFormat.Bpp4,
			8 => TileFormat.Bpp8,
			_ => throw new McpException(-32602, "bpp must be 2, 4, or 8"),
		};

		int rows = (count + columns - 1) / columns;
		int bytesPerTile = bpp * 8;  // 8x8 tile, bpp bits per pixel
		int totalSize = bytesPerTile * count;

		// Source bytes for the tile region.
		byte[] source = new byte[totalSize];
		DebugApi.GetMemoryValues(MemoryType.SnesVideoRam,
			(uint)address, (uint)(address + totalSize - 1), ref source);

		// Palette for the chosen format.
		DebugPaletteInfo paletteInfo = DebugApi.GetPaletteInfo(
			CpuType.Snes, new GetPaletteInfoOptions { Format = tileFormat });
		UInt32[] rgbPalette = paletteInfo.GetRgbPalette();

		var options = new GetTileViewOptions {
			MemType = MemoryType.SnesVideoRam,
			Format = tileFormat,
			Layout = TileLayout.Normal,
			Filter = TileFilter.None,
			Background = TileBackground.Default,
			Width = columns,
			Height = rows,
			StartAddress = 0,            // we already sliced into `source` at `address`
			Palette = paletteIdx,
			UseGrayscalePalette = false,
		};

		int outW = columns * 8;
		int outH = rows * 8;
		var pixels = new uint[outW * outH];
		unsafe {
			fixed(uint* pixelPtr = pixels) {
				DebugApi.GetTileView(CpuType.Snes, options, source, source.Length,
					rgbPalette, (IntPtr)pixelPtr);
			}
		}

		var result = RenderArgbToPng(pixels, outW, outH, scale, format,
			$"tilesheet_{address:X4}_bpp{bpp}_p{paletteIdx}");
		result["address"] = address;
		result["count"] = count;
		result["bpp"] = bpp;
		result["palette"] = paletteIdx;
		result["columns"] = columns;
		result["rows"] = rows;
		return result;
	}

	public static JsonNode RenderOam(JsonNode? args)
	{
		string mode = (args?["mode"]?.GetValue<string>() ?? "positioned").ToLowerInvariant();
		int scale = (int)((args?["scale"]?.GetValue<long>()) ?? 1);
		string format = (args?["format"]?.GetValue<string>() ?? "path").ToLowerInvariant();
		if(scale < 1 || scale > 8) {
			throw new McpException(-32602, "scale must be 1..8");
		}

		BaseState ppuState = DebugApi.GetPpuState(CpuType.Snes);
		BaseState ppuToolsState = DebugApi.GetPpuToolsState(CpuType.Snes);
		byte[] vram = DebugApi.GetMemoryState(MemoryType.SnesVideoRam);
		byte[] oam = DebugApi.GetMemoryState(MemoryType.SnesSpriteRam);
		DebugPaletteInfo paletteInfo = DebugApi.GetPaletteInfo(CpuType.Snes);
		UInt32[] rgbPalette = paletteInfo.GetRgbPalette();

		var options = new GetSpritePreviewOptions {
			Background = SpriteBackground.Transparent,
		};
		DebugSpritePreviewInfo info = DebugApi.GetSpritePreviewInfo(
			CpuType.Snes, options, ppuState, ppuToolsState);
		int w = (int)info.Width;
		int h = (int)info.Height;
		if(w == 0 || h == 0) {
			throw new McpException(-32603, "sprite preview canvas is empty (no PPU state?)");
		}

		// GetSpriteList writes into the full screenPreview canvas (positioned
		// mode). 'sheet' mode would need to lay out _spritePreviews tiles in a
		// grid — not yet wired; fall back to positioned for now and document.
		if(mode != "positioned" && mode != "sheet") {
			throw new McpException(-32602, "mode must be 'positioned' or 'sheet'");
		}

		var sprites = new DebugSpriteInfo[128];
		var spritePreviews = new uint[64 * 64 * 128];  // upper-bound: 64x64 max sprite x 128
		var pixels = new uint[w * h];
		unsafe {
			fixed(uint* canvasPtr = pixels) {
				DebugApi.GetSpriteList(ref sprites, ref spritePreviews, CpuType.Snes,
					options, ppuState, ppuToolsState, vram, oam, rgbPalette, (IntPtr)canvasPtr);
			}
		}

		if(mode == "sheet") {
			// Lay out the per-sprite previews in a 16x8 grid using the largest
			// observed sprite size. SNES sprites are 8x8, 16x16, 32x32, 64x64;
			// taking max from sprites[].Width/Height gives the cell pitch.
			int cellW = 16, cellH = 16;
			for(int i = 0; i < sprites.Length; i++) {
				if(sprites[i].Width > cellW) cellW = (int)sprites[i].Width;
				if(sprites[i].Height > cellH) cellH = (int)sprites[i].Height;
			}
			int gridCols = 16;
			int gridRows = (128 + gridCols - 1) / gridCols;
			int sheetW = cellW * gridCols;
			int sheetH = cellH * gridRows;
			var sheet = new uint[sheetW * sheetH];
			// Each sprite preview is laid out cellW x cellH in spritePreviews,
			// 64x64 stride per sprite slot. Copy slot[i] into grid cell (i%cols, i/cols).
			for(int i = 0; i < 128; i++) {
				int gx = (i % gridCols) * cellW;
				int gy = (i / gridCols) * cellH;
				int spriteW = (int)sprites[i].Width;
				int spriteH = (int)sprites[i].Height;
				if(spriteW == 0 || spriteH == 0) continue;
				int srcBase = i * 64 * 64;
				for(int y = 0; y < spriteH && y < cellH; y++) {
					for(int x = 0; x < spriteW && x < cellW; x++) {
						sheet[(gy + y) * sheetW + (gx + x)] = spritePreviews[srcBase + y * 64 + x];
					}
				}
			}
			var result2 = RenderArgbToPng(sheet, sheetW, sheetH, scale, format, "oam_sheet");
			result2["mode"] = "sheet";
			result2["cellWidth"] = cellW;
			result2["cellHeight"] = cellH;
			result2["spriteCount"] = sprites.Length;
			return result2;
		}

		var result = RenderArgbToPng(pixels, w, h, scale, format, "oam_positioned");
		result["mode"] = "positioned";
		result["spriteCount"] = sprites.Length;
		return result;
	}

	public static JsonNode RenderPalette(JsonNode? args)
	{
		string mode = (args?["mode"]?.GetValue<string>() ?? "grid").ToLowerInvariant();
		int swatch = (int)((args?["swatch"]?.GetValue<long>()) ?? 16);
		int? highlight = null;
		if(args?["highlight"] is JsonNode hl) {
			int h = (int)hl.GetValue<long>();
			if(h < 0 || h > 255) {
				throw new McpException(-32602, "highlight must be 0..255");
			}
			highlight = h;
		}
		string format = (args?["format"]?.GetValue<string>() ?? "path").ToLowerInvariant();
		if(swatch < 4 || swatch > 64) {
			throw new McpException(-32602, "swatch must be 4..64");
		}
		if(mode != "grid" && mode != "strip") {
			throw new McpException(-32602, "mode must be 'grid' or 'strip'");
		}

		DebugPaletteInfo paletteInfo = DebugApi.GetPaletteInfo(CpuType.Snes);
		UInt32[] rgb = paletteInfo.GetRgbPalette();
		// SNES gives us 256 colors but the helper handles arbitrary lengths.
		int total = (int)Math.Min(rgb.Length, 256);

		int cols, rows;
		if(mode == "grid") {
			cols = 16;
			rows = (total + 15) / 16;
		} else {
			cols = total;
			rows = 1;
		}
		int outW = cols * swatch;
		int outH = rows * swatch;
		var pixels = new uint[outW * outH];
		for(int idx = 0; idx < total; idx++) {
			int cx = idx % cols;
			int cy = idx / cols;
			uint rgba = 0xFF000000u | (rgb[idx] & 0x00FFFFFFu);  // force opaque
			int x0 = cx * swatch, y0 = cy * swatch;
			for(int y = y0; y < y0 + swatch; y++) {
				int rowBase = y * outW;
				for(int x = x0; x < x0 + swatch; x++) {
					pixels[rowBase + x] = rgba;
				}
			}
		}
		// Optional red outline on highlight cell — 1px border, drawn last.
		if(highlight is int hi && hi < total) {
			int cx = hi % cols;
			int cy = hi / cols;
			int x0 = cx * swatch, y0 = cy * swatch;
			int x1 = x0 + swatch - 1, y1 = y0 + swatch - 1;
			const uint red = 0xFFFF0000u;
			for(int x = x0; x <= x1; x++) {
				pixels[y0 * outW + x] = red;
				pixels[y1 * outW + x] = red;
			}
			for(int y = y0; y <= y1; y++) {
				pixels[y * outW + x0] = red;
				pixels[y * outW + x1] = red;
			}
		}

		var result = RenderArgbToPng(pixels, outW, outH, 1, format, $"palette_{mode}");
		result["mode"] = mode;
		result["swatch"] = swatch;
		result["colorCount"] = total;
		result["columns"] = cols;
		result["rows"] = rows;
		if(highlight.HasValue) result["highlight"] = highlight.Value;
		return result;
	}

	public static JsonNode RenderFilmstrip(JsonNode? args)
	{
		if(args == null) throw new McpException(-32602, "render_filmstrip requires arguments");
		int count = (int)RequireUInt(args, "count");
		int frameStep = (int)((args["frameStep"]?.GetValue<long>()) ?? 1);
		int columns = (int)((args["columns"]?.GetValue<long>()) ?? 0);  // 0 = single horizontal strip
		int scale = (int)((args["scale"]?.GetValue<long>()) ?? 1);
		bool label = (args["label"]?.GetValue<bool?>()) ?? true;
		string format = (args["format"]?.GetValue<string>() ?? "path").ToLowerInvariant();

		if(count < 2 || count > 64) throw new McpException(-32602, "count must be 2..64");
		if(frameStep < 1 || frameStep > 600) throw new McpException(-32602, "frameStep must be 1..600");
		if(scale < 1 || scale > 4) throw new McpException(-32602, "scale must be 1..4");
		if(columns < 0 || columns > count) throw new McpException(-32602, "columns must be 0..count");

		// Capture each frame: take screenshot, advance frameStep, repeat.
		// We use the same TakeScreenshot path so timing/sync matches the rest
		// of the toolchain. Each shot's path goes into a list; we re-decode +
		// stitch at the end.
		var frames = new List<(string path, uint frameNum)>();
		for(int i = 0; i < count; i++) {
			uint nowFrame = EmuApi.GetFrameCount();
			var shot = (JsonObject)TakeScreenshot(new JsonObject { ["format"] = "path" });
			frames.Add((shot["path"]!.GetValue<string>(), nowFrame));
			if(i < count - 1) {
				// Advance frameStep frames before the next shot. Use the same
				// poll loop run_frames uses so we stay deterministic.
				uint target = nowFrame + (uint)frameStep;
				EmuApi.Resume();
				var sw = System.Diagnostics.Stopwatch.StartNew();
				long maxMs = (long)(frameStep * (1000.0 / 60.0) * 2.0 + 200);
				while(sw.ElapsedMilliseconds < maxMs && EmuApi.GetFrameCount() < target) {
					System.Threading.Thread.Sleep(5);
				}
				EmuApi.Pause();
			}
		}

		// Decode each frame, then stitch them into a single canvas.
		var bitmaps = new SkiaSharp.SKBitmap[count];
		try {
			for(int i = 0; i < count; i++) {
				var bmp = SkiaSharp.SKBitmap.Decode(frames[i].path)
					?? throw new McpException(-32603, "failed to decode frame: " + frames[i].path);
				bitmaps[i] = bmp;
			}
			int cellW = bitmaps[0].Width * scale;
			int cellH = bitmaps[0].Height * scale;
			int gridCols = columns > 0 ? columns : count;
			int gridRows = (count + gridCols - 1) / gridCols;
			int outW = gridCols * cellW;
			int outH = gridRows * cellH;

			using var canvas = new SkiaSharp.SKBitmap(outW, outH);
			using(var g = new SkiaSharp.SKCanvas(canvas)) {
				g.Clear(SkiaSharp.SKColors.Black);
				using var paint = new SkiaSharp.SKPaint {
					FilterQuality = SkiaSharp.SKFilterQuality.None,
					IsAntialias = false,
				};
				using var labelPaint = new SkiaSharp.SKPaint {
					Color = SkiaSharp.SKColors.Yellow,
					TextSize = Math.Max(10, cellH / 18),
					IsAntialias = true,
				};
				using var labelStroke = new SkiaSharp.SKPaint {
					Color = SkiaSharp.SKColors.Black,
					TextSize = labelPaint.TextSize,
					IsAntialias = true,
					Style = SkiaSharp.SKPaintStyle.Stroke,
					StrokeWidth = 2,
				};
				for(int i = 0; i < count; i++) {
					int cx = (i % gridCols) * cellW;
					int cy = (i / gridCols) * cellH;
					g.DrawBitmap(bitmaps[i],
						new SkiaSharp.SKRect(cx, cy, cx + cellW, cy + cellH), paint);
					if(label) {
						string txt = $"f{frames[i].frameNum}";
						float lx = cx + 4;
						float ly = cy + labelPaint.TextSize + 2;
						g.DrawText(txt, lx, ly, labelStroke);
						g.DrawText(txt, lx, ly, labelPaint);
					}
				}
			}

			string dir = GetMcpCaptureFolder();
			string outPath = System.IO.Path.Combine(dir,
				$"filmstrip_{count}x_step{frameStep}_{DateTime.Now:yyyyMMdd_HHmmss_fff}.png");
			try { System.IO.Directory.CreateDirectory(dir); } catch { }
			using(var stream = System.IO.File.OpenWrite(outPath))
			using(var img = SkiaSharp.SKImage.FromBitmap(canvas))
			using(var data = img.Encode(SkiaSharp.SKEncodedImageFormat.Png, 100)) {
				data.SaveTo(stream);
			}

			var frameList = new JsonArray();
			foreach(var f in frames) frameList.Add(new JsonObject {
				["frame"] = f.frameNum,
				["path"] = f.path,
			});
			var result = new JsonObject {
				["path"] = outPath,
				["count"] = count,
				["frameStep"] = frameStep,
				["columns"] = gridCols,
				["rows"] = gridRows,
				["width"] = outW,
				["height"] = outH,
				["scale"] = scale,
				["frames"] = frameList,
			};
			if(format == "base64") {
				byte[] bytes = System.IO.File.ReadAllBytes(outPath);
				result["base64"] = Convert.ToBase64String(bytes);
				result["bytes"] = bytes.Length;
			}
			return result;
		} finally {
			for(int i = 0; i < count; i++) bitmaps[i]?.Dispose();
		}
	}

	public static JsonNode MemoryDiff(JsonNode? args)
	{
		if(args == null) throw new McpException(-32602, "memory_diff requires arguments");
		var regionsNode = args["regions"] as JsonArray
			?? throw new McpException(-32602, "regions must be an array");
		int frames = (int)RequireUInt(args, "frames");
		int maxChanges = (int)((args["maxChanges"]?.GetValue<long>()) ?? 256);
		if(frames < 1 || frames > 1_000_000) {
			throw new McpException(-32602, "frames must be 1..1000000");
		}
		if(maxChanges < 1 || maxChanges > 65536) {
			throw new McpException(-32602, "maxChanges must be 1..65536");
		}

		// Parse + validate regions up-front so a bad request fails before
		// we burn frames on the snapshot.
		var regions = new List<(MemoryType mem, uint addr, int len)>();
		foreach(JsonNode? rn in regionsNode) {
			if(rn is not JsonObject ro) {
				throw new McpException(-32602, "regions[] must contain objects");
			}
			string mtStr = ro["memoryType"]?.GetValue<string>()
				?? throw new McpException(-32602, "region.memoryType is required");
			if(!Enum.TryParse<MemoryType>(mtStr, ignoreCase: true, out var mt)) {
				throw new McpException(-32602, "unknown memoryType: " + mtStr);
			}
			uint addr = (uint)RequireUInt(ro, "address");
			int len = (int)RequireUInt(ro, "length");
			if(len < 1 || len > 0x10000) {
				throw new McpException(-32602, "region.length must be 1..65536");
			}
			regions.Add((mt, addr, len));
		}

		// Snapshot before. read_memory's auto-routing of $7E/$7F to snesWorkRam
		// belongs in the read tool; here, the caller picks memoryType so we
		// don't second-guess.
		var before = new byte[regions.Count][];
		for(int i = 0; i < regions.Count; i++) {
			var (mt, addr, len) = regions[i];
			var buf = new byte[len];
			DebugApi.GetMemoryValues(mt, addr, addr + (uint)len - 1, ref buf);
			before[i] = buf;
		}

		// Advance N frames (run_frames-style poll loop).
		uint startFrame = EmuApi.GetFrameCount();
		uint targetFrame = startFrame + (uint)frames;
		EmuApi.Resume();
		var sw = System.Diagnostics.Stopwatch.StartNew();
		long maxMs = (long)(frames * (1000.0 / 60.0) * 2.0 + 200);
		while(sw.ElapsedMilliseconds < maxMs && EmuApi.GetFrameCount() < targetFrame) {
			System.Threading.Thread.Sleep(5);
		}
		EmuApi.Pause();

		// Snapshot after + diff per region.
		int totalChanges = 0;
		var perRegion = new JsonArray();
		for(int i = 0; i < regions.Count; i++) {
			var (mt, addr, len) = regions[i];
			var after = new byte[len];
			DebugApi.GetMemoryValues(mt, addr, addr + (uint)len - 1, ref after);
			var changes = new JsonArray();
			int regionChanged = 0;
			for(int off = 0; off < len; off++) {
				if(before[i][off] == after[off]) continue;
				regionChanged++;
				totalChanges++;
				if(changes.Count < maxChanges) {
					changes.Add(new JsonObject {
						["address"] = addr + (uint)off,
						["oldValue"] = before[i][off],
						["newValue"] = after[off],
					});
				}
			}
			perRegion.Add(new JsonObject {
				["memoryType"] = mt.ToString(),
				["address"] = addr,
				["length"] = len,
				["changedCount"] = regionChanged,
				["truncated"] = regionChanged > maxChanges,
				["changes"] = changes,
			});
		}

		return new JsonObject {
			["frames"] = frames,
			["startFrame"] = startFrame,
			["endFrame"] = EmuApi.GetFrameCount(),
			["totalChanges"] = totalChanges,
			["regions"] = perRegion,
		};
	}

	private static readonly Dictionary<string, (DateTime mtime, PansyReader reader)> _pansyCache
		= new(StringComparer.OrdinalIgnoreCase);
	private static readonly object _pansyCacheLock = new();

	private static PansyReader LoadPansy(string path)
	{
		var info = new System.IO.FileInfo(path);
		if(!info.Exists) {
			throw new McpException(-32602, "pansy file not found: " + path);
		}
		lock(_pansyCacheLock) {
			if(_pansyCache.TryGetValue(path, out var cached) && cached.mtime == info.LastWriteTimeUtc) {
				return cached.reader;
			}
			byte[] bytes = System.IO.File.ReadAllBytes(path);
			PansyReader reader;
			try {
				reader = PansyReader.Load(bytes);
			} catch(Exception ex) {
				throw new McpException(-32602, "pansy parse failed: " + ex.Message);
			}
			_pansyCache[path] = (info.LastWriteTimeUtc, reader);
			return reader;
		}
	}

	public static JsonNode LookupPansy(JsonNode? args)
	{
		if(args == null) throw new McpException(-32602, "lookup_pansy requires arguments");
		string path = RequireString(args, "pansyFile");
		string pattern = args["pattern"]?.GetValue<string>() ?? "";
		int maxResults = (int)((args["maxResults"]?.GetValue<long>()) ?? 256);
		bool sectionsOnly = args["sectionsOnly"]?.GetValue<bool?>() ?? false;
		if(maxResults < 1 || maxResults > 65536) {
			throw new McpException(-32602, "maxResults must be 1..65536");
		}
		System.Text.RegularExpressions.Regex? rx = null;
		if(pattern.Length > 0) {
			try {
				rx = new System.Text.RegularExpressions.Regex(pattern);
			} catch(Exception ex) {
				throw new McpException(-32602, "invalid regex: " + ex.Message);
			}
		}

		PansyReader reader = LoadPansy(path);

		var sectionTable = new JsonArray();
		foreach(var s in reader.Sections) {
			sectionTable.Add(new JsonObject {
				["type"] = s.Type,
				["typeName"] = SectionTypeName(s.Type),
				["offset"] = s.Offset,
				["compressedSize"] = s.CompressedSize,
				["uncompressedSize"] = s.UncompressedSize,
			});
		}

		var result = new JsonObject {
			["path"] = path,
			["version"] = reader.Version,
			["flags"] = reader.Flags,
			["compressed"] = reader.IsCompressed,
			["platform"] = reader.Platform,
			["romSize"] = reader.RomSize,
			["romCrc32"] = reader.RomCrc32,
			["sectionCount"] = reader.Sections.Count,
			["sections"] = sectionTable,
			["symbolCount"] = reader.Symbols.Count,
			["commentCount"] = reader.Comments.Count,
			["memoryRegionCount"] = reader.MemoryRegions.Count,
		};

		if(sectionsOnly) {
			return result;
		}

		var symbols = new JsonArray();
		int symMatched = 0;
		foreach(var s in reader.Symbols) {
			if(rx != null && !rx.IsMatch(s.Name)) continue;
			if(symMatched++ >= maxResults) break;
			symbols.Add(new JsonObject {
				["address"] = s.Address,
				["type"] = s.Type,
				["typeName"] = SymbolTypeName(s.Type),
				["flags"] = s.Flags,
				["name"] = s.Name,
			});
		}

		var comments = new JsonArray();
		int comMatched = 0;
		foreach(var c in reader.Comments) {
			if(rx != null && !rx.IsMatch(c.Text)) continue;
			if(comMatched++ >= maxResults) break;
			comments.Add(new JsonObject {
				["address"] = c.Address,
				["type"] = c.Type,
				["text"] = c.Text,
			});
		}

		var regions = new JsonArray();
		int regMatched = 0;
		foreach(var r in reader.MemoryRegions) {
			if(rx != null && !rx.IsMatch(r.Name)) continue;
			if(regMatched++ >= maxResults) break;
			regions.Add(new JsonObject {
				["startAddress"] = r.StartAddress,
				["endAddress"] = r.EndAddress,
				["type"] = r.Type,
				["bank"] = r.Bank,
				["name"] = r.Name,
			});
		}

		result["symbols"] = symbols;
		result["comments"] = comments;
		result["memoryRegions"] = regions;
		result["truncated"] = new JsonObject {
			["symbols"] = symMatched > maxResults,
			["comments"] = comMatched > maxResults,
			["memoryRegions"] = regMatched > maxResults,
		};
		return result;
	}

	private static string SectionTypeName(uint type) => type switch {
		0x0001 => "CODE_DATA_MAP",
		0x0002 => "SYMBOLS",
		0x0003 => "COMMENTS",
		0x0004 => "MEMORY_REGIONS",
		0x0005 => "DATA_TYPES",
		0x0006 => "CROSS_REFS",
		0x0007 => "SOURCE_MAP",
		0x0008 => "METADATA",
		0x0009 => "CPU_STATE",
		0x000a => "BOOKMARKS",
		0x000b => "MULTI_TARGET_CROSS_REFS",
		_ => $"UNKNOWN_0x{type:X4}",
	};

	private static string SymbolTypeName(byte type) => type switch {
		1 => "LABEL",
		2 => "CONSTANT",
		3 => "ENUM",
		4 => "STRUCT",
		5 => "MACRO",
		6 => "LOCAL",
		7 => "ANONYMOUS",
		8 => "INTERRUPT_VECTOR",
		9 => "FUNCTION",
		_ => $"UNKNOWN_{type}",
	};

	public static JsonNode SymbolicDump(JsonNode? args)
	{
		if(args == null) throw new McpException(-32602, "symbolic_dump requires arguments");
		string symFile = RequireString(args, "symFile");
		uint addr = RequireUInt(args, "address");
		int length = (int)RequireUInt(args, "length");
		string unitStr = (args["unit"]?.GetValue<string>() ?? "byte").ToLowerInvariant();
		int maxDistance = (int)((args["maxDistance"]?.GetValue<long>()) ?? 64);

		int stride = unitStr switch {
			"byte" => 1,
			"word" => 2,
			"long" => 4,
			_ => throw new McpException(-32602, "unit must be 'byte', 'word', or 'long'"),
		};
		if(length < 1 || length > 4096) {
			throw new McpException(-32602, "length must be 1..4096 (units, not bytes)");
		}
		if(maxDistance < 0 || maxDistance > 65536) {
			throw new McpException(-32602, "maxDistance must be 0..65536");
		}

		// LoadSymFile returns one row per symbol; build a sorted-by-address
		// view so we can binary-search for the floor symbol per query.
		var rows = LoadSymFile(symFile);
		var sorted = new (string name, uint addr)[rows.Count];
		for(int i = 0; i < rows.Count; i++) sorted[i] = rows[i];
		Array.Sort(sorted, (a, b) => a.addr.CompareTo(b.addr));
		var sortedAddrs = new uint[sorted.Length];
		for(int i = 0; i < sorted.Length; i++) sortedAddrs[i] = sorted[i].addr;

		var entries = new JsonArray();
		for(int i = 0; i < length; i++) {
			uint cur = addr + (uint)(i * stride);
			// floor index = largest sortedAddrs[j] <= cur
			int lo = 0, hi = sortedAddrs.Length - 1, idx = -1;
			while(lo <= hi) {
				int mid = (lo + hi) >> 1;
				if(sortedAddrs[mid] <= cur) { idx = mid; lo = mid + 1; }
				else hi = mid - 1;
			}
			if(idx < 0) continue;
			long dist = (long)cur - (long)sortedAddrs[idx];
			if(dist > maxDistance) continue;
			entries.Add(new JsonObject {
				["address"] = cur,
				["symbol"] = sorted[idx].name,
				["symbolAddress"] = sortedAddrs[idx],
				["offset"] = (uint)dist,
			});
		}

		return new JsonObject {
			["symFile"] = symFile,
			["address"] = addr,
			["length"] = length,
			["unit"] = unitStr,
			["stride"] = stride,
			["symbolCount"] = sorted.Length,
			["matched"] = entries.Count,
			["entries"] = entries,
		};
	}

	// Track which CPU types we've already enabled the trace logger for, so
	// we don't burn the SetTraceOptions overhead on every trace_log call.
	private static readonly HashSet<CpuType> _traceLoggerEnabled = new();
	private static readonly object _traceLoggerLock = new();

	public static JsonNode TraceLog(JsonNode? args)
	{
		int count = (int)((args?["count"]?.GetValue<long>()) ?? 32);
		string cpuStr = args?["cpuType"]?.GetValue<string>() ?? "Snes";
		if(count < 1 || count > 30000) {
			throw new McpException(-32602, "count must be 1..30000");
		}
		if(!Enum.TryParse<CpuType>(cpuStr, ignoreCase: true, out var cpu)) {
			throw new McpException(-32602, "unknown cpuType: " + cpuStr);
		}

		// Enable trace logging for this CPU on first call. Use the same default
		// format Nexen's TraceLogger UI uses, so the output reads identically:
		//   "[Disassembly][Align,24] A:[A,4h] X:[X,4h] Y:[Y,4h] S:[SP,4h] D:[D,4h] DB:[DB,2h] P:[P,8] "
		lock(_traceLoggerLock) {
			if(!_traceLoggerEnabled.Contains(cpu)) {
				string formatStr = "[Disassembly][Align,24] A:[A,4h] X:[X,4h] Y:[Y,4h] S:[SP,4h] D:[D,4h] DB:[DB,2h] P:[P,8] ";
				var formatBytes = new byte[1000];
				byte[] fmtUtf8 = System.Text.Encoding.UTF8.GetBytes(formatStr);
				Array.Copy(fmtUtf8, formatBytes, Math.Min(fmtUtf8.Length, formatBytes.Length));
				var options = new InteropTraceLoggerOptions {
					Enabled = true,
					IndentCode = false,
					UseLabels = false,
					Condition = new byte[1000],
					Format = formatBytes,
				};
				DebugApi.SetTraceOptions(cpu, options);
				_traceLoggerEnabled.Add(cpu);
				// Trace logger only captures from the moment it's enabled.
				// Tell the caller their first call may be empty/short so they
				// can enable + run_frames + trace_log instead of expecting
				// pre-existing history.
			}
		}

		// GetExecutionTrace returns rows newest-first, and startOffset skips
		// rows from that newest end. Fetch from offset zero and filter while
		// retaining the newest `count` rows for the requested CPU. The trace
		// buffer interleaves all active CPUs, so skipping totalSize-count before
		// filtering can return the oldest segment and miss the fault boundary.
		uint totalSize = DebugApi.GetExecutionTraceSize();
		uint rowsToFetch = totalSize;

		var rows = new JsonArray();
		if(rowsToFetch > 0) {
			TraceRow[] fetched = DebugApi.GetExecutionTrace(0, rowsToFetch);
			// Filter to the requested CPU only — the buffer interleaves all active CPUs.
			foreach(TraceRow row in fetched) {
				if(row.Type != cpu) continue;
				rows.Add(new JsonObject {
					["pc"] = row.ProgramCounter,
					["bytes"] = row.GetByteCodeStr(),
					["text"] = row.GetOutput(),
				});
				if(rows.Count >= count) break;
			}
		}

		return new JsonObject {
			["cpuType"] = cpu.ToString(),
			["bufferSize"] = totalSize,
			["returned"] = rows.Count,
			["rows"] = rows,
		};
	}

	public static JsonNode WatchAddresses(JsonNode? args)
	{
		if(args == null) throw new McpException(-32602, "watch_addresses requires arguments");
		var addrsNode = args["addresses"] as JsonArray;
		if(addrsNode == null || addrsNode.Count == 0) {
			throw new McpException(-32602, "addresses must be a non-empty array");
		}
		int maxFrames = (int)((args["maxFrames"]?.GetValue<long>()) ?? 600);
		int maxEvents = (int)((args["maxEvents"]?.GetValue<long>()) ?? 256);
		if(maxFrames < 1 || maxFrames > 10000) throw new McpException(-32602, "maxFrames 1..10000");
		if(maxEvents < 1 || maxEvents > 4096) throw new McpException(-32602, "maxEvents 1..4096");

		// Parse the watch list. Each entry: {memoryType, address, name?}.
		int n = addrsNode.Count;
		var memTypes = new MemoryType[n];
		var addresses = new uint[n];
		var names = new string[n];
		for(int i = 0; i < n; i++) {
			var entry = addrsNode[i] as JsonObject ?? throw new McpException(-32602, "address entry must be object");
			string mtStr = entry["memoryType"]?.GetValue<string>() ?? throw new McpException(-32602, "missing memoryType");
			memTypes[i] = ParseMemoryType(mtStr);
			addresses[i] = (uint)(entry["address"]?.GetValue<long>() ?? throw new McpException(-32602, "missing address"));
			names[i] = entry["name"]?.GetValue<string>() ?? $"@{addresses[i]:X6}";
		}

		// Initial snapshot.
		var lastValues = new byte[n];
		for(int i = 0; i < n; i++) {
			lastValues[i] = DebugApi.GetMemoryValue(memTypes[i], addresses[i]);
		}

		var events = new JsonArray();
		int framesElapsed = 0;
		bool stop = false;
		object stopLock = new();

		// Hold a strong reference to the callback so the GC doesn't collect it
		// while the native side has the function pointer (NotificationListener
		// uses the same precaution — see comment at line 13 of that file).
		NotificationListener.NotificationCallback cb = (int type, IntPtr param) => {
			if(stop) return;
			if((ConsoleNotificationType)type != ConsoleNotificationType.PpuFrameDone) return;
			lock(stopLock) {
				if(stop) return;
				framesElapsed++;
				for(int i = 0; i < n; i++) {
					byte v = DebugApi.GetMemoryValue(memTypes[i], addresses[i]);
					if(v != lastValues[i]) {
						events.Add(new JsonObject {
							["frame"] = framesElapsed,
							["address"] = addresses[i],
							["name"] = names[i],
							["memoryType"] = memTypes[i].ToString(),
							["oldValue"] = lastValues[i],
							["newValue"] = v,
						});
						lastValues[i] = v;
						if(events.Count >= maxEvents) {
							stop = true;
							return;
						}
					}
				}
				if(framesElapsed >= maxFrames) stop = true;
			}
		};

		IntPtr listener = EmuApi.RegisterNotificationCallback(cb);
		try {
			EmuApi.Resume();
			// Spin until the callback flips `stop`. Bound the wall-clock wait
			// so a stuck emulator can't deadlock the MCP request — at max
			// emulation speed, 10000 frames typically completes in < 30s, so
			// 60s is a generous ceiling.
			System.Diagnostics.Stopwatch sw = System.Diagnostics.Stopwatch.StartNew();
			while(!stop && sw.Elapsed.TotalSeconds < 60) {
				System.Threading.Thread.Sleep(5);
			}
			EmuApi.Pause();
			for(int i = 0; i < 200 && !EmuApi.IsPaused(); i++) {
				System.Threading.Thread.Sleep(5);
			}
		} finally {
			EmuApi.UnregisterNotificationCallback(listener);
			GC.KeepAlive(cb);
		}

		return new JsonObject {
			["events"] = events,
			["framesElapsed"] = framesElapsed,
			["watched"] = n,
			["stopped"] = stop ? "budget-exhausted" : "wall-clock-timeout",
		};
	}

	public static JsonNode SaveStateSlot(JsonNode? args)
	{
		if(args == null) throw new McpException(-32602, "save_state_slot requires arguments");
		uint slot = RequireUInt(args, "slot");
		if(slot > 9) throw new McpException(-32602, "slot out of range (0..9)");
		EmuApi.SaveState(slot);
		return new JsonObject { ["slot"] = slot };
	}

	public static JsonNode LoadStateSlot(JsonNode? args)
	{
		if(args == null) throw new McpException(-32602, "load_state_slot requires arguments");
		uint slot = RequireUInt(args, "slot");
		if(slot > 9) throw new McpException(-32602, "slot out of range (0..9)");
		EmuApi.LoadState(slot);
		return new JsonObject { ["slot"] = slot };
	}

	public static JsonNode AddFrameHook(JsonNode? args)
	{
		args ??= new JsonObject();
		uint everyN = (uint)((args["everyN"]?.GetValue<long>()) ?? 1);
		if(everyN == 0) throw new McpException(-32602, "everyN must be > 0");
		string cpuStr = args["cpuType"]?.GetValue<string>() ?? "Snes";
		if(!Enum.TryParse<CpuType>(cpuStr, ignoreCase: true, out var cpu)) {
			throw new McpException(-32602, "unknown cpuType: " + cpuStr);
		}
		// Frame number ranges over uint32; address-range check passes if we
		// give a hook the full uint32 span. everyN is implemented via the
		// value-match: matchValueMask = (everyN-1) and matchValue = 0 only
		// matches when (frame & mask) == 0.
		uint mask = everyN - 1;
		bool isPow2 = everyN != 0 && (everyN & mask) == 0;
		if(!isPow2) {
			// Fallback: no value match, fire every frame and let the client filter.
			mask = 0;
		}
		int handle = DebugApi.McpAddHook((byte)McpHookKind.Frame, cpu, 0, uint.MaxValue, 0, mask, 0, 0);
		return new JsonObject {
			["handle"] = handle,
			["kind"] = "Frame",
			["everyN"] = isPow2 ? (long)everyN : 1,
			["cpuType"] = cpu.ToString(),
		};
	}

	public static JsonNode RecordAudio(JsonNode? args)
	{
		if(args == null) throw new McpException(-32602, "record_audio requires arguments");
		string path = RequireString(args, "path");
		Nexen.Interop.RecordApi.WaveRecord(path);
		return new JsonObject { ["path"] = path, ["recording"] = true };
	}

	public static JsonNode StopAudio(JsonNode? args)
	{
		Nexen.Interop.RecordApi.WaveStop();
		return new JsonObject { ["recording"] = false };
	}

	public static JsonNode RecordMovie(JsonNode? args)
	{
		if(args == null) throw new McpException(-32602, "record_movie requires arguments");
		string path = RequireString(args, "path");
		string author = args["author"]?.GetValue<string>() ?? "mesen-mcp";
		string description = args["description"]?.GetValue<string>() ?? "";
		string fromStr = args["from"]?.GetValue<string>() ?? "CurrentState";
		if(!Enum.TryParse<Nexen.Interop.RecordMovieFrom>(fromStr, ignoreCase: true, out var from)) {
			throw new McpException(-32602,
				"from must be 'CurrentState', 'StartWithoutSaveData', or 'StartWithSaveData'");
		}
		if(Nexen.Interop.RecordApi.MovieRecording() || Nexen.Interop.RecordApi.MoviePlaying()) {
			throw new McpException(-32603, "another movie is already recording or playing — call stop_movie first");
		}
		var options = new Nexen.Interop.RecordMovieOptions(path, author, description, from);
		Nexen.Interop.RecordApi.MovieRecord(options);
		return new JsonObject {
			["path"] = path,
			["author"] = author,
			["from"] = from.ToString(),
			["recording"] = true,
		};
	}

	public static JsonNode PlayMovie(JsonNode? args)
	{
		if(args == null) throw new McpException(-32602, "play_movie requires arguments");
		string path = RequireString(args, "path");
		if(!System.IO.File.Exists(path)) {
			throw new McpException(-32602, "movie file not found: " + path);
		}
		if(Nexen.Interop.RecordApi.MovieRecording() || Nexen.Interop.RecordApi.MoviePlaying()) {
			throw new McpException(-32603, "another movie is already recording or playing — call stop_movie first");
		}
		Nexen.Interop.RecordApi.MoviePlay(path);
		return new JsonObject {
			["path"] = path,
			["playing"] = true,
		};
	}

	public static JsonNode StopMovie(JsonNode? args)
	{
		bool wasRecording = Nexen.Interop.RecordApi.MovieRecording();
		bool wasPlaying = Nexen.Interop.RecordApi.MoviePlaying();
		if(wasRecording || wasPlaying) {
			Nexen.Interop.RecordApi.MovieStop();
		}
		return new JsonObject {
			["wasRecording"] = wasRecording,
			["wasPlaying"] = wasPlaying,
			["recording"] = Nexen.Interop.RecordApi.MovieRecording(),
			["playing"] = Nexen.Interop.RecordApi.MoviePlaying(),
		};
	}

	public static JsonNode MovieState(JsonNode? args)
	{
		return new JsonObject {
			["recording"] = Nexen.Interop.RecordApi.MovieRecording(),
			["playing"] = Nexen.Interop.RecordApi.MoviePlaying(),
		};
	}

	// Minimal WAV reader: header + 16/8-bit PCM data chunk. Nexen's WaveRecord
	// emits 16-bit stereo PCM at the SNES ~32kHz S-DSP sample rate; we accept
	// any sample rate/channel count but reject non-PCM formats.
	private struct WavData
	{
		public int SampleRate;
		public int Channels;
		public int BitsPerSample;
		public byte[] Data;          // raw PCM bytes (interleaved channels)
		public int FrameCount;       // samples-per-channel
	}

	private static WavData ReadWavPcm(string path)
	{
		byte[] bytes = System.IO.File.ReadAllBytes(path);
		if(bytes.Length < 44) {
			throw new McpException(-32602, "WAV too small (truncated): " + path);
		}
		if(bytes[0] != 'R' || bytes[1] != 'I' || bytes[2] != 'F' || bytes[3] != 'F'
			|| bytes[8] != 'W' || bytes[9] != 'A' || bytes[10] != 'V' || bytes[11] != 'E') {
			throw new McpException(-32602, "not a RIFF/WAVE file: " + path);
		}

		int sampleRate = 0, channels = 0, bps = 0, audioFormat = 0;
		int dataOffset = -1, dataSize = 0;
		int pos = 12;
		while(pos + 8 <= bytes.Length) {
			string id = System.Text.Encoding.ASCII.GetString(bytes, pos, 4);
			int size = System.Buffers.Binary.BinaryPrimitives.ReadInt32LittleEndian(bytes.AsSpan(pos + 4));
			pos += 8;
			if(id == "fmt ") {
				if(size < 16) throw new McpException(-32602, "fmt chunk too small");
				audioFormat = System.Buffers.Binary.BinaryPrimitives.ReadInt16LittleEndian(bytes.AsSpan(pos));
				channels = System.Buffers.Binary.BinaryPrimitives.ReadInt16LittleEndian(bytes.AsSpan(pos + 2));
				sampleRate = System.Buffers.Binary.BinaryPrimitives.ReadInt32LittleEndian(bytes.AsSpan(pos + 4));
				bps = System.Buffers.Binary.BinaryPrimitives.ReadInt16LittleEndian(bytes.AsSpan(pos + 14));
			} else if(id == "data") {
				dataOffset = pos;
				dataSize = size;
				break;
			}
			pos += size;
			if((size & 1) != 0) pos++;  // chunks are word-aligned
		}
		if(audioFormat != 1) {
			throw new McpException(-32602, $"unsupported WAV audioFormat={audioFormat} (only PCM=1 is handled)");
		}
		if(dataOffset < 0 || dataSize <= 0 || dataOffset + dataSize > bytes.Length) {
			throw new McpException(-32602, "WAV data chunk missing or out of range");
		}
		if(bps != 8 && bps != 16) {
			throw new McpException(-32602, $"unsupported bitsPerSample={bps} (only 8 and 16 are handled)");
		}
		if(channels < 1 || channels > 8) {
			throw new McpException(-32602, $"unreasonable channel count: {channels}");
		}

		var data = new byte[dataSize];
		Array.Copy(bytes, dataOffset, data, 0, dataSize);
		int bytesPerFrame = channels * (bps / 8);
		return new WavData {
			SampleRate = sampleRate,
			Channels = channels,
			BitsPerSample = bps,
			Data = data,
			FrameCount = dataSize / bytesPerFrame,
		};
	}

	// Decode N samples from a WAV's raw PCM bytes into a float[] in [-1, 1].
	// Mixes channels down to mono (mean) so callers don't have to think about
	// channel layout for fingerprinting / waveform rendering.
	private static float[] WavToMonoFloats(in WavData wav)
	{
		var mono = new float[wav.FrameCount];
		int ch = wav.Channels;
		if(wav.BitsPerSample == 16) {
			for(int i = 0; i < wav.FrameCount; i++) {
				int sum = 0;
				for(int c = 0; c < ch; c++) {
					int b = (i * ch + c) * 2;
					short s = (short)(wav.Data[b] | (wav.Data[b + 1] << 8));
					sum += s;
				}
				mono[i] = (sum / ch) / 32768.0f;
			}
		} else {  // 8-bit unsigned
			for(int i = 0; i < wav.FrameCount; i++) {
				int sum = 0;
				for(int c = 0; c < ch; c++) {
					sum += wav.Data[i * ch + c] - 128;
				}
				mono[i] = (sum / ch) / 128.0f;
			}
		}
		return mono;
	}

	public static JsonNode AudioFingerprint(JsonNode? args)
	{
		if(args == null) throw new McpException(-32602, "audio_fingerprint requires arguments");
		string path = RequireString(args, "path");
		var wav = ReadWavPcm(path);

		// SHA-256 the raw PCM bytes — stable across re-runs of the same audio.
		string sha256;
		using(var h = System.Security.Cryptography.SHA256.Create()) {
			sha256 = Convert.ToHexString(h.ComputeHash(wav.Data)).ToLowerInvariant();
		}

		// Per-second RMS bucketed at 1Hz resolution. Each bucket reports the
		// RMS of the mono-mixed sample stream over that second.
		float[] mono = WavToMonoFloats(wav);
		double durationSec = (double)wav.FrameCount / wav.SampleRate;
		int bucketCount = Math.Max(1, (int)Math.Ceiling(durationSec));
		int samplesPerBucket = Math.Max(1, wav.FrameCount / bucketCount);
		var rmsLevels = new JsonArray();
		float peak = 0;
		for(int i = 0; i < bucketCount; i++) {
			int start = i * samplesPerBucket;
			int end = Math.Min(wav.FrameCount, start + samplesPerBucket);
			if(start >= end) break;
			double sumSq = 0;
			for(int j = start; j < end; j++) {
				float s = mono[j];
				if(s > peak) peak = s;
				else if(-s > peak) peak = -s;
				sumSq += s * s;
			}
			double rms = Math.Sqrt(sumSq / (end - start));
			rmsLevels.Add(JsonValue.Create((float)rms));
		}

		return new JsonObject {
			["path"] = path,
			["sha256"] = sha256,
			["sampleRate"] = wav.SampleRate,
			["channels"] = wav.Channels,
			["bitsPerSample"] = wav.BitsPerSample,
			["frameCount"] = wav.FrameCount,
			["durationSec"] = durationSec,
			["peakAmplitude"] = peak,
			["rmsLevels"] = rmsLevels,
		};
	}

	public static JsonNode AudioWaveformPng(JsonNode? args)
	{
		if(args == null) throw new McpException(-32602, "audio_waveform_png requires arguments");
		string wavPath = RequireString(args, "path");
		int width = (int)((args["width"]?.GetValue<long>()) ?? 1024);
		int height = (int)((args["height"]?.GetValue<long>()) ?? 256);
		string format = (args["format"]?.GetValue<string>() ?? "path").ToLowerInvariant();
		if(width < 64 || width > 4096) throw new McpException(-32602, "width must be 64..4096");
		if(height < 32 || height > 1024) throw new McpException(-32602, "height must be 32..1024");

		string outputPath = args["outputPath"]?.GetValue<string>()
			?? System.IO.Path.ChangeExtension(wavPath, ".waveform.png");

		var wav = ReadWavPcm(wavPath);
		float[] mono = WavToMonoFloats(wav);

		// One column = one pixel-time bucket. For each bucket, find the
		// min/max sample value, then draw a vertical line connecting them.
		// Produces the classic two-tone amplitude-envelope waveform without
		// needing a downsampling library.
		int totalFrames = wav.FrameCount;
		float framesPerCol = (float)totalFrames / width;
		using var bmp = new SkiaSharp.SKBitmap(width, height,
			SkiaSharp.SKColorType.Bgra8888, SkiaSharp.SKAlphaType.Opaque);
		using(var canvas = new SkiaSharp.SKCanvas(bmp)) {
			canvas.Clear(SkiaSharp.SKColors.Black);
			using var paint = new SkiaSharp.SKPaint {
				Color = new SkiaSharp.SKColor(0xC0, 0xE0, 0xC0),  // soft green-white
				StrokeWidth = 1,
				IsAntialias = false,
			};
			using var midPaint = new SkiaSharp.SKPaint {
				Color = new SkiaSharp.SKColor(0x40, 0x40, 0x40),
				StrokeWidth = 1,
			};
			float midY = height / 2.0f;
			canvas.DrawLine(0, midY, width, midY, midPaint);

			for(int x = 0; x < width; x++) {
				int start = (int)(x * framesPerCol);
				int end = (int)((x + 1) * framesPerCol);
				if(end <= start) end = start + 1;
				if(end > totalFrames) end = totalFrames;
				if(start >= totalFrames) break;
				float lo = 1.0f, hi = -1.0f;
				for(int j = start; j < end; j++) {
					float s = mono[j];
					if(s < lo) lo = s;
					if(s > hi) hi = s;
				}
				float y0 = midY - hi * (height / 2.0f - 1);
				float y1 = midY - lo * (height / 2.0f - 1);
				canvas.DrawLine(x, y0, x, y1, paint);
			}
		}

		using(var stream = System.IO.File.OpenWrite(outputPath))
		using(var img = SkiaSharp.SKImage.FromBitmap(bmp))
		using(var data = img.Encode(SkiaSharp.SKEncodedImageFormat.Png, 100)) {
			data.SaveTo(stream);
		}

		var result = new JsonObject {
			["path"] = outputPath,
			["sourcePath"] = wavPath,
			["width"] = width,
			["height"] = height,
			["sampleRate"] = wav.SampleRate,
			["channels"] = wav.Channels,
			["frameCount"] = wav.FrameCount,
			["durationSec"] = (double)wav.FrameCount / wav.SampleRate,
		};
		if(format == "base64") {
			byte[] outBytes = System.IO.File.ReadAllBytes(outputPath);
			result["base64"] = Convert.ToBase64String(outBytes);
			result["bytes"] = outBytes.Length;
		}
		return result;
	}

	public static JsonNode GetAudioState(JsonNode? args)
	{
		var state = DebugApi.GetConsoleState<Nexen.Interop.SnesState>(ConsoleType.Snes);

		var spc = state.Spc;
		var dsp = state.Dsp;

		// Decode the per-voice DSP register block. Each voice has 8 register
		// slots at offsets x0..x9; voice index in high nibble. Surface the
		// most useful subset for "is the music playing": volume L/R, pitch,
		// source #, ADSR1/ADSR2, gain, envelope, current output.
		var voices = new JsonArray();
		for(int v = 0; v < 8; v++) {
			int b = v * 0x10;
			byte volL  = dsp.ExternalRegs[b + 0x00];
			byte volR  = dsp.ExternalRegs[b + 0x01];
			ushort pitch = (ushort)(dsp.ExternalRegs[b + 0x02] | (dsp.ExternalRegs[b + 0x03] << 8));
			byte src   = dsp.ExternalRegs[b + 0x04];
			byte adsr1 = dsp.ExternalRegs[b + 0x05];
			byte adsr2 = dsp.ExternalRegs[b + 0x06];
			byte gain  = dsp.ExternalRegs[b + 0x07];
			byte envx  = dsp.ExternalRegs[b + 0x08];
			byte outx  = dsp.ExternalRegs[b + 0x09];
			voices.Add(new JsonObject {
				["voice"] = v,
				["volL"] = (sbyte)volL,
				["volR"] = (sbyte)volR,
				["pitch"] = pitch,
				["sampleSrc"] = src,
				["adsr1"] = adsr1,
				["adsr2"] = adsr2,
				["gain"] = gain,
				["envelope"] = envx,
				["currentOutput"] = (sbyte)outx,
			});
		}

		// Master flags. $0C = MVOLL, $1C = MVOLR, $2C = EVOLL, $3C = EVOLR,
		// $4C = KON (key on), $5C = KOF (key off), $6C = FLG (mute/reset/echo),
		// $7C = ENDX (which voices ended).
		return new JsonObject {
			["spc"] = new JsonObject {
				["pc"] = spc.PC,
				["a"] = spc.A,
				["x"] = spc.X,
				["y"] = spc.Y,
				["sp"] = spc.SP,
				["cycle"] = spc.Cycle,
			},
			["dsp"] = new JsonObject {
				["mainVolL"] = (sbyte)dsp.ExternalRegs[0x0C],
				["mainVolR"] = (sbyte)dsp.ExternalRegs[0x1C],
				["echoVolL"] = (sbyte)dsp.ExternalRegs[0x2C],
				["echoVolR"] = (sbyte)dsp.ExternalRegs[0x3C],
				["keyOn"] = dsp.ExternalRegs[0x4C],
				["keyOff"] = dsp.ExternalRegs[0x5C],
				["flg"] = dsp.ExternalRegs[0x6C],
				["voicesEnded"] = dsp.ExternalRegs[0x7C],
			},
			["voices"] = voices,
		};
	}

	public static JsonNode ResetEmulator(JsonNode? args)
	{
		// Power-cycle (full reset) is what tests usually want — equivalent to
		// flipping the cart off and back on. Soft Reset preserves more
		// state. Default to PowerCycle so frame counter actually rewinds.
		bool power = args?["power"]?.GetValue<bool>() ?? true;
		if(power) {
			EmuApi.McpPowerCycle();
		} else {
			EmuApi.McpResetEmu();
		}
		// Reset is processed on the emu thread.  Waiting for
		// `frameCount < 30` is not a completion test: after a power-cycle
		// the counter is already zero, so that condition returns before the
		// reset has reached the emulation thread.  Wait for one post-reset
		// frame transition instead, preserving the caller's choice to pause
		// and arm hooks at the settled boundary.
		uint startFrame = EmuApi.GetFrameCount();
		for(int i = 0; i < 150; i++) {
			System.Threading.Thread.Sleep(20);
			if(EmuApi.GetFrameCount() != startFrame) break;
		}
		return new JsonObject { ["reset"] = true, ["power"] = power,
			["startFrame"] = startFrame, ["frameCount"] = EmuApi.GetFrameCount() };
	}

	public static JsonNode ReadDmaState(JsonNode? args)
	{
		// DMA registers live at $4300..$437F (8 channels × 16 bytes).
		// They're CPU-bus-readable in debug mode. Most fields are valid
		// for both DMA and HDMA; the active mode + interpretation
		// depends on the channel control register bits.
		var channels = new JsonArray();
		for(int ch = 0; ch < 8; ch++) {
			uint baseAddr = (uint)(0x4300 + ch * 0x10);
			byte[] regs = DebugApi.GetMemoryValues(MemoryType.SnesMemory, baseAddr, baseAddr + 10);
			channels.Add(new JsonObject {
				["channel"] = ch,
				["control"] = regs[0],
				["bbus"] = regs[1],
				["aBusAddrLo"] = regs[2],
				["aBusAddrMid"] = regs[3],
				["aBusBank"] = regs[4],
				["countLo"] = regs[5],
				["countHi"] = regs[6],
				["indirectBank"] = regs[7],
				["tableAddrLo"] = regs[8],
				["tableAddrHi"] = regs[9],
				["lineCounter"] = regs[10],
				// Convenience: 24-bit A-bus source as a single number.
				["aBusAddr"] = (uint)(regs[2] | (regs[3] << 8) | (regs[4] << 16)),
				["tableAddr"] = (ushort)(regs[8] | (regs[9] << 8)),
				["count"] = (ushort)(regs[5] | (regs[6] << 8)),
				["targetReg"] = $"$21{regs[1]:X2}",
			});
		}
		return new JsonObject { ["channels"] = channels };
	}

	public static JsonNode SaveState(JsonNode? args)
	{
		if(args == null) {
			throw new McpException(-32602, "save_state requires arguments");
		}
		string path = RequireString(args, "path");
		EmuApi.SaveStateFile(path);
		return new JsonObject { ["path"] = path };
	}

	public static JsonNode LoadState(JsonNode? args)
	{
		if(args == null) {
			throw new McpException(-32602, "load_state requires arguments");
		}
		string path = RequireString(args, "path");
		EmuApi.LoadStateFile(path);
		return new JsonObject { ["path"] = path };
	}

	public static JsonNode AddExecHook(JsonNode? args) => AddHookImpl(args, McpHookKind.Exec, "add_exec_hook");
	public static JsonNode AddReadHook(JsonNode? args) => AddHookImpl(args, McpHookKind.Read, "add_read_hook");
	public static JsonNode AddWriteHook(JsonNode? args) => AddHookImpl(args, McpHookKind.Write, "add_write_hook");

	private static JsonNode AddHookImpl(JsonNode? args, McpHookKind kind, string toolName)
	{
		if(args == null) throw new McpException(-32602, $"{toolName} requires arguments");
		uint addr = RequireUInt(args, "address");
		uint endAddr = (uint?)args["endAddress"]?.GetValue<long>() ?? addr;
		string cpuStr = args["cpuType"]?.GetValue<string>() ?? "Snes";
		if(!Enum.TryParse<CpuType>(cpuStr, ignoreCase: true, out var cpu)) {
			throw new McpException(-32602, "unknown cpuType: " + cpuStr);
		}
		if(endAddr < addr) {
			throw new McpException(-32602, "endAddress must be >= address");
		}
		uint matchValue = (uint)((args["matchValue"]?.GetValue<long>()) ?? 0);
		uint matchValueMask = (uint)((args["matchValueMask"]?.GetValue<long>()) ?? 0);
		uint xValue = (uint)((args["xValue"]?.GetValue<long>()) ?? 0);
		uint xMask = (uint)((args["xMask"]?.GetValue<long>()) ?? 0);
		int handle = DebugApi.McpAddHook((byte)kind, cpu, addr, endAddr, matchValue, matchValueMask, xValue, xMask);
		return new JsonObject {
			["handle"] = handle,
			["kind"] = kind.ToString(),
			["cpuType"] = cpu.ToString(),
			["address"] = addr,
			["endAddress"] = endAddr,
			["matchValue"] = matchValue,
			["matchValueMask"] = matchValueMask,
			["xValue"] = xValue,
			["xMask"] = xMask,
		};
	}

	public static JsonNode RemoveHook(JsonNode? args)
	{
		if(args == null) throw new McpException(-32602, "remove_hook requires arguments");
		int handle = (int)RequireUInt(args, "handle");
		bool ok = DebugApi.McpRemoveHook(handle);
		return new JsonObject {
			["handle"] = handle,
			["removed"] = ok,
		};
	}

	// Sym-file cache: filename -> (mtime, list of (name, addr)). Reparse if
	// the file's mtime changes — typical workflow has the user rebuilding
	// the ROM frequently and addresses shift each time.
	private static readonly Dictionary<string, (DateTime mtime, List<(string name, uint addr)> rows)> _symCache
		= new(StringComparer.OrdinalIgnoreCase);
	private static readonly object _symCacheLock = new();

	private static List<(string name, uint addr)> LoadSymFile(string path)
	{
		var info = new System.IO.FileInfo(path);
		if(!info.Exists) {
			throw new McpException(-32602, "sym file not found: " + path);
		}
		lock(_symCacheLock) {
			if(_symCache.TryGetValue(path, out var cached) && cached.mtime == info.LastWriteTimeUtc) {
				return cached.rows;
			}
			// WLA-DX format: "BBBB:OOOOOO NAME". Bank may be 4 hex digits, offset
			// is variable width. WRAM symbols encode the $7E bank in offset.
			var rows = new List<(string name, uint addr)>();
			var rx = new System.Text.RegularExpressions.Regex(
				@"^\s*[0-9A-Fa-f]{4}:([0-9A-Fa-f]+)\s+(\S+)\s*$");
			foreach(string line in System.IO.File.ReadAllLines(path)) {
				var m = rx.Match(line);
				if(!m.Success) continue;
				if(uint.TryParse(m.Groups[1].Value, System.Globalization.NumberStyles.HexNumber,
						null, out uint addr)) {
					rows.Add((m.Groups[2].Value, addr));
				}
			}
			_symCache[path] = (info.LastWriteTimeUtc, rows);
			return rows;
		}
	}

	public static JsonNode LookupSymbol(JsonNode? args)
	{
		if(args == null) throw new McpException(-32602, "lookup_symbol requires arguments");
		string path = RequireString(args, "symFile");
		string pattern = RequireString(args, "pattern");
		int maxResults = (int)((args["maxResults"]?.GetValue<long>()) ?? 64);
		System.Text.RegularExpressions.Regex rx;
		try {
			rx = new System.Text.RegularExpressions.Regex(pattern);
		} catch(Exception ex) {
			throw new McpException(-32602, "invalid regex: " + ex.Message);
		}
		var rows = LoadSymFile(path);
		var matches = new JsonArray();
		int count = 0;
		foreach(var (name, addr) in rows) {
			if(!rx.IsMatch(name)) continue;
			// Common HiROM convenience: project's bank 0 ROM symbols (offset
			// 0x0000..0xFFFF) run at $C0:offset at runtime. WRAM symbols
			// already encode their full $7E:xxxx address in the offset and
			// must not be remapped.
			uint romCpu;
			if(addr < 0x10000) {
				romCpu = 0xC00000u | (addr & 0xFFFFu);
			} else {
				romCpu = addr;
			}
			matches.Add(new JsonObject {
				["name"] = name,
				["address"] = addr,
				["romCpuAddr"] = romCpu,
			});
			if(++count >= maxResults) break;
		}
		return new JsonObject {
			["count"] = count,
			["totalSymbols"] = rows.Count,
			["matches"] = matches,
		};
	}

	public static JsonNode Disassemble(JsonNode? args)
	{
		if(args == null) throw new McpException(-32602, "disassemble requires arguments");
		uint addr = RequireUInt(args, "address");
		int count = (int)((args["count"]?.GetValue<long>()) ?? 16);
		if(count <= 0 || count > 256) throw new McpException(-32602, "count out of range (1..256)");
		string cpuStr = args["cpuType"]?.GetValue<string>() ?? "Snes";
		if(!Enum.TryParse<CpuType>(cpuStr, ignoreCase: true, out var cpu)) {
			throw new McpException(-32602, "unknown cpuType: " + cpuStr);
		}

		// Get the row index for our address, then pull `count` consecutive rows.
		int startRow = DebugApi.GetDisassemblyRowAddress(cpu, addr, 0);
		CodeLineData[] rows = DebugApi.GetDisassemblyOutput(cpu, (uint)startRow, (uint)count);

		var lines = new JsonArray();
		foreach(var r in rows) {
			lines.Add(new JsonObject {
				["address"] = r.Address,
				["text"] = r.Text,
				["byteCode"] = Convert.ToHexString(r.ByteCode, 0, Math.Min(r.OpSize, r.ByteCode.Length)),
				["opSize"] = r.OpSize,
			});
		}
		return new JsonObject {
			["count"] = rows.Length,
			["lines"] = lines,
		};
	}

	public static JsonNode RunUntil(JsonNode? args)
	{
		args ??= new JsonObject();
		int maxFrames = (int)((args["maxFrames"]?.GetValue<long>()) ?? 600);
		int hookHandle = (int)((args["hookHandle"]?.GetValue<long>()) ?? 0);
		if(maxFrames <= 0 || maxFrames > 1_000_000) {
			throw new McpException(-32602, "maxFrames out of range (1..1000000)");
		}

		uint startFrame = EmuApi.GetFrameCount();
		// Drain events queued by an earlier run.  The diagnostic match counter
		// is global, so it cannot identify the requested hook and allowed a
		// different hook's match to terminate this wait.
		if(hookHandle != 0) {
			var staleEvents = new Interop.McpHookEvent[256];
			while(DebugApi.McpDrainEvents(staleEvents, staleEvents.Length) != 0) { }
		}

		EmuApi.Resume();
		string reason = "maxFrames";
		Interop.McpHookEvent matchedEvent = default;
		bool matched = false;
		var observedEvents = new JsonArray();
		var sw = System.Diagnostics.Stopwatch.StartNew();
		// Bound the request so a genuinely wedged emulator cannot hang the
		// MCP loop forever; the bound is deliberately generous for this
		// interpreter's measured frame cost.
		long maxMs = (long)(maxFrames * 2000.0 + 1000);
		while(sw.ElapsedMilliseconds < maxMs) {
			System.Threading.Thread.Sleep(10);
			if(hookHandle != 0) {
				var events = new Interop.McpHookEvent[256];
				int eventCount = DebugApi.McpDrainEvents(events, events.Length);
				for(int i = 0; i < eventCount; i++) {
					observedEvents.Add(new JsonObject {
						["handle"] = events[i].Handle,
						["address"] = events[i].Address,
						["value"] = events[i].Value,
						["frame"] = events[i].FrameNumber,
						["kind"] = events[i].Kind.ToString(),
						["cpuType"] = events[i].Cpu.ToString(),
						["hostPc"] = events[i].HostPc,
						["hostSp"] = events[i].HostSp,
						["hostP"] = events[i].HostP,
						["hostE"] = events[i].HostE,
						["hostM"] = events[i].HostM,
						["hostX"] = events[i].HostX,
						["hostPbr"] = events[i].HostPbr,
						["hostD"] = events[i].HostD,
						["hostDbr"] = events[i].HostDbr,
						["hostA"] = events[i].HostA,
						["hostXReg"] = events[i].HostXReg,
						["hostY"] = events[i].HostY,
						["hostCycleCount"] = events[i].HostCycleCount,
					});
					if(events[i].Handle != hookHandle) continue;
					matchedEvent = events[i];
					matched = true;
					reason = "hookFired";
					break;
				}
				if(matched) break;
			}
			uint nowFrame = EmuApi.GetFrameCount();
			if(nowFrame - startFrame >= (uint)maxFrames) {
				reason = "maxFrames";
				break;
			}
		}
		EmuApi.Pause();
		for(int i = 0; i < 200; i++) {
			if(EmuApi.IsPaused()) break;
			System.Threading.Thread.Sleep(5);
		}

		uint endFrame = EmuApi.GetFrameCount();
		return new JsonObject {
			["reason"] = reason,
			["framesAdvanced"] = endFrame - startFrame,
			["isPaused"] = EmuApi.IsPaused(),
			["hookHandle"] = hookHandle,
			["matchedAddress"] = matched ? matchedEvent.Address : 0,
			["matchedValue"] = matched ? matchedEvent.Value : 0,
			["matchedFrame"] = matched ? matchedEvent.FrameNumber : 0,
			["matchedHostPc"] = matched ? matchedEvent.HostPc : 0,
			["matchedHostSp"] = matched ? matchedEvent.HostSp : 0,
			["matchedHostP"] = matched ? matchedEvent.HostP : 0,
			["matchedHostE"] = matched ? matchedEvent.HostE : 0,
			["matchedHostM"] = matched ? matchedEvent.HostM : 0,
			["matchedHostX"] = matched ? matchedEvent.HostX : 0,
			["matchedHostPbr"] = matched ? matchedEvent.HostPbr : 0,
			["matchedHostD"] = matched ? matchedEvent.HostD : 0,
			["matchedHostDbr"] = matched ? matchedEvent.HostDbr : 0,
			["matchedHostA"] = matched ? matchedEvent.HostA : 0,
			["matchedHostXReg"] = matched ? matchedEvent.HostXReg : 0,
			["matchedHostY"] = matched ? matchedEvent.HostY : 0,
			["matchedHostCycleCount"] = matched ? matchedEvent.HostCycleCount : 0,
			["observedEvents"] = observedEvents,
		};
	}

	public static JsonNode HookDiag(JsonNode? args)
	{
		DebugApi.McpHookDiagCounters(out ulong calls, out ulong matches,
			out ulong queued, out ulong dropped);
		return new JsonObject {
			["onMemoryOperationCalls"] = calls,
			["matchedEventsEmitted"] = matches,
			["queuedEvents"] = queued,
			["droppedEvents"] = dropped,
		};
	}

	public static JsonNode ListHooks(JsonNode? args)
	{
		var buf = new McpHook[256];
		int n = DebugApi.McpListHooks(buf, buf.Length);
		var arr = new JsonArray();
		for(int i = 0; i < n; i++) {
			var h = buf[i];
			arr.Add(new JsonObject {
				["handle"] = h.Handle,
				["kind"] = h.Kind.ToString(),
				["cpuType"] = h.Cpu.ToString(),
				["startAddr"] = h.StartAddr,
				["endAddr"] = h.EndAddr,
				["active"] = h.Active,
			});
		}
		return new JsonObject {
			["count"] = n,
			["hooks"] = arr,
		};
	}

	public static JsonNode GetPpuState(JsonNode? args)
	{
		var s = DebugApi.GetPpuState<SnesPpuState>(CpuType.Snes);

		var layers = new JsonArray();
		for(int i = 0; i < s.Layers.Length; i++) {
			var L = s.Layers[i];
			layers.Add(new JsonObject {
				["index"] = i,
				["tilemapAddr"] = L.TilemapAddress,
				["chrAddr"] = L.ChrAddress,
				["hscroll"] = L.HScroll,
				["vscroll"] = L.VScroll,
				["largeTiles"] = L.LargeTiles,
				["doubleWidth"] = L.DoubleWidth,
				["doubleHeight"] = L.DoubleHeight,
			});
		}

		var mainMask = new JsonArray();
		var subMask = new JsonArray();
		for(int i = 0; i < s.WindowMaskMain.Length; i++) {
			mainMask.Add(JsonValue.Create((int)s.WindowMaskMain[i]));
			subMask.Add(JsonValue.Create((int)s.WindowMaskSub[i]));
		}

		return new JsonObject {
			["frameCount"] = s.FrameCount,
			["scanline"] = s.Scanline,
			["forcedBlank"] = s.ForcedBlank,
			["brightness"] = s.ScreenBrightness,
			["bgMode"] = s.BgMode,
			["mainScreenLayers"] = s.MainScreenLayers,  // bit0=BG1 .. bit4=OBJ
			["subScreenLayers"] = s.SubScreenLayers,
			["mainScreenWindowMask"] = mainMask,
			["subScreenWindowMask"] = subMask,
			["mosaicSize"] = s.MosaicSize,
			["mosaicEnabled"] = s.MosaicEnabled,
			["layers"] = layers,
		};
	}

	public static JsonNode SetInput(JsonNode? args)
	{
		if(args == null) {
			throw new McpException(-32602, "set_input requires arguments");
		}
		uint port = RequireUInt(args, "port");
		uint buttons = RequireUInt(args, "buttons");
		// hold=true: set (or clear, when buttons==0) the debugger input override
		// and return immediately WITHOUT auto-clearing or advancing emulation.
		// This is the TAS-style primitive: the override persists across
		// subsequent run_frames calls until explicitly released with
		// set_input(buttons=0, hold=true). Lets a caller observe a manual
		// $4016 serial read (mailbox) while the button is held. When hold is
		// omitted/false, the legacy auto-advance-then-clear behavior applies
		// and "frames" is required.
		bool hold = OptionalBool(args, "hold", false);
		uint frames = hold ? 0 : RequireUInt(args, "frames");
		if(!hold && (frames == 0 || frames > 100_000)) {
			throw new McpException(-32602, "frames out of range (1..100000)");
		}
		if(port > 3) {
			throw new McpException(-32602, "port out of range (0..3)");
		}

		DebugControllerState state = new() {
			A = (buttons & 0x001) != 0,
			B = (buttons & 0x002) != 0,
			Select = (buttons & 0x004) != 0,
			Start = (buttons & 0x008) != 0,
			Up = (buttons & 0x010) != 0,
			Down = (buttons & 0x020) != 0,
			Left = (buttons & 0x040) != 0,
			Right = (buttons & 0x080) != 0,
			X = (buttons & 0x100) != 0,
			L = (buttons & 0x200) != 0,
			R = (buttons & 0x400) != 0,
			Y = (buttons & 0x800) != 0,
		};

		if(hold) {
			// Persist the override; emulation is advanced by the caller via
			// run_frames. buttons==0 -> default() state releases the override
			// (the next frame's normal poll clears the controller).
			DebugApi.SetInputOverrides(port, state);
			return new JsonObject {
				["port"] = port,
				["buttons"] = buttons,
				["hold"] = true,
				["isPaused"] = EmuApi.IsPaused(),
			};
		}

		// Apply the override, run the emulator forward for the requested
		// frames, then clear the override. Debugger input overrides win
		// against the default controller polling every frame.
		DebugApi.SetInputOverrides(port, state);
		EmuApi.Resume();
		double estMs = frames * (1000.0 / 60.0);
		System.Threading.Thread.Sleep((int)Math.Min(estMs * 1.2 + 20, 120_000));
		EmuApi.Pause();
		for(int i = 0; i < 200; i++) {
			if(EmuApi.IsPaused()) break;
			System.Threading.Thread.Sleep(5);
		}
		DebugApi.SetInputOverrides(port, default);

		return new JsonObject {
			["port"] = port,
			["buttons"] = buttons,
			["frames"] = frames,
			["isPaused"] = EmuApi.IsPaused(),
		};
	}

	public static JsonNode ReadMemory(JsonNode? args)
	{
		if(args == null) {
			throw new McpException(-32602, "read_memory requires arguments");
		}
		string memTypeStr = RequireString(args, "memoryType");
		uint address = RequireUInt(args, "address");
		uint length = RequireUInt(args, "length");
		if(length == 0) {
			return new JsonObject {
				["memoryType"] = memTypeStr,
				["address"] = address,
				["length"] = 0,
				["hex"] = "",
			};
		}
		if(length > 65536) {
			throw new McpException(-32602, "length > 65536");
		}

		MemoryType memType = ParseMemoryType(memTypeStr);

		// Auto-route SNES CPU bus addresses that hit WRAM through MemoryType.SnesWorkRam.
		// Nexen's DebugApi.GetMemoryValues(MemoryType.SnesMemory, ...) returns 0 for
		// $7E:0000-$7F:FFFF on SA-1 builds (CPU bus mapper quirk), even though Lua's
		// emu.read with the same address+memType reads correctly. Translating up here
		// means callers can keep passing standard CPU bus addresses ($7EF967, etc.)
		// without thinking about the mirror gymnastics.
		MemoryType effectiveType = memType;
		uint effectiveAddress = address;
		string? routedFrom = null;
		if(memType == MemoryType.SnesMemory) {
			// $7E:0000-$7F:FFFF: full WRAM banks
			if(address >= 0x7E0000 && address <= 0x7FFFFF) {
				effectiveType = MemoryType.SnesWorkRam;
				effectiveAddress = address & 0x1FFFF;
				routedFrom = "snesMemory";
			}
			// Low 8KB WRAM mirror: $00..$3F:$0000-$1FFF and $80..$BF:$0000-$1FFF.
			// (Banks $40..$7D and $C0..$FF map ROM at low pages, so skip.)
			else if((address & 0xFFFF) <= 0x1FFF) {
				uint bank = (address >> 16) & 0xFF;
				if(bank <= 0x3F || (bank >= 0x80 && bank <= 0xBF)) {
					effectiveType = MemoryType.SnesWorkRam;
					effectiveAddress = address & 0x1FFF;
					routedFrom = "snesMemory";
				}
			}
		}

		byte[] buffer = DebugApi.GetMemoryValues(effectiveType, effectiveAddress, effectiveAddress + length - 1);

		var result = new JsonObject {
			["memoryType"] = memTypeStr,
			["address"] = address,
			["length"] = length,
			["hex"] = Convert.ToHexString(buffer),
		};
		if(routedFrom != null) {
			result["routedTo"] = effectiveType.ToString();
			result["effectiveAddress"] = effectiveAddress;
		}
		return result;
	}

	// --- helpers ------------------------------------------------------------

	private static string RequireString(JsonNode args, string key)
	{
		var node = args[key];
		if(node == null) {
			throw new McpException(-32602, $"missing arg: {key}");
		}
		try {
			return node.GetValue<string>();
		} catch {
			throw new McpException(-32602, $"arg {key} must be a string");
		}
	}

	private static uint RequireUInt(JsonNode args, string key)
	{
		var node = args[key];
		if(node == null) {
			throw new McpException(-32602, $"missing arg: {key}");
		}
		try {
			long v = node.GetValue<long>();
			if(v < 0 || v > uint.MaxValue) {
				throw new McpException(-32602, $"arg {key} out of range: {v}");
			}
			return (uint)v;
		} catch(McpException) {
			throw;
		} catch {
			throw new McpException(-32602, $"arg {key} must be an integer");
		}
	}

	private static bool OptionalBool(JsonNode args, string key, bool dflt)
	{
		var node = args[key];
		if(node == null) {
			return dflt;
		}
		try {
			return node.GetValue<bool>();
		} catch {
			// Accept 0/1 integers too, for clients that don't emit JSON bools.
			try { return node.GetValue<long>() != 0; } catch { return dflt; }
		}
	}

	private static MemoryType ParseMemoryType(string name)
	{
		// Accept the exact Lua memType names for script portability, plus
		// the raw enum names for callers that think in C#/C++ terms.
		return name switch {
			"snesMemory" => MemoryType.SnesMemory,
			"snesWorkRam" => MemoryType.SnesWorkRam,
			"snesVideoRam" => MemoryType.SnesVideoRam,
			"snesCgRam" => MemoryType.SnesCgRam,
			"snesSpriteRam" => MemoryType.SnesSpriteRam,
			"snesPrgRom" => MemoryType.SnesPrgRom,
			"sa1Memory" => MemoryType.Sa1Memory,
			"sa1IRam" => MemoryType.Sa1InternalRam,
			_ when Enum.TryParse<MemoryType>(name, ignoreCase: true, out var parsed) => parsed,
			_ => throw new McpException(-32602, "unknown memoryType: " + name),
		};
	}
}

internal sealed class McpToolDesc
{
	public string Name { get; }
	public string Description { get; }
	public JsonNode? InputSchema { get; }

	public McpToolDesc(string name, string description, JsonNode? inputSchema)
	{
		Name = name;
		Description = description;
		InputSchema = inputSchema;
	}

	public JsonNode ToJson()
	{
		var obj = new JsonObject {
			["name"] = Name,
			["description"] = Description,
		};
		// Input schema is required by the MCP spec even for no-arg tools;
		// emit an empty object schema in that case so clients stay happy.
		obj["inputSchema"] = InputSchema?.DeepClone() ?? new JsonObject {
			["type"] = "object",
			["properties"] = new JsonObject(),
		};
		return obj;
	}
}
