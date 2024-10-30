# -*- mode: python ; coding: utf-8 -*-


a = Analysis(
    ['audio_splitter_gui.py'],
    pathex=[],
    binaries=[('ffmpeg/ffmpeg', 'ffmpeg'), ('ffmpeg/ffprobe', 'ffmpeg')],
    datas=[('tkdnd', 'tkdnd')],
    hiddenimports=['tkinter', 'pydub.utils', 'pydub', 'numpy'],
    hookspath=[],
    hooksconfig={},
    runtime_hooks=[],
    excludes=[],
    noarchive=False,
    optimize=0,
)
pyz = PYZ(a.pure)

exe = EXE(
    pyz,
    a.scripts,
    a.binaries,
    a.datas,
    [],
    name='ZQ SFX Audio Splitter',
    debug=False,
    bootloader_ignore_signals=False,
    strip=False,
    upx=True,
    upx_exclude=[],
    runtime_tmpdir=None,
    console=False,
    disable_windowed_traceback=False,
    argv_emulation=False,
    target_arch=None,
    codesign_identity=None,
    entitlements_file=None,
)
app = BUNDLE(
    exe,
    name='ZQ SFX Audio Splitter.app',
    icon=None,
    bundle_identifier=None,
)
