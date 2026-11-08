#!/usr/bin/env python3
"""Run the pathfinding regression scenarios and check obstacle crossings."""
import json
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
SCENARIOS = ('bez_przeszkod', 'sciana', 'zakret', 'przejscie', 'brak_drogi',
             'nowy_rozkaz', 'dwie_jednostki', 'przeciwnik', 'wolna_jednostka',
             'nowa_przeszkoda', 'oryginalny_teren', 'czcionki', 'ekrany_tekstu',
             'mijanie', 'mijanie_pion', 'mijanie_korytarz', 'mijanie_rozne',
             'mijanie_przeciwnikow', 'trzy_jednostki', 'wspolny_cel',
             'przedmioty_na_ziemi', 'lup_po_smierci', 'lup_na_brzegu', 'lup_przeciwnika',
             'ciasne_drzewa', 'przy_korzeniu', 'uwolnienie_z_drzewa',
             'las_oryginalny', 'las_oryginalny_7', 'znaczniki_przygod', 'dialog_przygody',
             'zbieranie_z_granicy', 'wskazowki_przygod', 'warstwy_i_mapa_zadan', 'paski_zdrowia', 'ziola_i_ekwipunek', 'premie_pociskow')


def verify_obstacles(scenario, trace):
    # An obstacle applies after the tick on which it was added. Scenarios can
    # create new obstacles during a run, so compare against each row's tick.
    tick = 0
    obstacles = []
    starts = {}
    for line in scenario.read_text(encoding='utf-8').splitlines():
        words = line.split('#', 1)[0].split()
        if not words:
            continue
        if words[0] == 'tick':
            tick += int(words[1])
        elif words[0] == 'obstacle':
            obstacles.append((tick, *map(int, words[2:6])))
        elif words[0] in ('unit', 'enemy_unit'):
            starts[int(words[1])] = (int(words[2]), int(words[3]))
    exited = set()
    for line in trace.read_text(encoding='utf-8').splitlines():
        row = json.loads(line)
        for index, (start, x1, y1, x2, y2) in enumerate(obstacles):
            if row['tick'] <= start:
                continue
            key = (row['unit'], index)
            inside = x1 <= row['x'] <= x2 and y1 <= row['y'] <= y2
            initial = starts.get(row['unit'])
            started_inside = start == 0 and initial and x1 <= initial[0] <= x2 and y1 <= initial[1] <= y2
            if not inside:
                exited.add(key)
            elif not started_inside or key in exited:
                raise ValueError(f"jednostka {row['unit']} wewnątrz przeszkody w kroku {row['tick']}")


def main():
    results = ROOT / 'tests/wyniki'
    results.mkdir(parents=True, exist_ok=True)
    output = Path(tempfile.mkdtemp(prefix='zestaw-', dir=results))
    failed = []
    for name in (*SCENARIOS, 'sciana_powtorzenie'):
        source = 'sciana' if name == 'sciana_powtorzenie' else name
        scenario = ROOT / 'tests/scenarios' / (source + '.txt')
        result_dir = output / name
        proc = subprocess.run([sys.executable, str(ROOT / 'tests/uruchom.py'), str(scenario),
                               '--output', str(result_dir), '--timeout', '90'],
                              capture_output=True, text=True)
        try:
            if proc.returncode:
                raise ValueError('scenariusz zwrócił błąd')
            verify_obstacles(scenario, result_dir / 'trace.jsonl')
            if name == 'sciana_powtorzenie':
                if (output / 'sciana/trace.jsonl').read_bytes() != (result_dir / 'trace.jsonl').read_bytes():
                    raise ValueError('powtórzony scenariusz ma inny zapis ruchu')
            print('OK:', name, flush=True)
        except (OSError, ValueError, KeyError) as exc:
            failed.append(name)
            print('BŁĄD:', name, exc, flush=True)
            print(proc.stdout + proc.stderr, flush=True)
    print('Wyniki:', output)
    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main())
