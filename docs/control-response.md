# Control response correction — 1.1.0

Owner feedback on 2026-10-12: moving most knobs, including Guitar volume and Fuzz, changed the sound too little. This patch restores the approved useful clean-to-driven and tone-control range; parameter IDs, state schema, MIDI behavior and oversampling latency are unchanged.

## Reproduction

Command: `build/HendrixGuitarChecks --probe-controls /absolute/path/to/output`.

The probe renders identical MIDI 57, velocity 100, 48 kHz guitar audio through fresh tone modules for each comparison. Each recording is two seconds; analysis skips the first 100 ms. Normal comparison settings match the screenshot: Guitar volume 0.14, Fuzz 0.65, Amp drive 0.65. Additional fuzz comparisons use normal Guitar volume 0.85. FFT brightness compares energy above/below 1.5 kHz; waveform differences remove overall level differences.

It also creates the actual editor and verifies that its Guitar volume slider updates APVTS, mutes rendered audio at zero, and restores audio when raised. This confirms the offscreen editor/processor path, rather than claiming direct operation of the owner's Logic window.

## Hypotheses and isolated probes

1. Two high-gain nonlinear stages saturate too early, flattening volume and fuzz response. Reducing amp gain alone raised the volume sweep from 1.08 to 4.48 dB; reducing fuzz pre-gain raised it to 8.94 dB.
2. Filter ranges are too narrow/high to change brightness substantially. Broader, geometric amp/cab ranges increased the measured brightness sweep; their original default cutoffs remain 5380/4750 Hz.
3. Vibe depth scales both sweep and blend, making low settings respond approximately quadratically. A square-root taper restores usable low-depth response while retaining the same zero/full endpoints.
4. Controls have conditional activation: Wah position requires Wah enabled; glide requires Mono legato; bend/vibrato require MIDI input. These remain intentional behavior. The UI-to-audio mute/unmute probe passed.

## Before/after

| Comparison | 1.0 | 1.1 |
| --- | ---: | ---: |
| Guitar volume 0.14 → 0.85, level change | 1.08 dB | 9.53 dB |
| Fuzz 0.35 → 0.85 at low guitar volume, level change | 3.59 dB | 10.26 dB |
| Fuzz 0.35 → 0.85 at normal guitar volume, level change | 0.41 dB | 5.27 dB |
| Same normal-volume fuzz sweep, normalized waveform difference | 0.116 | 0.297 |
| Amp tone full sweep, brightness change | 5.79 dB | 13.00 dB |
| Cab tone full sweep, brightness change | 0.95 dB | 12.27 dB |
| Vibe rate 0.5 → 8 Hz at depth 0.15, normalized waveform difference | 0.0168 | 0.1293 |

These are controlled signal measurements, not a claim of human listening approval or exact hardware emulation.

## Patch and acceptance checks

- Amp gain: 2 + 18 × drive → 1 + 4 × drive.
- Fuzz pre-gain: 1 + 40 × fuzz → 1 + 12 × fuzz.
- Geometric amp cutoff: 600 → 5380 → 12000 Hz, retaining its 0.55 default position.
- Geometric cab cutoff: 650 → 4750 → 11500 Hz, retaining its 0.5 default position.
- Square-root vibe-depth taper; smooth parameters and bounded output remain.

ControlResponseChecks requires at least 6 dB volume/low-volume fuzz level contrast, at least 6 dB amp/cab brightness contrast, a normalized waveform difference of 0.1 for vibe rate and 0.15 for middle-fuzz changes. The complete probe failed against the original 1.0 tone source, then passed against the revised source. The previous installed AU was left intact during experiments.

Existing project knob values continue to recall. Their tone changes intentionally with this recalibration. The full six-group suite passed (20.81 seconds). The installed 1.1.0 component passed Apple auval (exit 0, AU VALIDATION SUCCEEDED). A fresh focused review found no blocking issues; its one deferred test-direction improvement is recorded in [the review report](control-response-review.md). The extracted archive passed version/license/installer/hash/signature/native render/reset checks. The private v1.1.0 release is published and milestone 6 is complete.

## Release

[Private v1.1.0 release](https://github.com/sabcodes/hendrix-guitar/releases/tag/v1.1.0). Runtime source commit: 206d08b. Archive: 80,831,188 bytes; SHA-256 `0653ecca6c5b9830a06e987fcdf938f79db1e20068dc960550d9d4bc13abc7c4`. GitHub's uploaded digest matches the locally extracted/verified package.

The revised AU is installed. Save work and fully quit/reopen Logic to load it. Before/after captures are retained under ignored captures/control-response-before and captures/control-response-after for owner audition; temporary source variants were removed. Detailed listening preference remains the owner's feedback, rather than a claim that automated checks listened to the tone.
