# Башня (Tower)

Статус: согласовано, не реализовано (задача [T26](../stage3/T26-tower.md)).

README: HP, Power; «Прицельный выстрел» — стреляет в случайного юнита на расстоянии 2..5,
нанося **Power** урона; не может перемещаться.

## Команда

`SPAWN_TOWER unitId x y hp power` — `io::SpawnTower` (согласовано, С1).

## Проверки

| Поле | Условие | Код |
|---|---|---|
| `hp` | ≤ 0 | `unit.invalid_stat` |
| `power` | ≤ 0 | `unit.invalid_stat` |

Обе ошибки выводятся вместе (согласовано, С4). Проверки спавна — спец. 7.

## Сборка

`TowerSpec{id, position, hp, power}` → `makeTower`:

- компоненты: `Health(hp)`, `Power(power)`; **без** `Movement`;
- `occupiesCell = true`;
- behaviour = [`RangedAttack(stat<Power>, distance(2, 5))`].

## Поведение

- Выстрел: цели — `attack({ranged, physical}, {2,5})`, случайная, урон `Power`.
- Условия «нет соседей» нет (в README его нет, в отличие от Охотника): сосед не мешает
  выстрелу, но и не становится целью (дистанция 1).
- Нет целей — действие не выполнено, Башня бездействует.
- Без `Movement` Башня статична: `MARCH` на неё — `march.unit_cannot_move` (спец. 7),
  `MovementService` отказывает в любом шаге.
- По Ворону дистанция `{2,5}` превращается в `{1,4}` (правило `Flying`, [Raven.md](Raven.md)).

## Вывод

`UNIT_SPAWNED … unitType=Tower …` (согласовано, С2); выстрел — `UNIT_ATTACKED` с
`attackerUnitId` = id Башни.

## Приёмка

- `hp = 0, power = 0` — две ошибки `unit.invalid_stat` одним списком;
- стреляет `Power` по цели на дистанции 2 и 5, не стреляет на 1 и 6;
- сосед с `occupiesCell` выстрелу по цели на дистанции 2..5 не мешает;
- без целей — `false`, `world.activity().changes()` не изменился;
- `MARCH` на Башню — `march.unit_cannot_move`;
- `SPAWN_TOWER 1 0 0 10` — `parse.missing_argument` «power»; лишний аргумент —
  `parse.extra_argument`;
- (T30) по Ворону стреляет на дистанции 1..4, на 5 — нет.
