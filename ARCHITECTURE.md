# Архитектура симуляции

Статус: **утверждена и реализована** (задачи T01–T19). Решения с рассмотренными
вариантами — в [docs/adr](docs/adr/README.md), термины — в [GLOSSARY.md](GLOSSARY.md),
поведение модулей — в [SPECIFICATION.md](SPECIFICATION.md), деление на задачи — в
[TASKS.md](TASKS.md), известные недостатки — в [KNOWN_ISSUES.md](KNOWN_ISSUES.md).

Документ описывает устройство: что где лежит и как части связаны. Почему выбрано
именно так и что отвергли — в ADR, ссылки на них стоят в разделах.

## 1. Цели и ограничения

- Core принадлежит одному владельцу, Features пишут много разработчиков параллельно.
  Новый юнит, действие, характеристика или эффект добавляются **без правок Core**.
- В Core — механизмы и базовые правила из задания, а не контент. Каждый механизм
  нужен хотя бы одному юниту из задания или из планов; «универсального движка» нет
  ([ADR 0007](docs/adr/0007-no-mechanisms-without-consumers.md)). Правила мира из задания — HP со смертью и марш — живут в Core
  ([ADR 0006](docs/adr/0006-world-rules-in-core.md)).
- Часть Core закрыта намеренно: это ядро правил, а не точки расширения. Сюда
  относятся три фазы тика, условия конца симуляции, набор событий Core, флаг
  `occupiesCell`, оси `Intent` и `Effect`.
- Чёткие слои: Core не знает про файлы, строковые команды и формат вывода.
- До запуска симуляции сценарий проверяется целиком; при ошибках выводится полный
  список, симуляция не стартует.
- C++20, только стандартная библиотека, clang 15+, CMake (`cmake_minimum_required 3.27`).

## 2. Слои и зависимости

```
main ──► App ──────┬─────────────┬────────────┐
                   ▼             ▼            ▼
                   IO        Features ────► Core
```

| Слой | Ответственность | Может зависеть от |
|---|---|---|
| **IO** | DTO команд и событий (это и есть формат ввода/вывода), парсер сценария (с диагностикой), `EventLog` | stdlib |
| **Core** | мир, карта, юниты, ход из трёх фаз, таргетинг, применение эффектов, планировщик, события, диагностика | stdlib |
| **Features** | статы, действия, теги, статусы; спецификации юнитов (`HunterSpec`), их проверка и сборка | Core |
| **App** | разбор аргументов, загрузка и валидация сценария, адаптеры команд (`Bindings`), вывод событий и ошибок, коды выхода | всё |

Почему Core не знает формат ввода и вывода —
[ADR 0002](docs/adr/0002-core-independent-of-io.md).

Правила:
- **Core → никуда.** Core публикует события своих типов и ничего не знает про
  формат ввода и вывода.
- **Features → только Core.** Фича не знает формат ввода: она описывает свои
  параметры на языке предметной области (`HunterSpec`), сама проверяет их и
  возвращает ошибки без номеров строк.
- **Связи между слоями — только в App**, симметрично для ввода и вывода:
  - ввод: `App/Bindings` переводит DTO команды (`io::SpawnHunter`) в
    спецификацию фичи (`HunterSpec`) или в вызов API Core (`CREATE_MAP`, `MARCH`);
  - вывод: адаптеры событий в `App/Output` переводят доменные события
    Core/Features в `io::Events`, а `App/Output/EventBridge` передаёт их в `EventLog`.
- Адаптер — 5–10 строк без логики. Логика (проверки, сборка юнита) в адаптер
  не попадает.
- **Типы чисел меняются на той же границе.** `uint32_t` — только в IO: это формат
  файла и вывода. В Core и Features все величины — `int64_t` (координаты, HP, статы,
  урон, дистанции), беззнаковые — только для того, с чем не считают (`UnitId`,
  `Tick`, счётчики). Беззнаковый тип не гарантирует неотрицательность, а маскирует
  ошибку: `0 - 1` даёт 4294967295. Неотрицательность — инвариант, его проверяют
  валидация и исключения. Переводы `uint32_t ↔ int64_t` — только в `App/Bindings`
  (расширение без потерь) и `App/Output` (сужение с проверкой). Подробности — в
  SPECIFICATION.md, п. 1.4 и 10.5.
- **Границы слоёв проверяются автоматически.** `cmake/CheckLayerBoundaries.cmake`
  выполняется в каждой сборке: если файл слоя включает запрещённый слой (Core → IO,
  Features → App и т. п.), сборка падает со списком всех нарушений. Правило держится
  на проверке, а не только на ревью.
- **Общих списков, которые правят все, нет.** Адаптеры регистрируются сами,
  в своём файле (п. 5.4, [ADR 0004](docs/adr/0004-self-registration-of-adapters.md)),
  а теги объявляются там, где нужны (п. 4.9). Это убирает постоянные merge-конфликты
  при параллельной работе.

## 3. Структура каталогов

```
src/
  main.cpp                          — только вызов App
  App/
    Application                     — аргументы, коды выхода, верхний catch
    Scenario/ScenarioLoader         — парсинг → список шагов с номерами строк
    Scenario/ScenarioValidator      — проход 1: dry-run, сбор всех ошибок
    Scenario/ScenarioRunner         — проход 2: построение мира и запуск симуляции
    Scenario/CommandRegistry        — привязка DTO команды к обработчику
    Registration/Registrar          — механизм саморегистрации адаптеров (п. 5.4)
    Bindings/CoreBindings           — CREATE_MAP, MARCH → API Core
    Bindings/SwordsmanBinding       — io::SpawnSwordsman → SwordsmanSpec
    Bindings/HunterBinding          — io::SpawnHunter → HunterSpec
    Output/EventBridge              — доменное событие → io::Events → EventLog
    Output/CoreEventsOutput         — адаптеры событий Core (меняет владелец Core)
    DiagnosticPrinter               — форматирование ошибок в stderr
  IO/
    Commands/, Events/              — DTO (формат не меняется)
    System/CommandParser            — парсер, собирающий ошибки вместо исключений
    System/ParseNumber              — строгий разбор uint32_t (без operator>>)
    System/EventLog, PrintDebug     — вывод событий (исходный код IO)
  Core/
    Common/       Types (UnitId, Tick, Position, chebyshev), DistanceRange, Narrow,
                  Result, Diagnostic, Tag/TagSet, TypeName
    Map/          Map (размер, разреженная занятость клеток)
    Units/        Unit + UnitBlueprint, IComponent, ComponentStore (индексы правил цели и статусов),
                  CoreComponents (Health, Movement, Status), ComponentRequirements
    Actions/      IAction, MarchStep
    Interaction/  TargetSpec (Intent, Effect, TargetSpec, Hit), ITargetRule, Targeting,
                  Interaction
    World/        World (агрегат), UnitStore, SpatialQuery, ActivityTracker
    Simulation/   Simulation (фазы и условия конца), Scheduler, TurnContext, MovementService
    Events/       EventBus, CoreEvents
    Random/       IRandom, StdRandom
    Setup/        SetupContext (createMap, spawn, orderMarch → Result), SpawnRequest
  Features/
    Common/       CommonTags (melee, ranged, area, physical, flying),
                  TargetSpecs (attack, heal, buff, debuff), Stats, ValueSource (stat<T>,
                  distance), UnitBuilder, Attacks (MeleeAttack, RangedAttack)
    Units/        Swordsman, Hunter, Tower, Healer, Raven, Mine — спецификация, проверка,
                  сборка; способности одного юнита (MinorHeal, Flying, MineFuse) — рядом с ним
tests/            юнит-тесты по слоям; scenarios/ — сценарии с эталонами вывода
cmake/            CheckLayerBoundaries (границы слоёв), RunScenario (сценарные тесты)
docs/adr/         архитектурные решения
```

## 4. Модель Core

### 4.1 Юнит

Юнит — конфигурация, а не подкласс
([ADR 0001](docs/adr/0001-unit-components-and-behaviour.md)). Это:
- `UnitId`, имя типа (`"Swordsman"`), позиция;
- `occupiesCell` — занимает ли юнит клетку (базовое правило задания). По умолчанию
  `true`; Ворон и Мина — `false`;
- `TagSet tags` — свойства юнита, которые объявляют Features (`flying`…). Core
  смысла тегов не знает: их проверяют правила целей;
- набор компонентов (хранилище по `type_index`; `get<T>()`, `tryGet<T>()`, `has<T>()`).
  При добавлении компонент попадает в индекс, если это правило цели (`ITargetRule`)
  или статус, чтобы Core не перебирал все компоненты при каждом отборе целей и в
  каждой фазе Cleanup;
- поведение (`behaviour`) — упорядоченный список `IAction`.

Компоненты Core: `Health` (current, max), `Movement` (speed, очередь маршей), статусы.
Всё остальное — компоненты Features (`Strength`, `Agility`, `Range`, `Power`, `Spirit`…).

Все компоненты Core **необязательны**; отсутствие компонента — это правило, а не ошибка:

| Нет компонента | Пример | Что это значит |
|---|---|---|
| `Health` | Мина | юнит не бывает целью `Damage` / `Heal` и не может умереть; убирается только через `requestRemoval` |
| `Movement` | Башня, Мина | юнит статичен при любых условиях: `MARCH` на него — ошибка валидации `march.unit_cannot_move`; `MovementService` отказывает в любом перемещении, в том числе будущим эффектам вроде отбрасывания; `MarchStep` требует `Movement`, поэтому `UnitBuilder::build` не даст добавить его статичному юниту |
| статусы | большинство юнитов | на юнита ничего не наложено |

Код Core обязан работать с юнитом без любого из этих компонентов (контракт, п. 4.13).

Позиция должна совпадать с занятостью клеток в `Map`. Фичи изменить её не могут: им
доступен только `const Unit&` (п. 4.3), и это проверяет компилятор. Внутри Core позицию
меняет только `MovementService` — это соглашение, и охраняет его владелец Core.
Структурной гарантии (единственный владелец позиции — `Map`) пока нет: второго кода,
который двигает юнитов, в Core нет, а цена переделки — все чтения `unit.position()`.

### 4.2 Мир и его части

`World` — только агрегат, который владеет частями и связывает их. Логики в нём нет.

| Часть | Ответственность |
|---|---|
| `UnitStore` | хранение юнитов, порядок создания, пометки «мёртв» / «на удаление», удаление в Cleanup |
| `Map` | размер, занятость клеток |
| `SpatialQuery` | запросы только для чтения: кто в радиусе, соседи, расстояние; мёртвых по умолчанию не возвращает |
| `Scheduler` | отложенные эффекты по тикам |
| `ActivityTracker` | учёт активности за тик для правила «пустого тика» (п. 4.6) |

Фазы тика выполняет `Simulation`, а не `World`.

У частей мира, `Targeting`, `Interaction` и `Scheduler` нет абстрактных интерфейсов:
реализация у каждого одна, и все они в Core, который меняется редко. Абстракции стоят там, где реализаций
много или они приходят снаружи: `IAction`, `ITargetRule`, `IRandom`.

### 4.3 Действия и контекст хода

```cpp
class IAction {
public:
    virtual ~IAction() = default;
    // true — действие выполнено, ход юнита на этом закончен.
    // false — мир не менялся (контракт, п. 4.13).
    virtual bool tryExecute(const Unit& self, TurnContext& ctx) = 0;
    // Какие компоненты нужны действию; проверяется при сборке юнита.
    virtual void requirements(ComponentRequirements&) const {}
};
```

В фазе Main у юнита выполняется первое действие из `behaviour`, вернувшее `true`.
Если ни одно не выполнилось, юнит в этом тике бездействует.

Действия параметризуются, а не дублируются:
`MeleeAttack(stat<Strength>)`, `MeleeAttack(stat<Agility>)`,
`RangedAttack(stat<Power>, distance(2, 5))`, `RangedAttack(stat<Agility>, distance(2, stat<Range>))`.
`stat<T>` сам объявляет требование компонента `T`.

Значения статов читаются **в момент выполнения действия**, а не при сборке юнита.
`stat<T>` — единственная точка чтения статов, поэтому будущий модификатор стата
(бафф дальности, проклятие силы) встанет туда, а параметры действий менять не придётся.
Модификаторов статов сейчас нет: ни одному юниту из задания и планов они не нужны
(KNOWN_ISSUES.md, п. 12).

**Проверка компонентов при сборке.** `UnitBuilder::build` собирает требования всех
действий и проверяет, что у юнита есть нужные компоненты. Если `MeleeAttack(stat<Strength>)`
добавили юниту без `Strength`, это `std::logic_error` при сборке
(«Hunter: MeleeAttack requires Strength»), а не падение на каком-нибудь 500-м тике.

**`TurnContext` — узкие фасады**, а не доступ ко всему миру:

```cpp
class TurnContext {
public:
    Tick tick() const;
    const SpatialQuery& query() const;          // только чтение
    const Targeting& targeting() const;         // отбор целей с правилами цели
    Interaction& effects();                     // урон, лечение, наложение статуса
    MovementService& movement();                // перемещение с проверкой занятости и событиями
    void schedule(Tick at, UnitId source, Scheduler::Task task);   // отложенный эффект
    void requestRemoval(UnitId);                // удаление не через HP, только юнита без Health
    IRandom& random();
    template <class E> void publish(const E&);  // событие фичи
};
```

- Изменяемый `World` фичам недоступен. Любое изменение мира идёт через фасад,
  поэтому соблюдаются правила (занятость клеток, конвейер взаимодействий) и
  учитывается активность тика.
- `self` передаётся как `const Unit&`: состояние действия (например, «мина
  взведена») хранится в самом объекте действия — у каждого юнита свои экземпляры.

### 4.4 Ход: три фазы

```
Тик N
 ├─ 1. Scheduled — эффекты из Scheduler со сроком N, в порядке постановки (FIFO)
 ├─ 2. Main      — юниты в порядке создания; мёртвые и помеченные на удаление
 │                 пропускаются; выполняется первое подходящее действие
 └─ 3. Cleanup   — удаление мёртвых (HP ≤ 0) и помеченных requestRemoval,
                   снятие истёкших статусов, проверка конца симуляции
```

- Тик 1 — выполнение всех команд сценария. Ходы начинаются с тика 2.
- Порядок фазы Main — снимок порядка создания на начало фазы. Юниты, созданные
  во время тика (на будущее), действуют со следующего тика.
- `Scheduler` — единый механизм отложенных эффектов: «в тик T выполнить эффект от
  источника S». Его потребитель — взрыв Мины. Периодического урона (горения) нет: его
  нельзя сделать без правки Core (KNOWN_ISSUES.md, п. 12).

### 4.5 Смерть и удаление

Как трактуется правило задания о смерти — [ADR 0003](docs/adr/0003-unit-died-immediately.md).

- Когда HP становится ≤ 0, `UNIT_DIED` публикуется **сразу**.
- Мёртвый юнит больше не действует (даже если ещё не ходил в этом тике), не может
  быть целью и не учитывается в запросах «кто рядом».
- Клетку мёртвый занимает до фазы Cleanup, где он убирается с поля.
- `requestRemoval(unit)` — удаление не через HP (мина после взрыва); тоже в фазе Cleanup.
  Только для юнита без `Health`: иначе действие могло бы убрать юнита мимо правил цели
  и без `UNIT_DIED` (контракт, п. 4.13).

### 4.6 Конец симуляции

Почему контракт `IAction` проверяется во время работы и зачем лимит тиков —
[ADR 0009](docs/adr/0009-tick-limit-and-action-contract.md).

Проверяется в фазе Cleanup. Симуляция заканчивается, если выполнено любое условие:
1. на карте осталось ≤ 1 юнита (считаются все юниты, включая мину);
2. тик «пустой» **и** очередь `Scheduler` пуста.
   «Пустой» тик — ни одного выполненного действия, ни одного выполненного
   отложенного эффекта, ни одного удаления. Действие считается выполненным,
   даже если его эффект равен 0.
   Юниты с полным HP целями лечения не становятся: два Лекаря с полным HP не
   находят целей, тик пустой, симуляция завершается;
3. достигнут лимит в 30 000 тиков. В stderr — сообщение об остановке по лимиту,
   код выхода 0 (это не ошибка ввода).

**Кто считает активность.** Только Core, в одном месте — `ActivityTracker`. Фичи
ничего не сообщают сами, поэтому правило не ломается от забывчивости:
- `Simulation` отмечает действие, вернувшее `true`, и выполненный отложенный эффект;
- фаза Cleanup отмечает удаление;
- каждый изменяющий метод фасадов `TurnContext` увеличивает счётчик изменений.

Счётчик изменений заодно проверяет контракт `IAction`: если действие вернуло
`false`, а счётчик вырос, это `std::logic_error`. Проверка включена всегда, а не
только в отладочной сборке: это сравнение двух чисел, отключать её незачем, а
нарушение контракта в релизе тихо ломало бы правило «пустого тика».

### 4.7 Движение и марш

- Марш — базовое правило задания, поэтому `Movement`, очередь маршей, действие
  `MarchStep` и `MovementService` живут в Core
  ([ADR 0006](docs/adr/0006-world-rules-in-core.md): там же — почему перенос марша в Features сделали и
  откатили).
- `Movement { speed; deque<Position> marches; }`. Повторный `MARCH` встаёт в очередь.
- `MARCH_STARTED` публикуется, когда марш становится активным: первый — в тике 1,
  следующие — сразу после `MARCH_ENDED` предыдущего, в том же тике.
- Шаг: в одну из 8 соседних клеток, которая уменьшает расстояние до цели и свободна
  для этого юнита. Для юнита с `occupiesCell` клетка должна быть не занята, в том
  числе мёртвым до уборки; юниты без `occupiesCell` занятость игнорируют.
  - Сначала — прямой шаг `(sign(dx), sign(dy))`, если эта клетка подходит. Так
    движение предсказуемо: юнит идёт к цели по прямой, а не случайным зигзагом.
  - Если прямая клетка не подходит — случайная из остальных подходящих (обход).
  - Если подходящих нет — `MarchStep` возвращает `false`.
- За ход юнит делает до `speed` шагов. На каждую клетку — `UNIT_MOVED`.
- Приход в цель → `MARCH_ENDED`, из очереди берётся следующий марш.
- Порядок «атаковать или идти» в Core не зашит: его задаёт `behaviour` юнита
  (п. 4.3). `MarchStep` — обычное действие, и фича ставит его на любое место в
  списке. У Мечника и Охотника по заданию атака стоит раньше шага; юнит, у которого
  `MarchStep` первый, сначала идёт и атакует только когда идти некуда или марша нет.
- Марш не отменяется, если в этом тике выполнилось другое действие: юнит продолжит
  его в тике, где до `MarchStep` дойдёт очередь.

### 4.8 Карта и клетки

- `Map` — размер и разреженная занятость клеток юнитами с `occupiesCell`.
- Свойств клеток (болото, капкан) нет: ни одному юниту из задания и «Планов» они не
  нужны. Как их добавить, когда появится потребитель, — KNOWN_ISSUES.md, п. 12.

### 4.9 Взаимодействия: таргетинг и применение эффекта

Почему отбор целей и применение воздействий в Core —
[ADR 0005](docs/adr/0005-targeting-and-effects-in-core.md); почему урон и лечение —
закрытый набор операций — [ADR 0006](docs/adr/0006-world-rules-in-core.md).

Любое воздействие на юнита — урон, лечение, статус — описывается `TargetSpec`:

```cpp
enum class Intent { Hostile, Beneficial };   // против цели или в её пользу
enum class Effect { Damage, Heal, Status };  // что меняется у цели

struct TargetSpec {
    Intent intent;   // обязательные поля без значений по умолчанию:
    Effect effect;   // автор действия обязан выбрать оба
    TagSet tags;     // способ доставки + тип воздействия
    DistanceRange range;
};
```

| Ось | Значения | Кто проверяет |
|---|---|---|
| намерение `Intent` (Core) | `Hostile`, `Beneficial` | правила цели: «ближняя атака запрещена» |
| операция `Effect` (Core) | `Damage`, `Heal`, `Status` | `Targeting` (для `Damage` и `Heal` нужен `Health`) |
| способ доставки (теги) | `melee`, `ranged`, `area` | правила цели: иммунитет Ворона к ближнему бою |
| тип воздействия (теги) | `physical`, `magic`, `fire`, `explosion`, `heal` | правила цели: иммунитеты (запрет `debuff` с тегом `fire`) |

- `Intent` и `Effect` — enum в Core: значений мало, на них держатся правила.
  Ось `Intent` не растёт; новая операция (сбивание баффа, отбрасывание) — новое
  значение `Effect`, и правила, проверяющие `Intent`, его не замечают.
- Разработчики фич пользуются именованными конструкторами из
  `Features/Common/TargetSpecs` — те же четыре слова, но оси остаются раздельными:

  ```cpp
  TargetSpec attack(TagSet tags, DistanceRange r) { return {Intent::Hostile,    Effect::Damage, tags, r}; }
  TargetSpec heal  (TagSet tags, DistanceRange r) { return {Intent::Beneficial, Effect::Heal,   tags, r}; }
  TargetSpec buff  (TagSet tags, DistanceRange r) { return {Intent::Beneficial, Effect::Status, tags, r}; }
  TargetSpec debuff(TagSet tags, DistanceRange r) { return {Intent::Hostile,    Effect::Status, tags, r}; }
  ```

Теги Core не знает — у него есть только механизм `Tag`/`TagSet`; теги объявляют Features.

- Тег интернируется по строковому имени: `inline const Tag Fire{"fire"};`.
  Одинаковое имя в разных фичах — один и тот же тег, поэтому общего реестра
  тегов нет.
- Базовые теги, которыми пользуются почти все (`melee`, `ranged`, `area`,
  `physical`, `flying`), лежат в `Features/Common/CommonTags` и меняются редко.
  Тег конкретной механики (`fire`, `explosion`, `heal`) объявляется в файле фичи,
  которой он нужен.

Конвейер:

```
Действие (Features)
  │ 1. Отбор: ctx.targeting().select(source, spec)      // spec = attack(...), heal(...)…
  │      └─ правила цели (ITargetRule): разрешить / запретить / изменить дистанцию
  │ 2. Применение: ctx.effects().apply(Hit{spec, source, target, amount})
  │ 3. Результат: изменение Health + событие (UnitAttacked / UnitHealed)
  │ 4. Вторичные эффекты: ctx.effects().applyStatus(self, *target, debuff({...}, …), std::make_unique<Статус>(…))
```

```cpp
class ITargetRule {
public:
    virtual ~ITargetRule() = default;
    // nullopt — эту цель таким способом выбрать нельзя
    virtual std::optional<DistanceRange> adjust(const TargetSpec& incoming, DistanceRange range) const = 0;
};
```

Пример — правило цели Ворона:

```cpp
std::optional<DistanceRange> Flying::adjust(const TargetSpec& in, DistanceRange r) const {
    if (in.intent != Intent::Hostile) return r;             // лечение и баффы — без изменений
    if (in.tags.has(tags::Melee))     return std::nullopt;  // ближняя атака запрещена
    if (in.tags.has(tags::Ranged))    return r.shifted(-1, -1); // обе границы: {2,R} → {1,R-1}
    return r;                                                   // взрыв (area) — без изменений
}
```

- `Hit` берёт `Intent` и `Effect` из своего `TargetSpec`; отдельного поля «вид удара»
  нет, поэтому две записи одного и того же не разойдутся.
- Правила берутся из компонентов и статусов **цели**. Поэтому новый иммунитет
  добавляется в цель, а код атакующих не меняется.
- Юнит без `Health` целью `Damage` и `Heal` не бывает: `Targeting` отфильтровывает
  его сам. Поэтому мина «не может быть атакована» без отдельного правила.
- `apply` и `applyStatus` проверяют цель тем же `canTarget`, что и `Targeting`.
  Обойти правила цели, собрав `Hit` в обход отбора, нельзя: `apply` на такой цели —
  `std::logic_error` (это баг действия), `applyStatus` — `false` (правила цели могут
  запретить статус, это нормальный исход).
- `UNIT_ATTACKED` содержит урон и HP цели после удара (не меньше 0).
- Модификаторов удара (щит, резист, броня) нет: юнитам из задания и «Планов» они не
  нужны. Это осознанное решение, а не недосмотр — KNOWN_ISSUES.md, п. 12.

### 4.10 Статусы

Статус — компонент юнита со сроком жизни или бессрочный (флаг, бафф, иммунитет).
Это точка расширения для механик, которые меняют не HP, а состояние юнита. Статус может
реализовывать `ITargetRule` — запретить выбирать юнита целью или изменить дистанцию.
Класс статуса объявляется `final`: хранилище ищет компонент по точному типу (п. 4.13).

Статус накладывается через `ctx.effects().applyStatus(source, target, spec, status)`, где
`spec` — `buff(...)` или `debuff(...)`. Поэтому правила цели могут его запретить
(иммунитет — запрет `debuff` с нужным тегом). Истёкшие статусы снимаются в фазе Cleanup.

Чего статусом пока не сделать — ни одному юниту из задания и «Планов» это не нужно
(KNOWN_ISSUES.md, п. 12):
- периодический урон (горение): урон идёт через `Interaction::apply`, а он требует,
  чтобы источник был в мире и цель была в пределах его дистанции;
- ограничение по сроку, от которого зависит конец симуляции (невидимость, оглушение):
  снятие статуса не считается активностью, и симуляция может остановиться «пустым
  тиком» раньше, чем статус спадёт. Оглушению вдобавок нужна проверка перед ходом —
  в Core её нет.

Статус каждого типа на юните один: повторное наложение заменяет старый экземпляр
новым (SPECIFICATION.md, п. 5.3). Продление срока и стаки появятся вместе с первой
механикой, которой они нужны.

### 4.11 События

- `EventBus` в Core не знает конкретных типов событий: `publish<E>(const E&)`,
  подписчики по типу.
- События Core: `MapCreated`, `UnitSpawned`, `UnitMoved`, `MarchStarted`,
  `MarchEnded`, `UnitAttacked`, `UnitHealed`, `UnitDied`.
- Features при необходимости объявляют свои события и публикуют их через
  `ctx.publish(event)`.
- Перевод доменного события в `io::Events` — адаптер в `App/Output`, один файл на
  событие фичи. События Core из исходного формата — в одном файле `CoreEventsOutput`,
  его меняет владелец Core. Событие Core, которое попадает в вывод вместе с фичей
  (`UnitHealed` с Лекарем), получает отдельный файл этой фичи (`UnitHealedOutput.cpp`):
  разработчик Лекаря не ждёт владельца Core. Адаптер регистрируется сам
  (`SW_REGISTER_OUTPUT`, п. 5.4).
- `App/Output/EventBridge` подписывается на `EventBus` через зарегистрированные
  адаптеры и передаёт результат в `EventLog` вместе с номером тика. Два адаптера
  для одного типа события — ошибка при старте.
- `EventLog` пишет в `std::cout`, как в исходном коде IO.

### 4.12 Случайность

`IRandom` передаётся в Core снаружи. В приложении — `StdRandom` с seed от
`std::random_device`, в тестах — фиксированный seed или заглушка.

### 4.13 Контракты интерфейсов расширения

Реализация интерфейса обязана соблюдать его контракт — тогда любую реализацию
можно подставить без поломки системы.

| Интерфейс | Контракт |
|---|---|
| `IAction::tryExecute` | `false` — не вызывал изменяющих методов `TurnContext` (проверяется всегда, п. 4.6). `true` — действие выполнено, даже если эффект равен 0 |
| `IAction::requirements` | перечисляет все компоненты, которые действие читает у `self`; проверяется в `UnitBuilder::build` |
| `ITargetRule::adjust` | чистая функция без побочных эффектов. Явно проверяет нужную ось: правило про атаки проверяет `intent == Hostile`, а не только теги доставки. Правила цели применяются по очереди в порядке добавления, каждое получает дистанцию после предыдущего; `nullopt` любого правила — запрет |
| `Interaction::apply` | цель проходит `Targeting::canTarget` для этого `spec` (правила цели, дистанция, `Health`), иначе `std::logic_error` |
| задача `Scheduler` | захватывает только значения и `UnitId`, а не указатели и ссылки на юнитов, действия и статусы: к сроку задачи их могли удалить или заменить. Юнитов находит через `ctx.query().find()` и сама обрабатывает их отсутствие. Задачи не отменяются, удаление источника их не снимает |
| юнит без `Health` | не бывает целью `Effect::Damage` / `Effect::Heal`; любой код Core обязан работать с таким юнитом |
| компонент, статус | хранится по точному динамическому типу; класс объявляется `final`, иначе наследник не виден через `has<База>()` |
| `TurnContext::requestRemoval` | только для юнита без `Health`. Юнит с `Health` уходит с поля только через смерть, то есть через отбор целей, правила цели и `UNIT_DIED`; `requestRemoval` на нём — `std::logic_error` |
| юнит без `Movement` | никогда не меняет позицию: `MovementService` отказывает в перемещении, кто бы его ни запросил |

## 5. Features

### 5.1 Юниты из задания

Код здесь не копируется, чтобы не расходиться с ним; образец — Охотник:

- [Features/Units/Hunter.hpp](src/Features/Units/Hunter.hpp) — `HunterSpec`: id, позиция
  и статы в `int64_t` (в домене числа знаковые, п. 2). Про IO фича ничего не знает.
- [Features/Units/Hunter.cpp](src/Features/Units/Hunter.cpp) — `makeHunter`: проверяет
  статы > 0 и `range >= 2` и возвращает **все** найденные ошибки без номеров строк (их
  добавляет App). При ошибках — `builder.reject(errors)`: заявка на спавн всё равно
  несёт id, позицию и занятость клетки, и `SetupContext` проверит их тоже. Без ошибок —
  сборка через `UnitBuilder`: `Health`, `Agility`, `Strength`, `Range`, `Movement(1)`,
  behaviour = [`RangedAttack(stat<Agility>, distance(2, stat<Range>), RequireNoAdjacentOccupying{})`,
  `MeleeAttack(stat<Strength>)`, `MarchStep`]; `spawnRequest()` проверяет требования действий.
- [App/Bindings/HunterBinding.cpp](src/App/Bindings/HunterBinding.cpp) — единственное
  место, где встречаются IO и фича: `io::SpawnHunter` → `HunterSpec` → `setup.spawn(...)`,
  в конце файла `SW_REGISTER_BINDING(...)`.

Мечник собирается так же: `SwordsmanSpec` → `makeSwordsman`, behaviour =
[`MeleeAttack(stat<Strength>)`, `MarchStep`].

Проверка спецификации и сборка юнита — в одной функции `makeX`: причина изменения
у них одна — «что такое этот юнит», а разделение дало бы два файла на юнит без выгоды.

- `MeleeAttack` — `attack({melee, physical}, дистанция)`, по умолчанию `{1,1}`; Ворону —
  `distance(0, 1)` (параметр, а не флаг: src/Features/README.md).
- `RangedAttack` — `attack({ranged, physical}, дистанция)`; дистанция вычисляется
  при каждом выстреле из `distance(min, max)`, где граница — число или `stat<T>`.
- `RequireNoAdjacentOccupying` — выстрел запрещён, если в соседних клетках есть
  живой юнит с `occupiesCell`. Летающие, мины и мёртвые (до уборки) не мешают.

### 5.2 Как реализованы юниты из планов

Все четыре юнита сделаны новыми файлами по п. 5.3, `src/Core` для них не менялся; из
существующих файлов изменён только `Features/Common/Attacks` (дистанция у `MeleeAttack`).
Поведение, команды и критерии приёмки — [docs/units](docs/units/README.md), задачи —
[docs/stage3](docs/stage3/README.md).

| Юнит | Файлы | Сборка (всё в Features) |
|---|---|---|
| **Башня** | [Tower.cpp](src/Features/Units/Tower.cpp), `SpawnTower`, `TowerBinding` | `Health`, `Power`, без `Movement`; `RangedAttack(stat<Power>, distance(2, 5))`. `MARCH` на неё — ошибка валидации |
| **Лекарь** | [Healer.cpp](src/Features/Units/Healer.cpp), `SpawnHealer`, `HealerBinding`, `UnitHealed` + `UnitHealedOutput` | `Health`, `Spirit`, `Movement(1)`; `MinorHeal(stat<Spirit>, distance(0, 2))` с `heal({heal}, …)` и фильтром «HP < max», `MarchStep` |
| **Ворон** | [Raven.cpp](src/Features/Units/Raven.cpp), `SpawnRaven`, `RavenBinding` | `occupiesCell(false)`, тег `flying`, `Health`, `Agility`, `Movement(2)`, компонент `Flying : ITargetRule` (только для `Hostile`: `melee` → запрет, `ranged` → дистанция −1; п. 4.9); `MeleeAttack(stat<Agility>, distance(0, 1))`, `MarchStep` |
| **Мина** | [Mine.cpp](src/Features/Units/Mine.cpp), `SpawnMine`, `MineBinding`, `MineExploded` + `MineExplodedOutput` | `occupiesCell(false)`, `Power`, без `Health` (поэтому не бывает целью, п. 4.9) и без `Movement` (`MARCH` — ошибка валидации); `MineFuse(stat<Power>)` |

Способности одного юнита (`MinorHeal`, `Flying`, `MineFuse`) лежат рядом с ним, в
`Features/Units`; в `Features/Common` их переносят, когда понадобятся второму юниту.

**Мина по тикам.**
- Тик N, Main: `MineFuse` через `ctx.query()` видит юнита с `Health` на расстоянии 0…2
  (включая летающих; другие мины не в счёт — взрыв их не ранит) → мина взводится и
  ставит взрыв через `ctx.schedule(...)` на тик N+1. Задача захватывает id мины и копию
  `stat<Power>`, а не указатель на мину или на действие (контракт, п. 4.13). Это
  выполненное действие. Взведённая мина больше не действует.
- Тик N+1, Scheduled: взрыв. `MINE_EXPLODED` (событие фичи через `ctx.publish`) →
  `targeting().select(mine, attack({area, explosion}, {0,3}))` → `effects().apply` каждой
  цели в порядке создания (включая летающих: правило `Flying` не трогает `area`) →
  `UNIT_ATTACKED` с attacker = id мины → `requestRemoval(mine)`. Погибшие получают
  `UNIT_DIED` сразу и в Main не ходят.
- Тик N+1, Cleanup: мина и погибшие удаляются.
- Пока взрыв стоит в очереди, симуляция не может закончиться (п. 4.6).

**Лекарь.** Цели — живые юниты с `Health` и HP < MaxHp в радиусе 2, кроме самого
Лекаря. Случайная цель → `effects().apply(Hit{heal({heal}, {0,2}), self, target, spirit})`.
Дистанция 0 возможна: на клетке Лекаря может оказаться юнит, не занимающий клетку.
Правило `Flying` лечение не трогает: у него `Intent::Beneficial`. HP ограничено
MaxHp (равно HP при спавне), в `UnitHealed` — фактически восстановленное HP. Если целей
нет — `MarchStep`. Событие Core `UnitHealed` выводит отдельный адаптер
`App/Output/UnitHealedOutput.cpp` (п. 4.11); `CoreEventsOutput` не менялся.

**Ворон.** «Удар когтем» бьёт юнита в соседней клетке или на своей: Ворон не занимает
клетку и может стоять на клетке цели, а `{1,1}` её бы не достал. Поэтому дистанция
`{0,1}` — параметр `MeleeAttack`. Удар когтем — ближний бой, поэтому Ворон не бьёт Ворона.

### 5.3 Как добавить новый юнит

1. `IO/Commands/SpawnX.hpp` — DTO команды (формат ввода).
2. `Features/Units/X.{hpp,cpp}` — `XSpec`, его проверка и сборка юнита
   (`SpawnRequest makeX(const XSpec&)`); при необходимости новые статы, действия,
   теги, статусы в `Features/Common` или рядом с юнитом.
3. `App/Bindings/XBinding.cpp` — перевод `io::SpawnX` → `XSpec`, без логики,
   в конце файла `SW_REGISTER_BINDING(bindX)`.
4. Если нужен новый вид события в выводе — DTO в `IO/Events` и
   `App/Output/XEventOutput.cpp` с `SW_REGISTER_OUTPUT(...)`.

Core не меняется, общие файлы не меняются — все правки в новых файлах.

### 5.4 Механизм саморегистрации

```cpp
// App/Registration/Registrar.hpp
using BindingFn = void (*)(CommandRegistry&);
using OutputFn  = void (*)(EventBridge&);

// Статические переменные внутри функций (Meyers singleton): список создаётся
// при первом обращении, поэтому порядок инициализации между файлами не важен.
std::vector<BindingFn>& bindingList();
std::vector<OutputFn>&  outputList();

struct BindingRegistrar { explicit BindingRegistrar(BindingFn fn); };   // bindingList().push_back
struct OutputRegistrar  { explicit OutputRegistrar(OutputFn fn); };     // outputList().push_back

// [[maybe_unused]]: переменную никто не читает, работа — в конструкторе. Без пометки
// анализаторы IDE предлагают удалить и её, и функцию привязки.
#define SW_REGISTER_BINDING(fn) \
    namespace { [[maybe_unused]] const ::sw::app::BindingRegistrar SW_APP_CONCAT(swBindingRegistrar_, __LINE__){&(fn)}; }
#define SW_REGISTER_OUTPUT(fn) \
    namespace { [[maybe_unused]] const ::sw::app::OutputRegistrar SW_APP_CONCAT(swOutputRegistrar_, __LINE__){&(fn)}; }
```

```cpp
// App/Application.cpp и App/Scenario/ScenarioRunner.cpp
CommandRegistry registry;
for (const BindingFn bind : _options.bindings) bind(registry);   // по умолчанию bindingList()
EventBridge bridge(bus, log, clock);
for (const OutputFn bind : outputs) bind(bridge);               // по умолчанию outputList()
```

Почему саморегистрация, а не общий список, и как закрыты её риски (порядок
инициализации статиков, выброс файлов линкером, дубли регистраций) —
[ADR 0004](docs/adr/0004-self-registration-of-adapters.md).

## 6. Загрузка, валидация, ошибки

### 6.1 Конвейер

1. **Парсинг (IO).** Построчно, на пользовательском вводе без исключений. Каждая
   строка → DTO команды с номером строки или ошибка: неизвестная команда, не хватает
   аргументов, лишние аргументы, не число, отрицательное число, переполнение.
   `App/Scenario/CommandRegistry` связывает каждый DTO с его адаптером из
   `App/Bindings` — получается шаг сценария `{line, command DTO, handler}`.
2. **Проход 1 — проверка (App + Core + Features).** Шаги выполняются на отдельном
   мире с пустым приёмником событий. Каждый обработчик возвращает `Result`; при
   ошибке она записывается и обработка **продолжается**, чтобы найти все ошибки.
   Core и Features возвращают ошибки без привязки к файлу (код + сообщение);
   App добавляет к каждой файл, номер строки и имя команды.
3. **Итог.** Есть ошибки → полный список в stderr, stdout пустой, код выхода 2.
4. **Проход 2 — запуск.** Мир строится заново теми же шагами, уже с выводом событий
   (тик 1), затем симуляция.

Два прохода вместо буферизации событий: проще, а установка не использует
случайность, поэтому оба прохода дают одинаковый мир
([ADR 0008](docs/adr/0008-whole-scenario-validation.md)).

### 6.2 Что проверяется

| Код | Условие |
|---|---|
| `parse.unknown_command` | неизвестная команда |
| `parse.missing_argument`, `parse.extra_argument` | неверное число аргументов |
| `parse.invalid_number` | не число, отрицательное, переполнение |
| `map.not_created` | команда до `CREATE_MAP` (сообщается один раз) |
| `map.already_created` | повторный `CREATE_MAP` |
| `map.invalid_size` | W или H равны 0 |
| `unit.duplicate_id` | id уже занят (с указанием строки первого спавна) |
| `unit.out_of_bounds` | точка спавна вне карты |
| `unit.cell_occupied` | новый юнит занимает клетку, а она уже занята другим юнитом с `occupiesCell` |
| `unit.invalid_stat` | HP или любой стат = 0 |
| `hunter.invalid_range` | `range < 2` |
| `march.unit_not_found` | юнит не существует (в том числе `MARCH` до `SPAWN`) |
| `march.unit_cannot_move` | у юнита нет `Movement` |
| `march.out_of_bounds` | цель вне карты |
| `march.same_position` | цель совпадает с точкой, где юнит окажется к этому маршу (для очереди — конец предыдущего марша) |

Коды — строки с пространством имён, а не общий enum: фичи добавляют свои коды,
не трогая Core. Предупреждений нет — только ошибки.

### 6.3 Формат вывода ошибок

```
error: scenario.txt:7: SPAWN_HUNTER [unit.cell_occupied]: cell (9,0) is occupied by unit 2 (line 5)
error: scenario.txt:9: MARCH [march.unit_not_found]: unit 42 does not exist
error: scenario.txt:11: SPAWN_HUNTER [hunter.invalid_range]: range must be >= 2, got 1
3 errors, simulation not started.
```

### 6.4 Политика ошибок

| Вид | Обработка | Код выхода |
|---|---|---|
| ошибки ввода сценария | `Diagnostic` / `Result`, полный список | 2 |
| окружение: нет аргумента, файл не открылся | сообщение в stderr | 1 |
| нарушенный инвариант или контракт (баг) | исключение `std::logic_error` с контекстом, ловится в `Application` | 3 |
| нормальное завершение, в том числе по лимиту тиков | — | 0 |

## 7. Архитектурные решения

Решения с контекстом, рассмотренными вариантами и последствиями — в
[docs/adr](docs/adr/README.md):

| ADR | Решение | Где в этом документе |
|---|---|---|
| [0001](docs/adr/0001-unit-components-and-behaviour.md) | юнит — компоненты и список действий по приоритету (против наследования и ECS) | 4.1, 4.3 |
| [0002](docs/adr/0002-core-independent-of-io.md) | Core не зависит от IO, адаптеры в App | 2, 4.11 |
| [0003](docs/adr/0003-unit-died-immediately.md) | `UNIT_DIED` сразу, мёртвый не действует, удаление в Cleanup | 4.5 |
| [0004](docs/adr/0004-self-registration-of-adapters.md) | саморегистрация адаптеров | 5.4 |
| [0005](docs/adr/0005-targeting-and-effects-in-core.md) | отбор целей и применение воздействий в Core; оси `Intent` и `Effect` | 4.9 |
| [0006](docs/adr/0006-world-rules-in-core.md) | правила мира из задания — HP со смертью и марш — в Core; урон и лечение — закрытый набор | 1, 4.1, 4.7, 4.9 |
| [0007](docs/adr/0007-no-mechanisms-without-consumers.md) | механизм в Core — только с потребителем | 1, 4.8, 4.10 |
| [0008](docs/adr/0008-whole-scenario-validation.md) | проверка сценария целиком | 6 |
| [0009](docs/adr/0009-tick-limit-and-action-contract.md) | лимит тиков и проверка контракта `IAction` | 4.6, 4.13 |

## 8. Принятые решения по правилам

Решения о поведении, которые не тянут на отдельный ADR. Подробности и граничные
случаи — в [SPECIFICATION.md](SPECIFICATION.md), раздел 0.

- Команды — в тике 1, ходы — с тика 2.
- Шаг марша обходит занятую клетку через другую соседнюю, приближающую к цели;
  если такой нет — стоит. Повторный `MARCH` — в очередь.
- Конец: ≤ 1 юнита, или пустой тик при пустом `Scheduler`, или 30 000 тиков.
- Расстояние — Чебышёв. Лог из исходного `main.cpp` (его вывод сохранён в
  `docs/original_main_output.txt`) иллюстративный (на тиках 7–8 выстрел
  на дистанцию 5 при `range 4`).
- Мина считается юнитом; взводится только от юнитов с `Health` (в том числе летающих,
  но не от других мин), взрыв задевает летающих.
- Удар когтем Ворона достаёт и до своей клетки (`{0,1}`).
- Лекарь лечит только юнитов с неполным HP и себя не лечит.
- Выстрелу Охотника мешают только живые юниты, занимающие клетки; мёртвый сосед
  до уборки не мешает.
- Ограничения Ворона действуют только на атаки: правила цели проверяют
  `Intent::Hostile`, поэтому лечение и баффы Ворона не затрагивают.

## 9. Открытые вопросы

Нет. Все вопросы закрыты: решения — в разделе 8 и в [docs/adr](docs/adr/README.md).
