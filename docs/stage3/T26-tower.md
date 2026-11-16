# T26. Башня — S

[← Этап 3](README.md) · спецификация: [Tower.md](../units/Tower.md)

- **Сделать (только новые файлы):**
  - `src/IO/Commands/SpawnTower.hpp` — `SPAWN_TOWER`;
  - `src/Features/Units/Tower.{hpp,cpp}` — `TowerSpec`, `makeTower`: проверки и сборка по
    спецификации;
  - `src/App/Bindings/TowerBinding.cpp` — `io::SpawnTower` → `TowerSpec`,
    `SW_REGISTER_BINDING` в конце файла.
- **Зависит от:** —.
- **Тесты:** раздел «Приёмка» в [Tower.md](../units/Tower.md), кроме пунктов с пометкой
  (T30): фабрика и действие — `tests/Features/Units`, разбор команды — `tests/IO`,
  привязка — `tests/App`.
- **Сценарий** `tests/scenarios/tower`: Мечник идёт к Башне, Башня стреляет, пока он на
  дистанции 2..5; на дистанции 1 Башня молчит, бьёт Мечник.
- **Изменения спецификации:** в Tower.md — уточнения, найденные при реализации;
  основная SPECIFICATION.md не меняется.
- **Готово:** `git diff main -- src/Core` пуст, существующие файлы `src/` не изменены,
  эталоны прежних сценариев не изменены.
- **Итог (выполнена):** четыре новых файла в `src/` (`SpawnTower.hpp`, `Tower.{hpp,cpp}`,
  `TowerBinding.cpp`), существующие файлы не менялись. Тесты:
  `tests/Features/Units/TowerTests.cpp` (сборка, статы, дистанции 1/2/5/6, сосед не мешает,
  `MovementService` отказывает в шаге) и `tests/App/TowerAppTests.cpp` (спавн через
  приложение; ошибки разбора, статов и `MARCH` с точным текстом stderr). Сценарий
  `tower`: Башня стреляет на тиках 3–6 (дистанции 5..2), на тике 7 Мечник подходит
  вплотную, Башня молчит и на тике 10 разрушена. Уточнений спецификации нет.
