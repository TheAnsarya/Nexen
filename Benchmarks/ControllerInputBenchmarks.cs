using BenchmarkDotNet.Attributes;
using BenchmarkDotNet.Configs;
using BenchmarkDotNet.Order;
using Nexen.MovieConverter;

namespace Nexen.Benchmarks;

/// <summary>
/// Benchmarks for ControllerInput.Equals/GetHashCode correctness fix (#268)
/// and TAS allocation reductions (#269).
/// </summary>
[MemoryDiagnoser]
[Orderer(SummaryOrderPolicy.FastestToSlowest)]
[GroupBenchmarksBy(BenchmarkLogicalGroupRule.ByCategory)]
public sealed class ControllerInputBenchmarks {
	private ControllerInput _inputA = null!;
	private ControllerInput _inputB = null!;
	private Queue<int> _ringBuffer = null!;
	private Dictionary<string, bool> _cachedDict = new(12);
	private InputFrame _sampleFrame = null!;

	[GlobalSetup]
	public void Setup() {
		_inputA = new ControllerInput {
			A = true, B = true, Start = true,
			Type = ControllerType.Gamepad,
			MouseX = 100, MouseY = 200,
			MouseDeltaX = 5, MouseDeltaY = -3,
			MouseButton1 = true,
			AnalogX = 64, AnalogY = -32,
			AnalogRX = 10, AnalogRY = -10,
			TriggerL = 128, TriggerR = 64,
			PaddlePosition = 150,
			PowerPadButtons = 0x1234,
			KeyboardData = new byte[] { 0x01, 0x02, 0x03 }
		};

		_inputB = new ControllerInput {
			A = true, B = true, Start = true,
			Type = ControllerType.Gamepad,
			MouseX = 100, MouseY = 200,
			MouseDeltaX = 5, MouseDeltaY = -3,
			MouseButton1 = true,
			AnalogX = 64, AnalogY = -32,
			AnalogRX = 10, AnalogRY = -10,
			TriggerL = 128, TriggerR = 64,
			PaddlePosition = 150,
			PowerPadButtons = 0x1234,
			KeyboardData = new byte[] { 0x01, 0x02, 0x03 }
		};

		_ringBuffer = new Queue<int>();
		for (int i = 0; i < 500; i++) {
			_ringBuffer.Enqueue(i * 10);
		}

		_sampleFrame = new InputFrame(0);
		_sampleFrame.Controllers[0].A = true;
		_sampleFrame.Controllers[0].B = true;
		_sampleFrame.Controllers[0].Up = true;
	}

	#region Equals: OLD partial vs NEW complete

	[Benchmark(Baseline = true, Description = "OLD: Equals (6 fields only)")]
	[BenchmarkCategory("Equals")]
	public bool Equals_OldPartial() {
		return _inputA.ButtonBits == _inputB.ButtonBits &&
			_inputA.Type == _inputB.Type &&
			_inputA.MouseX == _inputB.MouseX &&
			_inputA.MouseY == _inputB.MouseY &&
			_inputA.AnalogX == _inputB.AnalogX &&
			_inputA.AnalogY == _inputB.AnalogY;
	}

	[Benchmark(Description = "NEW: Equals (all 18 fields)")]
	[BenchmarkCategory("Equals")]
	public bool Equals_NewComplete() {
		return _inputA.Equals(_inputB);
	}

	[Benchmark(Description = "NEW: NotEquals early-exit")]
	[BenchmarkCategory("Equals")]
	public bool Equals_NotEqual_EarlyExit() {
		var diff = new ControllerInput { A = false };
		return _inputA.Equals(diff);
	}

	#endregion

	#region GetHashCode: OLD XOR vs NEW HashCode builder

	[Benchmark(Baseline = true, Description = "OLD: XOR GetHashCode (4 fields)")]
	[BenchmarkCategory("HashCode")]
	public int HashCode_OldXor() {
		return _inputA.ButtonBits ^ (int)_inputA.Type ^
			(_inputA.MouseX ?? 0) ^ (_inputA.AnalogX ?? 0);
	}

	[Benchmark(Description = "NEW: HashCode builder (14 fields)")]
	[BenchmarkCategory("HashCode")]
	public int HashCode_NewBuilder() {
		return _inputA.GetHashCode();
	}

	#endregion

	#region Ring Buffer: OLD LINQ rebuild vs NEW in-place

	[Benchmark(Baseline = true, Description = "OLD: LINQ + new Queue rebuild")]
	[BenchmarkCategory("RingBuffer")]
	public int RingBuffer_OldLinq() {
		var ring = new Queue<int>(_ringBuffer);
		int fromFrame = 2500;
		ring = new Queue<int>(ring.Where(f => f < fromFrame));
		return ring.Count;
	}

	[Benchmark(Description = "NEW: In-place dequeue/re-enqueue")]
	[BenchmarkCategory("RingBuffer")]
	public int RingBuffer_NewInPlace() {
		var ring = new Queue<int>(_ringBuffer);
		int fromFrame = 2500;
		int count = ring.Count;
		for (int i = 0; i < count; i++) {
			int f = ring.Dequeue();
			if (f < fromFrame) {
				ring.Enqueue(f);
			}
		}
		return ring.Count;
	}

	#endregion

	#region Lua GetFrameInput: OLD new dict vs NEW cached dict

	[Benchmark(Baseline = true, Description = "OLD: new Dictionary per call")]
	[BenchmarkCategory("LuaDict")]
	public Dictionary<string, bool> LuaDict_NewEachCall() {
		var input = _sampleFrame.Controllers[0];
		return new Dictionary<string, bool> {
			["a"] = input.A, ["b"] = input.B,
			["x"] = input.X, ["y"] = input.Y,
			["l"] = input.L, ["r"] = input.R,
			["select"] = input.Select, ["start"] = input.Start,
			["up"] = input.Up, ["down"] = input.Down,
			["left"] = input.Left, ["right"] = input.Right
		};
	}

	[Benchmark(Description = "NEW: Cached dictionary (clear + repopulate)")]
	[BenchmarkCategory("LuaDict")]
	public Dictionary<string, bool> LuaDict_Cached() {
		var input = _sampleFrame.Controllers[0];
		_cachedDict.Clear();
		_cachedDict["a"] = input.A;
		_cachedDict["b"] = input.B;
		_cachedDict["x"] = input.X;
		_cachedDict["y"] = input.Y;
		_cachedDict["l"] = input.L;
		_cachedDict["r"] = input.R;
		_cachedDict["select"] = input.Select;
		_cachedDict["start"] = input.Start;
		_cachedDict["up"] = input.Up;
		_cachedDict["down"] = input.Down;
		_cachedDict["left"] = input.Left;
		_cachedDict["right"] = input.Right;
		return _cachedDict;
	}

	#endregion
}
