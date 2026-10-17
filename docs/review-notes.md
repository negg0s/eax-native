# Ревью кода eax-native (docs/review-notes.md)

Базовая точка: коммит `main` на момент ревью (`84f6b9c` и позже, ветка
`cleanup/eax-native`). Ревью статическое, на Windows-хосту без Linux-окружения
(по правилам сессии WSL/Docker не используются): сборка и ctest помечены как
"не проверено в этой сессии". Ранее (см. историю коммитов `fix R-001..R-031`)
сборка и ctest 3/3 прошли проверку в WSL Arch-окружении.

Серьёзность: high (падение/порча данных/RT-нарушение), medium (некорректная
поведение в редких сценариях), low (качество/удобство).

## Что уже исправлено до этой сессии (с disprov коммитами)

| ID | Серьёзность | Где | Суть | Коммит |
|---|---|---|---|---|
| R-025 | high | `eax-native/src/abi_dispatch.cpp` | некорректная семантика prop_id (COMMIT/ALLPARAMETERS/индивидуальные поля, deferred бит 31) | `18623e6` |
| R-026 | high | `eax-native/src/arena.cpp`, `voice_pool.cpp` | двойное освобождение арены; race-free freelist в MemoryArena; корректные деструкторы heap-fallback | `dfe4f22`, `1647c13`, `30ba6cc` |
| R-027 | medium | `eax-native/src/abi_dispatch.cpp` | `eax_buffer_lock` заворачивал out-of-bounds; теперь EAX_ERR_INVALID_PARAM | `dfe4f22` |
| R-028 | high | `eax-native/src/context.cpp` | fallback reverb не перенастраивался при смене EAX listener/sample rate; RT-аллокации в ReverbDSP | `c7c2329` |
| R-029 | low | `tools/eaxctl/main.cpp` | demo-пресет применялся молча с prop_id=0; парсинг `--duration` без try/catch | `c7c2329` |
| R-031 | high | `tools/eaxctl/main.cpp` | install клал 64-битную DLL и в system32, и в syswow64; теперь читается PE-заголовок (machine) | `4d4c89b` |

## Текущее состояние по файлам (после исправлений)

- `eax-native/src/abi_dispatch.cpp`: RT-safe; аллокация ареной в hot path
  отсутствует; prop_id декодирование с тестом. Осталось на low: полноценная
  таблица 26 пресетов EAX2 ENVIRONMENT (11) имеет только GENERIC/CAVE.
- `eax-native/src/eax_reverb.cpp/h`: пред-резерв буферов в конструкторе;
 .overflow: RT-аллокаций нет. Канон EAX2 id/units сверялся с
  `algorithm/eax.h` (doom3) и nxpascal `openal.pas`.
- `eax-native/src/voice_pool.cpp`: pre-allocated 64 голоса, atomic in-use,
  fallback heap только не на RT-пути.
- `wine-integration/dsound_shim.cpp`: COM ref counting в порядке
  (AddRef/Release/QueryInterface симметричны); IKsPropertySet форвардится.
- `pipewire-integration/`: фильтр не аллоцирует памяти на RT-пути; node
  construction вне RT.

## Осталось сделать (не блокирует работу)

- [low] Полная таблица EAX2 reverb пресетов (26 шт) вместо 2.
- [low] EAX 3.0/4.0 property sets: GUIDs не рекламируются (честно), подводка
  задокументирована в README.
- [low] `docs/`: отметить, что бенчмарк задержки - модельная картинка без
  реального PipeWire daemon в WSL.

## Проверка

Не проверено в этой сессии: `cmake -B build && cmake --build build && ctest`.
Автору на Linux: команды выше + `EAX_ENABLE_ASAN=1 ctest` для санитайзерного
прогона.
