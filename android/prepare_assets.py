#!/usr/bin/env python3
"""Package original resources without the player's save files."""
from pathlib import Path
import shutil

root = Path(__file__).resolve().parent
runtime = root.parent / 'run-macos'
assets = root / 'app/src/main/assets/runtime'
assets.mkdir(parents=True, exist_ok=True)
for name in ('legion', 'fonts'):
    source, dest = runtime / name, assets / name
    shutil.copytree(source, dest, dirs_exist_ok=True,
                    ignore=shutil.ignore_patterns('archiwum', '.DS_Store', 'zapis*'))
for name in ('Legion.config', 'smfk_logo'):
    shutil.copy2(runtime / name, assets / name)
(assets / 'legion/archiwum').mkdir(parents=True, exist_ok=True)
