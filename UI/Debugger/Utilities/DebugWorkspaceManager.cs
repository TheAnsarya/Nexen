using System;
using System.Collections.Generic;
using System.Diagnostics.CodeAnalysis;
using System.IO;
using System.Linq;
using System.Text;
using System.Text.Json;
using System.Threading.Tasks;
using Nexen.Config;
using Nexen.Debugger.Integration;
using Nexen.Debugger.Labels;
using Nexen.Interop;
using Nexen.Utilities;

namespace Nexen.Debugger.Utilities {
	/// <summary>
	/// Manages the debug workspace state for the currently loaded ROM.
	/// </summary>
	/// <remarks>
	/// <para>
	/// The DebugWorkspaceManager handles loading and saving of debug session data including
	/// labels, breakpoints, watch entries, and symbol provider information. Each ROM has
	/// its own workspace stored as a JSON file in the debugger folder.
	/// </para>
	/// <para>
	/// This class also manages automatic loading of debug symbol files from various formats:
	/// <list type="bullet">
	///   <item><description>DBG files (ca65/ld65 debug info)</description></item>
	///   <item><description>SYM files (WLA-DX, RGBDS, Bass, PCEAS symbols)</description></item>
	///   <item><description>CDB files (SDCC debug info)</description></item>
	///   <item><description>ELF files (ELF/DWARF debug info)</description></item>
	///   <item><description>MLB/nexen-labels files (Nexen native label format)</description></item>
	///   <item><description>FNS files (NESASM function labels)</description></item>
	///   <item><description>CDL files (Code/Data Logger data)</description></item>
	/// </list>
	/// </para>
	/// <para>
	/// Integrates with <see cref="PansyExporter"/> for automatic metadata export when saving.
	/// </para>
	/// </remarks>
	public static class DebugWorkspaceManager {
		/// <summary>
		/// The current debug workspace instance, or null if not yet loaded.
		/// </summary>
		private static DebugWorkspace? _workspace = null;

		/// <summary>
		/// ROM information for the currently loaded game.
		/// </summary>
		private static RomInfo _romInfo = new();

		/// <summary>
		/// File path to the workspace JSON file.
		/// </summary>
		private static string _path = "";

		/// <summary>
		/// Gets the current symbol provider used for source-level debugging.
		/// </summary>
		/// <value>
		/// The symbol provider instance, or <c>null</c> if no debug symbols are loaded.
		/// </value>
		/// <remarks>
		/// Symbol providers enable source-level debugging by mapping addresses to
		/// source file locations, variable names, and other debug information.
		/// </remarks>
		public static ISymbolProvider? SymbolProvider { get; private set; }

		/// <summary>
		/// Raised when the symbol provider changes (loaded, cleared, or replaced).
		/// </summary>
		/// <remarks>
		/// Subscribe to this event to update UI elements that depend on symbol information,
		/// such as source code displays or variable watches.
		/// </remarks>
		public static event EventHandler? SymbolProviderChanged;

		/// <summary>
		/// Gets the current debug workspace, loading it if necessary.
		/// </summary>
		/// <value>
		/// The debug workspace for the currently loaded ROM.
		/// </value>
		/// <remarks>
		/// This property lazily initializes the workspace by calling <see cref="Load()"/>
		/// if no workspace is currently loaded.
		/// </remarks>
		public static DebugWorkspace Workspace {
			get {
				if (_workspace == null) {
					Load();
				}

				return _workspace;
			}
		}

		/// <summary>
		/// Searches for a file matching the ROM path with the specified extension.
		/// </summary>
		/// <param name="ext">The file extension to search for (without leading dot).</param>
		/// <returns>
		/// The full path to the matching file if found; otherwise, <c>null</c>.
		/// </returns>
		/// <remarks>
		/// <para>
		/// Checks two naming conventions:
		/// <list type="number">
		///   <item><description>Same name with different extension: "game.nes" → "game.dbg"</description></item>
		///   <item><description>Appended extension: "game.nes" → "game.nes.dbg"</description></item>
		/// </list>
		/// </para>
		/// </remarks>
		private static string? GetMatchingFile(string ext) {
			string path = Path.ChangeExtension(_romInfo.RomPath, ext);
			if (File.Exists(path)) {
				return path;
			}

			path = _romInfo.RomPath + "." + ext;
			if (File.Exists(path)) {
				return path;
			}

			return null;
		}

		/// <summary>
		/// Loads or reloads the debug workspace for the current ROM.
		/// </summary>
		/// <remarks>
		/// <para>
		/// This method performs the following operations in order:
		/// <list type="number">
		///   <item><description>Saves any existing workspace before loading</description></item>
		///   <item><description>Retrieves ROM information from the emulator API</description></item>
		///   <item><description>Loads the workspace JSON file from the debugger folder</description></item>
		///   <item><description>Auto-loads debug symbol files based on user configuration</description></item>
		///   <item><description>Raises the <see cref="SymbolProviderChanged"/> event</description></item>
		/// </list>
		/// </para>
		/// <para>
		/// Auto-loading priority (stops at first successful load):
		/// DBG → SYM → CDB → ELF → MLB/nexen-labels → FNS
		/// </para>
		/// <para>
		/// CDL files are loaded independently as they provide code/data mapping, not symbols.
		/// </para>
		/// </remarks>
		[MemberNotNull(nameof(DebugWorkspaceManager._workspace))]
		public static void Load() {
			if (_workspace != null) {
				Save();
			}

			_romInfo = EmuApi.GetRomInfo();
			_path = Path.Combine(ConfigManager.DebuggerFolder, _romInfo.GetRomName() + ".json");

			LabelManager.SuspendEvents();
			_workspace = DebugWorkspace.Load(_path);

			SymbolProvider = null;
			if (ConfigManager.Config.Debug.Integration.AutoLoadDbgFiles) {
				string? dbgPath = GetMatchingFile(FileDialogHelper.DbgFileExt);
				if (dbgPath != null) {
					LoadDbgSymbolFile(dbgPath, false);
				}
			}

			if (SymbolProvider == null && ConfigManager.Config.Debug.Integration.AutoLoadSymFiles) {
				string? symPath = GetMatchingFile(FileDialogHelper.SymFileExt);
				if (symPath != null) {
					LoadSymFile(symPath, false);
				}
			}

			if (SymbolProvider == null && ConfigManager.Config.Debug.Integration.AutoLoadCdbFiles) {
				string? symPath = GetMatchingFile(FileDialogHelper.CdbFileExt);
				if (symPath != null) {
					LoadCdbFile(symPath, false);
				}
			}

			if (SymbolProvider == null && ConfigManager.Config.Debug.Integration.AutoLoadElfFiles) {
				string? symPath = GetMatchingFile(FileDialogHelper.ElfFileExt);
				if (symPath != null) {
					LoadElfFile(symPath, false);
				}
			}

			if (SymbolProvider == null && ConfigManager.Config.Debug.Integration.AutoLoadMlbFiles) {
				// Try Nexen native format first, then legacy Mesen format
				string? labelsPath = GetMatchingFile(FileDialogHelper.NexenLabelExt);
				if (labelsPath == null) {
					labelsPath = GetMatchingFile(FileDialogHelper.MesenLabelExt);
				}
				if (labelsPath != null) {
					LoadNexenLabelFile(labelsPath, false);
				}
			}

			if (SymbolProvider == null && ConfigManager.Config.Debug.Integration.AutoLoadFnsFiles) {
				string? fnsPath = GetMatchingFile(FileDialogHelper.NesAsmLabelExt);
				if (fnsPath != null) {
					LoadNesAsmLabelFile(fnsPath, false);
				}
			}

			if (ConfigManager.Config.Debug.Integration.AutoLoadCdlFiles) {
				string? cdlPath = GetMatchingFile(FileDialogHelper.CdlExt);
				if (cdlPath != null) {
					LoadCdlFile(cdlPath);
				}
			}

			LabelManager.ResumeEvents();
			SymbolProviderChanged?.Invoke(null, EventArgs.Empty);
		}

		/// <summary>
		/// Resets labels to defaults if configured to do so on import.
		/// </summary>
		/// <remarks>
		/// Checks <see cref="DebugIntegrationConfig.ResetLabelsOnImport"/> to determine
		/// whether to clear existing labels before importing new ones.
		/// </remarks>
		private static void ResetLabels() {
			if (ConfigManager.Config.Debug.Integration.ResetLabelsOnImport) {
				LabelManager.ResetLabels();
				DefaultLabelHelper.SetDefaultLabels();
			}
		}

		/// <summary>
		/// Loads debug symbols from a ca65/ld65 DBG file.
		/// </summary>
		/// <param name="path">Full path to the DBG file.</param>
		/// <param name="showResult">If <c>true</c>, displays the import result to the user.</param>
		/// <remarks>
		/// DBG files contain comprehensive debug information from the ca65 assembler
		/// and ld65 linker, including symbols, source file mappings, and segment info.
		/// </remarks>
		public static void LoadDbgSymbolFile(string path, bool showResult) {
			if (File.Exists(path) && Path.GetExtension(path).ToLower() == "." + FileDialogHelper.DbgFileExt) {
				ResetLabels();
				SymbolProvider = DbgImporter.Import(_romInfo.Format, path, ConfigManager.Config.Debug.Integration.ImportComments, showResult);
			}
		}

		/// <summary>
		/// Loads debug symbols from an ELF file with DWARF debug information.
		/// </summary>
		/// <param name="path">Full path to the ELF file.</param>
		/// <param name="showResult">If <c>true</c>, displays the import result to the user.</param>
		/// <remarks>
		/// ELF files are commonly produced by GCC-based toolchains for homebrew development.
		/// </remarks>
		public static void LoadElfFile(string path, bool showResult) {
			if (File.Exists(path) && Path.GetExtension(path).ToLower() == "." + FileDialogHelper.ElfFileExt) {
				ResetLabels();
				ElfImporter.Import(path, showResult, _romInfo.ConsoleType.GetMainCpuType());
			}
		}

		/// <summary>
		/// Loads debug symbols from an SDCC CDB (debug) file.
		/// </summary>
		/// <param name="path">Full path to the CDB file.</param>
		/// <param name="showResult">If <c>true</c>, displays the import result to the user.</param>
		/// <remarks>
		/// CDB files are produced by the Small Device C Compiler (SDCC),
		/// commonly used for Game Boy and SMS homebrew development.
		/// </remarks>
		public static void LoadCdbFile(string path, bool showResult) {
			if (File.Exists(path) && Path.GetExtension(path).ToLower() == "." + FileDialogHelper.CdbFileExt) {
				ResetLabels();
				SymbolProvider = SdccSymbolImporter.Import(_romInfo.Format, path, showResult);
			}
		}

		/// <summary>
		/// Loads debug symbols from a SYM file.
		/// </summary>
		/// <param name="path">Full path to the SYM file.</param>
		/// <param name="showResult">If <c>true</c>, displays the import result to the user.</param>
		/// <remarks>
		/// <para>
		/// SYM files can come from various assemblers. This method auto-detects the format:
		/// <list type="bullet">
		///   <item><description>WLA-DX format (contains "[labels]" or wlalink signature)</description></item>
		///   <item><description>RGBDS format (Game Boy assembler)</description></item>
		///   <item><description>PCEAS format (PC Engine assembler)</description></item>
		///   <item><description>Bass format (generic)</description></item>
		/// </list>
		/// </para>
		/// <para>
		/// WLA-DX importers are platform-specific: SNES, Game Boy, PC Engine, or SMS.
		/// </para>
		/// </remarks>
		public static void LoadSymFile(string path, bool showResult) {
			if (File.Exists(path) && Path.GetExtension(path).ToLower() == "." + FileDialogHelper.SymFileExt) {
				string symContent = File.ReadAllText(path);
				if (symContent.Contains("[labels]") || symContent.Contains("this file was created with wlalink")) {
					//Assume WLA-DX symbol files
					WlaDxImporter? importer = null;
					switch (_romInfo.ConsoleType) {
						case ConsoleType.Snes:
							importer = _romInfo.CpuTypes.Contains(CpuType.Gameboy) ? new GbWlaDxImporter() : new SnesWlaDxImporter();
							break;

						case ConsoleType.Gameboy: importer = new GbWlaDxImporter(); break;
						case ConsoleType.PcEngine: importer = new PceWlaDxImporter(); break;
						case ConsoleType.Sms: importer = new SmsWlaDxImporter(); break;
					}

					if (importer != null) {
						ResetLabels();
						importer.Import(path, showResult);
						SymbolProvider = importer;
					}
				} else {
					if (_romInfo.CpuTypes.Contains(CpuType.Gameboy)) {
						if (RgbdsSymbolFile.IsValidFile(path)) {
							RgbdsSymbolFile.Import(path, showResult);
						} else {
							BassLabelFile.Import(path, showResult, CpuType.Gameboy);
						}
					} else {
						if (_romInfo.ConsoleType == ConsoleType.PcEngine && PceasSymbolImporter.IsValidFile(symContent)) {
							PceasSymbolImporter importer = new PceasSymbolImporter();
							importer.Import(path, showResult);
							SymbolProvider = importer;
						} else if (_romInfo.ConsoleType == ConsoleType.PcEngine && LegacyPceasSymbolFile.IsValidFile(symContent)) {
							LegacyPceasSymbolFile.Import(path, showResult);
						} else {
							BassLabelFile.Import(path, showResult, _romInfo.ConsoleType.GetMainCpuType());
						}
					}
				}
			}
		}

		/// <summary>
		/// Loads labels from a Nexen/Mesen native label file.
		/// </summary>
		/// <param name="path">Full path to the .nexen-labels or .mlb file.</param>
		/// <param name="showResult">If <c>true</c>, displays the import result to the user.</param>
		/// <remarks>
		/// Supports both Nexen's native format (.nexen-labels) and the legacy Mesen format (.mlb).
		/// See <see cref="NexenLabelFile"/> for format details.
		/// </remarks>
		public static void LoadNexenLabelFile(string path, bool showResult) {
			string ext = Path.GetExtension(path).ToLower();
			if (File.Exists(path) && (ext == "." + FileDialogHelper.NexenLabelExt || ext == "." + FileDialogHelper.MesenLabelExt)) {
				ResetLabels();
				NexenLabelFile.Import(path, showResult);
			}
		}

		/// <summary>
		/// Loads function labels from a NESASM FNS file.
		/// </summary>
		/// <param name="path">Full path to the FNS file.</param>
		/// <param name="showResult">If <c>true</c>, displays the import result to the user.</param>
		/// <remarks>
		/// FNS files are produced by the NESASM assembler and contain function label definitions.
		/// Only applicable to NES ROMs.
		/// </remarks>
		public static void LoadNesAsmLabelFile(string path, bool showResult) {
			if (_romInfo.ConsoleType == ConsoleType.Nes && File.Exists(path) && Path.GetExtension(path).ToLower() == "." + FileDialogHelper.NesAsmLabelExt) {
				ResetLabels();
				NesasmFnsImporter.Import(path, showResult);
			}
		}

		/// <summary>
		/// Loads code/data log (CDL) information from a CDL file.
		/// </summary>
		/// <param name="path">Full path to the CDL file.</param>
		/// <remarks>
		/// CDL files track which ROM bytes have been accessed as code vs data,
		/// enabling better disassembly analysis. This data is separate from symbol information.
		/// </remarks>
		private static void LoadCdlFile(string path) {
			if (File.Exists(path) && Path.GetExtension(path).ToLower() == "." + FileDialogHelper.CdlExt) {
				DebugApi.LoadCdlFile(_romInfo.ConsoleType.GetMainCpuType().GetPrgRomMemoryType(), path);
			}
		}

		/// <summary>
		/// Loads a debug file based on its extension, auto-detecting the format.
		/// </summary>
		/// <param name="filename">Full path to the debug file to load.</param>
		/// <param name="showResult">If <c>true</c>, displays the import result to the user.</param>
		/// <remarks>
		/// <para>
		/// Supported extensions and their handlers:
		/// <list type="bullet">
		///   <item><description>.dbg → <see cref="LoadDbgSymbolFile"/></description></item>
		///   <item><description>.sym → <see cref="LoadSymFile"/></description></item>
		///   <item><description>.cdb → <see cref="LoadCdbFile"/></description></item>
		///   <item><description>.elf → <see cref="LoadElfFile"/></description></item>
		///   <item><description>.nexen-labels, .mlb → <see cref="LoadNexenLabelFile"/></description></item>
		///   <item><description>.fns → <see cref="LoadNesAsmLabelFile"/></description></item>
		///   <item><description>.cdl → <see cref="LoadCdlFile"/></description></item>
		/// </list>
		/// </para>
		/// <para>
		/// Raises <see cref="SymbolProviderChanged"/> if the symbol provider changes.
		/// </para>
		/// </remarks>
		public static void LoadSupportedFile(string filename, bool showResult) {
			ISymbolProvider? symbolProvider = SymbolProvider;
			SymbolProvider = null;

			switch (Path.GetExtension(filename).ToLower().Substring(1)) {
				case FileDialogHelper.DbgFileExt: LoadDbgSymbolFile(filename, showResult); break;
				case FileDialogHelper.SymFileExt: LoadSymFile(filename, showResult); break;
				case FileDialogHelper.CdbFileExt: LoadCdbFile(filename, showResult); break;
				case FileDialogHelper.ElfFileExt: LoadElfFile(filename, showResult); break;
				case FileDialogHelper.NexenLabelExt: LoadNexenLabelFile(filename, showResult); break;
				case FileDialogHelper.MesenLabelExt: LoadNexenLabelFile(filename, showResult); break;
				case FileDialogHelper.NesAsmLabelExt: LoadNesAsmLabelFile(filename, showResult); break;
				case FileDialogHelper.CdlExt: LoadCdlFile(filename); SymbolProvider = symbolProvider; break;
			}

			if (SymbolProvider != symbolProvider) {
				SymbolProviderChanged?.Invoke(null, EventArgs.Empty);
			}
		}

		/// <summary>
		/// Saves the current debug workspace to disk.
		/// </summary>
		/// <param name="releaseWorkspace">
		/// If <c>true</c>, releases the workspace reference after saving (for cleanup on ROM unload).
		/// </param>
		/// <remarks>
		/// <para>
		/// Saves all debug session data including labels, breakpoints, and watch entries
		/// to a JSON file in the debugger folder.
		/// </para>
		/// <para>
		/// Also triggers automatic Pansy metadata export via <see cref="PansyExporter.AutoExport"/>
		/// to keep the external metadata file synchronized with the workspace.
		/// </para>
		/// </remarks>
		public static void Save(bool releaseWorkspace = false) {
			_workspace?.Save(_path, _romInfo.CpuTypes);

			// Auto-export Pansy file with CDL data when saving workspace
			if (!string.IsNullOrEmpty(_romInfo.RomPath)) {
				var memoryType = _romInfo.ConsoleType.GetMainCpuType().GetPrgRomMemoryType();
				PansyExporter.AutoExport(_romInfo, memoryType);
			}

			if (releaseWorkspace) {
				_workspace = null;
			}
		}

		/// <summary>
		/// Timestamp of the last automatic save operation.
		/// </summary>
		private static DateTime _previousAutoSave = DateTime.MinValue;

		/// <summary>
		/// Performs an automatic save if sufficient time has elapsed since the last save.
		/// </summary>
		/// <remarks>
		/// <para>
		/// Auto-save is triggered when modifying labels, breakpoints, or watches to prevent
		/// data loss in case of a crash. Saves are throttled to at most once per 60 seconds
		/// to avoid excessive disk I/O.
		/// </para>
		/// <para>
		/// Includes Pansy export as part of the save operation.
		/// </para>
		/// </remarks>
		public static void AutoSave() {
			// Automatically save when changing a label/breakpoint/watch to avoid losing progress if a crash occurs
			if ((DateTime.Now - _previousAutoSave).TotalSeconds >= 60) {
				Save(); // Use Save() to include Pansy export
				_previousAutoSave = DateTime.Now;
			}
		}
	}

	/// <summary>
	/// Contains debug workspace data for a single CPU type.
	/// </summary>
	/// <remarks>
	/// Multi-CPU systems (e.g., SNES with main CPU and SA-1, or Super Game Boy)
	/// have separate workspace data for each processor.
	/// </remarks>
	public class CpuDebugWorkspace {
		/// <summary>
		/// Gets or sets the list of watch expression entries for this CPU.
		/// </summary>
		/// <value>
		/// A list of watch expression strings (e.g., "PlayerHP", "$7E0000").
		/// </value>
		public List<string> WatchEntries { get; set; } = new();

		/// <summary>
		/// Gets or sets the labels defined for this CPU's memory space.
		/// </summary>
		/// <value>
		/// A list of <see cref="CodeLabel"/> instances mapping addresses to names and comments.
		/// </value>
		public List<CodeLabel> Labels { get; set; } = new();

		/// <summary>
		/// Gets or sets the breakpoints configured for this CPU.
		/// </summary>
		/// <value>
		/// A list of <see cref="Breakpoint"/> instances defining execution/read/write break conditions.
		/// </value>
		public List<Breakpoint> Breakpoints { get; set; } = new();
	}

	/// <summary>
	/// Represents the complete debug workspace for a ROM, persisted as JSON.
	/// </summary>
	/// <remarks>
	/// <para>
	/// The workspace contains all debug session state that should survive across
	/// emulator sessions, including labels, breakpoints, watches, and table mappings.
	/// </para>
	/// <para>
	/// Workspaces are stored per-ROM in the debugger folder as "{RomName}.json".
	/// </para>
	/// </remarks>
	public class DebugWorkspace {
		/// <summary>
		/// Gets or sets the per-CPU workspace data, keyed by CPU type.
		/// </summary>
		/// <value>
		/// A dictionary mapping <see cref="CpuType"/> to <see cref="CpuDebugWorkspace"/>.
		/// </value>
		public Dictionary<CpuType, CpuDebugWorkspace> WorkspaceByCpu { get; set; } = new();

		/// <summary>
		/// Gets or sets the text table (TBL) mapping file paths.
		/// </summary>
		/// <value>
		/// An array of paths to TBL files used for text decoding.
		/// </value>
		public string[] TblMappings { get; set; } = [];

		/// <summary>
		/// Loads a debug workspace from a JSON file.
		/// </summary>
		/// <param name="path">Full path to the workspace JSON file.</param>
		/// <returns>
		/// The loaded <see cref="DebugWorkspace"/>, or a new empty workspace if the file
		/// doesn't exist or cannot be parsed.
		/// </returns>
		/// <remarks>
		/// <para>
		/// After loading, this method:
		/// <list type="number">
		///   <item><description>Resets all labels, breakpoints, and watches</description></item>
		///   <item><description>Sets default labels if workspace is empty</description></item>
		///   <item><description>Restores labels, breakpoints, and watches from the loaded data</description></item>
		/// </list>
		/// </para>
		/// </remarks>
		public static DebugWorkspace Load(string path) {
			DebugWorkspace dbgWorkspace = new();
			if (File.Exists(path)) {
				try {
					string fileData = File.ReadAllText(path);
					dbgWorkspace = (DebugWorkspace?)JsonSerializer.Deserialize(fileData, typeof(DebugWorkspace), NexenSerializerContext.Default) ?? new DebugWorkspace();
				} catch {
				}
			}

			LabelManager.ResetLabels();
			BreakpointManager.ClearBreakpoints();
			WatchManager.ClearEntries();

			if (dbgWorkspace.WorkspaceByCpu.Count == 0) {
				DefaultLabelHelper.SetDefaultLabels();
			} else {
				foreach ((CpuType cpuType, CpuDebugWorkspace workspace) in dbgWorkspace.WorkspaceByCpu) {
					WatchManager.GetWatchManager(cpuType).WatchEntries = workspace.WatchEntries;
					LabelManager.SetLabels(workspace.Labels);
					BreakpointManager.AddBreakpoints(workspace.Breakpoints);
				}
			}

			return dbgWorkspace;
		}

		/// <summary>
		/// Saves the debug workspace to a JSON file.
		/// </summary>
		/// <param name="path">Full path where the workspace JSON file will be written.</param>
		/// <param name="cpuTypes">The set of CPU types to save workspace data for.</param>
		/// <remarks>
		/// Collects current labels, breakpoints, and watches from the respective managers
		/// for each CPU type and serializes them to JSON.
		/// </remarks>
		public void Save(string path, HashSet<CpuType> cpuTypes) {
			WorkspaceByCpu = new();
			foreach (CpuType cpuType in cpuTypes) {
				CpuDebugWorkspace workspace = new();
				workspace.WatchEntries = WatchManager.GetWatchManager(cpuType).WatchEntries;
				workspace.Labels = LabelManager.GetLabels(cpuType);
				workspace.Breakpoints = BreakpointManager.GetBreakpoints(cpuType);
				WorkspaceByCpu[cpuType] = workspace;
			}

			FileHelper.WriteAllText(path, JsonSerializer.Serialize(this, typeof(DebugWorkspace), NexenSerializerContext.Default));
		}

		/// <summary>
		/// Resets the workspace to a clean state with default labels.
		/// </summary>
		/// <remarks>
		/// Clears all watches, breakpoints, and labels, then applies default labels
		/// for the current platform (e.g., NES PPU registers, SNES hardware addresses).
		/// </remarks>
		public void Reset() {
			foreach ((CpuType cpuType, CpuDebugWorkspace workspace) in WorkspaceByCpu) {
				WatchManager.GetWatchManager(cpuType).WatchEntries = new();
				workspace.Breakpoints.Clear();
				workspace.Labels.Clear();
				workspace.WatchEntries.Clear();
			}

			BreakpointManager.ClearBreakpoints();
			LabelManager.ResetLabels();
			DefaultLabelHelper.SetDefaultLabels();
			LabelManager.RefreshLabels(true);
		}
	}
}
