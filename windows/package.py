#!/usr/bin/env python3
"""Create the portable Windows bundle without personal saves."""
from pathlib import Path
import re
import shutil
import subprocess
import zipfile

ROOT = Path(__file__).resolve().parent.parent
WIN = ROOT / 'windows'
TOOLCHAIN = WIN / 'toolchain/llvm-mingw-20260616-ucrt-macos-universal'
SYSROOT = WIN / 'toolchain/msys2/ucrt64'
OUTPUT = ROOT / 'dist-windows/Legion-Windows'
SYSTEM = set('advapi32 bcrypt cfgmgr32 comctl32 comdlg32 crypt32 dinput8 dsound dwrite dwmapi gdi32 glu32 imm32 iphlpapi kernel32 kernelbase msvcrt ntdll ole32 oleaut32 opengl32 powrprof rpcrt4 setupapi shell32 shlwapi ucrtbase usp10 user32 userenv uuid version winmm winspool ws2_32 wtsapi32'.split())

def system_dll(name):
    stem = name.lower().removesuffix('.dll')
    return stem in SYSTEM or stem.startswith(('api-ms-win-', 'ext-ms-win-'))

def imports(path):
    report = subprocess.check_output([str(TOOLCHAIN/'bin/llvm-readobj'), '--coff-imports', str(path)], text=True)
    return re.findall(r'^\s+Name: (\S+\.dll)\s*$', report, flags=re.M | re.I)

def main():
    if OUTPUT.exists(): shutil.rmtree(OUTPUT)
    OUTPUT.mkdir(parents=True)
    shutil.copy2(ROOT/'build-windows/Legion.exe', OUTPUT/'Legion.exe')
    runtime = ROOT/'run-macos'
    ignore = shutil.ignore_patterns('archiwum', '.DS_Store', 'zapis*')
    for name in ('legion','fonts'): shutil.copytree(runtime/name, OUTPUT/name, ignore=ignore)
    (OUTPUT/'legion/archiwum').mkdir()
    for name in ('Legion.config','smfk_logo'): shutil.copy2(runtime/name,OUTPUT/name)
    # The work area is also respected by the executable on smaller displays.
    cfg=OUTPUT/'Legion.config'
    text=cfg.read_text().replace('Width =\t1280','Width =\t960').replace('Height = 1040','Height = 780').replace('Filtering = 4','Filtering = 0')
    cfg.write_text(text)
    shutil.copy2(WIN/'CZYTAJ-MNIE.txt',OUTPUT/'CZYTAJ-MNIE.txt')
    shutil.copytree(SYSROOT/'share/licenses', OUTPUT/'licenses')
    candidates={p.name.lower():p for directory in (TOOLCHAIN/'x86_64-w64-mingw32/bin',SYSROOT/'bin') for p in directory.glob('*.dll')}
    pending=[OUTPUT/'Legion.exe']
    # SDL_mixer discovers the module and Vorbis backends dynamically.
    for name in ('libmikmod-3.dll','libvorbisfile-3.dll'):
        pending.append(candidates[name])
    processed=set()
    while pending:
        path=pending.pop(); key=path.name.lower()
        if key in processed: continue
        processed.add(key)
        if path.parent!=OUTPUT: shutil.copy2(path, OUTPUT/path.name)
        for name in imports(path):
            key=name.lower()
            if key in processed or system_dll(name): continue
            if key not in candidates: raise RuntimeError(f'Missing Windows library {name}, required by {path.name}')
            pending.append(candidates[key])
    archive=OUTPUT.parent/'Legion-Windows.zip'
    with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as z:
        for f in sorted(OUTPUT.rglob('*')):
            if f.is_file(): z.write(f, f.relative_to(OUTPUT.parent))
            elif f.is_dir() and not any(f.iterdir()): z.writestr(str(f.relative_to(OUTPUT.parent)) + '/', '')
    print('Package:',archive)
    print('Bundled DLLs:',len(processed)-1)

if __name__=='__main__': main()
