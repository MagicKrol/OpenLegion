# Legion: kompilacja Windows 11 x86-64

Wersja wykorzystuje te same źródła gry co macOS i Android. Wynikiem jest `dist-windows/Legion-Windows.zip`; po rozpakowaniu należy uruchomić `Legion.exe`. Biblioteki i zasoby muszą pozostać w tym samym folderze. Własne zapisy pozostają w `%LOCALAPPDATA%\LegionPC\archiwum` i nie są dołączane do ZIP.

## Budowa na tym Macu

W katalogu źródeł:

```sh
make -f Makefile.windows -j6
make -f Makefile.windows package
```

Kompilator i zależności są lokalne w `windows/toolchain` i nie są częścią paczki gry. Jeżeli trzeba odtworzyć je na innym komputerze macOS:

```sh
mkdir -p windows/toolchain
curl -fL -o windows/toolchain/llvm-mingw-20260616-ucrt-macos-universal.tar.xz https://github.com/mstorsjo/llvm-mingw/releases/download/20260616/llvm-mingw-20260616-ucrt-macos-universal.tar.xz
shasum -a 256 windows/toolchain/llvm-mingw-20260616-ucrt-macos-universal.tar.xz
# Oczekiwana suma: 2cab02a2e964bd4aae981150a45985d07c657cfa8d244959eb9e2dcc5eedd7b1
tar -xJf windows/toolchain/llvm-mingw-20260616-ucrt-macos-universal.tar.xz -C windows/toolchain
python3 windows/fetch_deps.py
make -f Makefile.windows -j6 package
```

Skrypt pobiera oficjalne pakiety MSYS2 UCRT zgodne z `dependencies.lock.json`, weryfikuje SHA-256 i wymaga programu `zstd`. Skrypt pakujący zbiera przechodnie zależności DLL, w tym ładowane dynamicznie biblioteki muzyki, oraz ich licencje. Plik EXE zawiera manifest kodowania UTF-8 wymagany dla międzynarodowych ścieżek w Windows 11.

## Sprawdzenie

Na tym Macu `python3 windows/test_wine.py` rozpakowuje gotowy ZIP do katalogu tymczasowego i przechodzi test w jednej sesji Wine, w tym dwa autozapisy, ścieżkę Unicode dla zapisu i muzyki oraz wybór zapisów z menu. Wyniki i obrazy zapisuje w `tests/wyniki`.

Sprawdź na Windows 11 uruchomienie rozpakowanej gry, kliknięcia menu, dźwięk, zapis/odczyt oraz ponowny autozapis. Uruchomienie przez Wine na macOS jest dodatkowym testem zgodności, a nie zastępuje testu na Windows.
