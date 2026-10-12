# Hendrix Guitar AU Design Spec

**Status:** Written spec approved by the user; implementation plan awaiting review
**Date:** 2026-10-11

## Goal

Build a personal-use, MIDI-playable electric guitar instrument for Logic Pro on this Mac, focused on Hendrix-inspired expressive riffs and solos with a vintage analog tone.

## User and use

The instrument is for one user in Logic Pro. Notes will come from Logic's Musical Typing window (Command-K) on the computer keyboard. The mouse will operate the instrument controls. It is a software instrument, not an effect for processing a live guitar input.

The project will live in a new hendrix guitar folder separate from SerumSynth. It will have its own local Git repository and a private GitHub repository created with gh. Project stages will be tracked with GitHub milestones; completion of each stage will also be marked with a pushed Git tag.

## Product scope

The first version will build as a macOS Audio Unit instrument for Logic. It will not ship VST3, standalone, Windows, or Linux formats.

The name shown in Logic will be Hendrix Guitar. The presentation will be simple and usable, with vintage-inspired colors and mouse-operated controls. This is an original, personal-use instrument inspired by Hendrix's sound; it will not present itself as an official Hendrix product.

## Sound source and licensing

Use the Fender clean/direct samples from the FreePats electric-guitar library as the sampled core. The library is published under CC0 1.0, which permits reuse and modification. Preserve source and license details in the repository. Do not include audio extracted from Hendrix or other commercial YouTube recordings. Such recordings may be used as listening references only.

The project does not currently contain guitar-instrument samples. Package the selected FreePats sample data with the AU so the instrument works without external sample paths. The exact source archive variant and sample mapping will be selected after inspecting the library files during implementation.

Sources:
- [FreePats clean electric guitar](https://freepats.zenvoid.org/ElectricGuitar/clean-electric-guitar.html)
- [Creative Commons CC0 1.0](https://creativecommons.org/publicdomain/zero/1.0/)
- [YouTube Terms of Service](https://www.youtube.com/t/terms)

## Sound and performance behavior

The instrument will use a hybrid design: sampled guitar notes provide string tone and pick attack, while DSP provides continuous expression and vintage effects.

- MIDI note velocity controls pick intensity and contributes to the clean-to-driven response.
- A virtual guitar-volume control changes the level entering the effects and amp path, allowing driven tones to clean up as the control is reduced.
- Pitch-bend messages from Logic control bends; the bend range is adjustable.
- Modulation input controls vibrato.
- Overlapping notes can glide in legato mode. Polyphonic mode supports riffs, double-stops, and simple chords, with up to six simultaneous voices.
- A mouse control adds controllable sustain/feedback character. This is an effect, not a simulation of actual speaker-to-string feedback.

The instrument will provide these starting sounds:
1. Clean, touch-sensitive tone.
2. Driven lead.
3. Wah and fuzz lead.

## Effects and controls

The first version includes mouse-adjustable controls for:

- Fuzz/drive.
- Wah position.
- Uni-Vibe-style modulation rate and depth.
- Amp and cabinet tone.
- Guitar volume / input drive.
- Optional octave-up fuzz.
- Sustain/feedback amount.
- Master level.

The effect behavior will be vintage analog-inspired DSP. It will not claim to reproduce a specific recording, pedal, amplifier, or physical circuit exactly. Initial signal order will be wah, fuzz, optional octave-up, Uni-Vibe-style modulation, then amp/cab; controls will remain simple rather than exposing a general-purpose modular routing matrix.

## Host integration

- Accept MIDI note, velocity, pitch-bend, modulation, sustain, and note-off events from Logic.
- Save and restore parameter state with the Logic project.
- Expose the principal controls to Logic automation.
- Install/build as an AU instrument discoverable by Logic.

## Architecture

Use a standalone JUCE/CMake project, separate from the SerumSynth source tree. Keep responsibilities focused:

- A sampler voice maps the selected guitar samples to MIDI pitches and velocities.
- A small DSP chain handles drive, wah, modulation, amp/cab coloration, octave-up, and sustain.
- The JUCE processor handles MIDI, voice management, saved parameters, and audio rendering.
- The editor provides the small control surface and preset selection.

Use JUCE as a pinned Git submodule rather than copying the entire JUCE source tree into this repository. Avoid adding other dependencies unless the sample format or a measured need requires them.

## Out of scope

- Downloading, embedding, or distributing copyrighted Hendrix recordings or isolated stems.
- A multi-artist preset collection.
- A live-guitar input/effects mode.
- MPE or dedicated MIDI-guitar controller support in the first version.
- VST3, standalone, cross-platform builds, commercial release, or a complex pedal-routing editor.

## Acceptance criteria

- The project builds a macOS AU instrument in the separate repository.
- Logic can load the AU and play notes from Musical Typing.
- Mouse controls and presets produce the agreed clean, driven, wah/fuzz, and vintage-modulated range.
- Pitch bend, modulation/vibrato, legato glide, sustain/feedback control, and six-voice polyphony work as specified.
- Parameter changes are automatable and survive saving and reopening a Logic project.
- The packaged instrument does not require an external sample path and includes provenance/license information for its sample assets.
- The repository contains no audio ripped from YouTube or commercial Hendrix recordings.

## Tone research

The design prioritizes playing dynamics and changing tone over a single fixed distortion sound. Hendrix described his tone as depending on both amp and fretting; Roger Mayer's account of Woodstock documents a wah, Fuzz Face, and Uni-Vibe chain, while other sources show that equipment and studio treatments varied by era.

- [Hendrix's 1968 interview](https://www.guitarplayer.com/players/jimi-hendrix-1968-guitar-player-interview)
- [Roger Mayer interview](https://www.musicradar.com/news/fx-guru-roger-mayer-on-hendrix-anybody-can-buy-a-wah-and-just-make-it-go-wah-wah-wah-but-making-it-talk-is-something-else)
- [Hendrix archive: Are You Experienced sessions](https://www.jimihendrix.com/editorial/have-you-ever-been-experienced-celebrating-55-years-the-making-of-are-you-experienced/)
