# Лекарь (Healer)

Статус: согласовано, не реализовано (задача [T27](../stage3/T27-healer.md)).

README: HP, Spirit; «Малое исцеление» — восстанавливает **Spirit** единиц здоровья одному
случайному юниту в радиусе 2 клеток; если некого лечить, перемещается.

## Команда

`SPAWN_HEALER unitId x y hp spirit` — `io::SpawnHealer` (согласовано, С1).

## Проверки

| Поле | Условие | Код |
|---|---|---|
| `hp` | ≤ 0 | `unit.invalid_stat` |
| `spirit` | ≤ 0 | `unit.invalid_stat` |

Обе ошибки выводятся вместе (согласовано, С4). Проверки спавна — спец. 7.

## Сборка

`HealerSpec{id, position, hp, spirit}` → `makeHealer`:

- компоненты: `Health(hp)`, `Spirit(spirit)`, `Movement(1)`;
- `occupiesCell = true`;
- behaviour = [действие лечения, `MarchStep`].

Действие лечения в архитектуре названо `Heal(stat<Spirit>)`; имя класса не должно
путаться с конструктором `heal(...)` из `Features/Common/TargetSpecs`. Пока им пользуется
только Лекарь, действие лежит рядом с ним (`Features/Units/Healer.cpp`). Тег `heal`
объявляется там же (архит. 4.9).

## Поведение

```
цели = targeting().select(self, heal({heal}, {0,2}), фильтр: HP цели < max)
нет целей                → false (дальше — MarchStep)
иначе случайная цель     → effects().apply(Hit{spec, self, цель, Spirit}), true
```

- Себя Лекарь не лечит: `Targeting` никогда не выбирает источник (спец. 0.1).
- Юниты с полным HP целями не становятся. Два Лекаря с полным HP не находят целей — тик
  пустой, симуляция завершается `NothingHappened` (архит. 4.6).
- Дистанция 0 возможна: на клетке Лекаря может стоять юнит без `occupiesCell` (Ворон).
- Лечит любого юнита с `Health`, в том числе Башню и Ворона: правило `Flying` проверяет
  только `Intent::Hostile` (архит. 8). Мину не лечит — у неё нет `Health`.
- HP не поднимается выше `max` (HP при спавне); в событии — фактически восстановленное HP
  `min(Spirit, max − HP)` (спец. 5.3).
- Нет целей и нет марша — Лекарь бездействует.

## Вывод

- `UNIT_SPAWNED … unitType=Healer …` (согласовано, С2).
- `UNIT_HEALED healerUnitId=<id> targetUnitId=<id> amount=<n> targetHp=<hp>` (согласовано,
  С3): DTO `IO/Events/UnitHealed.hpp`, адаптер `App/Output/UnitHealedOutput.cpp` с
  `SW_REGISTER_OUTPUT`; `CoreEventsOutput.cpp` не меняется (спец. 10.4). Событие Core
  `UnitHealed` уже есть (спец. 2). Значения сужаются через `narrow` (спец. 10.5).

## Приёмка

- `hp = 0, spirit = 0` — две ошибки `unit.invalid_stat` одним списком;
- лечит раненого на дистанции 1 и 2, не лечит на 3;
- не лечит юнитов с полным HP: `false`, `world.activity().changes()` не изменился;
- себя не лечит, даже раненый;
- лечит раненого юнита без `occupiesCell` на своей клетке (дистанция 0);
- HP не выше `max`; при `Spirit` больше недостающего — `amount` = недостающее;
- без целей идёт по маршу; без целей и марша — бездействует;
- два Лекаря с полным HP — остановка `NothingHappened` на тике 2;
- вывод `UNIT_HEALED` в формате выше; значение вне `uint32_t` — `logic_error`, код 3;
- (T30) лечит Ворона.
