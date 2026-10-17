# Cleanup progress: eax-native
Base commit: 84f6b9c (main)   Branch: cleanup/eax-native   Started: 2026-10-18
Env: OS=Windows 11 Pro build 26200, host; Linux build pieces verified earlier via WSL (см. review-notes), в этой сессии WSL не используется по правилам.
Disk before: C free=180.5 GB, D free=1180.29 GB

## Tasks
| ID | Title | Status | Commit | Verification |
|---|---|---|---|---|
| EX-01 | Build+ctest по README | not-reproduced (Windows-хост, WSL запрещён правилами) | - | ранее пройдено: ctest 3/3 (см. review-notes.md) |
| EX-02 | Роот-сборка без MinGW | done | см. git log | option EAX_BUILD_WINE_SHIM, EAX_BUILD_PIPEWIRE (по умолчанию OFF на WIN32) |
| EX-03 | Warnings/sanitizers/build type/install | done | см. git log | опции EAX_WERROR(=off), EAX_ENABLE_ASAN/UBSAN, RelWithDebInfo, install rules |
| EX-10 | README title -> eax-native | done | см. git log | read |
| EX-11 | prebuilt документация+checksums | done | см. git log | docs/prebuilt.md |
| EX-12 | LICENSE | needs-author | - | отчёт: LGPL-2.1 (OpenAL Soft), динамическая линковка |
| EX-13 | AUR PKGBUILD | needs-author | - | есть только aur/.gitkeep, PKGBUILD нет |
| EX-14 | README: статус/ограничения/проверка/кредиты | done | см. git log | read |
| EX-15 | CI ubuntu | done (конфиг) | см. git log | .github/workflows/ci.yml, зелёность не проверена (нет выката) |
| D3 | docs/review-notes.md | done | см. git log | создан, категории+серьёзность |

## Decisions (defaults chosen) and how to roll back
- EAX_BUILD_WINE_SHIM default ON (сохранить прошлое поведение Linux), выключается -DEAX_BUILD_WINE_SHIM=off. На Windows с не-MinGW компилятором shim несовместим - README предупреждает.
- EAX_BUILD_PIPEWIRE default OFF на WIN32, ON иначе (старое поведение на Linux).
- EAX_WERROR default OFF (по工业 промпту без -Werror); вернуть -Werror можно -DEAX_WERROR=on.

## Not verified (and how the author can verify)
- Полная сборка+ctest на этой сессии не гонялись: на Linux: `cmake -B build -S . && cmake --build build && ctest --test-dir build --output-on-failure`.
- Санитайзеры: `-DEAX_ENABLE_ASAN=on -DEAX_ENABLE_UBSAN=on` на Linux/clang.

## needs-author
- LICENSE выбрать и добавить (OpenAL Soft LGPL-2.1 + учесть Wine LGPL при копировании фрагментов shim).
- AUR PKGBUILD не существует: если планируется публикация, создать и проверить makepkg/namcap.
- Пуш ветки cleanup/eax-native и PR (автоагент по правилам не пушит).

## Disk after: C free=..., D free=...; deleted: <...>; left: <...>
