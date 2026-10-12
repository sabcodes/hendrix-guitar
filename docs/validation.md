# Release validation

Preview checked on 2026-10-11; owner confirmed Logic project recall on 2026-10-12 (America/Los_Angeles).

## Confirmed

- Native Release AU and check executable build successfully with JUCE 8.0.6, C++17, macOS deployment target 15.0.
- Binary is Mach-O arm64.
- Final CTest: 5/5 groups passed (RelocationChecks, InstallChecks, GuitarChecks, BundleChecks, SampleImportChecks), 15.00 seconds.
- Engine and tone checks cover 44.1, 48 and 96 kHz, variable block sizes, lifecycle, finite bounded output, effect controls and latency.
- State checks cover all 17 stable IDs, recreation, malformed/wrong-root/future-version preservation, partial defaults and preset changes during notes.
- Bundle checks verify 120 sample regions, each recording's byte count and SHA-256, CC0 license and strict code signature.
- Installer check passes with paths containing spaces; an existing copy is archived, and an invalid source leaves the installed copy intact.
- The AU copied to Library/Caches/HendrixGuitar/Relocation With Spaces/Components plays MIDI through its native factory using bundled samples; observed preview peak 0.529194. Final CTest additionally relocates the 1.0.0 AU to a fresh path with spaces and verifies native AudioUnitReset silences it.
- Installed at /Users/sabarnoislam/Library/Audio/Plug-Ins/Components/Hendrix Guitar.component.
- Final installed 1.0.0: `auval -v aumu Hdg1 Hdxg` exits 0, AU VALIDATION SUCCEEDED. [Final validator output](validation/auval-release.txt); [preview output](validation/auval-preview.txt).
- Editor snapshot inspected; labels and initial numeric values corrected and covered by checks.
- Clean Touch, Driven Lead and Wah Fuzz audio references rendered to ignored captures/ for audition. Their peaks are 0.562013, 0.672838 and 0.635559.

## Owner test in Logic

The owner approved the installed working preview and requested finalization. On 2026-10-12 the owner explicitly confirmed that saving and reopening the Logic project restored the instrument settings correctly. This is human-reported host evidence; Codex did not operate Logic because UI access for the app was not enabled.

Expression, presets, parameter scheduling, lifecycle and bounds are covered by the automated engine/processor checks and Apple AU validation. We do not claim separate manual confirmation of every control or automation lane in Logic.

## Final review and fixes

A fresh-context whole-branch review found two Important issues and no Critical/Minor findings. Both were reproduced before fixing:

- Host reset left voices/effects active. The processor now overrides reset, clearing both modules. Direct processor and actual AudioUnitReset regressions failed first and now pass.
- JUCE could zero-fill damaged FLAC recordings without failing load. The importer now records file size/SHA-256; the loader verifies each before decoding with native macOS CommonCrypto. Truncated and same-size corrupt FLAC files, missing/malformed integrity, and importer metadata checks failed first and now pass.

All 120 sample mappings retain identical ranges, root notes, velocity layers and release values. The final suite is 5/5 after both fixes. See [review record and engineering decisions](final-review.md).

## Musical limits

- Six voices; source recordings cover MIDI 35–86. Outside notes transpose the nearest recording.
- Pitch bend bends all held notes on the same MIDI channel.
- Octave fuzz can be rough on chords; single notes work best.
- Harmonic sustain is a bounded synthetic blend, not an acoustic feedback simulation.
- Vintage-inspired coloration, not a circuit-perfect reproduction.
- Native arm64 AU for Logic on this Mac; no VST or Intel binary.

## Published package

Private GitHub release: [Hendrix Guitar v1.0.0](https://github.com/sabcodes/hendrix-guitar/releases/tag/v1.0.0).

Source/runtime commit: 07a20ab. Archive: hendrix-guitar-v1.0.0-macos-arm64.zip, 80,822,240 bytes. SHA-256: `060a311135db4e77e4b56bc625f0b04795764ca30622550fd769fa575a85e346`.

The ZIP was extracted into a fresh path with spaces and checked for 1.0.0 bundle metadata, executable installer, license copies, all sample sizes/hashes, strict code signature, native MIDI render and silence after AudioUnitReset. GitHub reports both archive and SHA256SUMS.txt uploaded; the remote archive digest matches the local digest.

The corrected component is installed. Save work and reopen Logic to load the updated binary in existing sessions. The previous preview is archived in Library/Audio/Plug-Ins/HendrixGuitar Backups.

## 1.1.0 control-response correction

The owner then reported weak response from most controls, including Guitar volume/Fuzz. [Diagnosis and controlled before/after measurements](control-response.md) identified early double saturation, narrow tone-filter ranges and weak low-depth vibe scaling. The editor-to-audio path passed an offscreen mute/unmute check.

The revised model reproduces the original failure when the original 1.0 tone source is restored, and passes the same regression with the calibrated model. All six CTest groups pass (20.81 seconds), including full MIDI/state/integrity/reset/packaging coverage. The installed 1.1.0 AU passes Apple validation; [full output](validation/control-response/auval.txt). Saved IDs/schema/MIDI/latency are stable; recalled values produce the revised tone intentionally.

The previous 1.0 instrument is archived by the installer. A fresh review found no blockers; one test-direction safeguard is deferred. The owner has not yet auditioned 1.1.0; signal response measurements are separate from listening preference.

The private [1.1.0 release](https://github.com/sabcodes/hendrix-guitar/releases/tag/v1.1.0) is uploaded with the verified archive and checksum. ZIP extraction to a fresh spaced path passed version 1.1.0 metadata, executable installer, licenses, bank hashes/signature, native MIDI rendering (peak 0.400399) and reset silence. Remote/local archive SHA-256 matches. Milestone 6 is complete.
