# T27. Лекарь и `UNIT_HEALED` — M

[← Этап 3](README.md) · спецификация: [Healer.md](../units/Healer.md)

- **Сделать (только новые файлы):**
  - `src/IO/Commands/SpawnHealer.hpp` — `SPAWN_HEALER`;
  - `src/IO/Events/UnitHealed.hpp` — DTO `UNIT_HEALED`;
  - `src/App/Output/UnitHealedOutput.cpp` — адаптер `core::events::UnitHealed` →
    `io::UnitHealed` через `narrow`, `SW_REGISTER_OUTPUT`; `CoreEventsOutput.cpp` не
    трогать;
  - `src/Features/Units/Healer.{hpp,cpp}` — `HealerSpec`, `makeHealer`, действие лечения и
    тег `heal`;
  - `src/App/Bindings/HealerBinding.cpp` — `io::SpawnHealer` → `HealerSpec`,
    `SW_REGISTER_BINDING`.
- **Зависит от:** —.
- **Тесты:** раздел «Приёмка» в [Healer.md](../units/Healer.md), кроме пунктов с пометкой
  (T30). Вывод `UNIT_HEALED` — в `tests/App/OutputTests.cpp` или рядом.
- **Сценарий** `tests/scenarios/healer`: Мечник ранит юнита, Лекарь рядом лечит его; в
  каждом тике ровно одна раненая цель в радиусе Лекаря.
- **Изменения спецификации:** уточнения — спец. 2 (убрать оговорку «DTO в IO появится
  вместе с Лекарем»), спец. 10.4 (сверить описание `UnitHealedOutput`); остальное — в
  Healer.md.
- **Готово:** `git diff main -- src/Core` пуст, существующие файлы `src/` не изменены,
  эталоны прежних сценариев не изменены.
- **Итог (выполнена):** шесть новых файлов в `src/`, существующие не менялись. Действие —
  `MinorHeal` (см. Healer.md). Тесты: `tests/Features/Units/HealerTests.cpp` (10 тестов,
  включая остановку двух Лекарей с полным HP на тике 2), `tests/App/HealerAppTests.cpp`
  (спавн, ошибки разбора и статов), в `tests/App/OutputTests.cpp` — вывод `UNIT_HEALED` и
  `narrow`. Прежний тест «событие без адаптера не выводится» был построен на `UnitHealed`;
  теперь у него адаптер, поэтому тест переписан на событие, объявленное в самом тесте, —
  проверяемое правило то же. Сценарий `healer`: Лекарь лечит раненого Мечника на
  тиках 2–8, после его гибели идёт к победителю и долечивает его (на тике 13 — `amount=1`,
  упор в max), затем погибает от него.
- **Изменения спецификации:** уточнения — спец. 2 (оговорка про DTO снята), спец. 10.4
  (пример события без адаптера снят, привязки — «по файлу на юнит»), Healer.md (имя
  `MinorHeal`, отрицательная величина лечит на 0). Требующих согласования нет.
