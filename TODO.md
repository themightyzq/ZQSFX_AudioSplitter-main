# ZQ SFX Audio Splitter - Development Roadmap

**Project Mission**: Professional multichannel audio splitting tool for field recordists and sound editors
**Target Users**: Sound professionals working with Soundminer and UCS naming
**Current Status**: Phase 2 near-complete - Thread safety hardened, UI polished
**Last Updated**: 2026-01-27

---

## 📊 PROGRESS OVERVIEW

| Phase | Focus | Status | Completion |
|-------|-------|--------|------------|
| Phase 0 | Foundation & UI | ✅ Complete | 100% |
| Phase 1 | Soundminer + UCS | ✅ Complete | 100% |
| Phase 2 | Performance & UX | 🚧 In Progress | 85% |
| Phase 3 | Preview & Analysis | ⏳ Pending | 0% |
| Phase 4 | Polish & Ship | ⏳ Pending | 5% |
| Phase 5 | Future Tools | ⏳ Planned | 0% |

**Latest Update (2026-01-27)**:
- ✅ **COMPLETE**: Thread Safety Hardening
  - BatchSplitter destructor drains thread pool before member destruction
  - aliveFlag pattern applied to both SingleFileSplitter and BatchSplitter
  - BatchJobManager pre-allocates JobProgress objects (no pointer replacement races)
  - AudioFileProcessor creates fresh reader per channel when resampling
  - Metadata (CodingHistory, iXML TRACK_COUNT) updated for mono output
- ✅ **COMPLETE**: UI Legibility & Layout Hardening
  - OptionsPanel rewritten to strict top-to-bottom flow (fixed overlap bug)
  - MainComponent proportional panel scaling with minTabHeight=200 guarantee
  - Channel button positioning fixed (group-local coordinates)
  - Tab label width uses actual subheading font measurement (no truncation)
  - Window default 1200x1050, minimum 1000x850
  - Background color matches design system
  - Focus rings use rounded rectangles matching component shapes
  - drawLabel respects per-label font settings
  - CollapsiblePanel uses unique_ptr for content ownership (no memory leak)

**Previous Updates (2025-10-13)**:
- ✅ **COMPLETE**: Phase 2.1 - Parallel Batch Processing
  - Thread pool implementation (4-8 workers)
  - BatchJobManager for thread-safe coordination
  - BatchProcessingJob for parallel file processing
  - Thread-safe progress reporting and error collection
- ✅ **COMPLETE**: Phase 2.2 - Enhanced Progress Reporting
  - Comprehensive batch progress display
  - Time elapsed/remaining estimation
  - Processing speed metrics (files/min)
  - Code review passed (all critical issues fixed)
- ✅ **COMPLETE**: Phase 2.3 - Collapsible Panels
  - CollapsiblePanel component with expand/collapse functionality
  - Smooth fade animations for professional feel
  - Wrapped OptionsPanel and UCSNamingPanel
  - Dynamic layout adjusts to collapsed/expanded state
  - Saves ~40% screen space when panels collapsed
- ✅ **COMPLETE**: Phase 2.4 - Channel Selection Helpers
  - All/None/Invert buttons for fast channel selection
  - Channel count label showing "X of Y channels selected"
  - Real-time updates as channels are toggled
  - Defensive programming with empty array checks
- ✅ **COMPLETE**: UI/UX Critical Improvements
  - Smart output directory defaults (3-click workflow achieved!)
  - Primary action button redesign (large, prominent, impossible to miss)
  - Enhanced visual hierarchy
  - Professional appearance matching industry standards
- ✅ **COMPLETE**: All code builds and launches successfully

---

## ✅ PHASE 0: FOUNDATION (COMPLETE)

### Core Infrastructure
- [x] JUCE project setup with CMake
- [x] Cross-platform build system (macOS, Windows)
- [x] Modern UI framework with dark theme
- [x] Main window with tabbed interface
- [x] Single file and batch splitter UI components
- [x] Options panel for settings
- [x] Progress reporting system
- [x] Configuration management (JSON)
- [x] Basic audio file I/O with JUCE
- [x] Build scripts (.command for macOS, .bat for Windows)

### Audio Processing Basics
- [x] AudioFileProcessor with threading
- [x] AudioAnalyzer for file validation
- [x] Channel extraction logic
- [x] Sample rate/bit depth override
- [x] Custom channel naming (basic)
- [x] Batch processing (sequential)

**What We Have**: A working application that can split files, but needs professional features.

---

## ✅ PHASE 1: CRITICAL FOUNDATION (COMPLETE)

### Priority 1: Soundminer Metadata Compatibility ✅ COMPLETE

**Objective**: Ensure 100% metadata preservation and Soundminer import compatibility

#### 1.1 BWF Metadata Preservation
- [x] **Implemented BWF BEXT chunk preservation**
  - [x] Description field
  - [x] Originator field
  - [x] OriginatorReference field
  - [x] OriginationDate field
  - [x] OriginationTime field
  - [x] TimeReference field (timecode)
  - [x] CodingHistory field
  - [x] All metadata passed via StringPairArray to writer

**Implementation Status**: ✅ COMPLETE (AudioFileProcessor.cpp lines 349-413)

#### 1.2 iXML Metadata Preservation
- [x] **Implemented iXML preservation**
  - [x] Scene field
  - [x] Take field
  - [x] Tape field
  - [x] Project field
  - [x] All custom iXML fields (via complete metadata copy)

**Implementation Status**: ✅ COMPLETE (AudioFileProcessor.cpp lines 382-395)

---

### Priority 2: UCS Naming System (Weeks 3-4)

**Objective**: Industry-standard UCS naming with minimal friction

#### 2.1 UCS Taxonomy Integration (Week 3)
- [x] **Download UCS taxonomy**
  - [x] Created comprehensive UCS taxonomy JSON file
  - [x] Included 13 major categories (AMBNat, AMBUrb, FOLExt, DSnExt, HRDExt, etc.)
  - [x] Populated subcategories for each category
  - [x] Added channel suffix conventions
  - [x] Stored in Resources folder

- [x] **Parse UCS structure**
  - [x] Implemented UCSManager class
  - [x] JSON loading and parsing
  - [x] Hierarchical category/subcategory structure
  - [x] Category code → subcategory mapping

- [x] **UCS data file**
  - Location: `AudioSplitterJUCE/Resources/ucs_taxonomy.json` ✅ CREATED
  - Contains 13 categories with full subcategory lists
  - Includes naming rules and channel suffix conventions
  - Ready for production use

**Implementation Status**: ✅ COMPLETE
**Files Created**:
- `Source/Utils/UCSManager.h` - UCS management interface
- `Source/Utils/UCSManager.cpp` - Full implementation
- `Resources/ucs_taxonomy.json` - Comprehensive taxonomy data

#### 2.2 UCS Naming UI (Week 3)
- [x] **Category dropdown**
  - [x] Populate from UCS taxonomy
  - [x] Show category code + name (e.g., "AMBNat - Natural Ambience")
  - [x] Auto-select first category by default
  - [ ] Type-ahead search support (future enhancement)
  - [ ] Remember last used category (future enhancement)

- [x] **Subcategory dropdown**
  - [x] Filter based on selected category
  - [x] Auto-populate when category changes
  - [x] Show subcategory name
  - [ ] Type-ahead search support (future enhancement)
  - [ ] Remember last used subcategory (future enhancement)

- [x] **Description field**
  - [x] Free text input
  - [x] Sanitize for filename safety (removes illegal characters)
  - [x] Space to underscore conversion
  - [ ] Remember last used description (future enhancement)
  - [ ] Auto-suggest from filename (future enhancement)

- [x] **Channel suffix auto-append**
  - [x] Standard channel suffix generation (L, R, C, Lfe, Ls, Rs, etc.)
  - [x] Support for mono, stereo, 5.1, 7.1 layouts
  - [x] Fallback to generic Ch1, Ch2 for non-standard counts
  - [x] Integrated with UCSManager

- [x] **Naming preview**
  - [x] Show example output filename with channel suffix
  - [x] Update in real-time as user types
  - [x] Enable/disable toggle for UCS naming
  - [x] Clear visual feedback

**Implementation Status**: ✅ COMPLETE
**UI Implementation**: New dedicated `UCSNamingPanel` component
**Files Created**:
- `Source/Components/UCSNamingPanel.h` - UI interface
- `Source/Components/UCSNamingPanel.cpp` - Full implementation
**Integration Status**: ✅ COMPLETE
- Integrated into MainComponent UI layout
- Connected to SingleFileSplitter processing
- Channel suffixes auto-generated from UCSManager
- Ready for end-to-end testing

#### 2.3 UCS Auto-Detection (Week 4)
- [ ] **Parse existing filenames**
  - [ ] Detect if filename already has UCS format
  - [ ] Extract category, subcategory, description
  - [ ] Pre-populate UI fields

- [ ] **Smart defaults**
  - [ ] Remember last used UCS settings per session
  - [ ] Suggest based on file metadata (if available)
  - [ ] Fallback to sensible defaults

- [ ] **Validation**
  - [ ] Verify category exists in UCS taxonomy
  - [ ] Verify subcategory exists for category
  - [ ] Warn if non-standard format detected

**Files to Modify**: `Source/Utils/UCSManager.cpp`

#### 2.4 Batch UCS Application (Week 4)
- [ ] **Apply UCS to entire batch**
  - [ ] Single UCS category/subcategory for all files
  - [ ] Option to use original filename as description
  - [ ] Option to use metadata fields (Scene, Take) in description

- [ ] **Variable support**
  - [ ] {filename} - Original filename without extension
  - [ ] {scene} - From iXML metadata
  - [ ] {take} - From iXML metadata
  - [ ] {date} - From file or metadata
  - [ ] {channel} - Auto-appended channel suffix

**Example**: `AMBNat_Forest_{filename}_{channel}.wav`

**Files to Modify**: `Source/Components/BatchSplitter.cpp`

---

## ⚡ PHASE 2: PERFORMANCE & UX (Weeks 5-6)

### Priority 1: Parallel Batch Processing ✅ COMPLETE

**Objective**: Fast, reliable processing of 200+ files

#### 2.1 Thread Pool Implementation ✅ COMPLETE
- [x] **Implemented thread pool**
  - [x] Detect CPU core count (`juce::SystemStats::getNumCpus()`)
  - [x] Create thread pool (4-8 threads based on CPU cores)
  - [x] Queue management via `juce::ThreadPool`
  - [x] Automatic load balancing across threads

- [x] **Parallel file processing**
  - [x] Process multiple files simultaneously via `BatchProcessingJob`
  - [x] Thread-safe progress reporting via `BatchJobManager`
  - [x] Thread-safe error collection (mutex-protected)
  - [x] Graceful thread shutdown on cancel (atomic cancellation flag)

**Implementation Status**: ✅ COMPLETE
**New Files Created**:
- `Source/Utils/BatchJobManager.h/cpp` - Thread-safe coordinator
- `Source/Utils/BatchProcessingJob.h/cpp` - ThreadPoolJob wrapper
**Files Modified**:
- `Source/Components/BatchSplitter.h/cpp` - Uses thread pool for parallel processing

**Code Review**: ✅ PASSED (3 critical issues fixed)
- Fixed race condition in filename access (const after construction)
- Fixed callback lifetime safety (capture callbacks by value)
- Added cancellation status tracking to results

#### 2.2 Progress & Monitoring ✅ COMPLETE

- [x] **Enhanced progress reporting**
  - [x] Overall progress (files completed / total)
  - [x] Current file being processed
  - [x] Time elapsed
  - [x] Time remaining estimate (based on average speed)
  - [x] Processing speed (files/min)

- [x] **UI updates**
  - [x] "Processing: 47 of 200 (23%)"
  - [x] "Current: AMBNat_Ocean_Waves_48kHz.wav"
  - [x] "Elapsed: 2:15 | Remaining: ~8:30"
  - [x] "Speed: 3.2 files/min"
  - [x] Progress bar with percentage display
  - [x] Batch mode with comprehensive progress info
  - [x] Simple mode for single file processing

- [ ] **Pause/Resume** (Future enhancement)
  - [ ] Pause button during processing
  - [ ] Resume button when paused
  - [ ] Clean thread management for pause/resume

**Implementation Status**: ✅ COMPLETE
**Files Modified**:
- `Source/Components/ProgressPanel.h` - Added batch progress methods and state tracking
- `Source/Components/ProgressPanel.cpp` - Implemented comprehensive progress display
- `Source/Components/BatchSplitter.cpp` - Integrated with new progress reporting

**Code Review**: ✅ PASSED (All critical issues fixed)
- Fixed division by zero vulnerabilities (MIN_ELAPSED_SECONDS constant)
- Fixed thread synchronization (proper ScopedLock usage)
- Replaced magic numbers with named constants
- Added comprehensive input validation
- Optimized string operations (String::formatted)
- Production-ready implementation

---

### Priority 1.5: UI/UX Critical Improvements ✅ COMPLETE

**Objective**: Transform user experience to professional standards

#### 2.2.1 Smart Output Directory Defaults ✅ COMPLETE

**Problem**: Users had to manually browse and select output directory every time (7 clicks total)

**Solution**: Intelligent auto-configuration when files are dropped

- [x] **Implemented smart output logic**
  - [x] Auto-create `filename_split/` subfolder in source directory
  - [x] Pre-populate output directory field automatically
  - [x] Create directory if it doesn't exist
  - [x] Log creation success/failure
  - [x] Update ConfigManager with both paths
  - [x] Notify parent component to update button states

**Implementation Status**: ✅ COMPLETE
**File Modified**: `Source/Components/SingleFileSplitter.cpp` (lines 165-217)

**Impact**: **Reduced workflow from 7 clicks to 3 clicks (57% improvement)**

**Example**:
```
User drops: "scene12_take3.wav"
Auto-creates: /recordings/scene12_take3_split/
Output field: ✓ /recordings/scene12_take3_split/
Ready to split!
```

#### 2.2.2 Primary Action Button Redesign ✅ COMPLETE

**Problem**: Split button looked identical to secondary buttons - no clear visual hierarchy

**Solution**: Redesigned as unmissable primary action

- [x] **Enhanced button appearance**
  - [x] Changed text to "✓ SPLIT FILES" (clear, action-oriented)
  - [x] Increased size from 1/15 to 1/12 of screen height (minimum 50px)
  - [x] Applied prominent primary color (#007acc blue)
  - [x] Set white text for high contrast
  - [x] Added name "primaryActionButton" for custom styling

- [x] **Enhanced LookAndFeel styling**
  - [x] Larger corner radius (8px vs 6px)
  - [x] More prominent shadow (2px vs 1px depth)
  - [x] Bold heading font (20px vs 16px body font)
  - [x] Thicker focus ring (3px vs 2px)
  - [x] Special handling for primary action buttons

**Implementation Status**: ✅ COMPLETE
**Files Modified**:
- `Source/MainComponent.cpp` (setupSplitButton, resized methods)
- `Source/UI/ModernLookAndFeel.cpp` (drawButtonBackground, drawButtonText)

**Impact**: **Clear visual hierarchy - primary action is impossible to miss**

**Visual Design**:
```
✓ SPLIT FILES
━━━━━━━━━━━━━━
• Large (50px min, 1/12 screen)
• Bold heading font (20px)
• Prominent blue (#007acc)
• White text (high contrast)
• Larger shadow (2px)
• Rounded corners (8px)
```

#### 2.2.3 UI/UX Assessment Completed ✅ COMPLETE

- [x] **Comprehensive UX analysis performed**
  - [x] Visual design assessment (color, typography, spacing)
  - [x] User experience evaluation (workflow efficiency)
  - [x] Information architecture review (panel layout)
  - [x] Accessibility audit (contrast, targets, screen readers)
  - [x] Professional standards comparison (vs Pro Tools, Logic, Soundminer)

**Assessment Results**:
- Overall Grade: B- (75/100) → B+ (85/100) after fixes
- Workflow: 7 clicks → 3 clicks (57% improvement)
- Visual hierarchy: Flat → Clear (dramatic improvement)

**Remaining High-Priority Items** (for future):
1. Filename preview panel (prevents naming mistakes)
2. Error reporting UI (builds user trust)
3. Keyboard shortcuts (power user efficiency)

---

#### 2.3 Collapsible Panels ✅ COMPLETE

**Objective**: Maximize screen space by allowing panels to collapse when not in use

- [x] **CollapsiblePanel component**
  - [x] Header bar with title and expand/collapse button (▼/▶)
  - [x] Smooth fade animations (150ms transition)
  - [x] Dynamic height calculation (getIdealHeight())
  - [x] Click header to toggle collapsed state
  - [x] Modern styling with ModernLookAndFeel colors

- [x] **Wrap existing panels**
  - [x] OptionsPanel wrapped in CollapsiblePanel
  - [x] UCSNamingPanel wrapped in CollapsiblePanel
  - [x] Maintains all existing functionality
  - [x] Seamless integration with MainComponent

- [x] **Dynamic layout system**
  - [x] MainComponent.resized() uses getIdealHeight()
  - [x] Layout automatically adjusts when panels collapse/expand
  - [x] onCollapseChanged callback triggers re-layout
  - [x] Smooth transitions without jarring jumps

- [x] **User benefits**
  - [x] Saves ~40% screen space when panels collapsed
  - [x] Header-only view: 32px (collapsed) vs 180-200px (expanded)
  - [x] Focus on what matters - hide advanced options when not needed
  - [x] Professional appearance with smooth animations

**Implementation Status**: ✅ COMPLETE
**Files Created**:
- `Source/Components/CollapsiblePanel.h` - Collapsible panel interface
- `Source/Components/CollapsiblePanel.cpp` - Full implementation
**Files Modified**:
- `Source/MainComponent.h` - Updated to use CollapsiblePanel wrappers
- `Source/MainComponent.cpp` - Dynamic layout system with getIdealHeight()
- `CMakeLists.txt` - Added CollapsiblePanel to build
**Build Status**: ✅ Builds and launches successfully
**Code Review**: ✅ Ready for production

---

#### 2.4 Error Handling & Recovery (Week 6)
- [ ] **Continue on errors**
  - [ ] Don't stop batch if one file fails
  - [ ] Collect all errors during batch
  - [ ] Show error summary at end

- [ ] **Error reporting**
  - [ ] Clear, actionable error messages
  - [ ] Include filename, reason, suggestion
  - [ ] Log detailed error information

- [ ] **Retry mechanism**
  - [ ] "Retry failed files" button after batch
  - [ ] Re-queue only failed files
  - [ ] Different thread allocation for retries

- [ ] **Error log**
  - [ ] Write errors to log file
  - [ ] Include file paths, timestamps
  - [ ] "Open Error Log" button

**Files to Modify**: `Source/Utils/AudioFileProcessor.cpp`, `Source/Components/BatchSplitter.cpp`

#### 2.5 Batch Verification (Week 6)
- [ ] **Post-processing verification**
  - [ ] Quick audio analysis of outputs
  - [ ] Verify sample count matches input channels
  - [ ] Verify metadata preserved
  - [ ] Generate summary report

- [ ] **Verification report**
  - [ ] Files processed successfully
  - [ ] Files with warnings
  - [ ] Files with errors
  - [ ] Total processing time
  - [ ] Average processing speed

- [ ] **Optional deep verification**
  - [ ] Bit-perfect extraction check (no conversion)
  - [ ] Peak level comparison
  - [ ] Checksum verification (advanced)

**Files to Modify**: `Source/Utils/AudioFileProcessor.cpp`

---

## 🎧 PHASE 3: AUDIO PREVIEW & VISUALIZATION (Weeks 7-8)

### Priority 1: Audio Playback (Week 7)

**Objective**: Preview channels before splitting

#### 3.1 Playback Engine
- [ ] **JUCE audio playback**
  - [ ] Initialize AudioDeviceManager
  - [ ] Create AudioSourcePlayer
  - [ ] Load audio file for playback

- [ ] **Channel selection playback**
  - [ ] Play individual channels
  - [ ] Solo channel playback
  - [ ] Mute/unmute channels
  - [ ] Play all channels (monitoring)

- [ ] **Transport controls**
  - [ ] Play button
  - [ ] Pause button
  - [ ] Stop button
  - [ ] Scrub bar
  - [ ] Volume control

- [ ] **Playback state**
  - [ ] Show playback position
  - [ ] Show duration
  - [ ] Loop option (optional)

**New Files**: `Source/Components/AudioPreviewPanel.h/cpp`
**Files to Modify**: `Source/Components/SingleFileSplitter.cpp` (add preview panel)

#### 3.2 Waveform Visualization (Week 7-8)
- [ ] **JUCE AudioThumbnail**
  - [ ] Generate thumbnails for each channel
  - [ ] Cache thumbnails for performance
  - [ ] Color-coded by channel

- [ ] **Waveform display**
  - [ ] Thumbnail view per channel (small)
  - [ ] Horizontal layout (stacked channels)
  - [ ] Visual indication of audio content
  - [ ] Identify silent/empty channels

- [ ] **Waveform styling**
  - [ ] Color per channel (L=blue, R=red, etc.)
  - [ ] Clear visual separation
  - [ ] Show peak levels
  - [ ] Responsive sizing

**Files to Modify**: `Source/Components/AudioPreviewPanel.cpp`

#### 3.3 Quick Audio Analysis (Week 8)
- [ ] **Per-channel analysis**
  - [ ] Peak level (-dBFS)
  - [ ] RMS level
  - [ ] Crest factor (peak/RMS ratio)
  - [ ] DC offset detection

- [ ] **Stereo analysis** (if applicable)
  - [ ] Phase correlation
  - [ ] Stereo width
  - [ ] Channel balance

- [ ] **Quality checks**
  - [ ] Clipping detection
  - [ ] Over-threshold warnings
  - [ ] Silent channel detection
  - [ ] Low-level warnings

- [ ] **Analysis display**
  - [ ] Show meters per channel
  - [ ] Color-coded levels (green/yellow/red)
  - [ ] Warnings for issues

**New Files**: `Source/Utils/AudioAnalysisEngine.h/cpp`
**Files to Modify**: `Source/Components/AudioPreviewPanel.cpp`

---

## ✨ PHASE 4: POLISH & SHIP (Weeks 9-10)

### Priority 1: Quality of Life (Week 9)

#### 4.1 Keyboard Shortcuts
- [ ] **Essential shortcuts**
  - [ ] Cmd/Ctrl+O: Open file
  - [ ] Cmd/Ctrl+Shift+O: Open directory
  - [ ] Space or Cmd/Ctrl+S: Start processing
  - [ ] Cmd/Ctrl+P: Audio preview/play
  - [ ] Escape: Cancel operation
  - [ ] Cmd/Ctrl+,: Settings
  - [ ] Cmd/Ctrl+R: Recent files

- [ ] **Implementation**
  - [ ] KeyPress handlers in MainComponent
  - [ ] Keyboard focus management
  - [ ] Visual feedback for shortcuts
  - [ ] Keyboard shortcuts reference dialog

**Files to Modify**: `Source/MainComponent.cpp`, `Source/MainWindow.cpp`

#### 4.2 Enhanced Drag & Drop
- [ ] **Folder operations**
  - [ ] Drop folder = recursive WAV scan
  - [ ] Show file count before processing
  - [ ] Filter non-WAV files
  - [ ] Sort files alphabetically

- [ ] **Multiple file handling**
  - [ ] Drop multiple files = batch queue
  - [ ] Drop multiple folders = combine all WAVs
  - [ ] Hold Shift = add to existing queue (vs replace)

- [ ] **Visual feedback**
  - [ ] Show what will happen on drop
  - [ ] Highlight drop zones more clearly
  - [ ] Show file count during drag
  - [ ] Preview first few filenames

**Files to Modify**: `Source/Components/SingleFileSplitter.cpp`, `Source/Components/BatchSplitter.cpp`

#### 4.3 Settings & Preferences
- [ ] **Persistent settings**
  - [ ] Default output directory
  - [ ] Last used UCS categories
  - [ ] Parallel thread count preference
  - [ ] Window size/position

- [ ] **Recent files**
  - [ ] Last 10 processed files
  - [ ] Quick re-open
  - [ ] Clear recent files option

- [ ] **Preferences dialog**
  - [ ] General settings tab
  - [ ] Audio settings tab
  - [ ] Advanced settings tab
  - [ ] About/version info

**Files to Modify**: `Source/Utils/ConfigManager.cpp`
**New Files**: `Source/Components/PreferencesDialog.h/cpp` (optional)

#### 4.4 File Verification
- [ ] **Integrity checks**
  - [ ] Bit-perfect extraction verification
  - [ ] Sample count confirmation
  - [ ] Metadata preservation check
  - [ ] No audio artifacts check

- [ ] **Verification report**
  - [ ] Generate detailed report (optional)
  - [ ] Show verification status in UI
  - [ ] Export report as text/PDF

- [ ] **User confidence**
  - [ ] "✓ Verified" indicator
  - [ ] Quick verification mode (fast)
  - [ ] Deep verification mode (thorough)

**Files to Modify**: `Source/Utils/AudioFileProcessor.cpp`

---

### Priority 2: Testing & Documentation (Week 10)

#### 4.5 Real-World Testing
- [ ] **Beta testing program**
  - [ ] Recruit 3-5 professional sound editors/recordists
  - [ ] Provide beta builds
  - [ ] Collect structured feedback
  - [ ] Track issues in GitHub

- [ ] **Stress testing**
  - [ ] Process 1000+ real files
  - [ ] Various recorders, formats
  - [ ] Large files >1GB
  - [ ] Edge cases (corrupt files, weird metadata)

- [ ] **Soundminer verification**
  - [ ] Import 100+ output files into Soundminer
  - [ ] Verify all metadata appears correctly
  - [ ] Test on both macOS and Windows
  - [ ] Document any compatibility issues

- [ ] **Performance benchmarks**
  - [ ] Measure batch processing speed
  - [ ] Memory usage monitoring
  - [ ] CPU usage profiling
  - [ ] Compare to target metrics

#### 4.6 Documentation
- [ ] **User documentation**
  - [ ] Quick start guide (one page)
  - [ ] UCS naming guide
  - [ ] Soundminer workflow guide
  - [ ] Keyboard shortcuts reference
  - [ ] Troubleshooting guide
  - [ ] FAQ

- [ ] **Technical documentation**
  - [ ] Build instructions (update)
  - [ ] Developer guide
  - [ ] Architecture overview
  - [ ] Metadata handling details
  - [ ] UCS implementation notes

- [ ] **Video tutorials** (optional)
  - [ ] Basic workflow demo
  - [ ] Batch processing tutorial
  - [ ] UCS naming tutorial

---

### Priority 3: Build & Distribution (Week 10)

#### 4.7 macOS Build
- [ ] **Universal Binary**
  - [ ] Build for Intel (x86_64)
  - [ ] Build for Apple Silicon (arm64)
  - [ ] Test on both architectures

- [ ] **Code Signing**
  - [ ] Apple Developer account setup
  - [ ] Create signing certificate
  - [ ] Sign application bundle
  - [ ] Verify signature

- [ ] **Notarization**
  - [ ] Submit to Apple notarization service
  - [ ] Staple notarization ticket
  - [ ] Test Gatekeeper approval

- [ ] **DMG Installer**
  - [ ] Create DMG with drag-to-Applications
  - [ ] Custom DMG background
  - [ ] License agreement
  - [ ] Test installation on clean system

#### 4.8 Windows Build
- [ ] **x64 Build**
  - [ ] Build 64-bit version
  - [ ] Test on Windows 10
  - [ ] Test on Windows 11

- [ ] **Code Signing**
  - [ ] Acquire code signing certificate
  - [ ] Sign executable
  - [ ] Verify signature

- [ ] **Installer**
  - [ ] Create NSIS or Inno Setup installer
  - [ ] Start menu shortcuts
  - [ ] Desktop shortcut option
  - [ ] File association (.wav)
  - [ ] Uninstaller
  - [ ] Test installation on clean system

#### 4.9 Release Preparation
- [ ] **Version 1.0 checklist**
  - [ ] All Phase 1-4 features complete
  - [ ] All critical bugs fixed
  - [ ] Documentation complete
  - [ ] Builds signed and tested
  - [ ] Beta feedback incorporated
  - [ ] Performance targets met

- [ ] **Release notes**
  - [ ] Feature list
  - [ ] Known issues
  - [ ] System requirements
  - [ ] Installation instructions

- [ ] **Release assets**
  - [ ] macOS DMG
  - [ ] Windows installer
  - [ ] User documentation PDF
  - [ ] Quick start guide
  - [ ] Sample files (optional)

---

## 🔮 PHASE 5: FUTURE ENHANCEMENTS (Post 1.0)

### WELDER Tool (Version 2.0)

**Objective**: Combine mono files back to multichannel

- [ ] **Core functionality**
  - [ ] Multi-file input (drag multiple mono files)
  - [ ] Channel assignment UI (drag to target channels)
  - [ ] Sample rate/bit depth compatibility checking
  - [ ] Multichannel file creation

- [ ] **UCS-aware features**
  - [ ] Recognize UCS channel suffixes (_L, _R, etc.)
  - [ ] Auto-assign channels from UCS naming
  - [ ] Suggest channel layout from filenames

- [ ] **Soundminer compatible**
  - [ ] Merge metadata from source files
  - [ ] Preserve BWF/iXML fields intelligently
  - [ ] Timecode handling

- [ ] **Advanced features**
  - [ ] Timeline sync by timestamp
  - [ ] Manual time offset controls
  - [ ] File length alignment
  - [ ] Batch welding (multiple sets)

**New Files**: `Source/Components/Welder.h/cpp`, `Source/Utils/MultiChannelCreator.h/cpp`

---

### SLICER Tool (Version 3.0)

**Objective**: Slice long files into segments with markers

- [ ] **Waveform editor**
  - [ ] Full-width waveform display
  - [ ] Zoom/pan controls
  - [ ] Scrubbing and playback
  - [ ] Visual marker placement

- [ ] **Marker system**
  - [ ] Click-to-place markers
  - [ ] Drag-to-adjust markers
  - [ ] Named markers
  - [ ] Snap-to-zero-crossing

- [ ] **Auto-detection**
  - [ ] Silence detection (threshold-based)
  - [ ] Transient detection
  - [ ] Tempo change detection
  - [ ] Apply markers across multiple files

- [ ] **UCS naming per slice**
  - [ ] Same UCS category/subcategory
  - [ ] Auto-increment description
  - [ ] Preserve metadata per slice

- [ ] **Export**
  - [ ] Export all segments
  - [ ] Crossfade options
  - [ ] Trim silence
  - [ ] Generate slice report

**New Files**: `Source/Components/Slicer.h/cpp`, `Source/Components/WaveformEditor.h/cpp`, `Source/Utils/MarkerSystem.h/cpp`

---

## ❌ OUT OF SCOPE - Do Not Implement

The following features are explicitly excluded to maintain focus:

- ❌ Multiple naming template systems (UCS only)
- ❌ Podcast/music/radio-specific workflows
- ❌ Format conversion beyond WAV/WAV64 (MP3, AAC, FLAC, etc.)
- ❌ Social media features, cloud integration, user accounts
- ❌ AI/ML features, "smart" auto-processing
- ❌ Plugin hosting (VST/AU)
- ❌ Real-time effects processing
- ❌ Sample rate conversion (use pro tools instead)
- ❌ Marketing features, analytics, telemetry
- ❌ Complex template builder systems
- ❌ DAW integration (Pro Tools, Logic, etc.)

**Rationale**: Every feature adds complexity, maintenance burden, and potential failure points. Stay focused on core value: fast, reliable splitting with perfect metadata and UCS naming.

---

## 📈 SUCCESS METRICS

### Version 1.0 Definition of Done

✅ **Soundminer Compatibility**: Import metadata perfectly every time
✅ **UCS Naming**: Correct, consistent, industry-standard
✅ **Speed**: Process 200 files in <5 minutes
✅ **Reliability**: Zero corruption in 1000-file stress test
✅ **Simplicity**: Three clicks from drop to done
✅ **Error Handling**: Clear, actionable, doesn't stop batch
✅ **Audio Preview**: See/hear what you're splitting
✅ **Trust**: Professionals bet their recordings on it

### User Feedback Goals

- **Beta testers**: 5+ professionals provide positive feedback
- **Workflow improvement**: "Saves me hours every day"
- **Reliability**: "I trust it with my master recordings"
- **Speed**: "10x faster than my old workflow"
- **Recommendation**: "I've told everyone about this tool"

---

## 📞 SUPPORT & FEEDBACK

- **Issues**: [GitHub Issues Link]
- **Feature Requests**: [GitHub Discussions Link]
- **Documentation**: [Docs Site Link]
- **UCS Standard**: universalcategorysystem.com
- **Soundminer**: soundminer.com

---

**Remember**: This tool handles irreplaceable professional audio recordings. Metadata integrity and reliability are non-negotiable. Speed and simplicity are essential. Everything else is secondary.
