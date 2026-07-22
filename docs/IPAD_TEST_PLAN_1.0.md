# iPad acceptance plan — SherrrBerrr 1.0

Use the exact IPA produced by the Codemagic `ios-ipad-development` workflow or the exact TestFlight build from `ios-testflight-internal`.

## Standalone gate

- Launch/relaunch 20 times with no crash.
- Audio starts and AUDITION produces sound.
- MIDI note-on/off and velocity behave correctly.
- SRC, XY, FLT and DST pages remain reachable in portrait and landscape.
- All visible controls are touch-operable; double-tap reset and fine adjustment work.
- Factory presets load; MUTATE remains bounded.
- `.sherrrberrr` save/load round-trips state.
- WAV/AIFF/FLAC import recalls after app restart through embedded sample state.
- Background/foreground and audio interruption do not leave the engine silent or unstable.

## AUv3 host gate

Test in at least one real AUv3 host on iPad.

- SherrrBerrr appears as a MusicDevice and opens its custom editor.
- Insert/remove repeatedly without host crash.
- Run four simultaneous instances; each retains independent state.
- Save host project, close host, reopen, and verify all instance states/presets/samples.
- Host automation writes/reads parameters without jumps or cross-instance leakage.
- MIDI, note-off gated mode, preset changes and XY mappings work while transport runs.
- Rotate/rescale host view; compact pages remain usable without clipped critical controls.
- 30-minute playback stress has no runaway memory growth, stuck notes, NaNs or audio dropout attributable to the plugin.

## Severity

- P0: crash, host crash, corrupted project/state, install/sign failure.
- P1: no audio, wrong DSP/state recall, AUv3 not discoverable, severe automation failure.
- P2: lifecycle/multi-instance/interruption defect with workaround.
- P3: touch/layout/cosmetic defect.

1.0 device acceptance requires zero open P0/P1 issues. P2/P3 fixes may ship as 1.0.x patches if they do not change the 128-parameter/state compatibility contract.
