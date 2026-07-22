# SherrrBerrr iOS 1.0 architecture

## Release topology

`SherrrBerrrCore` is the single static C++ DSP/parameter core. `GabberShermanKick` is the JUCE AudioProcessor wrapper. On iOS, JUCE emits an AUv3 MusicDevice and a Standalone container app from that same processor; desktop builds VST3 + Standalone from the same source.

## Frozen contracts

- 128 public parameters; IDs/order are unchanged from the hardened 0.8 baseline.
- `GSK4State`, schema version 4.
- Legacy state migration remains enabled.
- `.sherrrberrr` portable preset files serialize the same processor state.
- Imported sample content is embedded in state for recall.
- `Source/Dsp` and `Source/Parameters` are release-frozen for 1.0.

## Realtime safety

Sample replacement is decoded off the audio thread, published atomically, and committed by the audio thread. Lifecycle state gates processing across prepare/release transitions. Host state/sample metadata access is synchronized outside the realtime rendering path.

## iOS UI

The editor uses a responsive touch mode with SRC / XY / FLT / DST pages on constrained views and a denser overview where iPad host dimensions allow it. The minimum logical editor size is suitable for compact AUv3 hosts.

## Build identity

- Product: Kick flavored SherrrBerrr
- Version: 1.0.0
- iOS bundle ID: `com.selimbibberrr.sherrrberrr`
- Plugin code: `Gsk4`
- AUv3 main type: `kAudioUnitType_MusicDevice`
- Minimum iOS/iPadOS: 15.0
- Device family: iPhone + iPad

`codemagic.yaml` is the authoritative cloud-build entry point.
