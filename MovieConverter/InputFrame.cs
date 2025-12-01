namespace Mesen.MovieConverter;

/// <summary>
/// Represents a single frame of input in a TAS movie
/// </summary>
public class InputFrame
{
    /// <summary>
    /// Frame number (0-indexed)
    /// </summary>
    public int FrameNumber { get; set; }

    /// <summary>
    /// Input for each controller port (up to 8 ports)
    /// </summary>
    public ControllerInput[] Controllers { get; set; } = new ControllerInput[8];

    /// <summary>
    /// Special commands for this frame (reset, etc.)
    /// </summary>
    public FrameCommand Command { get; set; } = FrameCommand.None;

    /// <summary>
    /// Whether this is a lag frame (no input polled)
    /// </summary>
    public bool IsLagFrame { get; set; }

    public InputFrame()
    {
        for (int i = 0; i < Controllers.Length; i++)
        {
            Controllers[i] = new ControllerInput();
        }
    }
}

/// <summary>
/// Input state for a single controller
/// </summary>
public class ControllerInput
{
    // Standard SNES buttons
    public bool A { get; set; }
    public bool B { get; set; }
    public bool X { get; set; }
    public bool Y { get; set; }
    public bool L { get; set; }
    public bool R { get; set; }
    public bool Start { get; set; }
    public bool Select { get; set; }
    public bool Up { get; set; }
    public bool Down { get; set; }
    public bool Left { get; set; }
    public bool Right { get; set; }

    // Mouse/pointer input (for Super Scope, Mouse, etc.)
    public int? MouseX { get; set; }
    public int? MouseY { get; set; }
    public bool? MouseButton1 { get; set; }
    public bool? MouseButton2 { get; set; }

    /// <summary>
    /// The type of controller for this port
    /// </summary>
    public ControllerType Type { get; set; } = ControllerType.Gamepad;

    /// <summary>
    /// Convert to Mesen text format
    /// </summary>
    public string ToMesenFormat()
    {
        if (Type == ControllerType.None)
            return "";

        // Mesen format: BYsSudlrAXLR for SNES gamepad
        var chars = new char[12];
        chars[0] = B ? 'B' : '.';
        chars[1] = Y ? 'Y' : '.';
        chars[2] = Select ? 's' : '.';
        chars[3] = Start ? 'S' : '.';
        chars[4] = Up ? 'U' : '.';
        chars[5] = Down ? 'D' : '.';
        chars[6] = Left ? 'L' : '.';
        chars[7] = Right ? 'R' : '.';
        chars[8] = A ? 'A' : '.';
        chars[9] = X ? 'X' : '.';
        chars[10] = L ? 'l' : '.';
        chars[11] = R ? 'r' : '.';
        
        return new string(chars);
    }

    /// <summary>
    /// Parse from Mesen text format
    /// </summary>
    public static ControllerInput FromMesenFormat(string input)
    {
        var ctrl = new ControllerInput { Type = ControllerType.Gamepad };
        
        if (string.IsNullOrEmpty(input) || input.Length < 12)
            return ctrl;

        // Mesen format: BYsSudlrAXLR
        ctrl.B = input[0] != '.';
        ctrl.Y = input[1] != '.';
        ctrl.Select = input[2] != '.';
        ctrl.Start = input[3] != '.';
        ctrl.Up = input[4] != '.';
        ctrl.Down = input[5] != '.';
        ctrl.Left = input[6] != '.';
        ctrl.Right = input[7] != '.';
        ctrl.A = input[8] != '.';
        ctrl.X = input[9] != '.';
        ctrl.L = input[10] != '.';
        ctrl.R = input[11] != '.';

        return ctrl;
    }

    /// <summary>
    /// Convert to SMV 2-byte format
    /// </summary>
    public ushort ToSmvFormat()
    {
        ushort value = 0;
        
        // SMV bit layout (little-endian):
        // Byte 0: 00 00 00 00 R  L  X  A
        // Byte 1: Right Left Down Up Start Select Y B
        
        if (R) value |= 0x10;
        if (L) value |= 0x20;
        if (X) value |= 0x40;
        if (A) value |= 0x80;
        if (Right) value |= 0x0100;
        if (Left) value |= 0x0200;
        if (Down) value |= 0x0400;
        if (Up) value |= 0x0800;
        if (Start) value |= 0x1000;
        if (Select) value |= 0x2000;
        if (Y) value |= 0x4000;
        if (B) value |= 0x8000;

        return value;
    }

    /// <summary>
    /// Parse from SMV 2-byte format
    /// </summary>
    public static ControllerInput FromSmvFormat(ushort value)
    {
        var ctrl = new ControllerInput { Type = ControllerType.Gamepad };
        
        ctrl.R = (value & 0x10) != 0;
        ctrl.L = (value & 0x20) != 0;
        ctrl.X = (value & 0x40) != 0;
        ctrl.A = (value & 0x80) != 0;
        ctrl.Right = (value & 0x0100) != 0;
        ctrl.Left = (value & 0x0200) != 0;
        ctrl.Down = (value & 0x0400) != 0;
        ctrl.Up = (value & 0x0800) != 0;
        ctrl.Start = (value & 0x1000) != 0;
        ctrl.Select = (value & 0x2000) != 0;
        ctrl.Y = (value & 0x4000) != 0;
        ctrl.B = (value & 0x8000) != 0;

        return ctrl;
    }

    /// <summary>
    /// Convert to LSMV/FM2 text format (BYsSudlrAXLR)
    /// </summary>
    public string ToLsmvFormat()
    {
        var chars = new char[12];
        chars[0] = B ? 'B' : '.';
        chars[1] = Y ? 'Y' : '.';
        chars[2] = Select ? 's' : '.';
        chars[3] = Start ? 'S' : '.';
        chars[4] = Up ? 'u' : '.';
        chars[5] = Down ? 'd' : '.';
        chars[6] = Left ? 'l' : '.';
        chars[7] = Right ? 'r' : '.';
        chars[8] = A ? 'A' : '.';
        chars[9] = X ? 'X' : '.';
        chars[10] = L ? 'L' : '.';
        chars[11] = R ? 'R' : '.';
        
        return new string(chars);
    }

    /// <summary>
    /// Parse from LSMV text format
    /// </summary>
    public static ControllerInput FromLsmvFormat(string input)
    {
        var ctrl = new ControllerInput { Type = ControllerType.Gamepad };
        
        if (string.IsNullOrEmpty(input) || input.Length < 12)
            return ctrl;

        // LSMV format: BYsSudlrAXLR
        ctrl.B = char.ToUpper(input[0]) == 'B';
        ctrl.Y = char.ToUpper(input[1]) == 'Y';
        ctrl.Select = char.ToLower(input[2]) == 's';
        ctrl.Start = char.ToUpper(input[3]) == 'S';
        ctrl.Up = char.ToLower(input[4]) == 'u';
        ctrl.Down = char.ToLower(input[5]) == 'd';
        ctrl.Left = char.ToLower(input[6]) == 'l';
        ctrl.Right = char.ToLower(input[7]) == 'r';
        ctrl.A = char.ToUpper(input[8]) == 'A';
        ctrl.X = char.ToUpper(input[9]) == 'X';
        ctrl.L = char.ToUpper(input[10]) == 'L';
        ctrl.R = char.ToUpper(input[11]) == 'R';

        return ctrl;
    }
}

public enum ControllerType
{
    None,
    Gamepad,
    Mouse,
    SuperScope,
    Justifier,
    Multitap
}

[Flags]
public enum FrameCommand
{
    None = 0,
    SoftReset = 1,
    HardReset = 2,
    FdsInsert = 4,
    FdsSelect = 8,
    VsInsertCoin = 16
}
