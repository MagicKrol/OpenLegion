#!/usr/bin/env python3
"""Smoke test the packaged Windows EXE in one isolated Wine session on macOS."""
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parent.parent
ZIP = ROOT/'dist-windows/Legion-Windows.zip'
SCENARIO = ROOT/'tests/scenarios/windows_pakiet.txt'
WINE = Path('/usr/local/bin/wine64')
SERVER = Path('/usr/local/bin/wineserver')

def wine_path(path):
    return 'Z:' + str(path.resolve()).replace('/', '\\')

def main():
    if not ZIP.is_file() or not WINE.is_file():
        sys.exit('Build the Windows package and install Wine first.')
    results = ROOT/'tests/wyniki'; results.mkdir(parents=True,exist_ok=True)
    output = Path(tempfile.mkdtemp(prefix='windows-',dir=results)).resolve()
    with tempfile.TemporaryDirectory(prefix='legion-wine-') as tmp:
        temp=Path(tmp); work=temp/'game'; work.mkdir()
        with zipfile.ZipFile(ZIP) as z:
            for entry in z.infolist():
                path=Path(entry.filename)
                if path.is_absolute() or '..' in path.parts: raise ValueError('Unsafe ZIP path')
                if 'archiwum' in path.parts and not entry.is_dir(): raise ValueError('Package contains a saved game')
            z.extractall(work)
        game=work/'Legion-Windows'; scenario=temp/'scenario.txt'; shutil.copy2(SCENARIO,scenario)
        unicode_dir=temp/'Łódź-玩家'; unicode_dir.mkdir()
        env=os.environ.copy(); env.update(WINEPREFIX=str(temp/'wineprefix'),WINEDEBUG='-all',SDL_STDIO_REDIRECT='1',
            LEGION_AUTOMATION=wine_path(scenario),LEGION_TEST_OUTPUT=wine_path(output),
            LEGION_UNICODE_TEST_DIR=wine_path(unicode_dir))
        try:
            result=subprocess.run([str(WINE),str(game/'Legion.exe')],cwd=temp,env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=120)
        finally:
            subprocess.run([str(SERVER),'-k'],env=env,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL,timeout=10)
        for name in ('stdout.txt','stderr.txt'):
            path=game/name
            if path.exists(): shutil.copy2(path,output/('wine-'+name))
        log=(output/'wine-stdout.txt').read_text(errors='replace') if (output/'wine-stdout.txt').exists() else ''
        print(log); print('Wine exit:',result.returncode,'Results:',output)
        if result.returncode or 'TEST_OK' not in log: sys.exit(1)
        if not (unicode_dir/'LegionPC/archiwum/autosave').is_file() or not list(unicode_dir.glob('LegionPC-*.mod')):
            sys.exit('Missing save or music file in Unicode directory')

if __name__=='__main__': main()
