# ZQ SFX Audio Splitter

ZQ SFX Audio Splitter is a desktop app that splits a multichannel WAV file (up to 32 channels)
into mono files, one per channel. It preserves BWF and iXML metadata, copying it verbatim at
the RIFF chunk level, including for RF64 files over 4 GB, and can name single-file output to
the UCS (Universal Category System) convention. macOS (universal binary), Windows, and Linux.
Built with JUCE.

## Install

Download the current release from
https://github.com/themightyzq/ZQSFX_AudioSplitter-main/releases:
`ZQ-SFX-Audio-Splitter-macOS.zip`, `-Windows.zip`, or `-Linux.zip`.

The binaries are unsigned. On macOS, right-click the app and choose Open the first time,
since Gatekeeper blocks a plain double-click.

## Use

### Single file

1. Drag a multichannel WAV file into the app, or choose one with Browse. Dropping a file sets
   the output folder to `<filename>_split/` beside it; with Browse, choose the output folder.
2. Tick the channels to extract, set UCS naming if you want it, and click Split. One mono file
   is written per channel.
3. Click Cancel next to the progress bar to stop. Nothing is written or replaced.

### Batch

1. Switch to the Batch Split tab.
2. Drag a folder of WAV files in, or browse for one.
3. Click Split. Every channel of every WAV file in the folder is written as
   `<source>_chanN.wav`, or `<source>_<name>.wav` with custom channel names. Files are
   processed 4 to 8 at a time in parallel; a bad file is skipped and the rest of the batch
   continues. UCS naming applies to the Single File tab only.
4. Click Cancel next to the progress bar to stop. Files that finished stay; files still being
   written are discarded.

### Existing files

Output is written to a temporary file next to its destination, checked, and only then moved over
any existing file of the same name. If a write fails or you cancel, files already in the output
folder are left as they were. Two channels that would get the same file name, or an output that
would overwrite the source file, are refused before anything is written.

### Metadata preserved

- BWF BEXT: Description, Originator, OriginatorReference, OriginationDate, OriginationTime,
  TimeReference (timecode), CodingHistory
- iXML: Scene, Take, Tape, Project, track names, and other production fields, copied verbatim
  at the RIFF chunk level, including for RF64 files over 4 GB

Metadata preservation is covered by an automated test suite; validation against real recorder
files is in progress. The app is designed for files from Sound Devices (702T, 788T,
MixPre-6/10) and Zoom (F4/F6/F8/F8n Pro) recorders.

### UCS filenames

On the Single File tab, output files are named `Category_Subcategory_Description_Channel.wav`,
for example:

- `AMBNat_Forest_MorningBirds_L.wav`
- `DSnExt_Urban_CarPass_M.wav`
- `FOLExt_Footsteps_Gravel_R.wav`

### Options

- Sample rate override (11.025k, 22.05k, 44.1k, 48k, 96k, 192k). Conversion uses a windowed-sinc
  filter, so content above the new rate's Nyquist frequency is removed, not folded back. When the
  rate or bit depth changes, the output's bext TimeReference, iXML rate, timestamp and bit-depth
  fields are rewritten for the new format and a coding-history line records the conversion.
- Bit depth override (8-bit, 16-bit, 24-bit, 32-bit float)
- Custom channel names, comma-separated
- Stereo-to-mono: a 2-channel file with both channels ticked is written as one mono mix
  (L plus R, halved) named `<source>_mono.wav`, or `..._M.wav` with UCS naming. Files with other
  channel counts are split as usual.
- iXML preservation on/off
- The last-used folders, the custom channel names and which panels are collapsed are remembered
  between sessions

## Build from source

JUCE 8.0.8 and the zqsfx_ui module are fetched automatically by CMake (pinned by tag) on the
first configure, so an internet connection is needed then; after that they are cached in
`AudioSplitterJUCE/build/_deps/` and reused.

```bash
cd AudioSplitterJUCE

# macOS
./Build_Create.command
./Build_Launch.command

# Windows
Build_Create.bat
Build_Launch.bat
```

Requirements and build options are in `AudioSplitterJUCE/BUILD_INSTRUCTIONS.md`.

## Licence

GPL-3.0-or-later. See LICENSE. Built with JUCE.

Copyright (c) 2025-2026 ZQ SFX.

ZQ SFX, https://www.zq-sfx.com, connect@zq-sfx.com.
