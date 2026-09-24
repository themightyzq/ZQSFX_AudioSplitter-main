# ZQ SFX Audio Splitter

ZQ SFX Audio Splitter is a desktop app that splits a multichannel WAV file (up to 32 channels)
into mono files, one per channel. It preserves BWF and iXML metadata, copying it verbatim at
the RIFF chunk level, including for RF64 files over 4 GB, and names the output files to the
UCS (Universal Category System) convention. macOS (universal binary), Windows, and Linux.
Built with JUCE.

## Install

Download the current release from
https://github.com/themightyzq/ZQSFX_AudioSplitter-main/releases:
`ZQ-SFX-Audio-Splitter-macOS.zip`, `-Windows.zip`, or `-Linux.zip`.

The binaries are unsigned. On macOS, right-click the app and choose Open the first time,
since Gatekeeper blocks a plain double-click.

## Use

### Single file

1. Drag a multichannel WAV file into the app.
2. Click Split. Output files go to `<filename>_split/`, one mono file per channel.

### Batch

1. Switch to the Batch Split tab.
2. Drag a folder of WAV files in, or browse for one.
3. Set the UCS category, subcategory, and description.
4. Click Split. Files are processed 4 to 8 at a time in parallel; a bad file is skipped and
   the rest of the batch continues. You can cancel at any time.

### Metadata preserved

- BWF BEXT: Description, Originator, OriginatorReference, OriginationDate, OriginationTime,
  TimeReference (timecode), CodingHistory
- iXML: Scene, Take, Tape, Project, track names, and other production fields, copied verbatim
  at the RIFF chunk level, including for RF64 files over 4 GB

Metadata preservation is covered by an automated test suite; validation against real recorder
files is in progress. The app is designed for files from Sound Devices (702T, 788T,
MixPre-6/10) and Zoom (F4/F6/F8/F8n Pro) recorders.

### UCS filenames

Output files are named `Category_Subcategory_Description_Channel.wav`, for example:

- `AMBNat_Forest_MorningBirds_L.wav`
- `DSnExt_Urban_CarPass_M.wav`
- `FOLExt_Footsteps_Gravel_R.wav`

### Options

- Sample rate override (44.1k, 48k, 96k, 192k)
- Bit depth override (16-bit, 24-bit, 32-bit float)
- Custom channel names, comma-separated
- Stereo-to-mono conversion
- iXML preservation on/off
- Settings are remembered between sessions

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
