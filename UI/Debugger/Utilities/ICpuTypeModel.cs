using Nexen.Interop;

namespace Nexen.Debugger.Utilities; 
public interface ICpuTypeModel {
	CpuType CpuType { get; set; }
	void OnGameLoaded();
}
