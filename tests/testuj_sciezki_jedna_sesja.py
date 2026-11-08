#!/usr/bin/env python3
"""Exercise tactical pathfinding scenarios in one isolated game process."""
import json
from pathlib import Path
import subprocess
import sys
import tempfile

from testuj import verify_obstacles

ROOT = Path(__file__).resolve().parents[1]
CASES = (
    'bez_przeszkod', 'sciana', 'zakret', 'przejscie', 'brak_drogi',
    'nowy_rozkaz', 'dwie_jednostki', 'przeciwnik', 'wolna_jednostka',
    'nowa_przeszkoda', 'mijanie', 'mijanie_pion', 'mijanie_korytarz',
    'mijanie_rozne', 'mijanie_przeciwnikow', 'trzy_jednostki',
    'wspolny_cel', 'ciasne_drzewa', 'przy_korzeniu', 'uwolnienie_z_drzewa',
    'las_oryginalny', 'las_oryginalny_7', 'path_grupowy_atak',
    'path_ruchomy_cel', 'path_za_sklepem', 'path_sklep_wejscie',
    'path_sklep_zmiana', 'path_sklep_ostatni', 'path_grupowy_niedostepny',
    'path_atak_miasto', 'path_nalozone_postacie',
)


def main():
    results = ROOT / 'tests/wyniki'
    results.mkdir(parents=True, exist_ok=True)
    output = Path(tempfile.mkdtemp(prefix='sciezki-jedna-sesja-', dir=results))
    scenario = output / 'zestaw.txt'
    lines = []
    for index, name in enumerate(CASES):
        source = ROOT / 'tests/scenarios' / (name + '.txt')
        lines.append('test_case ' + name)
        for line in source.read_text(encoding='utf-8').splitlines():
            if index and line.startswith('scene '):
                lines.extend(('clear_test_units', line.replace('scene ', 'reload_scene ', 1)))
            else:
                lines.append(line)
    scenario.write_text('\n'.join(lines) + '\n', encoding='utf-8')
    run_dir = output / 'run'
    result = subprocess.run([sys.executable, str(ROOT / 'tests/uruchom.py'),
                             str(scenario), '--output', str(run_dir), '--timeout', '300'], text=True)
    if result.returncode:
        print('Niepowodzenie; wyniki:', output)
        return result.returncode
    segments = []
    last_tick = 0
    for line in (run_dir / 'trace.jsonl').read_text(encoding='utf-8').splitlines():
        row = json.loads(line)
        if row['tick'] < last_tick:
            segments.append([])
        if not segments:
            segments.append([])
        segments[-1].append(line)
        last_tick = row['tick']
    if len(segments) != len(CASES):
        raise ValueError(f'Liczba scenariuszy z ruchem: {len(segments)}, oczekiwano {len(CASES)}')
    for name, rows in zip(CASES, segments):
        trace = output / (name + '.jsonl')
        trace.write_text('\n'.join(rows) + '\n', encoding='utf-8')
        verify_obstacles(ROOT / 'tests/scenarios' / (name + '.txt'), trace)
        print('OK:', name, flush=True)
    print('Wyniki:', output)
    return 0


if __name__ == '__main__':
    sys.exit(main())
