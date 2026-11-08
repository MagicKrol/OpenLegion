# Legion — prototyp Android

To jest eksperymentalny port natywnego kodu gry C++ na Androida. Wersja ma osobny projekt Gradle, a źródła logiki współdzieli z wersją macOS. Wynik to testowy, podpisany kluczem debug APK dla `arm64-v8a` i Androida 7.0 lub nowszego. Na tym etapie nie traktuj go jako wydania produkcyjnego.

## Budowa

Potrzebne są JDK 17 lub nowsze, Android SDK z platformą 36, NDK 27.2.12479018, CMake 3.22.1 oraz lokalnie zainstalowany Gradle 8.11.1. Gradle wrapper nie jest publikowany. Przed budową wypakuj paczkę macOS do `run-macos/` w katalogu głównym repozytorium, aby udostępnić zasoby gry.

```sh
python3 android/prepare_assets.py
cd android
gradle :app:assembleDebug
```

APK powstaje w `app/build/outputs/apk/debug/app-debug.apk`. Zasoby kopiowane są z `run-macos`; skrypt celowo nie pakuje katalogu zapisów gracza. Klucz podpisu debug jest lokalny i pozostaje w `.toolchain/legion-debug.keystore`. Używaj tego samego klucza przy kolejnych kompilacjach debug, aby instalować je jako aktualizację.

Przy przenoszeniu projektu na inny komputer ustaw w `local.properties` właściwe `sdk.dir`. Jeśli NDK nie znajduje się w `.toolchain`, zainstaluj go w Android SDK albo ustaw `ndk.dir`. Potrzebna jest wersja NDK 27.2.12479018. Jeśli lokalny klucz debug nie istnieje, utwórz go przed pierwszą kompilacją poleceniem:

```sh
mkdir -p .toolchain
keytool -genkeypair -v -keystore .toolchain/legion-debug.keystore -storepass android -alias androiddebugkey -keypass android -keyalg RSA -keysize 2048 -validity 10000 -dname 'CN=Android Debug,O=Android,C=US'
```

`vendor/` zawiera SDL 2, sdl12-compat, GL4ES, SDL_image 1.2, SDL_mixer 1.2, SDL_ttf, FreeType, libpng, libjpeg-turbo i MikMod wraz z ich licencjami. Biblioteki są potrzebne do zgodności ze starszym silnikiem i oryginalnymi zasobami gry.

## Pakiet do testów wewnętrznych w Google Play

Z katalogu `android/` zbuduj pakiet AAB i podpisz go kluczem przesyłania Legionu. Przy kolejnych wydaniach zwiększ `versionCode` w `app/build.gradle` i użyj tego samego klucza.

```sh
python3 prepare_assets.py
gradle --no-daemon :app:bundleRelease
jarsigner -keystore .toolchain/play-upload/legion-upload.p12 \
  -storepass:file .toolchain/play-upload/password \
  -signedjar app/build/outputs/bundle/release/Legion-play-upload.aab \
  app/build/outputs/bundle/release/app-release.aab legion-upload
```

Klucz i plik z hasłem znajdują się w lokalnym, ignorowanym katalogu `.toolchain/play-upload/`. Przechowuj ich kopię w bezpiecznym miejscu. Do Play Console prześlij wyłącznie podpisany plik `Legion-play-upload.aab`.

## Sterowanie prototypu

- Krótkie dotknięcie odpowiada lewemu kliknięciu.
- Przytrzymanie odpowiada prawemu przyciskowi myszy.
- Na mapie głównej rozsuwanie i zsuwanie dwóch palców zmienia powiększenie od 1× do 4×. Pojedynczym palcem można przesuwać mapę.
- W mieście i na polu bitwy gest powiększania jest wyłączony; przeciągnięcie jednym palcem przesuwa widok.
- Wpisywanie nazw korzysta z systemowego okna tekstowego.
- Aplikacja zachowuje proporcje gry, a kliknięcia są przeliczane dla aktualnego powiększenia i bocznych marginesów.

## Weryfikacja

```sh
clang++ -std=c++11 -Wall -Wextra -Werror tests/zoom_math_test.cpp -o /tmp/legion-zoom-math-test
/tmp/legion-zoom-math-test
python3 tests/inspect_apk.py
```

Test sprawdza obliczanie widoku, przycinania i przeliczania dotyku, ale nie zastępuje uruchomienia na telefonie lub emulatorze. APK należy sprawdzić na urządzeniu w zakresie uruchamiania, dźwięku, zapisu, sterowania i gestów.

### Sceny testowe na urządzeniu

W kompilacji debug można otworzyć niezapisywaną scenę miasta, bez wczytywania rozgrywki:

```sh
adb shell am start -n org.legion.game/.LegionActivity --ez legion_test_city true
```

Dodanie `--ez legion_test_dialog true` otwiera od razu rozmowę o zadaniu z imieniem „Baldur”. Zwykłe uruchomienie nie włącza tych scen i nie zmienia zapisów.

Sklep lub spichlerz można otworzyć bezpośrednio w osobnej, niezapisywanej scenie testowej:

```sh
adb shell am start -n org.legion.game/.LegionActivity --ez legion_test_shop true
adb shell am start -n org.legion.game/.LegionActivity --ez legion_test_shop true --ez legion_test_granary true
```

Ekwipunek z punktami do rozdysponowania otwiera `--ez legion_test_stats true`.
