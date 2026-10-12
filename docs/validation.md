# Preview validation

Checked on this Mac on 2026-10-11 (America/Los_Angeles).

## Confirmed

- Native Release AU and check executable build successfully with JUCE 8.0.6, C++17, macOS deployment target 15.0.
- Binary is Mach-O arm64.
- CTest: 4/4 groups passed (InstallChecks, GuitarChecks, BundleChecks, SampleImportChecks), 9.46 seconds.
- Engine and tone checks cover 44.1, 48 and 96 kHz, variable block sizes, lifecycle, finite bounded output, effect controls and latency.
- State checks cover all 17 stable IDs, recreation, malformed/wrong-root/future-version preservation, partial defaults and preset changes during notes.
- Bundle checks verify 120 sample regions, referenced recordings, CC0 license and strict code signature.
- Installer check passes with paths containing spaces; an existing copy is archived, and an invalid source leaves the installed copy intact.
- The AU copied to Library/Caches/HendrixGuitar/Relocation With Spaces/Components plays MIDI through its native factory using bundled samples; observed peak 0.529194.
- Installed at /Users/sabarnoislam/Library/Audio/Plug-Ins/Components/Hendrix Guitar.component.
- `auval -v aumu Hdg1 Hdxg`: exit 0, AU VALIDATION SUCCEEDED. [Full validator output](validation/auval-preview.txt).
- Editor snapshot inspected; labels and initial numeric values corrected and covered by checks.
- Clean Touch, Driven Lead and Wah Fuzz audio references rendered to ignored captures/ for audition. Their peaks are 0.562013, 0.672838 and 0.635559.

## Pending owner test in Logic

Create a software instrument track and select AU Instruments → sabcodes → Hendrix Guitar → Stereo. Press Command-K and try the three presets, bends (1/2), modulation (3–8), sustain (Tab), overlapping notes with Mono legato, and chords with legato disabled. Try wah/volume automation, stop playback, then save and reopen the project to check recall.

Actual Logic loading, listening and project recall have not yet been confirmed. Milestone 5 and the v1.0.0 release remain open until those checks and the final whole-branch review are complete.

## Musical limits

- Six voices; source recordings cover MIDI 35–86. Outside notes transpose the nearest recording.
- Pitch bend bends all held notes on the same MIDI channel.
- Octave fuzz can be rough on chords; single notes work best.
- Harmonic sustain is a bounded synthetic blend, not an acoustic feedback simulation.
- Vintage-inspired coloration, not a circuit-perfect reproduction.
- Native arm64 AU for Logic on this Mac; no VST or Intel binary.
