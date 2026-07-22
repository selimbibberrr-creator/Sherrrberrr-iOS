# SherrrBerrr iOS 1.0

Production source for **Kick flavored SherrrBerrr** on iOS/iPadOS. One JUCE/C++ DSP and state core builds as:

- **AUv3 MusicDevice** for iOS/iPadOS hosts;
- **Standalone iOS/iPadOS app** containing the AUv3 extension;
- desktop **VST3 + Standalone** from the same core.

## 1.0 release contract

- Version: `1.0.0`
- iOS bundle ID: `com.selimbibberrr.sherrrberrr`
- iOS deployment target: `15.0+`
- Device family: universal iPhone + iPad
- Public parameter contract: exactly **128 parameters**; existing IDs/order retained
- State root: `GSK4State`, schema `4`; older schema and `GSK3State` migration retained
- Portable presets: `.sherrrberrr`
- Imported samples: WAV/AIFF/FLAC, embedded in state for recall
- Same DSP core for VST3, AUv3 and Standalone

The 1.0 release plumbing does **not** alter the DSP/parameter source files from the hardened 0.8 baseline. `tools/validate_ios_1_0.py` fingerprints all 34 critical DSP/parameter files on every CI build.

## Codemagic

`codemagic.yaml` is committed at repository root and contains three workflows:

1. **SherrrBerrr 1.0 - iOS Simulator Compile** — no Apple credentials required. This is the first workflow to run; it proves the Xcode/JUCE project compiles as AUv3 + Standalone.
2. **SherrrBerrr 1.0 - Signed iPad Development IPA** — creates/fetches Development signing files, builds an installable IPA for registered devices and verifies the AUv3 `.appex` is embedded.
3. **SherrrBerrr 1.0 - TestFlight Internal Upload** — App Store signing and upload for internal TestFlight testing without external beta review submission.

The workflows pin **Xcode 26.4** and **JUCE 8.0.10** for reproducibility. Build numbers are generated as `1.0.$CM_BUILD_NUMBER` while the user-facing version remains `1.0.0`.

### First Codemagic build

Run **`ios-simulator-smoke` first**. It needs no signing setup.

For signed builds, create a Codemagic environment-variable group named **`appstore_credentials`** containing these secret variables:

- `APP_STORE_CONNECT_PRIVATE_KEY`
- `APP_STORE_CONNECT_KEY_IDENTIFIER`
- `APP_STORE_CONNECT_ISSUER_ID`
- `CERTIFICATE_PRIVATE_KEY`

The unsigned smoke workflow records the exact generated Standalone + AUv3 bundle IDs as `bundle_ids.txt`. Register those identifiers in Apple Developer if needed. The signed workflows then discover the same IDs and create/fetch a matching provisioning profile for each before `xcode-project use-profiles` and IPA export.

See [`docs/CODEMAGIC_SETUP_1.0.md`](docs/CODEMAGIC_SETUP_1.0.md) for the exact UI steps and [`docs/IPAD_TEST_PLAN_1.0.md`](docs/IPAD_TEST_PLAN_1.0.md) for device/host validation.

## Local macOS build

```bash
git clone --branch 8.0.10 --depth 1 https://github.com/juce-framework/JUCE.git .deps/JUCE
export JUCE_DIR="$PWD/.deps/JUCE"
./tools/configure_ios.sh build-ios
cmake --build build-ios --config Release --target GabberShermanKick_AUv3 GabberShermanKick_Standalone
```

A signed local archive can be created with `tools/archive_ios_release.sh` after setting `DEVELOPMENT_TEAM` and `JUCE_DIR`.

## Validation

```bash
python3 tools/validate_ios_1_0.py
```

The release repository is intentionally cleaned of obsolete beta packaging files. Active release documentation is:

- `docs/RELEASE_NOTES_1.0.md`
- `docs/CODEMAGIC_SETUP_1.0.md`
- `docs/IPAD_TEST_PLAN_1.0.md`
- `docs/IOS_RELEASE_VALIDATION_1.0.json`
- `docs/ARCHITECTURE_1.0.md`
