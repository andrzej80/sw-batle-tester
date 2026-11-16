# T29. Мина и `MINE_EXPLODED` — M

[← Этап 3](README.md) · спецификация: [Mine.md](../units/Mine.md)

- **Сделать (только новые файлы):**
  - `src/IO/Commands/SpawnMine.hpp` — `SPAWN_MINE`;
  - `src/Features/Units/Mine.{hpp,cpp}` — `MineSpec`, `makeMine`, действие `MineFuse`,
    событие фичи `MineExploded`, тег `explosion`;
  - `src/IO/Events/MineExploded.hpp` — DTO `MINE_EXPLODED`;
  - `src/App/Output/MineExplodedOutput.cpp` — адаптер `features::MineExploded` →
    `io::MineExploded`, `SW_REGISTER_OUTPUT`;
  - `src/App/Bindings/MineBinding.cpp` — `io::SpawnMine` → `MineSpec`,
    `SW_REGISTER_BINDING`.
- **Зависит от:** —.
- **Внимание:** задача взрыва захватывает только `UnitId` мины (контракт `Scheduler`,
  архит. 4.13); взведённая Мина возвращает `false` и мир не меняет (контракт `IAction`).
  Это первый в проекте потребитель `Scheduler`, `requestRemoval` и юнита без `Health` —
  если что-то из них не подходит, стоп и согласование (правила этапа).
- **Тесты:** раздел «Приёмка» в [Mine.md](../units/Mine.md), кроме пунктов с пометкой
  (T30). Порядок событий в тике взрыва — отдельным тестом на `EventBus`.
- **Сценарий** `tests/scenarios/mine`: Мечник идёт по маршу мимо Мины, Мина взводится и
  взрывается; у Мечника достаточно HP, чтобы дойти до цели.
- **Изменения спецификации:** в Mine.md — уточнения, найденные при реализации;
  основная SPECIFICATION.md не меняется.
- **Готово:** `git diff main -- src/Core` пуст, существующие файлы `src/` не изменены,
  эталоны прежних сценариев не изменены.
