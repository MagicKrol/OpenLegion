#!/usr/bin/env python3
"""Fetch a SHA-256-verified Windows UCRT dependency sysroot from MSYS2."""
import hashlib
import io
import json
from pathlib import Path
import subprocess
import tarfile
import urllib.parse
import urllib.request

HERE = Path(__file__).resolve().parent
CACHE = HERE / 'toolchain'
CACHE.mkdir(parents=True, exist_ok=True)
DB = CACHE / 'ucrt64.db'
BASE = 'https://repo.msys2.org/mingw/ucrt64/'
PACKAGES = '''SDL SDL_image SDL_mixer SDL_ttf libpng libjpeg-turbo zlib freetype
brotli bzip2 harfbuzz glib2 graphite2 libffi pcre2 gettext-runtime libiconv
libtiff libwebp jbigkit lerc libdeflate xz zstd giflib
libvorbis libogg libmikmod libmad flac libgcc libstdc++ libwinpthread'''.split()

def extract_zst(path):
    return subprocess.check_output(['zstd', '-d', '-c', str(path)])

def get_database():
    if not DB.exists(): urllib.request.urlretrieve(BASE + 'ucrt64.db', DB)
    result = {}
    with tarfile.open(fileobj=io.BytesIO(extract_zst(DB)), mode='r:') as tf:
        for member in tf:
            if not member.name.endswith('/desc'): continue
            fields = {}; key = None
            for line in tf.extractfile(member).read().decode().splitlines():
                if line.startswith('%') and line.endswith('%'):
                    key = line[1:-1]; fields[key] = []
                elif line and key:
                    fields[key].append(line)
            result[fields['NAME'][0]] = fields
    return result

def main():
    lockfile = HERE / 'dependencies.lock.json'
    if lockfile.exists():
        locked = {p['name']: p for p in json.loads(lockfile.read_text())}
    else:
        db = get_database()
        locked = {name: {'file': meta['FILENAME'][0], 'sha256': meta['SHA256SUM'][0]} for name,meta in db.items()}
    sysroot = CACHE / 'msys2'; sysroot.mkdir(exist_ok=True)
    lock = []
    for short in PACKAGES:
        name = 'mingw-w64-ucrt-x86_64-' + short
        meta = locked[name]; filename = meta['file']; digest = meta['sha256']
        archive = CACHE / filename
        if not archive.exists() or hashlib.sha256(archive.read_bytes()).hexdigest() != digest:
            print('Downloading', filename, flush=True)
            urllib.request.urlretrieve(BASE + urllib.parse.quote(filename), archive)
        if hashlib.sha256(archive.read_bytes()).hexdigest() != digest:
            raise RuntimeError('Invalid SHA-256: ' + filename)
        lock.append({'name': name, 'file': filename, 'sha256': digest})
        with tarfile.open(fileobj=io.BytesIO(extract_zst(archive)), mode='r:') as tf:
            members = [m for m in tf if m.name.startswith(('ucrt64/bin/', 'ucrt64/include/', 'ucrt64/lib/', 'ucrt64/share/licenses/'))]
            tf.extractall(sysroot, members=members, filter='data')
    (HERE / 'dependencies.lock.json').write_text(json.dumps(lock, indent=2) + '\n')
    print('Windows dependencies:', sysroot / 'ucrt64')

if __name__ == '__main__': main()
