using Nexen.Interop;
using ReactiveUI.SourceGenerators;

namespace Nexen.Config; 
public sealed partial class MovieRecordConfig : BaseConfig<MovieRecordConfig> {
	[Reactive] public partial RecordMovieFrom RecordFrom { get; set; } = RecordMovieFrom.CurrentState;
	[Reactive] public partial string Author { get; set; } = "";
	[Reactive] public partial string Description { get; set; } = "";
}
