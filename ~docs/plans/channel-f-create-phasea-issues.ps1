$items = @(
	@{t='[972.1] Channel F core skeleton: console/cpu/bus/video/audio/input classes'; l='cpp,channel-f,high-priority,accuracy'; b='Parent: #972`nCreate compile-safe class skeletons for Channel F console and subsystem boundaries with deterministic stubs and serialization placeholders.'},
	@{t='[972.2] Channel F deterministic frame loop scaffold + save-state smoke tests'; l='cpp,channel-f,savestates,testing,high-priority'; b='Parent: #972`nImplement no-op deterministic RunFrame/Reset/Serialize behavior and add smoke tests validating deterministic state transitions.'},
	@{t='[997.1] Register Channel F in runtime factory and ROM extension routing'; l='cpp,ui,channel-f,high-priority'; b='Parent: #997`nWire Channel F console type into runtime creation path and ROM extension routing without enabling full gameplay yet.'},
	@{t='[1018.1] Channel F core microbenchmark harness (frame tick + opcode metadata)'; l='performance,testing,channel-f,medium-priority'; b='Parent: #1018`nAdd initial benchmark scenarios for Channel F scaffolding hot paths and metadata lookup to establish baseline costs.'}
)

foreach($i in $items) {
	$url = gh issue create --repo TheAnsarya/Nexen --title $i.t --body $i.b --label $i.l
	$num = ($url -split '/')[-1]
	gh issue comment $num --repo TheAnsarya/Nexen --body-file ~docs/plans/channel-f-origin-prompt.md
	Write-Output "#$num $($i.t)"
}
