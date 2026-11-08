#!/usr/bin/env python3
"""Package a relocatable x86-64 macOS build without player saves."""
from pathlib import Path
import re
import shutil
import subprocess

ROOT = Path(__file__).resolve().parent.parent
RUNTIME = ROOT / 'run-macos'
OUT = ROOT / 'dist-macos'
PACKAGE = OUT / 'OpenLegion-macOS'
APP = PACKAGE / 'Legion.app'
FRAMEWORKS = APP / 'Contents/Frameworks'
EXE = APP / 'Contents/MacOS/legion'
HOMEBREW = {
    'libfreetype.6.dylib': Path('/usr/local/opt/freetype/lib/libfreetype.6.dylib'),
    'libpng16.16.dylib': Path('/usr/local/opt/libpng/lib/libpng16.16.dylib'),
    'libjpeg.8.dylib': Path('/usr/local/opt/jpeg-turbo/lib/libjpeg.8.dylib'),
}

def run(*cmd):
    return subprocess.check_output([str(c) for c in cmd], text=True)

def dependencies(path):
    lines = run('otool', '-arch', 'x86_64', '-L', path).splitlines()[1:]
    return [line.strip().split(' (compatibility version')[0] for line in lines if ' (compatibility version' in line]

def main():
    if PACKAGE.exists(): shutil.rmtree(PACKAGE)
    PACKAGE.mkdir(parents=True)
    shutil.copytree(RUNTIME/'Legion.app', APP, ignore=shutil.ignore_patterns('Frameworks', '.DS_Store'))
    FRAMEWORKS.mkdir()
    for name in ('legion', 'fonts'):
        shutil.copytree(RUNTIME/name, PACKAGE/name, ignore=shutil.ignore_patterns('archiwum', '.DS_Store', 'zapis*'))
    (PACKAGE/'legion/archiwum').mkdir()
    for name in ('Legion.config', 'smfk_logo'): shutil.copy2(RUNTIME/name, PACKAGE/name)
    shutil.copytree(ROOT/'macos/licenses', PACKAGE/'licenses')
    for library, filename in [('freetype','LICENSE.TXT'), ('libpng','LICENSE'), ('jpeg-turbo','LICENSE.md')]:
        shutil.copy2(Path('/usr/local/opt')/library/filename, PACKAGE/'licenses'/f'{library}-{filename}')
    pending = [EXE]
    visited = set()
    while pending:
        path = pending.pop()
        if path in visited: continue
        visited.add(path)
        for dep in dependencies(path):
            if dep.startswith(('/System/Library/', '/usr/lib/')): continue
            name = Path(dep).name
            if name == path.name: continue
            if not (dep.startswith('@executable_path/../Frameworks/') or dep.startswith('/usr/local/opt/')):
                raise RuntimeError(f'Unexpected dependency {dep} in {path}')
            target = FRAMEWORKS/name
            if not target.exists():
                source = HOMEBREW.get(name, RUNTIME/'Legion.app/Contents/Frameworks'/name)
                if not source.exists(): raise RuntimeError(f'Missing {dep}, required by {path}')
                if 'x86_64' not in run('lipo', '-archs', source).split():
                    raise RuntimeError(f'No x86_64 slice in {source}')
                shutil.copy2(source, target)
                target.chmod(0o755)
                pending.append(target)
            if dep.startswith('/usr/local/opt/'):
                subprocess.run(['install_name_tool', '-change', dep, '@executable_path/../Frameworks/'+name, str(path)], check=True)
    subprocess.run(['codesign', '--force', '--deep', '--sign', '-', str(APP)], check=True)
    subprocess.run(['codesign', '--verify', '--deep', '--strict', str(APP)], check=True)
    archive = OUT/'OpenLegion-macOS.zip'
    if archive.exists(): archive.unlink()
    subprocess.run(['ditto', '-c', '-k', '--norsrc', '--keepParent', str(PACKAGE), str(archive)], check=True)
    print(f'Package: {archive}; bundled dylibs: {len(visited)-1}')

if __name__ == '__main__': main()
