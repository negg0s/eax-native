# eax-native

Стек для Linux, который возвращает в играх под Proton/Wine аппаратно-подобный DirectSound3D и EAX звук через OpenAL Soft и PipeWire.

## Как это работает

1. Грузим Windows PE `dsound.dll` shim в префикс Proton (копия в `system32`/`syswow64`, override `dsound=native,builtin`).
2. Shim передаёт DirectSound3D и EAX 1.0/2.0 property sets в нативную библиотеку `libeax-native.so`.
3. Библиотека транслирует координаты (left-handed DS3D в right-handed OpenAL), аттенюацию, панинги, EAX реверберацию (EFX `AL_EFFECT_EAXREVERB`, обстакл/окклюзия через low-pass фильтры) в OpenAL Soft.
4. PipeWire-фильтр держит низкую задержку цепочки обработки.

## Стек

C++20, CMake, OpenAL Soft (EFX, SOFT loopback, SOFT source latency), PipeWire (SPA DSP node), MinGW (кросс-компиляция PE shim), Catch2 (тесты), GDB/objdump (верификация).

## Настройка окружения

```
sudo pacman -S --needed base-devel cmake clang gcc openal catch2 mingw-w64-gcc
```

Требуется pipewire и pipewire-pulse в системе пользователя.

## Сборка и запуск

Сборка нативной библиотеки и тестов:

```
cmake -B build -S . -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Release -j$(nproc)
cmake --build build -j$(nproc)
ctest --test-dir build --output-on-failure
```

Тесты: `eax_unit_tests`, `wine_dsound_compat_test`, `pipewire_filter_test`.

Кросс-компиляция Windows PE `dsound.dll` (32 и 64 бит):

```
./ci/build_windows_pe.sh
```

Замер задержки (PipeWire quantum, буферы, полный путь):

```
./ci/run_latency_bench.sh
```

Демо-сцена (реверберация CAVE, headless):

```
LD_LIBRARY_PATH=build/eax-native ./build/tools/eaxctl/eaxctl demo --headless --duration 5
```

Установка в префикс Proton игры (копирует PE shim нужной архитектуры в system32/syswow64 и прописывает DLL override):

```
LD_LIBRARY_PATH=build/eax-native ./build/tools/eaxctl/eaxctl install --appid <steam_appid>
```

## Структура

- `eax-native/`: нативная библиотека, C ABI, спатиализатор, EAX реверб, тесты
- `wine-integration/`: PE `dsound.dll` shim, COM заголовки
- `pipewire-integration/`: фильтр задержки для PipeWire
- `tools/`: `eaxctl` (установка и демо), бенчмарк задержки
- `docs/`: архитектура, результаты фаз, роадмап
- `prebuilt/`: готовые PE shim'ы (32/64 бит), см. `docs/prebuilt.md`

## Статус и ограничения

- EAX 1.0 и 2.0 property sets обработаны; EAX 3.0/4.0 не рекламируются и не включены
- Таблица EAX2 reverb пресетов: пока только GENERIC и CAVE, остальные env ставят номер без тела пресета
- Проверено: сборка, ctest, headless demo, автосборка PE shim'ов из MinGW (см. `docs/review-notes.md`)
- Не проверено на реальных играх: процесс использования в конкретных играх автора
  не документирован: реальные замеры - фаза 4
- Windows-only хост не поддерживается: это Linux-стек

## Как проверить, что shim работает

После `eaxctl install` и запуска игры:

```
LD_LIBRARY_PATH=build/eax-native ./build/tools/eaxctl/eaxctl status
```

`status` должен показать обе архитектуры как `Installed` и DLL override.
Если игра не видит shim - напрямую найти dsound.dll в `system32`/`syswow64`
префикса можно и вручную.

## Известные проблемы

См. `docs/review-notes.md`: там же история исправлений (R-001..R-031).

## Благодарности

- OpenAL Soft (LGPL-2.1): основной рендер и EFX
- Wine: базовая идея DLL override и COM интеграция
- PipeWire: низкий латентный аудиограф

