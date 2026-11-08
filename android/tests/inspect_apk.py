#!/usr/bin/env python3
"""Sanity-check the installable Android artifact without running an emulator."""
import sys
import zipfile
from pathlib import Path

apk = Path(sys.argv[1]) if len(sys.argv) > 1 else Path(__file__).resolve().parents[1] / 'app/build/outputs/apk/debug/app-debug.apk'
with zipfile.ZipFile(apk) as archive:
    bad = archive.testzip()
    if bad:
        raise SystemExit(f'Corrupt ZIP member: {bad}')
    names = set(archive.namelist())
    for name in ('liblog.so', 'libz.so'):
        assert f'lib/arm64-v8a/{name}' not in names, f'Unexpected Android system library: {name}'
    for name in ('libSDL2.so', 'libSDL.so', 'libGL.so', 'libSDL_image.so', 'libSDL_mixer.so', 'libSDL_ttf.so', 'libc++_shared.so', 'libmain.so'):
        path = 'lib/arm64-v8a/' + name
        assert path in names, f'Missing {path}'
        elf = archive.read(path)[:20]
        assert elf[:4] == b'\x7fELF' and elf[4] == 2 and elf[5] == 1 and int.from_bytes(elf[18:20], 'little') == 183, f'Not ARM64 ELF: {path}'
    for path in ('AndroidManifest.xml', 'classes.dex', 'assets/runtime/Legion.config', 'assets/runtime/legion/mapa2.org', 'assets/runtime/fonts/FUNDR___.TTF'):
        assert path in names, f'Missing {path}'
    assert not any('archiwum/zapis' in name.lower() for name in names), 'Player save found in APK'
print(f'APK_OK {apk}')
