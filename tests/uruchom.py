#!/usr/bin/env python3
"""Run an isolated Legion scenario without changing the player's data."""
import argparse
import datetime
import hashlib
import json
import os
import re
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
RUNTIME = ROOT / 'run-macos'
BINARY = RUNTIME / 'Legion.app/Contents/MacOS/legion'


def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def save_map(scenario, trace, output):
    """Create a whole-map diagram in world coordinates, independent of camera."""
    if not trace.is_file():
        return
    starts, obstacles, paths = {}, [], {}
    for line in scenario.read_text(encoding='utf-8').splitlines():
        parts = line.split('#', 1)[0].split()
        try:
            if len(parts) >= 4 and parts[0] in ('unit', 'enemy_unit'):
                starts[int(parts[1])] = (int(parts[2]), int(parts[3]))
            elif len(parts) >= 6 and parts[0] == 'obstacle':
                obstacles.append(tuple(map(int, parts[2:6])))
        except ValueError:
            continue
    for line in trace.read_text(encoding='utf-8').splitlines():
        try:
            row = json.loads(line)
            paths.setdefault(row['unit'], []).append(row)
        except (ValueError, KeyError):
            continue
    svg = ['<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 640 560">',
           '<rect width="640" height="512" fill="#e7efd8" stroke="#394a33"/>']
    for x1, y1, x2, y2 in obstacles:
        svg.append(f'<rect x="{x1}" y="{y1}" width="{x2-x1}" height="{y2-y1}" fill="#cc514a" fill-opacity=".6" stroke="#8d2722"/>')
    colors = ('#1645bb', '#b71987', '#7b4b00', '#00816b', '#5b2997')
    for unit, rows in sorted(paths.items()):
        color = colors[(unit - 1) % len(colors)]
        points = [starts.get(unit, (rows[0]['x'], rows[0]['y']))]
        points.extend((r['x'], r['y']) for r in rows)
        coords = ' '.join(f'{x},{y}' for x, y in points)
        svg.append(f'<polyline points="{coords}" fill="none" stroke="{color}" stroke-width="3"/>')
        x, y = points[0]
        svg.append(f'<circle cx="{x}" cy="{y}" r="6" fill="{color}"/>')
        x, y = points[-1]
        svg.append(f'<circle cx="{x}" cy="{y}" r="6" fill="white" stroke="{color}" stroke-width="3"/>')
        x, y = rows[-1]['target_x'], rows[-1]['target_y']
        svg.append(f'<path d="M{x-6} {y-6} l12 12 m0 -12 l-12 12" stroke="{color}" stroke-width="3"/>')
    svg.append('<text x="8" y="535" font-family="sans-serif" font-size="14">Punkt: start · Obręcz: koniec · Krzyż: cel · Czerwony: dodana przeszkoda</text></svg>')
    (output / 'trasa.svg').write_text('\n'.join(svg), encoding='utf-8')


def main():
    parser = argparse.ArgumentParser(description='Uruchom scenariusz testowy LegionPC na osobnej kopii danych.')
    parser.add_argument('scenario', type=Path, help='plik scenariusza')
    parser.add_argument('--output', type=Path, help='pusty katalog wyników (domyślnie tests/wyniki)')
    parser.add_argument('--timeout', type=int, default=90, help='limit czasu w sekundach (domyślnie 90)')
    parser.add_argument('--exact-window', action='store_true', help='wyłącz dopasowanie do ekranu w tymczasowej kopii konfiguracji')
    args = parser.parse_args()
    scenario = args.scenario.resolve()
    if not scenario.is_file() or not BINARY.is_file():
        parser.error('Brak scenariusza lub programu gry; skompiluj projekt i sprawdź ścieżki.')
    if args.timeout <= 0:
        parser.error('Timeout musi być dodatni.')
    if args.output:
        output = args.output.resolve()
        output.mkdir(parents=True, exist_ok=True)
        if any(output.iterdir()):
            parser.error('Katalog wyników musi być pusty.')
    else:
        results = ROOT / 'tests/wyniki'
        results.mkdir(parents=True, exist_ok=True)
        output = Path(tempfile.mkdtemp(prefix=scenario.stem + '-', dir=results))
    shutil.copy2(scenario, output / 'scenariusz.txt')
    scenario_copy = output / 'scenariusz.txt'
    started = datetime.datetime.now(datetime.timezone.utc).isoformat()
    status = {'scenario': str(scenario), 'scenario_sha256': sha256(scenario_copy),
              'binary_sha256': sha256(BINARY), 'started_utc': started}
    with tempfile.TemporaryDirectory(prefix='legionpc-test-') as temp:
        work = Path(temp)
        for name in ('legion', 'fonts'):
            shutil.copytree(RUNTIME / name, work / name)
        for name in ('Legion.config', 'smfk_logo'):
            shutil.copy2(RUNTIME / name, work / name)
        if args.exact_window:
            config = work / 'Legion.config'
            content = config.read_text(encoding='utf-8')
            content, count = re.subn(r'(?m)^\s*FitToDisplay\s*=\s*\S+[^\n]*$', 'FitToDisplay = False', content)
            config.write_text(content if count else content + '\nFitToDisplay = False\n', encoding='utf-8')
        env = os.environ.copy()
        # Keep automated runs silent without changing the player's settings.
        env.update(SDL_AUDIODRIVER='dummy', LEGION_AUTOMATION=str(scenario_copy), LEGION_TEST_WORKDIR=str(work),
                   LEGION_TEST_OUTPUT=str(output))
        try:
            proc = subprocess.run([str(BINARY), 'PL'], cwd=work, env=env,
                                  stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                  timeout=args.timeout)
            log = proc.stdout.decode('utf-8', errors='replace')
            status['exit_code'] = proc.returncode
            status['success'] = proc.returncode == 0 and 'TEST_OK ticks=' in log
        except subprocess.TimeoutExpired as exc:
            log = (exc.stdout or b'').decode('utf-8', errors='replace')
            log += '\nTEST ERROR: przekroczono limit czasu\n'
            status.update(exit_code=None, success=False, timeout=True)
    (output / 'gra.log').write_text(log, encoding='utf-8')
    (output / 'status.json').write_text(json.dumps(status, indent=2, ensure_ascii=False) + '\n', encoding='utf-8')
    save_map(scenario_copy, output / 'trace.jsonl', output)
    for line in log.splitlines():
        if line.startswith(('TEST_', 'TEST ERROR', '{"event"')):
            print(line)
    if not status['success']:
        print('Test zakończył się błędem; kod:', status['exit_code'], file=sys.stderr)
    print('Wyniki:', output)
    return 0 if status['success'] else 1


if __name__ == '__main__':
    sys.exit(main())
