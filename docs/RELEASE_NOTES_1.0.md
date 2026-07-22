# SherrrBerrr iOS 1.0.0

## Release status

1.0 promotes the hardened 0.8 AUv3/Standalone architecture to the production release line. The 128-parameter/state contract is frozen; 1.0 focuses on reproducible Apple builds, release identity, signing, packaging, iPad validation, and toolchain compatibility.

## Included

- Shared JUCE/C++ DSP core for desktop VST3, iOS AUv3 and Standalone.
- Touch-first SRC / XY / FLT / DST compact pages plus larger iPad overview layout.
- AUv3 lifecycle hardening, multi-instance isolation, safe sample publication and state restore.
- Eight factory starting points plus portable `.sherrrberrr` preset import/export.
- Embedded loaded-sample state and legacy schema migration.
- Production app icon, version `1.0.0`, stable iOS bundle ID.
- Codemagic unsigned compile, signed Development IPA, and internal TestFlight workflows.
- Signed IPA verification step requiring an embedded `.appex` before a build is considered successful.
- Apple/libc++ compatibility patch for shared sample publication: C++20 `std::atomic<std::shared_ptr<T>>` is replaced by atomic `shared_ptr` load/store operations during deterministic source bootstrap. This is a concurrency/toolchain compatibility change only; synthesis, filtering, distortion and parameter semantics are unchanged.

## Compatibility contract

No parameter IDs were renamed or reordered. State schema remains v4 to preserve 0.8 projects/presets. Legacy `GSK3State` and older schema migration paths remain active.

## Release blocker policy

A 1.0 Codemagic build is accepted only when the simulator compile workflow succeeds and the signed iPad build passes the device/host matrix in `IPAD_TEST_PLAN_1.0.md`. Any P0/P1 failure produces a 1.0.x patch without changing the public parameter contract.
