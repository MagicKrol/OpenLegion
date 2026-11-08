# Automatyczne testy ruchu LegionPC

Narzędzie uruchamia wydzielony tryb skompilowanej gry na kopii danych. Pozwala odtworzyć scenę terenową, ustawić jednostki i przeszkody, nadać rozkaz ruchu i zapisać przebieg do późniejszego porównania. Sprawdza także renderowanie i edycję tekstu. Nie wymaga serwera.

## Uruchomienie na macOS

Z katalogu `analiza/zrodla_legionpc`:

```sh
make -f Makefile.macos -j4
python3 tests/uruchom.py tests/scenarios/bez_przeszkod.txt
python3 tests/uruchom.py tests/scenarios/sciana.txt
python3 tests/uruchom.py tests/scenarios/czcionki.txt
python3 tests/uruchom.py tests/scenarios/ekrany_tekstu.txt
python3 tests/uruchom.py tests/scenarios/mijanie_korytarz.txt
python3 tests/uruchom.py tests/scenarios/przedmioty_na_ziemi.txt
python3 tests/uruchom.py tests/scenarios/lup_po_smierci.txt
python3 tests/uruchom.py tests/scenarios/zbieranie_z_granicy.txt
python3 tests/uruchom.py tests/scenarios/ciasne_drzewa.txt
python3 tests/uruchom.py tests/scenarios/las_oryginalny.txt
python3 tests/uruchom.py tests/scenarios/znaczniki_przygod.txt
python3 tests/uruchom.py tests/scenarios/dialog_przygody.txt
python3 tests/uruchom.py tests/scenarios/wskazowki_przygod.txt
python3 tests/uruchom.py tests/scenarios/skalowanie_x2.txt
python3 tests/uruchom.py tests/scenarios/paski_zdrowia.txt
python3 tests/uruchom.py tests/scenarios/strzaly_pauza.txt
python3 tests/uruchom.py tests/scenarios/ziola_i_ekwipunek.txt
python3 tests/uruchom.py tests/scenarios/autozapis.txt
python3 tests/uruchom.py tests/scenarios/wybor_zapisu_macos.txt
python3 tests/uruchom.py tests/scenarios/zamiana_i_znacznik.txt
python3 tests/uruchom.py tests/scenarios/sklep_i_lupy.txt
python3 tests/uruchom.py tests/scenarios/warstwy_i_mapa_zadan.txt
python3 tests/testuj.py  # cały zestaw regresyjny
```

Gra otwiera na chwilę okno; środowisko uruchamiające musi mieć dostęp do interfejsu graficznego macOS. Dane do testu są kopiowane do tymczasowego katalogu. Pliki gracza nie są zapisywane. Do wskazania pustego katalogu wyników można dodać `--output /ścieżka/do/katalogu`, a limit czasu zmienić przez `--timeout 90`. Program zwraca kod 0 po udanym scenariuszu, a 1 w przypadku błędu gry, przekroczonego limitu albo niespełnionego warunku testu.

Konfiguracja w `run-macos/Legion.config` ustawia okno na 1280 × 1040, czyli dwa razy więcej w każdej osi niż podstawowy obraz 640 × 520. `FitToDisplay = True` zmniejsza okno z zachowaniem proporcji, jeśli nie mieści się w obszarze ekranu dostępnym po uwzględnieniu paska i Docka. Ustawienie `False` wymusza rozmiar z sekcji `Window`. W testach parametr `--exact-window` wyłącza dopasowanie tylko w tymczasowej kopii konfiguracji.

Każde uruchomienie pozostawia osobny katalog w `tests/wyniki/`:

- `scenariusz.txt` — faktycznie uruchomiona kopia poleceń;
- `trace.jsonl` — stan jednostek po każdym kroku, w tym pozycja, cel i tryb ruchu;
- `trasa.svg` — schemat całej mapy, narysowanych przeszkód i zapisanych tras (kropka: start, pierścień: koniec, krzyż: cel);
- `poczatek.png`, `koniec.png` — przykładowe klatki gry, jeśli scenariusz je zapisuje;
- `gra.log` i `status.json` — log i wynik, w tym sumy SHA-256 scenariusza i programu.

## Format scenariusza

Jedno polecenie na wiersz, `#` oznacza komentarz. Najpierw ustaw ziarno i scenę; można przełączyć scenę poleceniem `reload_scene`.

```text
seed 42
scene 2 clear
unit 1 80 200 0 40
obstacle 60 250 110 300 300
ui_move 1 480 200
camera 250 200
snapshot start.png
tick 350
report 1
snapshot koniec.png
expect_reached 1
```

| Polecenie | Znaczenie |
| --- | --- |
| `seed liczba` | Ustawia ziarno generatora losowego. |
| `projectile_hit id wróg rasa lewa prawa siła` | Sprawdza przygotowanie i rzeczywiste trafienie pocisku, oczekiwaną siłę, zakres obrażeń oraz zużycie amunicji. Scenariusz `premie_pociskow.txt` obejmuje katapulty i łuki w obu rękach. |
| `expect_display szerokość wysokość` | Sprawdza rozmiar okna i obszaru renderowania. |
| `expect_scaled_display` | Sprawdza powiększony obszar renderowania i jego proporcje. |
| `health id aktualne maksymalne` | Ustawia zdrowie jednostki w scenariuszu. |
| `stat_setup id siła szybkość życie` | Ustawia statystyki testowej postaci i czyści jej ekwipunek. |
| `wear id slot przedmiot`, `unwear id slot`, `consume id przedmiot` | Zakłada i zdejmuje przedmioty oraz używa ziół przez rzeczywistą funkcję przeliczania. Sloty 0–4 to głowa, korpus, nogi i ręce. |
| `inventory_consume id przedmiot` | Używa przedmiotu przez rzeczywiste okno ekwipunku. |
| `expect_stats id siła pełna_siła szybkość szybkość_bez_kary` | Sprawdza statystyki, w tym premię ponad widocznym limitem siły. |
| `save_reload` | Zapisuje i odczytuje testowy stan w tymczasowej kopii danych. |
| `pause_bars 0\|1` | Wyłącza lub włącza rysowanie pasków zdrowia w testowej scenie. |
| `expect_health_bar id 0\|1` | Porównuje render sceny z paskami i bez nich w pobliżu jednostki. |
| `expect_health_fill id piksele` | Sprawdza długość kolorowej części paska w skali gry. |
| `action_pause_cycle id` | Uruchamia akcję, zatrzymuje ją i sprawdza stan pasków w obu trybach. |
| `scene typ clear` | Ładuje teren i usuwa jego pierwotne strefy kolizji. Grafika terenu, w tym drzewa, pozostaje widoczna, ale w tym trybie nie oznacza przeszkody. |
| `scene typ village` | Ładuje teren z testową studnią. |
| `reload_scene typ tryb` | Zmienia teren w tym samym uruchomieniu i resetuje stan testowej sceny. |
| `world_map` | Przechodzi na mapę świata w ramach testu. |
| `map_city id x y` | Ustawia pozycję miasta do testu mapy. |
| `expect_world_quest id 0\|1` | Sprawdza, czy miasto ma aktywną wskazówkę zadania. |
| `expect_world_marker_rendered id` | Sprawdza, czy znak miasta jest faktycznie widoczny. |
| `expect_ground_hidden sektor slot` | Sprawdza, czy ikona przedmiotu jest zasłonięta. |
| `scene typ original` | Zachowuje oryginalne strefy kolizji generowane przez grę. Na mapie SVG pokazane są tylko przeszkody jawnie dodane poleceniem `obstacle`, nie oryginalne. |
| `unit id x y rasa szybkość` | Tworzy jednostkę gracza. ID 1–10; rasa 0–9; szybkość 1–100. |
| `enemy_unit id x y rasa szybkość` | Tworzy jednostkę przeciwnika. Numery jednostek w scenariuszu muszą być różne. |
| `obstacle id x1 y1 x2 y2` | Dodaje prostokątną strefę kolizji; ID 60–119. W klatkach oznaczona na czerwono. |
| `move id x y` | Nadaje cel ruchu bezpośrednio w danych jednostki. |
| `ui_move id x y` | Wybiera cel za pomocą syntetycznego kliknięcia w prawdziwej funkcji `RUCH`; sprawdza, czy gra przyjęła wskazany cel. Dotyczy jednostek gracza. |
| `tick liczba` | Wykonuje do 10 000 kolejnych kroków logiki ruchu, zapisując stan każdej jednostki. |
| `camera x y` | Ustawia kamerę wokół punktu na mapie. |
| `snapshot nazwa.png` | Zapisuje aktualny obraz okna gry. |
| `report id` | Wypisuje stan jednostki do logu. |
| `zone x y` | Wypisuje identyfikator strefy kolizji w danym punkcie mapy. |
| `expect_free x y` | Sprawdza, czy punkt nie znajduje się w zablokowanej strefie terenu. |
| `click x y` | Wysyła kliknięcie lewym przyciskiem jako zdarzenia SDL we współrzędnych okna i sprawdza ich odbiór przez obsługę wejścia. |
| `key kod` | Wysyła naciśnięcie i zwolnienie klawisza SDL, np. `97` to `a`, oraz sprawdza odbiór. |
| `font_sample` i `expect_fonts` | Rysują próbkę dwóch wielkości czcionek i sprawdzają widoczność. |
| `expect_input_edit` | Sprawdza edycję nazwy zawierającej znaki UTF-8, w tym wpisanie polskiej litery z Alt. |
| `army_sample`, `name_sample` | Rysują odpowiednio panel legionu i pytanie o imię za pomocą rzeczywistych funkcji gry. |
| `expect_reached id` | Wymaga zakończenia ruchu w odległości do 4 jednostek od celu w każdej osi. |
| `expect_stuck id kroki` | Wymaga, żeby jednostka nie zmieniła pozycji przez podaną liczbę kroków i nadal miała aktywny ruch. |
| `expect_idle_near id promień` | Sprawdza zatrzymanie jednostki blisko celu, np. gdy zajmuje go sojusznik. |
| `ground sektor slot id` | Wstawia lub usuwa (`id` 0) przedmiot z danego miejsca na ziemi. |
| `equip id slot przedmiot` i `kill id` | Ustawiają wyposażenie i wywołują rzeczywistą procedurę śmierci. |
| `expect_ground sektor slot id` | Sprawdza zawartość slotu ziemi. |
| `expect_ground_spread sektor` | Sprawdza, czy widoczne przedmioty są oddzielone i pozostają w sektorze podnoszenia. |
| `expect_ground_rendered sektor slot` | Porównuje obraz z ikoną i bez niej w okolicy przedmiotu. |
| `expect_nearby x y sektor slot 0\|1` | Sprawdza, czy slot ziemi znajduje się wśród czterech pozycji dostępnych z danego miejsca. |
| `inventory_pick id sektor slot przedmiot` | W rzeczywistym oknie ekwipunku przenosi pierwszy widoczny przedmiot z ziemi do plecaka i sprawdza wynik. |
| `inventory_drop id sektor slot przedmiot` | W oknie ekwipunku przenosi przedmiot z pierwszego slotu plecaka do drugiego slotu ziemi i sprawdza wynik. |
| `city id` | Rozpoczyna wizytę testową w mieście i losuje oferty mieszkańców. |
| `quest_offer id 0\|1` | Ustawia dla mieszkańca brak lub dostępność nowego zadania. |
| `quest_state slot typ poziom miasto osoba ostatnie_miasto` | Ustawia stan zadania do sprawdzenia informatorów. |
| `expect_quest_markers id oferta informacje` | Sprawdza, czy mieszkaniec ma nową ofertę i czy zna nowe informacje (wartości 0 lub 1), niezależnie od pierwszeństwa znaku. |
| `dialog_offer gracz mieszkaniec` | Klika „Nowe zadanie” w rzeczywistym oknie rozmowy, sprawdza utworzenie zadania i zapisuje obraz odświeżonego okna. |
| `expect_quest_marker mieszkaniec !\|?\|-` | Sprawdza, jaki pojedynczy znak jest widoczny nad mieszkańcem. |
| `dialog_quest gracz mieszkaniec slot postęp` | Pyta o zadanie w oknie rozmowy i sprawdza, czy jego etap zmienił się o 0 albo 1. |
| `expect_answer_contains tekst` | Sprawdza treść ostatniej odpowiedzi dotyczącej zadania. |
| `load_save slot` | Wczytuje zapis z tymczasowej kopii danych, bez zmiany oryginalnego pliku. |

Oferta nowego zadania jest losowana raz dla każdego mieszkańca przy wejściu do miasta i pozostaje widoczna przez tę wizytę, dopóki nie zostanie wykorzystana lub nie zniknie możliwość jej przyjęcia. Po ponownym wejściu oferty losowane są ponownie.

Mapa ma wymiary 640 × 512, współrzędne rosną w prawo i w dół. Przykład `sciana.txt` sprawdza, czy jednostka omija ścianę i dociera do celu. `brak_drogi.txt` sprawdza całkowitą blokadę. Zestaw `tests/testuj.py` obejmuje też zakręt, przejście, zmianę celu, nową przeszkodę, jednostki gracza i przeciwnika, oryginalny teren i powtarzalność wyników.

## Zakres i ograniczenia

Test korzysta z oryginalnych funkcji tworzenia postaci, generowania terenu, stref kolizji i `A_RUCH`, tej samej funkcji poruszania, której używa właściwa akcja gry. `move` ustawia rozkaz bezpośrednio w danych jednostki, natomiast `ui_move` uruchamia prawdziwą funkcję wyboru celu i podaje kliknięcie przez SDL. Kolejne kroki nie uruchamiają całej pętli bitwy: nie symulują AI, walki, czasu rzeczywistego ani reszty interfejsu. `click` i `key` testują wprowadzenie zdarzeń do systemu wejścia. `ui_move` sprawdza wybór celu, ale nie przechodzi przez wybór przycisku ruchu w głównej pętli gry. Są to więc testy logiki ruchu, a nie dowód poprawności całej rozgrywki. Narzędzie tworzy scenę terenową; nie ładuje konkretnego zapisu gry ani całej misji.
