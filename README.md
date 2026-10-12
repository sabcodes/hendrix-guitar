# Hendrix Guitar

A personal-use MIDI guitar Audio Unit for Logic on Apple silicon. Bundled FreePats direct guitar samples feed wah, fuzz, octave-up, vibe and amp/cab coloration. Inspired by vintage Hendrix tones; an original instrument.

## Try it in Logic

1. Create a **Software Instrument** track.
2. In the instrument slot, choose **AU Instruments → sabcodes → Hendrix Guitar → Stereo**.
3. Open its window, choose **Clean Touch**, then press **Command-K** for Musical Typing.
4. Try **Driven Lead** and **Wah Fuzz**. Enable Wah and move Wah position, or automate it in Logic.
5. Musical Typing: **1/2** pitch bend, **3–8** modulation/vibrato, **Tab** sustain, **Z/X** octave.
6. Enable **Mono legato** and overlap notes to glide. Disable it for double-stops and chords.

If Logic was open during installation, quit and reopen it after saving your work. If the instrument is absent, open Logic's Plug-in Manager, find sabcodes / Hendrix Guitar, and use Reset & Rescan Selection.

Guitar volume acts before distortion, so lowering it cleans up the tone. Output is the final level. Harmonic sustain adds a bounded sustaining blend; it releases with the note.

Six voices; source recordings cover MIDI 35–86. Notes beyond that range transpose the nearest sample. Pitch bend affects every held note on the same MIDI channel. Octave fuzz works best on single notes and can sound rough on chords.

## Build and install

Requires the macOS Command Line Tools, CMake and Python 3. JUCE is pinned as a submodule. Samples are already in this repo.

```sh
git submodule update --init --recursive
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_OSX_DEPLOYMENT_TARGET=15.0 -DBUILD_TESTING=ON
cmake --build build --target HendrixGuitar_AU HendrixGuitarChecks HendrixGuitarRelocationChecks -j 4
ctest --test-dir build --output-on-failure
bash Scripts/install_au.sh
auval -v aumu Hdg1 Hdxg
```

The generated AU is outside the synced Desktop, at ~/Library/Caches/HendrixGuitar/AU/Hendrix Guitar.component. Its exact path is also written to build/au-bundle-path.txt.

Installation goes to ~/Library/Audio/Plug-Ins/Components/Hendrix Guitar.component. An existing installed copy is archived in ~/Library/Audio/Plug-Ins/HendrixGuitar Backups before replacement. The installer accepts optional source bundle and destination directory arguments for development.

## Status and provenance

See [validation notes](docs/validation.md), [approved spec](docs/superpowers/specs/2026-10-11-hendrix-guitar-design.md), and [third-party notices](THIRD_PARTY_NOTICES.md). All guitar recordings are CC0 FreePats FSBS direct samples. No Hendrix recordings are included.

Logic listening and project save/reopen checks are pending the owner's preview test. This is a development preview.
