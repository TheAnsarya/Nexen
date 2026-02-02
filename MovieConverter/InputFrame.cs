using System.Runtime.CompilerServices;

namespace Nexen.MovieConverter;

/// <summary>
/// Represents a single frame of input in a TAS movie.
/// Contains input for all controller ports plus frame metadata.
/// </summary>
public sealed class InputFrame : IEquatable<InputFrame> {
	/// <summary>
	/// Maximum number of controller ports supported
	/// </summary>
	public const int MaxPorts = 8;

	/// <summary>
	/// Frame number (0-indexed from start of movie)
	/// </summary>
	public int FrameNumber { get; set; }

	/// <summary>
	/// Input state for each controller port (up to 8 ports)
	/// </summary>
	public ControllerInput[] Controllers { get; set; } = new ControllerInput[MaxPorts];

	/// <summary>
	/// Special command for this frame (reset, disk insert, etc.)
	/// </summary>
	public FrameCommand Command { get; set; } = FrameCommand.None;

	/// <summary>
	/// Whether this frame is a lag frame (input was not polled by game)
	/// </summary>
	public bool IsLagFrame { get; set; }

	/// <summary>
	/// Optional comment/annotation for this frame (for TAS editors)
	/// </summary>
	public string? Comment { get; set; }

	/// <summary>
	/// FDS disk number for disk change commands (0-based)
	/// </summary>
	public byte? FdsDiskNumber { get; set; }

	/// <summary>
	/// FDS disk side for disk change commands (0=A, 1=B)
	/// </summary>
	public byte? FdsDiskSide { get; set; }

	/// <summary>
	/// Creates a new InputFrame with empty controller inputs
	/// </summary>
	public InputFrame() {
		for (int i = 0; i < Controllers.Length; i++) {
			Controllers[i] = new ControllerInput();
		}
	}

	/// <summary>
	/// Creates a new InputFrame with the specified frame number
	/// </summary>
	/// <param name="frameNumber">0-indexed frame number</param>
	public InputFrame(int frameNumber) : this() {
		FrameNumber = frameNumber;
	}

	/// <summary>
	/// Get input for a specific port
	/// </summary>
	/// <param name="port">Port index (0-based)</param>
	/// <returns>Controller input for that port</returns>
	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public ControllerInput GetPort(int port) {
		ArgumentOutOfRangeException.ThrowIfNegative(port);
		ArgumentOutOfRangeException.ThrowIfGreaterThanOrEqual(port, MaxPorts);
		return Controllers[port];
	}

	/// <summary>
	/// Check if any controller has input on this frame
	/// </summary>
	public bool HasAnyInput {
		[MethodImpl(MethodImplOptions.AggressiveInlining)]
		get {
			foreach (ControllerInput ctrl in Controllers) {
				if (ctrl.HasInput) {
					return true;
				}
			}

			return false;
		}
	}

	/// <summary>
	/// Convert to Nexen input log format (pipe-separated ports)
	/// </summary>
	/// <param name="portCount">Number of ports to include</param>
	/// <returns>Single line for input log</returns>
	public string ToNexenLogLine(int portCount = 2) {
		var parts = new List<string>(portCount + 3);

		// Command prefix (if any)
		if (Command != FrameCommand.None) {
			parts.Add(GetCommandPrefix());
		}

		// Controller inputs
		for (int i = 0; i < portCount && i < Controllers.Length; i++) {
			parts.Add(Controllers[i].ToNexenFormat());
		}

		// Lag frame marker
		if (IsLagFrame) {
			parts.Add("LAG");
		}

		// Comment (if any)
		if (!string.IsNullOrEmpty(Comment)) {
			parts.Add($"# {Comment}");
		}

		return string.Join("|", parts);
	}

	/// <summary>
	/// Parse from Nexen input log format
	/// </summary>
	/// <param name="line">Single line from input log</param>
	/// <param name="frameNumber">Frame number to assign</param>
	/// <returns>Parsed InputFrame</returns>
	public static InputFrame FromNexenLogLine(string line, int frameNumber) {
		var frame = new InputFrame(frameNumber);

		// Handle empty lines
		if (string.IsNullOrWhiteSpace(line)) {
			return frame;
		}

		// Split by pipe
		string[] parts = line.Split('|');
		int portIndex = 0;

		foreach (string part in parts) {
			string trimmed = part.Trim();

			// Skip empty parts
			if (string.IsNullOrEmpty(trimmed)) {
				continue;
			}

			// Check for special markers
			if (trimmed.StartsWith('#')) {
				// Comment
				frame.Comment = trimmed[1..].Trim();
				continue;
			}

			if (trimmed == "LAG") {
				frame.IsLagFrame = true;
				continue;
			}

			if (trimmed.StartsWith("CMD:")) {
				// Command
				frame.Command = ParseCommand(trimmed[4..]);
				continue;
			}

			// Controller input
			if (portIndex < MaxPorts && trimmed.Length >= 8) {
				frame.Controllers[portIndex] = ControllerInput.FromNexenFormat(trimmed);
				portIndex++;
			}
		}

		return frame;
	}

	/// <summary>
	/// Create a deep copy of this frame
	/// </summary>
	public InputFrame Clone() {
		var clone = new InputFrame(FrameNumber) {
			Command = Command,
			IsLagFrame = IsLagFrame,
			Comment = Comment,
			FdsDiskNumber = FdsDiskNumber,
			FdsDiskSide = FdsDiskSide
		};

		for (int i = 0; i < Controllers.Length; i++) {
			clone.Controllers[i] = Controllers[i].Clone();
		}

		return clone;
	}

	/// <summary>
	/// Check equality with another InputFrame
	/// </summary>
	public bool Equals(InputFrame? other) {
		if (other is null) {
			return false;
		}

		if (ReferenceEquals(this, other)) {
			return true;
		}

		if (FrameNumber != other.FrameNumber || Command != other.Command ||
			IsLagFrame != other.IsLagFrame) {
			return false;
		}

		for (int i = 0; i < Controllers.Length; i++) {
			if (Controllers[i] != other.Controllers[i]) {
				return false;
			}
		}

		return true;
	}

	public override bool Equals(object? obj) => Equals(obj as InputFrame);

	public override int GetHashCode() {
		var hash = new HashCode();
		hash.Add(FrameNumber);
		hash.Add(Command);
		hash.Add(IsLagFrame);
		foreach (ControllerInput ctrl in Controllers) {
			hash.Add(ctrl.ButtonBits);
		}

		return hash.ToHashCode();
	}

	public static bool operator ==(InputFrame? left, InputFrame? right) =>
		left is null ? right is null : left.Equals(right);

	public static bool operator !=(InputFrame? left, InputFrame? right) => !(left == right);

	private string GetCommandPrefix() {
		var commands = new List<string>(4);

		if (Command.HasFlag(FrameCommand.SoftReset)) {
			commands.Add("SOFT_RESET");
		}

		if (Command.HasFlag(FrameCommand.HardReset)) {
			commands.Add("HARD_RESET");
		}

		if (Command.HasFlag(FrameCommand.FdsInsert)) {
			commands.Add($"FDS_INSERT:{FdsDiskNumber ?? 0}:{FdsDiskSide ?? 0}");
		}

		if (Command.HasFlag(FrameCommand.FdsSelect)) {
			commands.Add($"FDS_SELECT:{FdsDiskNumber ?? 0}:{FdsDiskSide ?? 0}");
		}

		if (Command.HasFlag(FrameCommand.VsInsertCoin)) {
			commands.Add("VS_COIN");
		}

		if (Command.HasFlag(FrameCommand.VsServiceButton)) {
			commands.Add("VS_SERVICE");
		}

		if (Command.HasFlag(FrameCommand.ControllerSwap)) {
			commands.Add("CTRL_SWAP");
		}

		if (Command.HasFlag(FrameCommand.Pause)) {
			commands.Add("PAUSE");
		}

		if (Command.HasFlag(FrameCommand.PowerOff)) {
			commands.Add("POWER_OFF");
		}

		return $"CMD:{string.Join(',', commands)}";
	}

	private static FrameCommand ParseCommand(string commandStr) {
		FrameCommand command = FrameCommand.None;
		string[] parts = commandStr.Split(',');

		foreach (string part in parts) {
			string trimmed = part.Trim().ToUpperInvariant();
			if (trimmed.StartsWith("FDS_INSERT:") || trimmed.StartsWith("FDS_SELECT:")) {
				// Parse disk info: FDS_INSERT:0:1 means disk 0, side B
				command |= trimmed.StartsWith("FDS_INSERT") ? FrameCommand.FdsInsert : FrameCommand.FdsSelect;
				continue;
			}

			command |= trimmed switch {
				"SOFT_RESET" => FrameCommand.SoftReset,
				"HARD_RESET" => FrameCommand.HardReset,
				"FDS_INSERT" => FrameCommand.FdsInsert,
				"FDS_SELECT" => FrameCommand.FdsSelect,
				"VS_COIN" => FrameCommand.VsInsertCoin,
				"VS_SERVICE" => FrameCommand.VsServiceButton,
				"CTRL_SWAP" => FrameCommand.ControllerSwap,
				"PAUSE" => FrameCommand.Pause,
				"POWER_OFF" => FrameCommand.PowerOff,
				_ => FrameCommand.None
			};
		}

		return command;
	}

	public override string ToString() => $"Frame {FrameNumber}: {ToNexenLogLine()}";
}
