namespace Diz.Import.Mesen;

/// <summary>
/// Mesen2 Protocol message types for DiztinGUIsh live streaming.
/// This maps to the C++ DiztinguishProtocol.h in Mesen2.
/// </summary>
public enum MesenMessageType : byte
{
    // Connection Management
    Handshake = 0x01,
    Disconnect = 0x02,
    Error = 0x03,
    
    // Execution Tracing  
    ExecTrace = 0x04,
    
    // CDL (Code/Data Logger) Updates
    CdlUpdate = 0x05,
    
    // Memory Operations
    MemoryDump = 0x06,
    MemoryWrite = 0x07,
    MemoryRead = 0x08,
    
    // Debugging
    Breakpoint = 0x09,
    CpuState = 0x0A,
    
    // Labels and Symbols
    LabelAdd = 0x0B,
    LabelUpdate = 0x0C,
    LabelDelete = 0x0D,
    
    // Frame Tracking
    FrameStart = 0x0E,
    FrameEnd = 0x0F
}

/// <summary>
/// Handshake message from Mesen2 server.
/// Sent immediately upon connection to identify protocol version.
/// </summary>
public struct MesenHandshakeMessage
{
    public byte ProtocolVersion;      // Currently 1
    public uint EmulatorVersionMajor; // e.g., 0 for v0.7.0
    public uint EmulatorVersionMinor; // e.g., 7 for v0.7.0
    public uint EmulatorVersionPatch; // e.g., 0 for v0.7.0
    public ushort ServerPort;         // TCP port server is listening on
    public uint RomSize;              // Size of loaded ROM in bytes
    public uint RomCrc32;             // CRC32 checksum of ROM
    
    public override string ToString() => 
        $"Protocol v{ProtocolVersion}, Mesen2 v{EmulatorVersionMajor}.{EmulatorVersionMinor}.{EmulatorVersionPatch}, Port {ServerPort}, ROM {RomSize} bytes (CRC32: 0x{RomCrc32:X8})";
}

/// <summary>
/// CPU execution trace message.
/// Sent for each CPU instruction executed with SNES-specific flags.
/// </summary>
public struct MesenExecTraceMessage
{
    public uint PC;              // Program Counter (24-bit SNES address)
    public byte Opcode;          // Instruction opcode
    public bool MFlag;           // M flag (accumulator size: true=8bit, false=16bit)
    public bool XFlag;           // X flag (index size: true=8bit, false=16bit)  
    public byte DataBank;        // Data Bank register (DB)
    public ushort DirectPage;    // Direct Page register (D)
    public uint EffectiveAddr;   // Effective address for operand (if applicable)
    
    public override string ToString() => 
        $"PC:${PC:X6} Op:${Opcode:X2} M:{(MFlag ? "8" : "16")} X:{(XFlag ? "8" : "16")} DB:${DataBank:X2} D:${DirectPage:X4} EA:${EffectiveAddr:X6}";
}

/// <summary>
/// CDL (Code/Data Logger) update message.
/// Sent when emulator determines a byte's usage type (code, data, etc.).
/// </summary>
public struct MesenCdlUpdateMessage
{
    public uint Address;     // SNES address that was updated
    public byte CdlFlags;    // CDL flags indicating usage type
    
    // CDL flag definitions (matches SNES emulator conventions)
    public bool IsCode => (CdlFlags & 0x01) != 0;           // Executed as CPU instruction
    public bool IsData => (CdlFlags & 0x02) != 0;           // Read as data
    public bool IsIndirectData => (CdlFlags & 0x04) != 0;   // Read as indirect pointer
    public bool IsIndexedData => (CdlFlags & 0x08) != 0;    // Read with indexing
    
    public override string ToString() => 
        $"${Address:X6}: {(IsCode ? "C" : "")}{(IsData ? "D" : "")}{(IsIndirectData ? "I" : "")}{(IsIndexedData ? "X" : "")} (0x{CdlFlags:X2})";
}

/// <summary>
/// CPU state snapshot message.
/// Contains complete CPU register state at a specific moment.
/// </summary>
public struct MesenCpuStateMessage
{
    public uint PC;            // Program Counter
    public ushort A;           // Accumulator
    public ushort X;           // X Index Register  
    public ushort Y;           // Y Index Register
    public ushort SP;          // Stack Pointer
    public byte ProcessorFlags; // P Register (Status flags)
    public byte DataBank;      // Data Bank Register
    public ushort DirectPage;  // Direct Page Register
    public bool EmulationMode; // 6502 emulation mode flag
    
    // Processor flag helpers
    public bool CarryFlag => (ProcessorFlags & 0x01) != 0;
    public bool ZeroFlag => (ProcessorFlags & 0x02) != 0;
    public bool InterruptFlag => (ProcessorFlags & 0x04) != 0;
    public bool DecimalFlag => (ProcessorFlags & 0x08) != 0;
    public bool IndexFlag => (ProcessorFlags & 0x10) != 0;     // X flag (16/8 bit index)
    public bool MemoryFlag => (ProcessorFlags & 0x20) != 0;    // M flag (16/8 bit accumulator)
    public bool OverflowFlag => (ProcessorFlags & 0x40) != 0;
    public bool NegativeFlag => (ProcessorFlags & 0x80) != 0;
    
    public override string ToString() => 
        $"PC:${PC:X6} A:${A:X4} X:${X:X4} Y:${Y:X4} SP:${SP:X4} P:${ProcessorFlags:X2} DB:${DataBank:X2} D:${DirectPage:X4} {(EmulationMode ? "EMU" : "NAT")}";
}

/// <summary>
/// Memory dump message.
/// Contains a contiguous block of memory data from specified address range.
/// </summary>
public struct MesenMemoryDumpMessage
{
    public uint StartAddress;      // Starting SNES address
    public ushort Size;            // Number of bytes in data array
    public byte[] Data;            // Memory contents (length = Size)
    
    public override string ToString() => 
        $"Memory ${StartAddress:X6}-${StartAddress + Size - 1:X6} ({Size} bytes)";
}

/// <summary>
/// Label operation message.
/// For synchronizing labels/symbols between Mesen2 and DiztinGUIsh.
/// </summary>
public struct MesenLabelMessage
{
    public uint Address;        // SNES address for label
    public string LabelName;    // Label text (empty for delete operations)
    public string Comment;      // Optional comment text
    
    public override string ToString() => 
        $"${Address:X6}: {LabelName}" + (string.IsNullOrEmpty(Comment) ? "" : $" // {Comment}");
}

/// <summary>
/// Frame boundary messages.
/// Sent at start/end of each emulated video frame for synchronization.
/// </summary>
public struct MesenFrameMessage
{
    public uint FrameNumber;    // Incrementing frame counter
    public bool IsStart;        // true for frame start, false for frame end
    
    public override string ToString() => 
        $"Frame #{FrameNumber} {(IsStart ? "START" : "END")}";
}

/// <summary>
/// Error message from Mesen2 server.
/// Sent when server encounters problems or needs to disconnect.
/// </summary>
public struct MesenErrorMessage
{
    public ushort ErrorCode;    // Error identifier
    public string ErrorText;    // Human-readable error description
    
    // Common error codes
    public const ushort ERROR_PROTOCOL_VERSION_MISMATCH = 1;
    public const ushort ERROR_ROM_NOT_LOADED = 2;
    public const ushort ERROR_EMULATION_STOPPED = 3;
    public const ushort ERROR_MEMORY_ACCESS_FAILED = 4;
    public const ushort ERROR_INVALID_REQUEST = 5;
    
    public override string ToString() => 
        $"Error {ErrorCode}: {ErrorText}";
}