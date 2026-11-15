# Спецификации модулей

Документ уточняет [ARCHITECTURE.md](ARCHITECTURE.md) до уровня, по которому можно
писать код и тесты: API модулей, точное поведение, граничные случаи и критерии
приёмки. Если документы расходятся, архитектура задаёт **устройство**, а этот
документ — **поведение**. Деление на задачи — в [TASKS.md](TASKS.md).

Статус: реализовано. По ходу реализации спецификация уточнялась: изменения поведения —
только после согласования (они помечены «согласовано»), сигнатуры здесь — набросок,
источник правды для них — код и тесты.

Пространства имён: `sw::core`, `sw::features`, `sw::io`, `sw::app`.

## 0. Решения, принятые в этом документе

Эти пункты уточняют поведение там, где архитектура его не задавала. Все
**подтверждены**:

1. **`Targeting` никогда не выбирает источник целью самого себя.** Поэтому правило
   «Лекарь себя не лечит» выполняется без отдельного фильтра.
2. **Шаг марша — сначала прямой.** Если свободна клетка `(sign(dx), sign(dy))`,
   юнит идёт в неё; иначе — случайная из остальных свободных клеток, уменьшающих
   расстояние. Так движение предсказуемо и похоже на лог из исходного `main.cpp` (`docs/original_main_output.txt`).
3. **Движение за тик заканчивается на приходе в цель.** Остаток бюджета `speed`
   не переносится на следующий марш из очереди; следующий марш стартует в том же
   тике (`MARCH_STARTED`), а идти начинает со следующего.
4. **Ошибки-последствия подавляются.** Если спавн юнита не прошёл проверку, команды
   `MARCH` для этого id ошибок не дают — первопричина уже в списке.
5. **Пустой сценарий** (нет `CREATE_MAP`) — одна ошибка `map.not_created` без номера строки.
6. **Конец симуляции проверяется и сразу после установки.** Если после тика 1 на
   карте ≤ 1 юнита, ходы не начинаются.
7. **Дальность Охотника — стат-компонент `Range`**, как Strength и Agility. Тип
   дистанции в Core называется `DistanceRange`, чтобы не занимать это имя. Границы
   дистанции действия задаются числом или статом и читаются при каждом выстреле,
   поэтому будущий бафф дальности не требует менять параметры действия.
8. **`Scheduler` принимает эффекты только на будущие тики.** Эффект на текущий или
   прошедший тик — `std::logic_error`.
9. **Проверка контракта `IAction` включена всегда**, а не только в отладочной сборке:
   это сравнение двух чисел, отключать её незачем.
10. **Сдвиг дистанции — пересечение с `[0, ∞)`.** Границы сдвигаются независимо,
    результат пересекается с неотрицательными дистанциями; пустое пересечение —
    `nullopt`. Ограничение обеих границ нулём отклонено: сильный дебафф давал бы
    атаку по своей клетке вместо отключения атаки.

Пункты 2, 7 и 9 внесены в ARCHITECTURE.md (п. 4.7, 4.3 и 5.1, 4.6 и 4.13).

## 1. Core / Common

### 1.1 Базовые типы

```cpp
// Идентификатор — обёртка: с id не считают, и его нельзя перепутать с количеством
struct UnitId { uint32_t value; };                    // ==, !=, <, std::hash
using Tick = uint64_t;                                // только ++ и сравнение

struct Position { int64_t x; int64_t y; };            // ==, !=, std::hash
int64_t chebyshev(Position a, Position b);            // max(|dx|, |dy|)

struct DistanceRange {                                // включительно: [min, max]
    int64_t min; int64_t max;
    bool contains(int64_t d) const;                   // min <= d <= max
    bool empty() const;                               // min > max
    // Новый отрезок [min + minDelta, max + maxDelta] пересекается с [0, ∞).
    // Пустое пересечение → nullopt (досягаемости не осталось).
    std::optional<DistanceRange> shifted(int64_t minDelta, int64_t maxDelta) const;
};
```

### 1.2 Диагностика и Result

```cpp
struct Diagnostic {
    Diagnostic(std::string code, std::string message,
               std::optional<UnitId> relatedUnit = std::nullopt);
    std::string code;                    // "unit.duplicate_id"
    std::string message;                 // "unit id 1 already exists"
    std::optional<UnitId> relatedUnit;   // App добавит "(unit 1 spawned at line 3)"
};

class Diagnostics {                      // все найденные ошибки, в порядке обнаружения
public:
    void add(Diagnostic);
    void add(std::string code, std::string message, std::optional<UnitId> relatedUnit = {});
    void append(const Diagnostics&);
    bool empty() const; size_t size() const; const Diagnostic& operator[](size_t) const;
    // begin(), end()
};

template <class T> class [[nodiscard]] Result;   // либо T, либо непустые Diagnostics
template <>        class [[nodiscard]] Result<void>;
```

- `Result` создаётся из значения, из `Diagnostics` или из одного `Diagnostic`;
  пустой список ошибок — `std::logic_error`. `Result<void>{}` — успех.
- `ok()`; `value()` при ошибке — `std::logic_error`: значения не существует.
  `errors()` при успехе — пустой список: это правдивый ответ «ошибок нет», поэтому
  сбор ошибок может всегда писать `all.append(result.errors())` без проверки `ok()`.
- У временного `Result` `errors()` возвращает список по значению, а не ссылку: иначе
  `for (const auto& e : make().errors())` обходил бы уничтоженный объект (нашёл ASan).
- `[[nodiscard]]`: результат проверки нельзя молча проигнорировать.
- У `Diagnostic` есть конструктор, а не только агрегатная инициализация:
  `{"code", "message"}` без `relatedUnit` иначе даёт `-Wmissing-field-initializers`.

### 1.3 Теги

```cpp
class Tag {                                // интернируется по имени
public:
    explicit Tag(std::string_view name);   // одинаковое имя → один и тот же id; пустое — logic_error
    std::string_view name() const;
    // ==, <=> — по id
};
class TagSet {                             // без повторов, равенство не зависит от порядка
public:
    TagSet(std::initializer_list<Tag>);
    void add(Tag);
    bool has(Tag) const;
    bool hasAny(const TagSet&) const;
    // empty(), size(), begin(), end()
};
```

- Реестр имён — статическая переменная внутри функции; повторная регистрация имени
  возвращает существующий id. Имена хранятся в `std::deque`, поэтому `name()` может
  отдавать `string_view`: ссылки не меняются при добавлении тегов.
- Реестр не потокобезопасен: симуляция однопоточная.
- Файлы: `Core/Common/Types`, `DistanceRange`, `Narrow`, `Diagnostic`, `Result`, `Tag`.

**Приёмка:** `Tag("fire") == Tag("fire")` в разных единицах трансляции;
для `DistanceRange::shifted`:

| Исходная | Сдвиг | Отрезок после сдвига | Результат (∩ `[0, ∞)`) | Случай |
|---|---|---|---|---|
| `{2,4}` | `(-1, -1)` | `[1, 3]` | `{1,3}` | Ворон: обе границы −1 |
| `{2,4}` | `(0, +2)` | `[2, 6]` | `{2,6}` | бафф только максимальной дальности |
| `{2,4}` | `(+1, 0)` | `[3, 4]` | `{3,4}` | сдвиг только минимальной границы |
| `{0,1}` | `(-1, -1)` | `[-1, 0]` | `{0,0}` | в пересечении осталась дистанция 0 |
| `{1,1}` | `(-2, -2)` | `[-1, -1]` | `nullopt` | ни одной неотрицательной дистанции |
| `{2,4}` | `(+3, 0)` | `[5, 4]` | `nullopt` | отрезок пустой |

`{0,0}` и `nullopt` — разные значения: `{0,0}` — действие достаёт до своей клетки
(юнитов, не занимающих клетку), `nullopt` — не достаёт ни до кого. Сдвиг никогда
не создаёт досягаемость, которой не было в сдвинутом отрезке: сильный дебафф
отключает действие, а не превращает его в удар по своей клетке.

### 1.4 Типы чисел и арифметика

**Граница проходит по слою: `uint32_t` — только в IO, внутри всё знаковое.**

| Где | Тип | Почему |
|---|---|---|
| IO: DTO команд и событий | `uint32_t` | это формат файла и вывода, он не меняется |
| Core и Features: координаты, размеры карты, HP, статы, урон, лечение, дистанции, скорость, стоимость шага | `int64_t` | естественная арифметика с минусом (`dx = -1`, сдвиг дистанции, HP ниже 0); любое `uint32_t` помещается без потерь |
| `UnitId` | обёртка над `uint32_t` | с идентификатором не считают |
| `Tick`, счётчики (`ActivityTracker`) | `uint64_t` | только `++` и сравнение |
| размеры контейнеров и индексы | `size_t` | так их возвращает стандартная библиотека; в арифметику домена не смешиваются |

Правила:
- В Core и Features нет беззнаковых типов, кроме перечисленных в таблице.
  Неотрицательность — инвариант, его проверяют валидация и `std::logic_error`
  в конструкторах, а не тип.
- Переводы между `uint32_t` и `int64_t` — только в App, ровно в двух местах
  (п. 10.5): ввод (`App/Bindings`) и вывод (`App/Output`).
- Сужение `int64_t → uint32_t` — только через `narrow<uint32_t>(int64_t)`: значение
  вне диапазона — `std::logic_error`. Это баг, а не ошибка ввода: на вход попадают
  только проверенные `uint32_t`.
- Координаты соседей считаются в `int64_t`; клетка вне карты отсекается проверкой
  `Map::contains`, переполнения нет.

**Приёмка:** `narrow<uint32_t>(-1)` и `narrow<uint32_t>(4294967296)` — исключение;
`narrow<uint32_t>(4294967295)` — 4294967295; сосед клетки `(0,0)` слева не создаётся
как `(4294967295, 0)`, а отсекается как клетка вне карты.

## 2. Core / Events

```cpp
class EventBus {
public:
    template <class E> void subscribe(std::function<void(const E&)>);
    template <class E> void publish(const E&) const;   // синхронно, в порядке подписки
};
```

События Core (поля совпадают с `io::Events`, кроме `UnitHealed` — его DTO в IO
появится вместе с Лекарем):

| Событие | Поля |
|---|---|
| `MapCreated` | width, height |
| `UnitSpawned` | unitId, unitType, position |
| `UnitMoved` | unitId, position |
| `MarchStarted` | unitId, from, target |
| `MarchEnded` | unitId, position |
| `UnitAttacked` | attackerId, targetId, damage, targetHp |
| `UnitHealed` | healerId, targetId, amount, targetHp |
| `UnitDied` | unitId |

- Публикация без подписчиков — не ошибка (используется в проходе проверки).
- Подписчик может публиковать другие события: они доставляются сразу, вложенно.
- Подписка во время публикации и пустой обработчик — `std::logic_error` (нарушение
  инварианта, архитектура п. 6.4).
- Файлы: `Core/Events/EventBus.hpp`, `Core/Events/CoreEvents.hpp` (пространство имён
  `sw::core::events`).

**Приёмка:** два подписчика вызываются по порядку; событие одного типа не
доходит до подписчика другого типа.

## 3. Core / Units

### 3.1 Unit

```cpp
class Unit {
public:
    UnitId id() const;
    std::string_view type() const;     // "Swordsman"
    Position position() const;         // меняет только MovementService
    bool occupiesCell() const;
    const TagSet& tags() const;
    bool isDead() const;               // HP ≤ 0
    bool isRemovalRequested() const;

    template <class T> bool has() const;
    template <class T> const T& get() const;     // нет компонента → std::logic_error
    template <class T> const T* tryGet() const;
    const ComponentStore& components() const;    // targetRules(), hitModifiers() — индекс хуков
};
```

- Создаётся из `UnitBlueprint`, который заполняет `UnitBuilder` (Features).
- Фичи видят юнита только как `const Unit&`; изменяющие методы (`setPosition`,
  `requestRemoval`, неконстантные `get`, `behaviour()`) вызывает Core.
- `isDead()` — по `Health`: HP ≤ 0. Юнит без `Health` не умирает.
- `ComponentStore`: по одному компоненту каждого типа; второй компонент того же типа —
  `std::logic_error`. Компонент, реализующий `ITargetRule` / `IHitModifier`, при
  добавлении попадает в индекс хуков, при удалении — убирается из него. Порядок хуков —
  порядок добавления. Можно добавить компонент, тип которого известен только во время
  выполнения (статусы).
- Интерфейсы `IAction`, `ITargetRule`, `IHitModifier` объявлены здесь же (T04), типы
  `TargetSpec`, `Hit`, `TurnContext` — объявлены заранее и определяются в T06–T07.

### 3.2 Компоненты Core

```cpp
struct Health   { int64_t current; int64_t max; };      // max = HP при спавне
struct Movement { int64_t speed; std::deque<Position> marches; };   // speed ≥ 1
class  Status : public IComponent {                     // база статусов
public:
    explicit Status(std::optional<Tick> expiresAt);     // nullopt — бессрочный
};
```

- HP после удара может быть ниже 0 (это и есть смерть); в событиях HP выводится как
  `max(0, current)`. Типы — п. 1.4.
- `Health(hp)` с hp ≤ 0 и `Movement(speed)` со speed < 1 — `std::logic_error`: такой
  спавн фабрика юнита обязана отклонить ошибкой ввода раньше.

### 3.3 ComponentRequirements

```cpp
class ComponentRequirements {
public:
    template <class T> void require();
    std::vector<std::string> missingIn(const ComponentStore&) const;   // имена недостающих
};
```

- Требования проверяет конструктор `Unit`, поэтому проверку не обойти, как бы юнит ни
  собирался. Недостающие компоненты всех действий — одним `std::logic_error`.
- Имена действий и компонентов в сообщении — читаемые имена типов (`typeName`,
  `Core/Common/TypeName`): объявлять свои имена компонентам и действиям не нужно.

**Приёмка:** юнит без `Movement` с `MarchStep` не собирается
(`std::logic_error`, в тексте — тип юнита, действие и компонент).

## 4. Core / Map и World

### 4.1 Map

```cpp
class Map {
public:
    Map(int64_t width, int64_t height);                // ≤ 0 → std::logic_error
    bool contains(Position) const;
    std::optional<UnitId> occupant(Position) const;
    void occupy(Position, UnitId);                      // занята / вне карты → logic_error
    void release(Position, UnitId);
    // Раскладывает свойство по спискам интерфейсов, которые реализует F
    // (проверка на этапе компиляции: F реализует хотя бы один)
    template <class F> void addCellFeature(Position, std::shared_ptr<F>);
    int64_t moveCost(Position, const Unit&) const;      // по умолчанию 1; результат ≥ 1
    void    triggerEnter(Position, const Unit&, TurnContext&) const;
};
```

- Занятость хранят только юниты с `occupiesCell`. Мёртвый держит клетку до Cleanup.
- `neighbourCells(Position)` — соседние клетки внутри карты в постоянном порядке
  (по строкам сверху вниз, в строке слева направо); считаются в `int64_t`, у края
  карты лишние отсекаются.
- Модификаторы стоимости и триггеры клетки применяются в порядке добавления;
  модификатор получает стоимость после предыдущего.
- **Хранение разреженное:** занятость — `std::unordered_map<Position, UnitId>`,
  свойства клеток — `std::unordered_map<Position, …>`. Формат допускает карту до
  4294967295 × 4294967295, плотная сетка такого размера в память не поместится.
  Ограничение размера карты не вводится: формат его не задаёт.

**Приёмка:** карта `4294967295 × 4294967295` создаётся без выделения памяти под
клетки; юнит спавнится в её угол `(4294967294, 4294967294)`.

### 4.2 UnitStore

- Хранит юнитов в порядке создания; `find(UnitId)`, `orderedIds()` — снимок порядка
  для фазы Main, `ordered()` — сами юниты.
- Отдельной пометки «мёртв» нет: смерть определяется по `Health` (`Unit::isDead`),
  удаление не через HP — `Unit::requestRemoval`.
- `removePending(Map&)` — удаляет мёртвых и помеченных, освобождает их клетки, порядок
  остальных не меняет, возвращает число удалённых.
- `World::place(Unit)` ставит юнита на карту и в хранилище вместе: клетка вне карты,
  занятая клетка (для `occupiesCell`) и повторный id — `std::logic_error`
  (ошибки ввода ловит `SetupContext` раньше). `World::removePending()` — Cleanup.

### 4.3 SpatialQuery (только чтение)

```cpp
class SpatialQuery {
public:
    // Живые юниты на расстоянии в range от center, в порядке создания
    std::vector<const Unit*> unitsInRange(Position center, DistanceRange range) const;
    std::vector<const Unit*> neighbours(Position center) const;   // расстояние 1
    bool isFreeFor(const Unit&, Position) const;                  // с учётом occupiesCell
    const Unit* find(UnitId) const;
    size_t aliveCount() const;   // все юниты на карте, включая мину; мёртвые до Cleanup не считаются
};
```

- «Живой» — не мёртвый и не помеченный на удаление. Запросы возвращают только живых;
  `isFreeFor` при этом учитывает и мёртвого: он держит клетку до Cleanup.

### 4.4 ActivityTracker

```cpp
class ActivityTracker {
public:
    void beginTick();
    void markAction(); void markScheduled(); void markRemoval(); void markChange();
    uint64_t changes() const;     // счётчик изменений — для проверки контракта IAction
    bool tickWasEmpty() const;    // ни action, ни scheduled, ни removal
};
```

**Приёмка:** спавн на занятую клетку `occupiesCell`-юнитом невозможен, юнитом без
`occupiesCell` — возможен; `unitsInRange` не возвращает мёртвых; `removePending`
освобождает клетку.

## 5. Core / Interaction

### 5.1 TargetSpec, Hit, интерфейсы

Как в архитектуре (п. 4.9): `Intent`, `Effect`, `TargetSpec`, `ITargetRule`,
`IHitModifier`.

```cpp
struct Hit {
    TargetSpec spec;
    UnitId     source;
    UnitId     target;
    int64_t    amount;   // ≥ 0 на входе и после каждого модификатора
};
```

- У `TargetSpec` нет конструктора по умолчанию: без него `TargetSpec{}` молча дал бы
  первые значения `Intent` и `Effect`, а автор действия обязан выбрать оба.
- Файлы: `Core/Interaction/TargetSpec.hpp` (`Intent`, `Effect`, `TargetSpec`, `Hit`).

### 5.2 Targeting

```cpp
class Targeting {
public:
    // Кандидаты в порядке создания; выбор случайного — на стороне действия
    std::vector<const Unit*> select(const Unit& source, const TargetSpec& spec,
                                    const std::function<bool(const Unit&)>& filter = {}) const;
};
```

Алгоритм для каждого юнита на карте:
1. пропустить: источник, мёртвых, помеченных на удаление;
2. для `Effect::Damage` / `Effect::Heal` пропустить юнитов без `Health`;
3. применить правила цели по порядку: `range = rule->adjust(spec, range)`;
   `nullopt` — пропустить;
4. оставить, если `range.contains(chebyshev(source, target))` и `filter` пропускает.

Шаги 1–4 для одной цели — `canTarget(source, target, spec)`; им же пользуется
`applyStatus`.

### 5.3 Interaction

```cpp
class Interaction {
public:
    void apply(const Hit& hit);                         // урон или лечение
    bool applyStatus(const Unit& source, const Unit& target, const TargetSpec& spec,
                     std::unique_ptr<Status> status);   // false — правила цели запретили
};
```

- Цель `apply` обязана быть выбрана через `Targeting`: нет цели, она мертва или у неё
  нет `Health` — `std::logic_error`. Отрицательный `amount` и `Effect::Status` в
  `apply` — тоже `std::logic_error`.
- **Повторное наложение статуса** (согласовано): если у цели уже есть статус того же
  типа, решает он сам — `Status::onReapplied(incoming)`:
  - `Reapply::Replace` (по умолчанию) — новый экземпляр заменяет старый: обновляются
    срок и параметры; в индексе хуков он встаёт в конец;
  - `Reapply::Keep` — остаётся старый (он мог изменить себя: продлить срок через
    `setExpiresAt`, добавить стак), новый отбрасывается.

  В обоих случаях `applyStatus` возвращает `true` и отмечает изменение мира.

`apply`:
1. модификаторы цели по возрастанию `priority()`, при равенстве — в порядке
   добавления; после каждого — проверка `amount ≥ 0` (иначе `std::logic_error`);
2. `Damage`: `current -= amount`, публикуется `UnitAttacked{source, target, amount, max(0, current)}`;
   если `current ≤ 0` и юнит ещё не мёртв — пометка «мёртв» и сразу `UnitDied`;
3. `Heal`: `current = min(max, current + amount)`, публикуется `UnitHealed`
   (с `amount` после модификаторов, в том числе 0);
4. `ActivityTracker::markChange()`.

**Приёмка:** урон больше HP даёт `targetHp = 0` и одно `UnitDied`; повторный удар по
мёртвому невозможен (его не выбирает `Targeting`); лечение не поднимает HP выше max;
модификатор, обнуливший урон, даёт `UnitAttacked` с `damage = 0`.

## 6. Core / Simulation

### 6.1 Scheduler

```cpp
class Scheduler {
public:
    using Task = std::function<void(TurnContext&)>;
    void schedule(Tick now, Tick at, UnitId source, Task);   // at ≤ now → logic_error
    std::vector<Entry> takeDue(Tick now);                    // в порядке постановки
    bool empty() const;
};
```

- `Scheduler` и номер текущего тика хранятся в `World` (`tick()`, `advanceTick()`):
  номер тика нужен и выводу событий, а не только симуляции.

### 6.2 TurnContext и MovementService

`TurnContext` — как в архитектуре (п. 4.3). Изменяющие методы фасадов вызывают
`ActivityTracker::markChange()`: `schedule(at, source, task)`, `requestRemoval(id)`,
`publish(event)` (события фич), а также всё, что делают `effects()` и `movement()`.
`random()` изменением не считается.

```cpp
class MovementService {
public:
    // Один шаг в соседнюю клетку. false — у юнита нет Movement, клетка вне карты,
    // не соседняя или занята для этого юнита.
    bool stepTo(const Unit&, Position cell, TurnContext&);   // контекст — для триггеров клетки
    void addMarch(const Unit&, Position target);             // первый в очереди — MarchStarted
    void finishMarch(const Unit&);                           // MarchEnded, следующий — MarchStarted
};
```

`stepTo` при успехе: освобождает и занимает клетки (для `occupiesCell`), меняет
позицию, публикует `UnitMoved`, вызывает `IEnterTrigger` клетки.

`IRandom::index(count)` — случайный индекс в `[0, count)`; `StdRandom` — `mt19937_64` с
seed, в приложении seed от `std::random_device`.

### 6.3 MarchStep

```
нет Movement или очередь пуста          → false
budget = speed; moved = false
пока budget > 0 и юнит не в цели и жив (триггер клетки мог его убить):
    кандидаты = соседние клетки, уменьшающие расстояние до цели и свободные для юнита
    если пусто → break
    cell = прямая клетка (sign(dx), sign(dy)), если она среди кандидатов,
           иначе случайная из кандидатов
    cost = map.moveCost(cell, unit)
    если moved и cost > budget → break        // первый шаг разрешён всегда
    movement.stepTo(unit, cell); budget -= cost; moved = true
если юнит в цели: finishMarch → return true      // события — изменение мира
return moved
```

### 6.4 Simulation

```cpp
class Simulation {
public:
    Simulation(World&, const EventBus&, IRandom&, Tick tickLimit = 30'000);   // < 2 → logic_error
    // Тик 1 уже прошёл (установка). Возвращает причину остановки.
    StopReason run();   // OneOrNoUnitsLeft | NothingHappened | TickLimit
    std::optional<StopReason> runTick();   // один тик — для тестов
    TurnContext& context();
};
```

- Снятие истёкших статусов не делает тик непустым: по правилу «пустой тик» считаются
  только действия, отложенные эффекты и удаления юнитов.

Цикл:

```
если aliveCount ≤ 1 → OneOrNoUnitsLeft                // сразу после установки
for tick = 2 …:
    tracker.beginTick()
    Scheduled: выполнить эффекты тика, markScheduled на каждый
    Main: для id из снимка порядка создания:
        пропустить мёртвых, помеченных на удаление, удалённых
        для action в behaviour:
            before = tracker.changes()
            if action.tryExecute(unit, ctx): markAction; break
            if tracker.changes() != before: logic_error   // контракт IAction
    Cleanup: removed = store.removePending(); markRemoval при removed > 0;
             снять истёкшие статусы
    aliveCount ≤ 1                        → OneOrNoUnitsLeft
    tickWasEmpty && scheduler.empty       → NothingHappened
    tick == tickLimit                     → TickLimit
```

**Приёмка:**
- юнит, убитый в фазе Main до своего хода, в этом тике не действует;
- два юнита вне досягаемости без маршей — остановка `NothingHappened` на тике 2;
- взведённая мина не даёт остановиться, пока взрыв в очереди;
- тестовое действие, которое меняет мир и возвращает `false`, даёт `std::logic_error`;
- лимит тиков в тесте задаётся маленьким (например, 5).

## 7. Core / Setup

```cpp
struct SpawnRequest {          // фабрика юнита знает это всегда, даже при неверных статах
    UnitId id; Position position; bool occupiesCell;
    Result<Unit> unit;         // собранный юнит или ошибки фабрики
};

class SetupContext {
public:
    Result<void> createMap(int64_t width, int64_t height);
    Result<void> spawn(SpawnRequest request);        // ошибки фабрики пробрасываются
    Result<void> orderMarch(UnitId, Position target);
    Result<void> finish();                           // конец сценария: не было CREATE_MAP
    void noteFailedMapCommand();                     // строка CREATE_MAP не разобралась
    World* world();                                  // nullptr — карта не создана
};
```

- Фабрика юнита возвращает `SpawnRequest`, а не только `Result<Unit>`: по id, позиции и
  занятости клетки проверки идут и тогда, когда юнит не собрался. Без этого у спавна с
  плохими статами и занятой клеткой нельзя было бы показать обе ошибки.

| Метод | Проверки (код ошибки) |
|---|---|
| `createMap` | повторно (даже если первая команда была с ошибкой) — `map.already_created`; W или H = 0 — `map.invalid_size` |
| `spawn` | ошибки фабрики как есть; нет карты — `map.not_created` (один раз за сценарий); id занят — `unit.duplicate_id` (relatedUnit = id); вне карты — `unit.out_of_bounds`; клетка занята — `unit.cell_occupied` (relatedUnit = занявший) |
| `orderMarch` | id отклонённого спавна — без ошибки (п. 0.4); нет юнита — `march.unit_not_found`; нет `Movement` — `march.unit_cannot_move`; цель вне карты — `march.out_of_bounds`; цель совпадает с концом очереди (или позицией, если очередь пуста) — `march.same_position` |

- Успешный `spawn` публикует `UnitSpawned`; первый марш юнита публикует
  `MarchStarted` сразу (тик 1), следующие только встают в очередь.
- Все проверки одного вызова собираются вместе (у `spawn` с плохими статами и
  занятой клеткой — обе ошибки).
- **Последствия неудачного `CREATE_MAP`** (согласовано): если команда `CREATE_MAP` была,
  но с ошибкой, команды, которым нужна карта, `map.not_created` не дают — первопричина
  уже в списке. Юниты при этом отклоняются молча, но ошибки их статов выводятся.
- Повторный id при спавне принадлежит уже стоящему юниту, поэтому `MARCH` на этот id
  относится к первому юниту и проверяется как обычно.
- Ошибки `march.*` про существующего юнита несут `relatedUnit`: App добавит строку его
  спавна.

## 8. IO

### 8.1 CommandParser

```cpp
struct ParseDiagnostic { size_t line; std::string command; std::string code; std::string message; };

class CommandParser {
public:
    template <class TCommand>
    CommandParser& add(std::function<void(TCommand, size_t line)> handler);  // дубль → logic_error
    std::vector<ParseDiagnostic> parse(std::istream&);   // не бросает на пользовательском вводе
};
```

- Пустые строки и строки, начинающиеся с `//`, пропускаются (как сейчас).
- **Число не читается через `operator>>`.** Проверено на clang 15.0.7 с libstdc++ 13
  и с libc++ 15 — поведение одинаковое, `failbit` не ставится ни в одном случае:

  | Токен | `operator>>` в `uint32_t` | `std::from_chars` |
  |---|---|---|
  | `-5` | 4294967291 | `invalid_argument` |
  | `+5` | 5 | `invalid_argument` |
  | `5x` | 5, остаток `x` в потоке | ok, разобран только `5` |
  | `1.5` | 1, остаток `.5` в потоке | ok, разобран только `1` |
  | `4294967296` | `failbit` | `result_out_of_range` |

  `from_chars` сам отвергает знак, но принимает префикс (`5x` → 5). Поэтому разбор
  поля `uint32_t` такой:
  1. токен читается как строка;
  2. в нём допускаются только цифры `0–9` — так отсекаются `-5`, `+5`, `5x`, `1.5`;
  3. значение получает `std::from_chars`; `errc::result_out_of_range` — переполнение.
- Тип поля в `visit` проверяется на этапе компиляции: `uint32_t` — разбор выше,
  `std::string` — токен как есть, другой тип — `static_assert`. Новый DTO не может
  незаметно вернуться к `operator>>`.
- Ошибка числа — `parse.invalid_number` с именем поля из `visit`:
  «hp: expected non-negative integer, got "-5"», «hp: value exceeds 4294967295».
- Не хватает токенов — `parse.missing_argument` с именем первого отсутствующего поля;
  лишние — `parse.extra_argument`.
- Строка с ошибкой разбора в handler не передаётся.
- В строке выводится только первая ошибка поля: после неё позиция в строке уже не
  совпадает с полями, следующие ошибки были бы последствиями. Ошибки разных строк
  собираются все.
- У `parse.unknown_command` поле `command` пустое. Тексты: `unknown command 'X'`,
  `missing argument 'range'`, `unexpected extra argument '4'`,
  `hp: expected non-negative integer, got "-5"`, `width: value exceeds 4294967295, got …`.
- Строки из одних пробелов пропускаются; лишние пробелы и табуляция между аргументами и
  окончания строк Windows (`\r\n`) допустимы.
- Разбор числа — `IO/System/ParseNumber.hpp` (`parseUInt32`), его же проверяют тесты.

### 8.2 EventLog

`log(tick, event)` пишет `[tick] NAME field=value ...` в `std::cout`. Формат не менялся;
конец строки — `'\n'` вместо `std::endl` (`performance-avoid-endl` в clang-tidy): буфер
сбрасывается при выходе из программы, а не после каждой строки.

**Приёмка:** `SPAWN_HUNTER 1 0 0 10 5 1` → `missing_argument: range`;
`CREATE_MAP 10 x` → `invalid_number: height`; `MARCH 1 2 3 4` → `extra_argument`;
для поля `uint32_t`:

| Токен | Результат |
|---|---|
| `-5` | `invalid_number`: expected non-negative integer |
| `+5`, `5x`, `1.5`, `0x10` | `invalid_number`: expected non-negative integer |
| `4294967295` | 4294967295 |
| `4294967296` | `invalid_number`: value exceeds 4294967295 |
| `007` | 7 — ведущие нули допустимы |

## 9. Features

### 9.1 Common

- `CommonTags`: `melee`, `ranged`, `area`, `physical`, `flying`.
- `TargetSpecs`: `attack`, `heal`, `buff`, `debuff` (архитектура, п. 4.9).
- `Stats`: `Strength`, `Agility`, `Range`, `Power`, `Spirit` — `struct { int64_t value; }`.
- Спецификации юнитов (`HunterSpec` и др.) — поля `int64_t`; `UnitId` и `Position` —
  типы Core (п. 1.1, 1.4).
- `stat<T>` — источник значения стата: читает компонент `T` у юнита в момент вызова
  и объявляет требование компонента `T`. Это единственная точка чтения статов:
  будущие модификаторы статов (баффы, проклятия) добавляются в `stat<T>` внутри
  Features, без правок Core и без правок действий.
- `distance(min, max)` — источник дистанции действия; каждая граница — число или
  `stat<T>`. Вычисляется в `DistanceRange` при каждом выполнении действия;
  если `min > max` (например, дальность уменьшили ниже 2) — целей нет.
- `UnitBuilder`:

```cpp
UnitBuilder(UnitId, std::string type, Position);    // occupiesCell = true
UnitBuilder& occupiesCell(bool);
UnitBuilder& tag(Tag);
template <class C, class... A> UnitBuilder& with(A&&...);
template <class Act, class... A> UnitBuilder& action(A&&...);
Unit build();                                  // проверка requirements → logic_error со списком недостающего
SpawnRequest spawnRequest();                   // заявка на спавн собранного юнита
SpawnRequest reject(Diagnostics errors) const; // заявка с ошибками фабрики (п. 7)
```

- `requirePositive(errors, name, value)` — `unit.invalid_stat` «hp must be positive, got 0».
- Числа для действий — `ValueSource`: константа или `stat<T>`; `distance(min, max)` —
  `Distance` из двух `ValueSource`. Файлы: `Features/Common/CommonTags`, `TargetSpecs`,
  `Stats`, `ValueSource`, `UnitBuilder`, `Attacks`.

### 9.2 Действия

| Действие | Поведение |
|---|---|
| `MeleeAttack(stat<T>)` | цели — `select(self, attack({melee, physical}, {1,1}))`; нет — `false`; иначе случайная, `apply(Hit{…, amount = max(0, T)})`, `true` |
| `RangedAttack(stat<T>, distance(min, max), condition?)` | если `condition` не выполнено — `false`; дистанция вычисляется из `distance`; цели — `attack({ranged, physical}, дистанция)`; дальше как `MeleeAttack` |
| `RequireNoAdjacentOccupying` | `false`, если среди `neighbours(self)` есть живой юнит с `occupiesCell` |

- **Отрицательный стат урона** (согласовано): если характеристика урона ниже нуля
  (будущий дебафф), удар всё равно происходит — с уроном 0, действие выполнено.

### 9.3 Юниты

| Юнит | Spec | Проверки | Сборка |
|---|---|---|---|
| Swordsman | id, pos, hp, strength | hp, strength > 0 (`unit.invalid_stat`) | `Health`, `Strength`, `Movement(1)`; [`MeleeAttack(stat<Strength>)`, `MarchStep`] |
| Hunter | id, pos, hp, agility, strength, range | hp, agility, strength > 0 (`unit.invalid_stat`); range ≥ 2 (`hunter.invalid_range`) | `Health`, `Agility`, `Strength`, `Range`, `Movement(1)`; [`RangedAttack(stat<Agility>, distance(2, stat<Range>), RequireNoAdjacentOccupying)`, `MeleeAttack(stat<Strength>)`, `MarchStep`] |

**Приёмка:** Охотник с соседом бьёт `Strength`; без соседей и с целью на дистанции
2..range стреляет `Agility`; цель на дистанции 1 и сосед-летающий в тестовом
юните не мешают выстрелу; Мечник без соседей и без марша бездействует; один и тот
же объект `distance(2, stat<Range>)` на юнитах с разным `Range` даёт разные
дистанции — значение не зафиксировано при сборке.

- Фабрики — `SpawnRequest makeSwordsman(const SwordsmanSpec&)` и
  `SpawnRequest makeHunter(const HunterSpec&)` (`Features/Units`); тип юнита в
  `UNIT_SPAWNED` — `Swordsman` и `Hunter`.
- `range` проверяется только правилом `hunter.invalid_range`: `range = 0` даёт одну эту
  ошибку, а не ещё и `unit.invalid_stat`.
- Тест `ExampleScenario_plays_by_the_rules_with_real_units` прогоняет
  `commands_example.txt` через `SetupContext` и `Simulation`: тики 1–6 совпадают с логом
  исходного `main.cpp`, с тика 7 — расхождение из-за выстрела на дистанцию 5 в исходном
  логе (KNOWN_ISSUES.md, п. 4).

## 10. App

### 10.1 Application

```cpp
struct ApplicationOptions {                    // по умолчанию — как в main
    std::vector<BindingFn> bindings = bindingList();
    std::vector<OutputFn>  outputs  = outputList();
    std::function<std::unique_ptr<IRandom>()> random;   // StdRandom с seed от random_device
    Tick tickLimit = 30'000;
};
explicit Application(ApplicationOptions options = {});   // тесты подставляют свои части
int Application::run(int argc, const char* const* argv, std::ostream& errors) const;
// main: return Application{}.run(argc, argv, std::cerr);
```

- `run` не бросает: любое исключение печатается как `internal error` с кодом 3.
- Коды — константы `exit_code::Success / Environment / InvalidScenario / InternalError`.
- Сообщения приложения — в `errors` (stderr); stdout занят только событиями.

| Ситуация | stderr | код |
|---|---|---|
| аргументов не 1 | `usage: sw_battle_test <scenario-file>` | 1 |
| файл не открылся | `error: cannot open file '<path>'` | 1 |
| ошибки сценария | список (п. 10.3) + `N errors, simulation not started.` | 2 |
| `std::logic_error` / другое исключение | `internal error: <what>` | 3 |
| остановка по лимиту | `simulation stopped: tick limit (30000) reached` | 0 |
| нормальный конец | — | 0 |

### 10.2 Scenario

- `CommandRegistry::on<TCommand>(handler(const TCommand&, SetupContext&) -> Result<void>)`
  запоминает обработчик; дубль — `logic_error`. `installInto(parser, steps)` регистрирует
  все команды в `CommandParser`.
- `loadScenario(input, registry)` — возвращает шаги `ScenarioStep{line, command, execute}`
  и ошибки разбора. Шаг можно выполнить несколько раз: на этом держатся два прохода.
- `validateScenario` (проход 1) — новый мир с `EventBus` без подписчиков; выполняет
  все разобравшиеся шаги (и тогда, когда в других строках есть ошибки разбора — список
  должен быть полным), собирает ошибки, к каждой добавляет строку и имя команды; для
  `relatedUnit` ищет строку успешного спавна этого юнита. Строка спавна узнаётся без
  знания конкретных команд: юнит, появившийся в мире после шага, заспавнен этим шагом.
  Строка `CREATE_MAP` с ошибкой разбора — это неудачный `CREATE_MAP`
  (`SetupContext::noteFailedMapCommand`), последствия подавляются по правилу п. 7.
- `runScenario` (проход 2) — новый мир с подключённым `EventBridge`, все шаги, затем
  `Simulation::run`. Ошибка шага здесь — `std::logic_error`: проход 1 её пропустил.
- Остановка по лимиту — сообщение в stderr, код 0.

### 10.3 Формат ошибок

```
error: <file>:<line>: <COMMAND> [<code>]: <message>[ (unit <id> spawned at line <L>)]
error: <file>: [<code>]: <message>                 — ошибка без строки (пустой сценарий)
<N> error(s), simulation not started.
```

Ошибки выводятся в порядке строк; ошибки одной строки — в порядке обнаружения;
ошибки без строки — в конце.

- Для нераспознанной команды имя команды не печатается:
  `error: s.txt:2: [parse.unknown_command]: unknown command 'FLY'`.
- Итог: `1 error, …` / `N errors, simulation not started.`
- Хвост `(unit <id> spawned at line <L>)` — только когда строка спавна известна.
- Печать — `App/DiagnosticPrinter` (`printScenarioErrors`, тип `ScenarioError`).

### 10.4 Output

- `EventBridge(bus, log, clock)`; `on<E>(std::function<void(const E&, EventLog&, Tick)>)` —
  дубль типа или пустой адаптер — `logic_error`; подписывается на `EventBus`. Номер тика
  берётся из `clock` в момент события (в приложении — `World::tick()`).
- `CoreEventsOutput` переводит 7 событий Core в `io::Events` (`UnitHealed` — с Лекарем:
  до тех пор событие без адаптера просто не выводится).
- Адаптеры команд — `App/Bindings`: `CoreBindings` (`CREATE_MAP`, `MARCH`),
  `SwordsmanBinding`, `HunterBinding`; каждый регистрируется сам (`SW_REGISTER_BINDING`).

### 10.5 Переводы типов на границе

Единственные места, где встречаются `uint32_t` из IO и `int64_t` домена (п. 1.4):

| Направление | Где | Как |
|---|---|---|
| ввод: DTO → домен | `App/Bindings` | `uint32_t → int64_t` — расширение без потерь; `unitId → UnitId{…}` |
| вывод: домен → DTO | `App/Output` | `int64_t → uint32_t` через `narrow`; HP уже приведён к `max(0, current)` в событии Core |

**Приёмка:** адаптер ввода не содержит проверок диапазона (они не нужны); вывод
события со значением вне `uint32_t` — `std::logic_error` (код выхода 3).

## 11. Сборка и тесты

- CMake-цели: `sw_core`, `sw_features`, `sw_io` — статические библиотеки (в них нет
  саморегистрации); `sw_app` — OBJECT-библиотека (саморегистрация, архитектура п. 5.4);
  `sw_battle_test` — исполняемый файл; `sw_tests` — тесты.
- Тесты — собственный минимальный фреймворк (`TEST`, `CHECK`, `CHECK_THROWS`) без
  внешних зависимостей; запуск через `ctest`.
- Сценарные тесты используют `IRandom`-заглушку, выбирающую первый вариант, и
  сравнивают stdout/stderr с эталоном. Эталоны — `tests/scenarios/<имя>/expected.{out,err,code}`
  (см. `tests/scenarios/README.md`). Сценарии составлены без случайного выбора, поэтому
  их дополнительно проверяет настоящий `sw_battle_test` (ctest `scenario.<имя>`,
  `cmake/RunScenario.cmake`). Новый каталог сценария подхватывается без правок CMake.
- Флаги: `-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion`; сборка без
  предупреждений на clang 15+. `-Wconversion` и `-Wsign-conversion` ловят неявное смешение
  знаковых и беззнаковых типов и сужение на этапе компиляции — правило п. 1.4
  проверяет компилятор, а не только ревью.
- Тестовая сборка — с `-fsanitize=undefined,implicit-conversion` (clang). Проверено
  на clang 15: `uint32_t u = int64_t{-1}` даёт `runtime error: implicit conversion …
  changed the value to 4294967295`. Неявная смена знака или усечение, если всё же
  случились, видны в выводе теста.
- Санитайзер `unsigned-integer-overflow` не включается: беззнаковое переполнение в
  C++ определено, а сам санитайзер срабатывает на штатном коде стандартной
  библиотеки. Проверено: в libstdc++ 13 — на `std::mt19937` (`bits/random.tcc`),
  который использует `StdRandom`; в libc++ 15 на том же коде отчётов нет.
- `-Wconversion` на clang 15 сообщает о сужении `int64_t → uint32_t` как
  `-Wshorten-64-to-32` — проверено.
- Окружение сборки на Ubuntu 24.04: `clang-15`, `libclang-rt-15-dev` (рантайм
  санитайзеров — без него тестовая сборка не линкуется), `cmake` (в 24.04 — 3.28,
  это ≥ 3.27; в 22.04 — 3.22, не подходит).
- **Проверка границ слоёв** (`cmake/CheckLayerBoundaries.cmake`): правила включений —
  Core → Core; Features → Core, Features; IO → IO; App → все; файлы прямо в `src/`
  (`main.cpp`) не ограничены. Слой файла и включаемого пути — первый каталог
  относительно `src/`; `"…"` разрешается сначала относительно файла, затем через
  `src/`. Цель `sw_layer_boundaries` — зависимость каждого слоя, поэтому проверка
  выполняется при сборке любой цели и повторяется при изменении любого исходника.
  Нарушения выводятся все сразу, сборка падает. Тесты `ctest`:
  `layer_boundaries_reports_all_violations` (набор из трёх заведомых нарушений в
  `tests/LayerBoundaries/fixtures`) и `layer_boundaries_project_is_clean`.
- Предупреждения как ошибки — стандартной опцией `-DCMAKE_COMPILE_WARNING_AS_ERROR=ON`,
  как в CI; своей опции у проекта нет.
- Форматирование — `clang-format-18` по `.clang-format` проекта (версия закреплена в
  CI: разные версии форматируют по-разному). clang-format 15 этот файл не читает:
  в нём ключи версии 16+.