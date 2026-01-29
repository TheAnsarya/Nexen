#pragma once
#include "pch.h"
#include <stack>
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include "Debugger/DebugTypes.h"
#include "Utilities/SimpleLock.h"

class Debugger;
class LabelManager;
class IDebugger;

/// <summary>
/// Binary and unary operators for expression evaluation.
/// </summary>
/// <remarks>
/// Operator categories:
/// - Arithmetic: Addition, Substration, Multiplication, Division, Modulo
/// - Bitwise: ShiftLeft, ShiftRight, BinaryAnd, BinaryOr, BinaryXor, BinaryNot
/// - Logical: Equal, NotEqual, SmallerThan, SmallerOrEqual, GreaterThan, GreaterOrEqual, LogicalAnd, LogicalOr
/// - Memory access: Bracket (read 8-bit), Braces (read 16-bit), ReadDword (read 32-bit)
/// - Unary: Plus, Minus, BinaryNot, LogicalNot, AbsoluteAddress
/// 
/// Enum values:
/// - Start at 2000000000000 to avoid collision with EvalValues
/// - Sequential numbering for fast precedence lookup
/// </remarks>
enum EvalOperators : int64_t {
	// Binary operators
	Multiplication = 2000000000000,
	Division,
	Modulo,
	Addition,
	Substration,
	ShiftLeft,
	ShiftRight,
	SmallerThan,
	SmallerOrEqual,
	GreaterThan,
	GreaterOrEqual,
	Equal,
	NotEqual,
	BinaryAnd,
	BinaryXor,
	BinaryOr,
	LogicalAnd,
	LogicalOr,

	// Unary operators
	Plus,
	Minus,
	BinaryNot,
	LogicalNot,
	AbsoluteAddress,
	ReadDword, // Read dword (32-bit)

	// Used to read ram address
	Bracket, // Read byte (8-bit)
	Braces,  // Read word (16-bit)

	// Special value, not used as an operator
	Parenthesis,
};

/// <summary>
/// CPU register and system value identifiers for expression evaluation.
/// </summary>
/// <remarks>
/// Platform register coverage:
/// - 6502 (NES): A, X, Y, SP, PC, PS (flags), DB, PB
/// - 65816 (SNES): + DB, PB, DP (direct page), K (program bank), M/X flags
/// - Z80 (SMS/GB): A, B, C, D, E, F, H, L, AF, BC, DE, HL, IX, IY, SP, PC, I, R
/// - Z80 alternate: Alt A-HL registers
/// - ARM7TDMI (GBA): R0-R15, SrcReg, DstReg
/// - Super FX (GSU): R0-R15, SFR, PBR, RomBR, RamBR
/// 
/// Special values:
/// - PPU state: PpuFrameCount, PpuCycle, PpuScanline, PpuVramAddress
/// - Interrupts: Nmi, Irq
/// - Memory operation: IsRead, IsWrite, IsDma, IsDummy, Value, Address
/// - Platform-specific: Sprite0Hit, VerticalBlank, etc.
/// 
/// Enum values:
/// - Start at 3000000000000 to avoid collision with EvalOperators
/// - Sequential numbering for fast token lookup
/// </remarks>
enum EvalValues : int64_t {
	RegA = 3000000000000,
	RegX,
	RegY,

	R0,   ///< ARM/GSU R0 register
	R1,   ///< ARM/GSU R1 register
	R2,   ///< ARM/GSU R2 register
	R3,   ///< ARM/GSU R3 register
	R4,   ///< ARM/GSU R4 register
	R5,   ///< ARM/GSU R5 register
	R6,   ///< ARM/GSU R6 register
	R7,   ///< ARM/GSU R7 register
	R8,   ///< ARM/GSU R8 register
	R9,   ///< ARM/GSU R9 register
	R10,  ///< ARM/GSU R10 register
	R11,  ///< ARM/GSU R11 register
	R12,  ///< ARM/GSU R12 register
	R13,  ///< ARM/GSU R13 register (SP)
	R14,  ///< ARM/GSU R14 register (LR)
	R15,  ///< ARM/GSU R15 register (PC)
	SrcReg,   ///< ARM source register
	DstReg,   ///< ARM destination register
	SFR,      ///< GSU status/flag register
	PBR,      ///< GSU program bank register
	RomBR,    ///< GSU ROM bank register
	RamBR,    ///< GSU RAM bank register

	RegB,     ///< Z80 B register
	RegC,     ///< Z80 C register
	RegD,     ///< Z80 D register
	RegE,     ///< Z80 E register
	RegF,     ///< Z80 F (flags) register
	RegH,     ///< Z80 H register
	RegL,     ///< Z80 L register
	RegAF,    ///< Z80 AF register pair
	RegBC,    ///< Z80 BC register pair
	RegDE,    ///< Z80 DE register pair
	RegHL,    ///< Z80 HL register pair
	RegIX,    ///< Z80 IX index register
	RegIY,    ///< Z80 IY index register

	RegAltA,  ///< Z80 alternate A register
	RegAltB,  ///< Z80 alternate B register
	RegAltC,  ///< Z80 alternate C register
	RegAltD,  ///< Z80 alternate D register
	RegAltE,  ///< Z80 alternate E register
	RegAltF,  ///< Z80 alternate F register
	RegAltH,  ///< Z80 alternate H register
	RegAltL,  ///< Z80 alternate L register
	RegAltAF, ///< Z80 alternate AF register pair
	RegAltBC, ///< Z80 alternate BC register pair
	RegAltDE, ///< Z80 alternate DE register pair
	RegAltHL, ///< Z80 alternate HL register pair
	RegI,     ///< Z80 interrupt vector register
	RegR,     ///< Z80 refresh register

	RegTR,    ///< 65816 transfer register
	RegTRB,   ///< 65816 test and reset bits
	RegRP,    ///< 65816 register position
	RegDP,    ///< 65816 direct page
	RegDR,    ///< 65816 data register
	RegSR,    ///< 65816 status register
	RegK,     ///< 65816 program bank
	RegM,     ///< 65816 memory mode flag
	RegN,     ///< 65816 negative flag

	RegPB,    ///< 65816 program bank
	RegP,     ///< 6502 processor status
	RegMult,  ///< 65816 multiply result

	RegMDR,   ///< Memory data register
	RegMAR,   ///< Memory address register
	RegDPR,   ///< Direct page register

	RegSP,    ///< Stack pointer (all platforms)
	RegDB,    ///< 65816 data bank
	RegPS,    ///< Processor status (all platforms)

	RegPC,    ///< Program counter (all platforms)
	PpuFrameCount,    ///< PPU frame counter
	PpuCycle,         ///< PPU cycle in current scanline
	PpuHClock,        ///< PPU horizontal clock
	PpuScanline,      ///< PPU scanline number

	PpuVramAddress,       ///< PPU VRAM address
	PpuTmpVramAddress,    ///< PPU temporary VRAM address

	Nmi,              ///< NMI interrupt flag
	Irq,              ///< IRQ interrupt flag
	Value,            ///< Memory operation value
	Address,          ///< Memory operation address
	MemoryAddress,    ///< Memory operation absolute address
	IsWrite,          ///< Memory operation is write
	IsRead,           ///< Memory operation is read
	IsDma,            ///< Memory operation is DMA
	IsDummy,          ///< Memory operation is dummy (no side effects)
	OpProgramCounter, ///< Program counter at operation

	RegPS_Carry,      ///< Processor status carry flag
	RegPS_Zero,
	RegPS_Interrupt,
	RegPS_Memory,
	RegPS_Index,
	RegPS_Decimal,
	RegPS_Overflow,
	RegPS_Negative,

	Sprite0Hit,
	VerticalBlank,
	SpriteOverflow,
	SpriteCollision,

	SpcDspReg,

	PceVramTransferDone,
	PceSatbTransferDone,
	PceScanlineDetected,
	PceIrqVdc2,
	PceSelectedPsgChannel,
	PceSelectedVdcRegister,

	SmsVdpAddressReg,
	SmsVdpCodeReg,

	CPSR,

	RegAX,
	RegBX,
	RegCX,
	RegDX,

	RegAL,
	RegBL,
	RegCL,
	RegDL,

	RegAH,
	RegBH,
	RegCH,
	RegDH,

	RegCS,
	RegDS,
	RegES,
	RegSS,

	RegSI,
	RegDI,
	RegBP,
	RegIP,

	FirstLabelIndex,
};

enum class EvalResultType : int32_t {
	Numeric = 0,
	Boolean = 1,
	Invalid = 2,
	DivideBy0 = 3,
	OutOfScope = 4
};

class StringHasher {
public:
	size_t operator()(const std::string& t) const {
		// Quick hash for expressions - most are likely to have different lengths, and not expecting dozens of breakpoints, either, so this should be fine.
		return t.size();
	}
};

/// <summary>
/// Compiled expression data (RPN + labels).
/// </summary>
struct ExpressionData {
	deque<int64_t> RpnQueue;  ///< Reverse Polish Notation queue (operators and operands)
	vector<string> Labels;    ///< Referenced label names (for label → value lookup)
};

/// <summary>
/// Evaluates conditional expressions for breakpoints and debugger.
/// </summary>
/// <remarks>
/// Expression syntax:
/// - Arithmetic: +, -, *, /, %, ** (power)
/// - Bitwise: &, |, ^, ~, <<, >>
/// - Logical: &&, ||, ==, !=, <, <=, >, >=
/// - Memory access: [addr] (8-bit), {addr} (16-bit), @addr (32-bit)
/// - Labels: $labelName
/// - Registers: A, X, Y, SP, PC, etc. (platform-specific)
/// - Special values: scanline, cycle, frame, isread, iswrite
/// 
/// Expression compilation:
/// 1. Tokenize: Split expression into tokens (numbers, operators, labels, registers)
/// 2. Parse: Convert infix to Reverse Polish Notation (RPN) using shunting-yard algorithm
/// 3. Cache: Store compiled RPN in _cache (keyed by expression string)
/// 4. Evaluate: Execute RPN queue with current CPU/PPU state
/// 
/// RPN evaluation:
/// - Stack-based execution (no recursion, fast)
/// - Operators pop operands from stack, push result
/// - Memory access resolves addresses at evaluation time
/// - Label lookup via LabelManager
/// 
/// Performance optimizations:
/// - RPN cache (compile once, evaluate many times)
/// - Inline operator precedence checks
/// - Fast hash for expression cache (string length)
/// - Lock-free evaluation (cache lock only during compilation)
/// 
/// Use cases:
/// - Conditional breakpoints: "[0x7E0000] > 100"
/// - Watchpoints: "A != 0xFF && iswrite"
/// - Trace logger conditions: "A == 0xFF && iswrite"
/// - Memory viewer expressions: "[0x2000] & 0x80"
/// </remarks>
class ExpressionEvaluator {
private:
	static const vector<string> _binaryOperators;       ///< Binary operator strings ("+", "-", "*", etc.)
	static const vector<int> _binaryPrecedence;         ///< Binary operator precedence (1-10)
	static const vector<string> _unaryOperators;        ///< Unary operator strings ("-", "+", "~", "!")
	static const vector<int> _unaryPrecedence;          ///< Unary operator precedence
	static const unordered_set<string> _operators;      ///< All valid operator strings

	unordered_map<string, ExpressionData, StringHasher> _cache;  ///< RPN cache (expression → compiled data)
	SimpleLock _cacheLock;  ///< Cache access lock

	Debugger* _debugger;           ///< Main debugger instance
	IDebugger* _cpuDebugger;       ///< CPU-specific debugger
	LabelManager* _labelManager;   ///< Label/symbol manager
	CpuType _cpuType;              ///< Target CPU type
	MemoryType _cpuMemory;         ///< Target CPU memory type

	/// <summary>
	/// Check if token is an operator.
	/// </summary>
	/// <param name="token">Token string</param>
	/// <param name="precedence">Output operator precedence</param>
	/// <param name="unaryOperator">True if unary operator</param>
	/// <returns>True if token is operator</returns>
	bool IsOperator(string token, int& precedence, bool unaryOperator);
	
	/// <summary>
	/// Get operator enum from token.
	/// </summary>
	/// <param name="token">Operator token</param>
	/// <param name="unaryOperator">True if unary operator</param>
	/// <returns>Operator enum</returns>
	EvalOperators GetOperator(string token, bool unaryOperator);
	
	/// <summary>
	/// Get available register/value tokens for current CPU.
	/// </summary>
	/// <returns>Map of token name → EvalValues enum</returns>
	unordered_map<string, int64_t>* GetAvailableTokens();
	
	/// <summary>
	/// Check for special tokens (labels, hex/binary literals).
	/// </summary>
	/// <param name="expression">Expression string</param>
	/// <param name="pos">Current position (updated on match)</param>
	/// <param name="output">Output token string</param>
	/// <param name="data">Expression data (labels stored here)</param>
	/// <returns>True if special token found</returns>
	bool CheckSpecialTokens(string expression, size_t& pos, string& output, ExpressionData& data);

	// Platform-specific token getters (register names → EvalValues)
	unordered_map<string, int64_t>& GetSnesTokens();
	unordered_map<string, int64_t>& GetSpcTokens();
	unordered_map<string, int64_t>& GetGsuTokens();
	unordered_map<string, int64_t>& GetCx4Tokens();
	unordered_map<string, int64_t>& GetNecDspTokens();
	unordered_map<string, int64_t>& GetSt018Tokens();
	unordered_map<string, int64_t>& GetGameboyTokens();
	unordered_map<string, int64_t>& GetNesTokens();
	unordered_map<string, int64_t>& GetPceTokens();
	unordered_map<string, int64_t>& GetSmsTokens();
	unordered_map<string, int64_t>& GetGbaTokens();
	unordered_map<string, int64_t>& GetWsTokens();

	// Platform-specific value getters (EvalValues → current register value)
	int64_t GetSnesTokenValue(int64_t token, EvalResultType& resultType);
	int64_t GetSpcTokenValue(int64_t token, EvalResultType& resultType);
	int64_t GetGsuTokenValue(int64_t token, EvalResultType& resultType);
	int64_t GetCx4TokenValue(int64_t token, EvalResultType& resultType);
	int64_t GetNecDspTokenValue(int64_t token, EvalResultType& resultType);
	int64_t GetSt018TokenValue(int64_t token, EvalResultType& resultType);
	int64_t GetGameboyTokenValue(int64_t token, EvalResultType& resultType);
	int64_t GetNesTokenValue(int64_t token, EvalResultType& resultType);
	int64_t GetPceTokenValue(int64_t token, EvalResultType& resultType);
	int64_t GetSmsTokenValue(int64_t token, EvalResultType& resultType);
	int64_t GetGbaTokenValue(int64_t token, EvalResultType& resultType);
	int64_t GetWsTokenValue(int64_t token, EvalResultType& resultType);

	/// <summary>
	/// Convert value to boolean result.
	/// </summary>
	/// <param name="value">Input value</param>
	/// <param name="resultType">Output result type (Boolean)</param>
	/// <returns>0 (false) or 1 (true)</returns>
	bool ReturnBool(int64_t value, EvalResultType& resultType);

	/// <summary>
	/// Process tokens shared across all platforms (PC, cycle, frame, etc.).
	/// </summary>
	/// <param name="token">Token string</param>
	/// <returns>Token value or -1 if not shared token</returns>
	int64_t ProcessSharedTokens(string token);

	/// <summary>
	/// Tokenize expression and extract next token.
	/// </summary>
	/// <param name="expression">Expression string</param>
	/// <param name="pos">Current position (updated)</param>
	/// <param name="data">Expression data</param>
	/// <param name="success">Output success flag</param>
	/// <param name="previousTokenIsOp">True if previous token was operator</param>
	/// <returns>Next token string</returns>
	string GetNextToken(string expression, size_t& pos, ExpressionData& data, bool& success, bool previousTokenIsOp);
	
	/// <summary>
	/// Process special operators (brackets, braces, memory reads).
	/// </summary>
	/// <param name="evalOp">Operator enum</param>
	/// <param name="opStack">Operator stack (shunting-yard)</param>
	/// <param name="precedenceStack">Precedence stack</param>
	/// <param name="outputQueue">RPN output queue</param>
	/// <returns>True if operator processed</returns>
	bool ProcessSpecialOperator(EvalOperators evalOp, std::stack<EvalOperators>& opStack, std::stack<int>& precedenceStack, vector<int64_t>& outputQueue);
	
	/// <summary>
	/// Convert infix expression to Reverse Polish Notation.
	/// </summary>
	/// <param name="expression">Infix expression string</param>
	/// <param name="data">Output RPN data</param>
	/// <returns>True if conversion successful</returns>
	/// <remarks>
	/// Shunting-yard algorithm:
	/// 1. Read tokens left to right
	/// 2. Numbers go directly to output queue
	/// 3. Operators go to operator stack (pop higher precedence first)
	/// 4. Left paren goes to operator stack
	/// 5. Right paren pops until left paren
	/// 6. At end, pop all operators to output
	/// </remarks>
	bool ToRpn(string expression, ExpressionData& data);
	
	/// <summary>
	/// Internal evaluation with caching.
	/// </summary>
	/// <param name="expression">Expression string</param>
	/// <param name="resultType">Output result type</param>
	/// <param name="operationInfo">Memory operation context</param>
	/// <param name="addressInfo">Address context</param>
	/// <param name="success">Output success flag</param>
	/// <returns>Evaluation result</returns>
	int64_t PrivateEvaluate(string expression, EvalResultType& resultType, MemoryOperationInfo& operationInfo, AddressInfo& addressInfo, bool& success);
	
	/// <summary>
	/// Get cached RPN or compile new.
	/// </summary>
	/// <param name="expression">Expression string</param>
	/// <param name="success">Output success flag</param>
	/// <returns>Compiled RPN data or nullptr if invalid</returns>
	ExpressionData* PrivateGetRpnList(string expression, bool& success);

protected:
public:
	/// <summary>
	/// Constructor for expression evaluator.
	/// </summary>
	/// <param name="debugger">Main debugger instance</param>
	/// <param name="cpuDebugger">CPU-specific debugger</param>
	/// <param name="cpuType">Target CPU type</param>
	ExpressionEvaluator(Debugger* debugger, IDebugger* cpuDebugger, CpuType cpuType);

	/// <summary>
	/// Evaluate compiled RPN expression.
	/// </summary>
	/// <param name="data">Compiled RPN data</param>
	/// <param name="resultType">Output result type</param>
	/// <param name="operationInfo">Memory operation context (for isread, iswrite, value, etc.)</param>
	/// <param name="addressInfo">Address context</param>
	/// <returns>Evaluation result</returns>
	int64_t Evaluate(ExpressionData& data, EvalResultType& resultType, MemoryOperationInfo& operationInfo, AddressInfo& addressInfo);
	
	/// <summary>
	/// Evaluate expression string.
	/// </summary>
	/// <param name="expression">Expression string (compiled and cached)</param>
	/// <param name="resultType">Output result type</param>
	/// <param name="operationInfo">Memory operation context</param>
	/// <param name="addressInfo">Address context</param>
	/// <returns>Evaluation result</returns>
	int64_t Evaluate(string expression, EvalResultType& resultType, MemoryOperationInfo& operationInfo, AddressInfo& addressInfo);
	
	/// <summary>
	/// Get compiled RPN for expression.
	/// </summary>
	/// <param name="expression">Expression string</param>
	/// <param name="success">Output success flag</param>
	/// <returns>Compiled RPN data</returns>
	ExpressionData GetRpnList(string expression, bool& success);

	/// <summary>
	/// Get all available tokens (registers, operators, etc.) as string list.
	/// </summary>
	/// <param name="tokenList">Output token list (tab-separated)</param>
	void GetTokenList(char* tokenList);

	/// <summary>
	/// Validate expression syntax.
	/// </summary>
	/// <param name="expression">Expression string</param>
	/// <returns>True if expression valid</returns>
	bool Validate(string expression);

#if _DEBUG
	/// <summary>
	/// Run expression evaluator unit tests.
	/// </summary>
	void RunTests();
#endif
};