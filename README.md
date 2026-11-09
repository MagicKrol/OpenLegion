# Pobierz grę

- [Pobierz OpenLegion na Windows 11](https://github.com/MagicKrol/OpenLegion/raw/refs/heads/main/builds/OpenLegion-Windows.zip)
- [Pobierz OpenLegion na macOS](https://github.com/MagicKrol/OpenLegion/raw/refs/heads/main/builds/OpenLegion-macOS.zip)
- [Pobierz OpenLegion na Androida (wersja testowa)](https://github.com/MagicKrol/OpenLegion/raw/refs/heads/main/builds/OpenLegion-Android-debug.zip)

## Najnowsze zmiany

- Zmienione: Przedmioty w terenie odkrywają się w pobliżu postaci; większe stosy można przeglądać stronami w ekwipunku.
- Naprawione: Klonowanie przedmiotów między zapisami i legionami oraz związane ze śmiercią postaci.
- Naprawione: Po zwycięstwie lokacje zadań nie generują ponownie skarbów; po odwrocie można wrócić, a stan łupów zostaje zachowany.
- Naprawione: Ekwipunek poległych postaci wypada na ziemię, a oszczepy i kamienie katapulty zużywają się dopiero po oddaniu strzału.
- Dodane: Możliwość wykrywania niebezpiecznych miejsc na bagnach.
- Naprawione: Przedmioty w ekwipunku pola bitwy zachowują swoje miejsca podczas zbierania łupów.
- Zmienione: Każdy kamień do katapulty sprzedaje się za 1 sztukę złota, także przy sprzedaży wszystkich łupów.
- Zmienione: Podczas generowania terenu pojawia się 0–2 ziół w mieście i 1–4 poza miastem. Wcześniej pozostawione przedmioty oraz nagrody i skarby z przygód mogą zmienić tę liczbę.
- Naprawione: Zioła i mikstury nie są zużywane, gdy nie mogą już dać postaci żadnego pozytywnego efektu; przedmiot pozostaje na kursorze.
- Naprawione: Premia ogra do katapulty zwiększa siłę pocisku o 10 zamiast ją obniżać.
- Dodane: Sprzedaż wszystkich przedmiotów pozostawionych na mapie po zakończeniu akcji w mieście lub wiosce, z podglądem wartości łupów.
- Naprawione: Postacie wykonujące grupowy rozkaz ataku szukają drogi do przeciwnika i omijają inne jednostki oraz przeszkody.
- Naprawione: Postać idąca za sklep omija wejście; sklep otwiera się po wybraniu drzwi jako celu.
- Dodane: Kolorowe znaczniki rozkazów nad postaciami: zielony — bez rozkazu, żółty — ruch, czerwony — atak, niebieski — pozostałe czynności, takie jak czary wspierające.
- Dodane: Przechodzenie między postaciami w ekwipunku strzałkami w lewo i w prawo.
- Naprawione: Najechanie myszką na założony przedmiot pokazuje jego nazwę również w sklepie i na ekranie łupów.

## Lista zmian

- Dodane: Wersja gry na Androida ze sterowaniem dotykowym.
- Dodane: Wersja dla Windows 11 uruchamiana z pliku EXE.
- Dodane: Skalowanie obrazu gry na macOS.
- Dodane: Powiększanie gestem na mapie głównej w wersji Android.
- Dodane: Automatyczny zapis przy wejściu do terenu, wyjściu z niego, zakończeniu tury oraz po zamknięciu ekwipunku na mapie głównej.
- Dodane: Osobny slot autozapisu, którego nie można nadpisać ręcznie.
- Dodane: Paski zdrowia nad widocznymi postaciami podczas aktywnej pauzy.
- Dodane: Licznik pozostałych strzał na ikonie amunicji podczas akcji.
- Dodane: Znaczniki `!` nad postaciami z nowym zadaniem oraz `?` nad postaciami mającymi nowe informacje o zadaniu.
- Dodane: Znacznik `?` nad miastem na mapie głównej, gdy można tam zdobyć nowe informacje o zadaniu.
- Dodane: Ikony przedmiotów leżących na ziemi oraz migająca obwódka wyróżniająca ekwipunek.
- Dodane: Ekran zbierania przedmiotów z całej mapy po zakończeniu zwycięskiej akcji.
- Dodane: Dwa niezależne zestawy przedmiotów w rękach, przełączane przyciskami `1 / 2` lub klawiszem `W`.
- Dodane: Możliwość zamiany przedmiotu trzymanego na kursorze z przedmiotem w zajętym slocie.
- Dodane: Informacje o parametrach przedmiotów w ekwipunku i sklepie.
- Dodane: Karta wyposażenia postaci oraz przełączanie zestawów rąk w sklepie.
- Dodane: Okno pozwalające wielokrotnie wydawać doświadczenie bez zamykania go po każdym punkcie.
- Dodane: Kontekstowe rozkazy prawym przyciskiem myszy i gestami dotykowymi.
- Dodane: Menu wyboru celu na mapie, gdy w miejscu dotknięcia znajduje się kilka obiektów.
- Dodane: Przyciski zamykania ekwipunku i sklepu na Androidzie.
- Dodane: Wybór i poprawianie celu ruchu legionu z przyciskami zatwierdzenia lub anulowania.
- Dodane: Ułatwiono zbieranie przedmiotów przez zwiększenie tolerancji podnoszenia z ziemi.
- Dodane: Ziele można zjeść bez zdejmowania hełmu.
- Naprawione: Blokowanie się postaci, które wchodziły sobie w drogę.
- Naprawione: Zakleszczanie się postaci podczas przechodzenia między przeszkodami i drzewami.
- Naprawione: Zużywanie strzał podczas poprawiania celu w aktywnej pauzie — strzała jest odejmowana dopiero po rozpoczęciu lotu.
- Naprawione: Utrata części siły po zjedzeniu ziela przy założonym wyposażeniu.
- Naprawione: Losowe pojawianie się strzał podczas generowania miasta.
- Naprawione: Limit rozwoju cech jest sprawdzany bez uwzględniania premii z wyposażenia.

## Uruchomienie

Rozpakuj pobrane archiwum do katalogu, w którym masz prawo zapisu, i uruchom znajdujący się w nim `Legion.exe` (Windows) albo `Legion.app` (macOS). Zachowaj wszystkie pliki i katalogi razem z programem: gra korzysta z danych w tym samym katalogu. Na macOS zapisuje tam również rozgrywkę; w Windows zapisy znajdują się w `%LOCALAPPDATA%\LegionPC\archiwum`.

Wersja Android jest testowa, podpisana kluczem debug; wymaga urządzenia arm64 z Androidem 7.0 lub nowszym.

Wersja macOS jest przeznaczona dla procesorów Intel; na Macach z Apple Silicon wymaga Rosetty 2. Aplikacja nie jest notaryzowana przez Apple, więc przy pierwszym uruchomieniu może być konieczne dopuszczenie jej w ustawieniach prywatności i ochrony macOS.

## Kod źródłowy

OpenLegion rozwija kod [LegionPC](http://sourceforge.net/projects/legionpc/). Oryginalna dystrybucja LegionPC podaje, że zasoby gry z wersji Amiga opublikowano za zgodą jej autorów. Informacje o licencjach bibliotek dołączono do paczek i ich kodu źródłowego; nie deklarujemy nowej licencji obejmującej cały projekt.

W katalogach `Engine2d` i `game` znajduje się wspólny kod gry, a w `android`, `macos` i `windows` pliki dla poszczególnych platform. Repozytorium nie zawiera osobnej kopii zasobów gry ani skompilowanych bibliotek: przed budową rozpakuj paczkę macOS jako `run-macos/`, tak aby obok `Legion.app` znalazły się `legion`, `fonts`, `Legion.config` oraz `smfk_logo`. Instrukcje wersji Windows znajdują się w [windows/README.md](windows/README.md), a Androida w [android/README.md](android/README.md). Android wymaga lokalnego SDK, NDK, Gradle 8.11.1 i utworzenia własnego klucza debug; wrapper Gradle i lokalne klucze nie są publikowane.

### Kompilacja macOS

Po rozpakowaniu paczki do `run-macos/` zainstaluj narzędzia Xcode oraz biblioteki libpng i libjpeg-turbo dla x86-64 w `/usr/local`. Następnie uruchom `make -f Makefile.macos -j6` w katalogu repozytorium. Aby utworzyć nową paczkę, użyj `python3 macos/package_release.py`; skrypt wymaga także biblioteki FreeType x86-64 w `/usr/local` i jej plików licencyjnych.
