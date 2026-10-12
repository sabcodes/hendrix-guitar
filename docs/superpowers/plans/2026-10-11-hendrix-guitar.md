# Hendrix Guitar Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Deliver a playable Hendrix-inspired guitar AU in Logic on this Mac.

**Architecture:** A concrete GuitarEngine module owns sample mapping, MIDI voices, and expression behind load/prepare/render/reset. A concrete VintageTone module owns the fixed vintage effects chain behind prepare/process/reset. The JUCE processor connects both to saved parameters; the editor changes those parameters.

**Tech Stack:** JUCE 8.0.6 at commit 51a8a6d7aeae7326956d747737ccf1575e61e209, CMake 3.22+, C++17, Python 3 standard library for sample import, native macOS tar for archive extraction.

**Spec:** ../specs/2026-10-11-hendrix-guitar-design.md

## Global Constraints

- Personal-use macOS AU instrument for Logic only; no VST3 or standalone.
- Separate repository at /Users/sabarnoislam/Desktop/hendrix guitar.
- Private GitHub repository: https://github.com/sabcodes/hendrix-guitar; use gh.
- Notes come from Logic Musical Typing (Command-K); controls use the mouse.
- Use FreePats Fender direct samples under CC0 and preserve provenance/license.
- Package samples inside the AU; no external runtime sample paths.
- Support six voices, bends, vibrato, legato glide, sustain/feedback, double-stops, and simple chords.
- Effects order: wah, fuzz, optional octave-up, Uni-Vibe-style modulation, amp/cab.
- Save parameter state in Logic and expose principal controls to automation.
- Use a pinned JUCE submodule; no additional runtime dependency.
- Do not incorporate Hendrix/YouTube audio.
- Keep file loading, allocations, locks, and logging outside the audio callback.

## Review Focus

1. Missing or corrupt bundled assets: silent output and a visible load error, without repeated file access while rendering. Task 1 owns checks.
2. Repeated/overlapping notes, sustain changes, and transport stop: bounded voice count and no stuck sound. Task 2 owns checks.
3. Extreme controls, rapid automation, varying block sizes and sample rates: finite bounded output and correctly reported DSP latency. Task 3 owns checks.
4. Malformed/partial saved state or a preset change during playback: stable parameter IDs, safe defaults, and correct recall. Task 4 owns checks.
5. Relocated AU and paths containing spaces: samples resolve from the loaded plug-in bundle, including after the source directory is unavailable. Task 5 owns checks.

## Implementation choices

Build native arm64 for this Mac (macOS 15.6.1); deployment target 15.0. AU manufacturer code Hdxg, subtype Hdg1, bundle ID com.sabcodes.hendrixguitar, product name Hendrix Guitar. Use stereo output, no audio input, and MIDI input.

Use the unprocessed FreePats release 2022-09-11, SFZ+FLAC archive:
https://github.com/freepats/electric-guitar-FSBS-direct/releases/download/2022-09-11/EGuitarFSBS-direct-SFZ%2BFLAC-20220911.7z
SHA-256: 7ab2a4551bb8847342d6fd10cc56e0642fcaf43f3be8d665269c4dc9bfbbd599.

The inspected upstream mapping covers MIDI 35–86, uses four alternatives per zone, has velocity split 92/93 through much of its range, and a 0.2-second release. Preserve its actual regions; do not manufacture missing velocity layers. Choose alternatives deterministically in rotation. Out-of-range notes use the nearest end region transposed, with the sampled range shown in the editor help.

Defaults: polyphonic mode; bend ±2 semitones with adjustable range 1–12; legato glide 70 ms adjustable 5–250; vibrato 5.5 Hz with maximum depth 35 cents, scaled by CC1. CC64 holds/releases notes; CC120 clears all sound immediately; CC123 releases voices. In legato mode, overlapping notes glide with last-note priority and return to a still-held note on release.

Voice feedback is a bounded, smoothly introduced harmonic sustain blend tied to the voice envelope, including after the source recording ends. Keep the blend at most 25%; release it on note-off unless sustain is held. Do not use an unbounded recirculating audio loop.

## File structure and interfaces

- CMakeLists.txt, .gitmodules, .gitignore: build, JUCE pin, ignored build/archive/capture output.
- Scripts/import_samples.py: checksum, safe extraction, fixed-library SFZ conversion, asset audit.
- Resources/Guitar/: FLAC samples, mapping.json, LICENSE.txt, source.json.
- Source/GuitarEngine.h/.cpp: sample data and voices; no abstract sampler adapter.
- Source/VintageTone.h/.cpp: fixed DSP chain.
- Source/Parameters.h/.cpp: parameter layout, setting snapshots, three presets.
- Source/PluginProcessor.h/.cpp: JUCE host adapter, bundle location, state recall.
- Source/PluginEditor.h/.cpp: mouse controls, presets, load status and Musical Typing help.
- Tests/TestMain.cpp, Tests/GuitarChecks.cpp, Tests/SampleImportChecks.py: JUCE built-in UnitTest runner and Python unittest; no new test framework.
- Scripts/install_au.sh, README.md, THIRD_PARTY_NOTICES.md: installation, usage, provenance.

Runtime interfaces:
```cpp
struct PerformanceSettings {
    bool legato = false;
    float bendRangeSemitones = 2, glideMs = 70;
    float vibratoRateHz = 5.5f, vibratoDepthCents = 35, feedback = 0;
};
class GuitarEngine {
public:
    juce::Result load(const juce::File& directory); // before rendering; fail atomically
    void prepare(double sampleRate, int maxBlockSize);
    void render(juce::AudioBuffer<float>& output,
                const juce::MidiBuffer& midi, const PerformanceSettings& settings);
    void reset();
};
struct ToneSettings {
    float guitarVolume = .85f, master = .65f, fuzz = .35f;
    bool wahEnabled = false;
    float wahPosition = .5f, vibeRateHz = 2.5f, vibeDepth = .15f;
    float ampDrive = .4f, ampTone = .55f, cabTone = .5f, octave = 0;
};
class VintageTone {
public:
    void prepare(double sampleRate, int maxBlockSize);
    void process(juce::AudioBuffer<float>& audio, const ToneSettings& settings);
    void reset();
    int latencySamples() const;
};
juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
PerformanceSettings readPerformanceSettings(const juce::AudioProcessorValueTreeState&);
ToneSettings readToneSettings(const juce::AudioProcessorValueTreeState&);
void applyPreset(juce::AudioProcessorValueTreeState&, int presetIndex);
```

The processor stores the Result from load for editor status. The processor exposes its APVTS as a member named parameters for editor attachments. Helper functions for bundle resolution and sample regions remain private implementations; do not expose per-effect objects to the editor.

Local test helpers in Tests/GuitarChecks.cpp (not production interfaces): writeSineBank(const juce::File&, double sampleRate, double frequency) writes a temporary sample bank rooted at MIDI 57; rms(const juce::AudioBuffer<float>&), dominantFrequency(const juce::AudioBuffer<float>&, double sampleRate), powerAt(const juce::AudioBuffer<float>&, double sampleRate, double frequency), and finiteAndBounded(const juce::AudioBuffer<float>&, float limit) provide signal assertions. Implement these helpers with JUCE audio writers and straightforward deterministic signal measurements.

## Task 1: Deliver a playable sampled AU

**Files:** Create CMakeLists.txt, .gitignore, JUCE submodule, Scripts/import_samples.py, Resources/Guitar/, Source/GuitarEngine.h/.cpp, Source/PluginProcessor.h/.cpp, Tests/TestMain.cpp, Tests/GuitarChecks.cpp, Tests/SampleImportChecks.py, THIRD_PARTY_NOTICES.md.

**Interfaces:** Produce GuitarEngine load/prepare/render/reset, PerformanceSettings, and HendrixGuitarAudioProcessor. Initially use fixed settings and JUCE's generic editor. Tone processing is added in Task 3.

- [ ] Add JUCE as a submodule pinned to the commit above.
- [ ] Define the CMake HendrixGuitar AU target with the metadata above and a HendrixGuitarChecks console target.
- [ ] Implement Tests/TestMain.cpp using JUCE UnitTestRunner and nonzero exit on failures; register GuitarChecks and SampleImportChecks with CTest.
- [ ] Write failing import checks: test_checksum_mismatch, test_path_escape, test_inherited_zones, test_missing_sample, and test_unknown_opcode. Use a small synthetic SFZ fixture containing global/group/region inheritance; assert exact root/ranges/velocity/release and four alternatives.
- [ ] Run `python3 -m unittest discover -s Tests -p SampleImportChecks.py -v`; confirm the importer checks fail before implementation.
- [ ] Implement `import_bank(archive: pathlib.Path, destination: pathlib.Path) -> dict` in Scripts/import_samples.py. Check the pinned digest before extraction; reject absolute/traversal/symlink archive entries. Use native tar; convert only the observed SFZ opcodes sample, key, lokey, hikey, lovel, hivel, pitch_keycenter, lorand, hirand, ampeg_release. Reject unknown opcodes and unresolved files. Emit inherited regions to mapping.json and record URL, digest, version and license in source.json. Preserve FLAC and license; ignore the downloaded archive.
- [ ] Download the pinned release with curl --fail --location into an ignored build/cache directory. Import into Resources/Guitar. Audit region coverage 35–86, velocity 1–127, and each referenced decoded sample. Confirm individual committed assets are below GitHub's file limit.
- [ ] Write failing GuitarChecks: load of missing/corrupt assets fails; after failure a render produces zeros; a synthetic 220 Hz sample at its root produces nonzero finite output after note-on; no MIDI produces silence; sample-offset note-on does not sound early. Assert at the module interface:
```cpp
expect(engine.load(missingDirectory).failed());
engine.render(audio, noMidi, defaults);
expectEquals(audio.getMagnitude(0, audio.getNumSamples()), 0.0f);
// Valid fixture: note starts at offset 32.
expectEquals(audio.getMagnitude(0, 32), 0.0f);
expect(audio.getMagnitude(32, audio.getNumSamples() - 32) > 0.0f);
```
- [ ] Implement load and rendering, using JUCE AudioFormatManager for FLAC decode before the callback, immutable shared region buffers and six preallocated voices. Apply pitch ratio from source root/sample rate, interpolate playback, honor layer mapping and round-robin alternatives, ramp attacks/releases. Clamp MIDI/manifest values; fail the complete load if any referenced asset is invalid.
- [ ] Implement processor MIDI rendering and stereo output. Resolve the component binary with macOS dladdr on a local function address, then its Contents/Resources/Guitar; never use the Logic executable or source working directory. Copy resources into the AU in CMake. Configure/build with the command below; run CTest and inspect the bundle for resources.
- [ ] Commit `feat: add playable sampled guitar AU`. Push main, close milestone 2 only after its checks pass, create/push `v0.2.0-playable`.

## Task 2: Add expression and reliable note lifecycle

**Files:** Modify Source/GuitarEngine.h/.cpp and Tests/GuitarChecks.cpp.

**Interfaces:** Use PerformanceSettings; keep the engine interface unchanged. Produce working bends, vibrato, glide, sustain/feedback and six-voice polyphony.

- [ ] Write failing checks for six simultaneous notes and a seventh note's predictable voice stealing; repeated same-pitch notes; note-off after sustain release; CC120 immediate silence; CC123 release; legato last-note priority and returning to a held note.
- [ ] Use a synthetic periodic sample to assert pitch wheel maximum with range 2 changes frequency by `pow(2, 2.0 / 12.0)` within 1%; CC1 zero disables vibrato and full-scale enables it. Assert glide reaches target within its configured 70 ms, without replaying the pick attack.
- [ ] Add assertions using the synthetic bank and test helpers (audio buffers refer to the just-described event sequences):
```cpp
expectWithinAbsoluteError(checks::dominantFrequency(bentAudio, 48000.0) / 220.0,
                          std::pow(2.0, 2.0 / 12.0), 0.01);
expectEquals(afterAllSoundOff.getMagnitude(0, afterAllSoundOff.getNumSamples()), 0.0f);
expect(releasedTail.getMagnitude(0, releasedTail.getNumSamples()) < 1.0e-5f);
expect(checks::powerAt(afterSteal, 48000.0, 220.0) <
       checks::powerAt(afterSteal, 48000.0, 220.0 * std::sqrt(2.0)) * 0.01);
```
  Use six notes 57–62 followed by 63 for the steal case; inspect after an 8 ms stealing fade. Inspect the released tail one second after sustain release. Channel 2 sustain/bend must not alter channel 1 notes.
- [ ] Run GuitarChecks and confirm these new cases fail.
- [ ] Implement MIDI dispatch at event offsets and held-note bookkeeping by MIDI channel, with smoothed pitch transitions and sample-rate-independent vibrato. Keep note-off/repeated-note handling deterministic. Voice stealing fades the oldest voice to prevent a discontinuity.
- [ ] Implement bounded harmonic sustain/feedback as specified; assert feedback=1 still decays to silence after released notes, and transport/reset clears sample and harmonic state. Test zero-length buffers and block partitions 1, 64, 257, 1024 with the same event stream.
- [ ] Run GuitarChecks at 44.1, 48 and 96 kHz; inspect output metrics for finite samples and correct release. Commit `feat: add guitar expression and voice lifecycle`, push, close milestone 3, and push `v0.3.0-expression`.

## Task 3: Implement the vintage tone chain

**Files:** Create Source/VintageTone.h/.cpp; modify Source/PluginProcessor.h/.cpp, CMakeLists.txt, Tests/GuitarChecks.cpp.

**Interfaces:** Produce ToneSettings and VintageTone prepare/process/reset/latencySamples. Processor calls the engine, then tone processing and reports its latency to Logic.

- [ ] Write failing cases that exercise the public tone interface: silence stays silent after reset; all settings at extrema remain finite with peaks ≤1; input-volume reduction cleans up gain rather than only reducing final level; enabled wah changes the spectrum; octave-up raises the second harmonic; vibe depth 0 removes modulation.
- [ ] Add signal assertions to the tone cases. Use identical harmonic input for low/high guitar volume and compare distortion ratios:
```cpp
expect(checks::finiteAndBounded(extremeAudio, 1.0f));
expectEquals(silentAudio.getMagnitude(0, silentAudio.getNumSamples()), 0.0f);
expect(checks::powerAt(lowDrive, sr, 660.0) / checks::powerAt(lowDrive, sr, 220.0) <
       checks::powerAt(highDrive, sr, 660.0) / checks::powerAt(highDrive, sr, 220.0));
expect(checks::powerAt(octaveOn, sr, 440.0) > checks::powerAt(octaveOff, sr, 440.0));
```
- [ ] Test block sizes 0, 1, 64, 257 and blocks larger than prepare's capacity by chunking; test 44.1/48/96 kHz and rapid parameter changes. Run GuitarChecks to confirm new checks fail.
- [ ] Implement the fixed chain with JUCE DSP: pre-effect guitar volume, resonant swept wah, filtered asymmetric fuzz, optional rectified octave-up with DC removal, four-stage modulated all-pass vibe, driven amp EQ/waveshaping and cabinet filter coloration. Use smooth parameters, 4× JUCE oversampling for nonlinear stages, DC filtering and a bounded final output stage. Keep all temporary storage preallocated.
- [ ] Wire processor reset/release and latency reporting. Check `processor.getLatencySamples() == tone.latencySamples()` after prepare.
- [ ] Run CTest, build AU, and render reference clean/driven/wah passages to ignored files for listening. Tuning should serve the reference character, with no claim of circuit-perfect hardware.
- [ ] Commit `feat: add vintage guitar tone chain` and push. Leave milestone 4 open until Task 4 completes.

## Task 4: Add controls, presets and state recall

**Files:** Create Source/Parameters.h/.cpp, Source/PluginEditor.h/.cpp; modify Source/PluginProcessor.h/.cpp, CMakeLists.txt, Tests/GuitarChecks.cpp.

**Interfaces:** Produce the parameter functions above. Editor only accesses APVTS and load status; it does not reach into engine voices or DSP internals.

Parameter IDs: guitarVolume, master, fuzz, wahEnabled, wahPosition, vibeRate, vibeDepth, ampDrive, ampTone, cabTone, octave, feedback, legato, glideMs, bendRange, vibratoRate, vibratoDepth. Use ToneSettings defaults/ranges 0–1 except vibeRate 0.1–10 Hz; PerformanceSettings ranges are specified above, with vibratoRate 0.5–10 Hz and vibratoDepth 0–100 cents. Keep IDs stable.

- [ ] Write failing state tests: round-trip all IDs; changed settings survive processor recreation; malformed XML, wrong root and unsupported version preserve safe current defaults; omitted parameters receive defaults; preset changes during active notes do not lose note-off handling.
- [ ] Pin state recall through the processor interface:
```cpp
original.parameters.getParameter("wahPosition")->setValueNotifyingHost(0.8f);
original.getStateInformation(state);
restored.setStateInformation(state.getData(), static_cast<int>(state.getSize()));
expectWithinAbsoluteError(restored.parameters.getRawParameterValue("wahPosition")->load(),
                          0.8f, 1.0e-6f);
restored.setStateInformation("bad", 3);
expectWithinAbsoluteError(restored.parameters.getRawParameterValue("wahPosition")->load(),
                          0.8f, 1.0e-6f);
```
  Parameter IDs from the declared list must all round-trip. A valid partial state uses declared defaults for omitted IDs, while malformed state preserves the current values.
- [ ] Run GuitarChecks to confirm failures.
- [ ] Implement APVTS layout, atomic snapshots per block, and state root HendrixGuitarState with schemaVersion=1. Validate the root/version, merge recognized finite values into a fresh default parameter state, then replace state; never deserialize on the audio callback.
- [ ] Implement three named starting presets: Clean Touch, Driven Lead, Wah Fuzz. Clean Touch: fuzz=0, wah off, octave=0, ampDrive=.2. Driven Lead: fuzz=.65, wah off, octave=0, ampDrive=.65. Wah Fuzz: fuzz=.7, wah on, wahPosition=.6, ampDrive=.6. Keep other values at declared defaults; mouse preset selection uses parameter notifications/gestures.
- [ ] Build a compact vintage-inspired editor with rotary sliders/toggles attached to APVTS, preset selector, legato/bend/glide controls, load-error display, and help for Command-K, bend 1/2, modulation 3–8, sustain Tab and sampled note range. Provide readable labels, keyboard focus, and useful tooltips; no custom note-input system.
- [ ] Run CTest and build AU. Commit `feat: add guitar controls presets and saved state`, push, close milestone 4, and push `v0.4.0-tone`.

## Task 5: Install and validate in Logic

**Files:** Create Scripts/install_au.sh and README.md; update THIRD_PARTY_NOTICES.md and docs/validation.md.

**Interfaces:** Installer consumes the built Hendrix Guitar.component and installs to ~/Library/Audio/Plug-Ins/Components. Preserve an existing installed copy as a backup; do not modify SerumSynth.

- [ ] Run the complete CTest checks and Release build. Verify arm64 binary with file, bundled manifest/sample count and license, and no sample dependency on Desktop/SerumSynth.
- [ ] Implement a quoted-path installer and usage instructions. Relocate a copy of the bundle to a path containing spaces; confirm the bundle-based resource resolver still returns its Guitar directory.
- [ ] Install the AU and run `auval -v aumu Hdg1 Hdxg`. Confirm exit zero and validation passed. If AU validation fails, diagnose and fix before release.
- [ ] In Logic, create a software instrument track, select Hendrix Guitar, and play via Command-K. Check presets, velocity, bends, modulation, legato, sustain release, double-stops and chords; automate wah/volume; stop playback; save/reopen the project and confirm recall. Use desktop tools for what is available; if Logic requires a human-only step, request that specific step and keep the milestone open until confirmed.
- [ ] Record actual check results, plugin install path and known limitations in docs/validation.md. Include that global MIDI pitch bend bends all held notes and octave fuzz can be rough on chords; these are musical limits of the first version.
- [ ] Commit `release: validate Hendrix Guitar in Logic`, push, close milestone 5, push `v1.0.0` and create a private-repo GitHub release containing the AU archive and provenance. Claim completion only with the recorded host/build evidence.

## Commands and tracking

Configure:
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_OSX_DEPLOYMENT_TARGET=15.0 -DBUILD_TESTING=ON
```
Build: `cmake --build build --target HendrixGuitar_AU HendrixGuitarChecks -j 4`
Checks: `ctest --test-dir build --output-on-failure`

At each completed task, commit only its files, run its checks before claiming success, push main, and update the associated milestone through gh. Never close a milestone or push its completion tag while required checks remain incomplete.

Milestone 1 (spec and implementation plan) closes only after the human reviews this plan and chooses execution. Then commit any review changes, push, and create/push `v0.1.0-planning`.

## Execution handoff

The written spec was approved in chat. This plan is awaiting review and execution-method choice. Recommended: native execution in this session, followed by a fresh whole-branch reviewer, because the five tasks share the same engine/processor and their interfaces are tightly connected. Subagent-driven execution is available if the user prefers a fresh implementer/reviewer for every task.
