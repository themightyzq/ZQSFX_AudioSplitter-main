# ZQ SFX Audio Splitter

Professional multichannel audio splitting tool for **field recordists** and **sound editors** working with **Soundminer** and **UCS** (Universal Category System) naming conventions.

---

## Built For Professionals

**Fast. Reliable. Standards-Compliant.**

- **Soundminer Compatible** -- Preserves all BWF/iXML metadata for seamless import
- **UCS Naming Standard** -- Industry-standard naming from [universalcategorysystem.com](http://universalcategorysystem.com)
- **Batch Processing** -- Process entire folders with parallel threading (4-8 simultaneous files)
- **Cross-Platform** -- Native macOS (Universal Binary) and Windows (x64)
- **Simple** -- Three clicks from drop to done

---

## Key Features

### Metadata Preservation

All broadcast metadata is preserved during splitting so your files import cleanly into Soundminer and other professional tools.

- **BWF BEXT**: Description, Originator, OriginatorReference, OriginationDate, OriginationTime, TimeReference (timecode), CodingHistory
- **iXML**: Scene, Take, Tape, Project, and all custom fields
- **Tested with**: Sound Devices (702T, 788T, MixPre-6, MixPre-10) and Zoom (F4, F6, F8, F8n Pro)

### UCS Naming

Full UCS taxonomy support with category/subcategory dropdowns, description field, and automatic channel suffixes.

**Format**: `Category_Subcategory_Description_Channel.wav`

**Examples**:
- `AMBNat_Forest_MorningBirds_L.wav`
- `DSnExt_Urban_CarPass_M.wav`
- `FOLExt_Footsteps_Gravel_R.wav`

### Batch Processing

- Parallel processing with 4-8 worker threads (scales to CPU cores)
- Progress tracking: file count, percentage, elapsed/remaining time, processing speed
- Continues on errors -- one bad file won't stop the rest of the batch
- Cancel at any time without corrupting completed files

### Single File Splitting

- Drag-and-drop or browse for input file
- Per-channel selection with Select All / None / Invert helpers
- Output directory auto-created alongside source file
- Live filename preview with UCS naming

### Options

- Sample rate override (44.1k, 48k, 96k, 192k)
- Bit depth override (16-bit, 24-bit, 32-bit float)
- Custom channel names (comma-separated)
- Stereo-to-mono conversion
- iXML preservation toggle
- Persistent settings (remembers your last-used configuration)

---

## Supported Formats

| | Details |
|---|---|
| **Input** | WAV, BWF (Broadcast Wave), WAV64 (>4GB files) |
| **Output** | WAV with full metadata preservation |
| **Channels** | 1--32 |
| **Sample Rates** | 44.1kHz, 48kHz, 96kHz, 192kHz |
| **Bit Depths** | 16-bit, 24-bit, 32-bit float |

---

## Quick Start

### Single File (3-Click Workflow)

1. **Drag** your multichannel audio file into the app
2. Output directory is auto-created as `filename_split/`
3. Click **SPLIT FILES**

### Batch Processing

1. Switch to the **Batch Split** tab
2. Drag a folder of WAV files (or browse)
3. Set UCS category/subcategory and description
4. Click **Split**
5. All files processed in parallel with consistent naming

---

## Installation

### macOS

1. Download `ZQ SFX Audio Splitter.dmg`
2. Drag to Applications folder
3. First launch: right-click > Open to bypass Gatekeeper

### Windows

1. Download `ZQ SFX Audio Splitter Setup.exe`
2. Run installer
3. Launch from Start Menu

### Build from Source

```bash
cd AudioSplitterJUCE

# macOS
./Build_Create.command
./Build_Launch.command

# Windows
Build_Create.bat
Build_Launch.bat
```

See [BUILD_INSTRUCTIONS.md](AudioSplitterJUCE/BUILD_INSTRUCTIONS.md) for detailed requirements and build options.

---

## Roadmap

- **Audio Preview** -- Waveform visualization, channel preview, peak analysis
- **WELDER** -- Combine mono files back into multichannel
- **SLICER** -- Marker-based waveform slicing

---

## Technical Details

- **Language**: C++17
- **Framework**: [JUCE](https://juce.com)
- **Build System**: CMake 3.22+
- **Platforms**: macOS 11+ (Universal Binary), Windows 10/11 (x64)
- **Architecture**: Thread-safe processing with mutex-protected state and atomic progress tracking

---

## Issues & Feedback

Report bugs and request features on [GitHub Issues](https://github.com/themightyzq/ZQSFX_AudioSplitter-main/issues).

---

## Credits

**Created by**: ZQ SFX
**Built for**: Professional field recordists and sound editors

**Acknowledgements**:
- [JUCE](https://juce.com) -- Audio application framework
- [Universal Category System](http://universalcategorysystem.com) -- Industry naming standard
- [Soundminer](https://www.soundminer.com) -- Audio database management

---

## License

All rights reserved. See [LICENSE](LICENSE) for details.
