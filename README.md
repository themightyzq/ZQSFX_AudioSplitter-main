# ZQ SFX Audio Splitter

Professional multichannel audio splitting tool for **field recordists** and **sound editors** working with **Soundminer** and **UCS** (Universal Category System) naming conventions.

---

## 🎯 Built For Professionals

**Fast. Reliable. Standards-Compliant.**

- **Soundminer Compatible**: Preserves all BWF/iXML metadata
- **UCS Naming Standard**: Industry-standard naming from universalcategorysystem.com
- **Batch Processing**: Process multiple files with sequential processing
- **Clean Architecture**: Thread-safe, maintainable codebase
- **Simple**: Three clicks from drop to done

**Status**: Phase 1 Complete - Core features implemented and ready for use

---

## ✨ Key Features

### 🗂️ Metadata Integrity
- **BWF BEXT Preservation**: Description, Originator, Timecode, CodingHistory
- **iXML Preservation**: Scene, Take, Tape, Project, custom fields
- **Complete Metadata Copy**: All metadata fields preserved via JUCE
- **Designed for Sound Devices**: 702T, 788T, MixPre series compatibility
- **Designed for Zoom Recorders**: F4, F6, F8, F8n Pro compatibility
- **Soundminer Ready**: Metadata format compatible with Soundminer import

### 📛 UCS Naming System
- **Category/Subcategory Selection**: Full UCS taxonomy support
- **Standard Format**: `Category_Subcategory_Description_Channel.wav`
- **Example**: `AMBNat_Forest_MorningBirds_L.wav`
- **Auto-Detection**: Recognizes existing UCS format in filenames
- **Smart Defaults**: Remembers last-used categories

### ⚡ Batch Processing
- **Parallel Processing**: Process 4-8 files simultaneously (based on CPU cores)
- **Comprehensive Progress Tracking**:
  - Files completed count with percentage
  - Current file being processed
  - Time elapsed and remaining estimates
  - Processing speed (files/min)
- **Thread-Safe Architecture**: Mutex-protected state, atomic counters
- **Error Handling**: Clear error messages and logging

### 🎧 Audio Preview *(Coming Soon)*
- **Preview Channels**: Listen before splitting
- **Visual Waveforms**: See audio content per channel
- **Peak Level Analysis**: Identify clipping and issues
- **Solo/Mute**: Verify which channels contain what

---

## 🚀 Quick Start

### Single File (3-Click Workflow!)
1. **Drag** audio file into app
2. *(Output directory auto-created)*
3. Click **✓ SPLIT FILES**

That's it! Files are split into individual channels in a `filename_split/` folder.

### Advanced Options
- Select specific channels (all selected by default)
- Choose UCS Category/Subcategory for professional naming
- Add custom descriptions
- Override sample rate or bit depth

### Batch Processing
1. Drag folder with WAV files
2. Set UCS Category/Subcategory for entire batch
3. Choose naming pattern
4. Click **Split**
5. All files processed with consistent naming

---

## 💻 Installation

### macOS
1. Download `ZQ SFX Audio Splitter.dmg`
2. Drag to Applications folder
3. Open (first time: right-click → Open to bypass Gatekeeper)

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

See [BUILD_INSTRUCTIONS.md](AudioSplitterJUCE/BUILD_INSTRUCTIONS.md) for detailed build documentation.

---

## 📋 Supported Formats

**Input**: WAV, BWF (Broadcast Wave Format), WAV64 (>4GB files)
**Output**: WAV with full metadata preservation
**Channel Counts**: 1-32 channels
**Sample Rates**: 44.1kHz, 48kHz, 96kHz, 192kHz
**Bit Depths**: 16-bit, 24-bit, 32-bit float

### Tested Recorders
- **Sound Devices**: 702T, 788T, MixPre-6, MixPre-10
- **Zoom**: F4, F6, F8, F8n Pro

---

## 🎓 How to Use

### Understanding UCS Naming

UCS (Universal Category System) is the industry standard for organizing sound effects. Files are named:

```
Category_Subcategory_Description_Channel.wav
```

**Examples**:
- `AMBNat_Forest_MorningBirds_L.wav` - Natural ambience, forest, morning birds, left channel
- `DSnExt_Urban_CarPass_M.wav` - Design external, urban, car pass, mono
- `FOLExt_Footsteps_Gravel_R.wav` - Foley external, footsteps, gravel, right channel

Visit [universalcategorysystem.com](http://universalcategorysystem.com) for the complete taxonomy.

### Workflow Tips

1. **Metadata is Critical**: Always verify Soundminer import after splitting
2. **Batch Smart**: Process entire day's recordings in one batch
3. **UCS Consistency**: Use the same category/subcategory for related sounds
4. **Preview First**: Use audio preview to verify channels before splitting
5. **Verify Output**: Check sample counts and metadata preservation

---

## 🗺️ Roadmap

### Current Status
- **Phase 0**: Foundation & UI ✅ COMPLETE
- **Phase 1**: Metadata & UCS Naming ✅ COMPLETE
  - BWF/iXML metadata preservation
  - UCS taxonomy system
  - Thread-safe architecture

- **Phase 2**: Performance & UX 🚧 IN PROGRESS (75% complete)
  - ✅ Parallel batch processing (4-8 files simultaneously)
  - ✅ Enhanced progress reporting (time estimates, speed metrics)
  - ✅ Smart output directory defaults (3-click workflow)
  - ✅ Primary action button redesign
  - ✅ Collapsible panels (Options and UCS panels)
  - ✅ Channel selection helpers (All/None/Invert buttons with live count)
  - ⏳ Error reporting UI (coming next)

### Coming Soon
- **Phase 3**: Audio preview & analysis
  - Waveform visualization
  - Channel preview
  - Peak level analysis

### Future
- **Phase 4**: Polish & ship (installers, code signing)
- **Phase 5**: WELDER tool (combine mono → multichannel)
- **Phase 6**: SLICER tool (waveform editing)

See [TODO.md](TODO.md) for detailed development roadmap.

---

## ⚙️ Technical Details

- **Framework**: Built with JUCE (C++)
- **Performance**: Native C++ with direct audio file access
- **Cross-Platform**: Native macOS (Universal Binary) and Windows (x64)
- **Memory Efficient**: Optimized for batch processing
- **Thread Safe**: Mutex-protected shared state, atomic progress tracking

### Why JUCE?

JUCE provides professional-grade audio application development:
- **Native Speed**: Direct audio file access, no subprocess overhead
- **Metadata Control**: Precise BWF/iXML handling via AudioFormatReader/Writer
- **Cross-Platform**: Single codebase for macOS and Windows
- **Professional Tools**: Industry-standard framework for audio software

---

## 📖 Documentation

### For Users
- **Quick Start**: This README
- **Build Instructions**: [BUILD_INSTRUCTIONS.md](AudioSplitterJUCE/BUILD_INSTRUCTIONS.md)

### For Developers
- **Development Guide**: [TODO.md](TODO.md)
- **Development Rules**: [CLAUDE.md](CLAUDE.md)
- **Implementation Summary**: [IMPLEMENTATION_SUMMARY.md](IMPLEMENTATION_SUMMARY.md)
- **Completion Report**: [COMPLETION_REPORT.md](COMPLETION_REPORT.md)

### For Testing
- **Automated Verification**: [TEST_RESULTS.md](TEST_RESULTS.md)

### External Standards
- **UCS Standard**: [universalcategorysystem.com](http://universalcategorysystem.com)
- **Soundminer**: [soundminer.com](https://www.soundminer.com)

---

## 🤝 Contributing

This is a professional tool for the audio community. Contributions welcome!

### Development Focus
1. **Metadata Preservation**: Non-negotiable priority
2. **Code Quality**: Clean, maintainable, thread-safe
3. **UCS Compliance**: Follow the standard strictly
4. **Performance**: Optimize for real-world workflows
5. **Reliability**: Error handling that builds trust

### Current Status
- **Phase 1**: Complete ✅ (Metadata preservation, UCS naming)
- **Phase 2**: Next up (Parallel processing, performance)

See [CLAUDE.md](CLAUDE.md) for development practices and [TODO.md](TODO.md) for current priorities.

---

## 🐛 Known Issues

See [GitHub Issues](https://github.com/yourrepo/issues) for current bugs and feature requests.

---

## 📜 License

[Your License Here - e.g., MIT, GPL, etc.]

---

## 🙏 Credits

**Created by**: [Your Name/Team]
**For**: Professional field recordists and sound editors
**Inspired by**: The need for fast, reliable multichannel splitting with perfect metadata

**Special Thanks**:
- Soundminer for setting the standard in audio database management
- Universal Category System for the industry naming standard
- JUCE framework for professional audio development tools
- The field recording community for feedback and testing

---

## 📞 Support & Community

- **Issues**: [GitHub Issues]
- **Discussions**: [GitHub Discussions]
- **Email**: [your@email.com]

---

## 🎯 Project Goals

1. **Trust**: Handle irreplaceable recordings with care
2. **Standards**: Soundminer + UCS = industry standard compliance
3. **Simplicity**: Focused tool, no bloat
4. **Quality**: Clean, maintainable, professional codebase
5. **Open**: Transparent development, community-driven

---

**Made with ❤️ for the professional audio community**
