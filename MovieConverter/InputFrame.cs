using System.Runtime.CompilerServices;
using System.Text;

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
	/// Private constructor for Clone — skips ControllerInput initialization
	/// since Clone immediately overwrites all controllers.
	/// Saves 8 * FrameCount allocations during deep copy.
	/// </summary>
	private InputFrame(int frameNumber, bool _) {
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
		var sb = new StringBuilder((portCount * 14) + 16);

		// Command prefix (if any)
		if (Command != FrameCommand.None) {
			sb.Append(GetCommandPrefix());
			sb.Append('|');
		}

		// Controller inputs
		for (int i = 0; i < portCount && i < Controllers.Length; i++) {
			if (i > 0) {
				sb.Append('|');
			}

			sb.Append(Controllers[i].ToNexenFormat());
		}

		for (int i = 0; i < portCount && i < Controllers.Length; i++) {
			if (Controllers[i].PaddlePosition is byte paddlePosition) {
				sb.Append("|P");
				sb.Append(i + 1);
				sb.Append("X:");
				sb.Append(paddlePosition);
			}
		}

		// Lag frame marker
		if (IsLagFrame) {
			sb.Append("|LAG");
		}

		// Comment (if any)
		if (!string.IsNullOrEmpty(Comment)) {
			sb.Append("|# ");
			sb.Append(Comment);
		}

		return sb.ToString();
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

		// Split by pipe using span-based parsing to avoid string[] allocation
		// Buffer sized for worst case: 1 CMD + 8 ports + 8 paddle + 1 LAG + 1 comment + margin
		ReadOnlySpan<char> span = line.AsSpan();
		Span<Range> ranges = stackalloc Range[24];
		int count = span.Split(ranges, '|');
		int portIndex = 0;

		for (int i = 0; i < count; i++) {
			ReadOnlySpan<char> part = span[ranges[i]].Trim();

			// Skip empty parts
			if (part.IsEmpty) {
				continue;
			}

			// Check for special markers
			if (part[0] == '#') {
				// Comment — must allocate string for storage
				frame.Comment = part[1..].Trim().ToString();
				continue;
			}

			if (part.Equals("LAG", StringComparison.Ordinal)) {
				frame.IsLagFrame = true;
				continue;
			}

			if (part.StartsWith("CMD:", StringComparison.Ordinal)) {
				// Command — also parses FDS disk number/side if present
				frame.Command = ParseCommand(part[4..], out byte? fdsDisk, out byte? fdsSide);
				if (fdsDisk.HasValue) frame.FdsDiskNumber = fdsDisk;
				if (fdsSide.HasValue) frame.FdsDiskSide = fdsSide;
				continue;
			}

			if (TryParsePaddleMetadata(part, out int paddlePort, out byte paddlePosition)) {
				frame.Controllers[paddlePort].PaddlePosition = paddlePosition;
				continue;
			}

			// Controller input
			if (portIndex < MaxPorts && part.Length >= 8) {
				frame.Controllers[portIndex] = ControllerInput.FromNexenFormat(part);
				portIndex++;
			}
		}

		return frame;
	}

	private static bool TryParsePaddleMetadata(ReadOnlySpan<char> part, out int port, out byte value) {
		port = -1;
		value = 0;

		if (part.Length < 5 || part[0] != 'P') {
			return false;
		}

		int xMarker = part.IndexOf("X:", StringComparison.Ordinal);
		if (xMarker <= 1) {
			return false;
		}

		if (!int.TryParse(part[1..xMarker], out int portOneBased)) {
			return false;
		}

		port = portOneBased - 1;
		if (port < 0 || port >= MaxPorts) {
			port = -1;
			return false;
		}

		if (!byte.TryParse(part[(xMarker + 2)..], out value)) {
			port = -1;
			return false;
		}

		return true;
	}

	/// <summary>
	/// Create a deep copy of this frame
	/// </summary>
	public InputFrame Clone() {
		var clone = new InputFrame(FrameNumber, false) {
			Command = Command,
			IsLagFrame = IsLagFrame,
			Comment = Comment,
			FdsDiskNumber = FdsDiskNumber,
			FdsDiskSide = FdsDiskSide
		};

		int sourceLen = Controllers.Length;
		for (int i = 0; i < clone.Controllers.Length; i++) {
			clone.Controllers[i] = i < sourceLen
				? Controllers[i].Clone()
				: new ControllerInput();
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
		var sb = new StringBuilder(64);
		sb.Append("CMD:");

		bool first = true;
		void AppendCmd(string cmd) {
			if (!first) sb.Append(',');
			sb.Append(cmd);
			first = false;
		}

		if (Command.HasFlag(FrameCommand.SoftReset)) {
			AppendCmd("SOFT_RESET");
		}

		if (Command.HasFlag(FrameCommand.HardReset)) {
			AppendCmd("HARD_RESET");
		}

		if (Command.HasFlag(FrameCommand.FdsInsert)) {
			if (!first) sb.Append(',');
			sb.Append("FDS_INSERT:");
			sb.Append(FdsDiskNumber ?? 0);
			sb.Append(':');
			sb.Append(FdsDiskSide ?? 0);
			first = false;
		}

		if (Command.HasFlag(FrameCommand.FdsSelect)) {
			if (!first) sb.Append(',');
			sb.Append("FDS_SELECT:");
			sb.Append(FdsDiskNumber ?? 0);
			sb.Append(':');
			sb.Append(FdsDiskSide ?? 0);
			first = false;
		}

		if (Command.HasFlag(FrameCommand.VsInsertCoin)) {
			AppendCmd("VS_COIN");
		}

		if (Command.HasFlag(FrameCommand.VsServiceButton)) {
			AppendCmd("VS_SERVICE");
		}

		if (Command.HasFlag(FrameCommand.ControllerSwap)) {
			AppendCmd("CTRL_SWAP");
		}

		if (Command.HasFlag(FrameCommand.Pause)) {
			AppendCmd("PAUSE");
		}

		if (Command.HasFlag(FrameCommand.PowerOff)) {
			AppendCmd("POWER_OFF");
		}

		if (Command.HasFlag(FrameCommand.Atari2600Select)) {
			AppendCmd("A26_SELECT");
		}

		if (Command.HasFlag(FrameCommand.Atari2600Reset)) {
			AppendCmd("A26_RESET");
		}

		return sb.ToString();
	}

	private static FrameCommand ParseCommand(ReadOnlySpan<char> commandStr, out byte? fdsDiskNumber, out byte? fdsDiskSide) {
		FrameCommand command = FrameCommand.None;
		fdsDiskNumber = null;
		fdsDiskSide = null;
		Span<Range> ranges = stackalloc Range[16];
		int count = commandStr.Split(ranges, ',');

		for (int i = 0; i < count; i++) {
			ReadOnlySpan<char> trimmed = commandStr[ranges[i]].Trim();
			if (trimmed.StartsWith("FDS_INSERT:", StringComparison.OrdinalIgnoreCase) ||
				trimmed.StartsWith("FDS_SELECT:", StringComparison.OrdinalIgnoreCase)) {
				bool isInsert = trimmed.StartsWith("FDS_INSERT", StringComparison.OrdinalIgnoreCase);
				command |= isInsert ? FrameCommand.FdsInsert : FrameCommand.FdsSelect;
				// Parse disk number and side from "FDS_INSERT:N:S" or "FDS_SELECT:N:S"
				int prefixLen = isInsert ? "FDS_INSERT:".Length : "FDS_SELECT:".Length;
				ReadOnlySpan<char> args = trimmed[prefixLen..];
				int colonIdx = args.IndexOf(':');
				if (colonIdx >= 0) {
					if (byte.TryParse(args[..colonIdx], out byte disk)) {
						fdsDiskNumber = disk;
					}
					if (byte.TryParse(args[(colonIdx + 1)..], out byte side)) {
						fdsDiskSide = side;
					}
				} else if (byte.TryParse(args, out byte diskOnly)) {
					fdsDiskNumber = diskOnly;
				}
				continue;
			}

			if (trimmed.Equals("SOFT_RESET", StringComparison.OrdinalIgnoreCase)) {
				command |= FrameCommand.SoftReset;
			} else if (trimmed.Equals("HARD_RESET", StringComparison.OrdinalIgnoreCase)) {
				command |= FrameCommand.HardReset;
			} else if (trimmed.Equals("FDS_INSERT", StringComparison.OrdinalIgnoreCase)) {
				command |= FrameCommand.FdsInsert;
			} else if (trimmed.Equals("FDS_SELECT", StringComparison.OrdinalIgnoreCase)) {
				command |= FrameCommand.FdsSelect;
			} else if (trimmed.Equals("VS_COIN", StringComparison.OrdinalIgnoreCase)) {
				command |= FrameCommand.VsInsertCoin;
			} else if (trimmed.Equals("VS_SERVICE", StringComparison.OrdinalIgnoreCase)) {
				command |= FrameCommand.VsServiceButton;
			} else if (trimmed.Equals("CTRL_SWAP", StringComparison.OrdinalIgnoreCase)) {
				command |= FrameCommand.ControllerSwap;
			} else if (trimmed.Equals("PAUSE", StringComparison.OrdinalIgnoreCase)) {
				command |= FrameCommand.Pause;
			} else if (trimmed.Equals("POWER_OFF", StringComparison.OrdinalIgnoreCase)) {
				command |= FrameCommand.PowerOff;
			} else if (trimmed.Equals("A26_SELECT", StringComparison.OrdinalIgnoreCase)) {
				command |= FrameCommand.Atari2600Select;
			} else if (trimmed.Equals("A26_RESET", StringComparison.OrdinalIgnoreCase)) {
				command |= FrameCommand.Atari2600Reset;
			}
		}

		return command;
	}

	public override string ToString() => $"Frame {FrameNumber}: {ToNexenLogLine()}";
}
