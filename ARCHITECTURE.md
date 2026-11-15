# Архитектура симуляции

Статус: **черновик на утверждение**. Код по этому документу ещё не написан.

## 1. Цели и ограничения

- Core принадлежит одному владельцу, Features пишут много разработчиков параллельно.
  Новый юнит, действие, характеристика или эффект добавляются **без правок Core**.
- В Core — механизмы и базовые правила из задания, а не контент. Каждый механизм
  нужен хотя бы одному юниту из задания или из планов; «универсального движка» нет.
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
- **Общих списков, которые правят все, нет.** Адаптеры регистрируются сами,
  в своём файле (п. 5.4), а теги объявляются там, где нужны (п. 4.9). Это
  убирает постоянные merge-конфликты при параллельной работе.

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
    System/EventLog
  Core/
    Common/       Types (UnitId, Tick, Position), Distance (Чебышёв), Range,
                  Result, Diagnostic, Tag/TagSet
    Map/          Map (размер, занятость клеток), IMoveCostModifier, IEnterTrigger
    Units/        Unit, хранилище компонентов, Health, Movement, Status,
                  ComponentRequirements
    Actions/      IAction, Behaviour (список по приоритету), MarchStep
    Interaction/  Intent, Effect, TargetSpec, ITargetRule, Targeting, Hit,
                  IHitModifier, Interaction
    World/        World (агрегат), UnitStore, SpatialQuery, ActivityTracker
    Simulation/   Simulation (фазы), Scheduler, TurnContext, Termination
    Events/       EventBus, CoreEvents
    Random/       IRandom, StdRandom
    Setup/        SetupContext (createMap, spawn, orderMarch → Result)
  Features/
    Common/       CommonTags (melee, ranged, area, physical, flying),
                  TargetSpecs (attack, heal, buff, debuff), Stats,
                  Actions (MeleeAttack, RangedAttack), UnitBuilder
    Units/        Swordsman, Hunter — спецификация, проверка, сборка
```

## 4. Модель Core

### 4.1 Юнит

Юнит — это:
- `UnitId`, имя типа (`"Swordsman"`), позиция;
- `occupiesCell` — занимает ли юнит клетку (базовое правило задания). По умолчанию
  `true`; Ворон и Мина — `false`;
- `TagSet tags` — свойства юнита, которые объявляют Features (`flying`…). Core
  смысла тегов не знает: их проверяют правила целей и свойства клеток;
- набор компонентов (хранилище по `type_index`; `get<T>()`, `tryGet<T>()`, `has<T>()`).
  При добавлении компонент индексируется по интерфейсам-хукам (`ITargetRule`,
  `IHitModifier`), чтобы Core не перебирал все компоненты при каждом ударе;
- `Behaviour` — упорядоченный список `IAction`.

Компоненты Core: `Health` (current, max), `Movement` (speed, очередь маршей), статусы.
Всё остальное — компоненты Features (`Strength`, `Agility`, `Range`, `Power`, `Spirit`…).

Все компоненты Core **необязательны**; отсутствие компонента — это правило, а не ошибка:

| Нет компонента | Пример | Что это значит |
|---|---|---|
| `Health` | Мина | юнит не бывает целью `Damage` / `Heal` и не может умереть; убирается только через `requestRemoval` |
| `Movement` | Башня, Мина | юнит статичен при любых условиях: `MARCH` на него — ошибка валидации `march.unit_cannot_move`; `MovementService` отказывает в любом перемещении, в том числе будущим эффектам вроде отбрасывания; `MarchStep` требует `Movement`, поэтому `UnitBuilder::build` не даст добавить его статичному юниту |
| статусы | большинство юнитов | на юнита ничего не наложено |

Код Core обязан работать с юнитом без любого из этих компонентов (контракт, п. 4.13).

Позицию и `occupiesCell` нельзя менять напрямую: позиция меняется только через
`movement()` (п. 4.3), чтобы занятость клеток в `Map` всегда была согласована.

### 4.2 Мир и его части

`World` — только агрегат, который владеет частями и связывает их. Логики в нём нет.

| Часть | Ответственность |
|---|---|
| `UnitStore` | хранение юнитов, порядок создания, пометки «мёртв» / «на удаление», удаление в Cleanup |
| `Map` | размер, занятость клеток, свойства клеток |
| `SpatialQuery` | запросы только для чтения: кто в радиусе, соседи, расстояние; мёртвых по умолчанию не возвращает |
| `Scheduler` | отложенные эффекты по тикам |
| `ActivityTracker` | учёт активности за тик для правила «пустого тика» (п. 4.6) |

Фазы тика выполняет `Simulation`, а не `World`.

У частей мира, `Targeting`, `Interaction` и `Scheduler` нет абстрактных интерфейсов:
реализация у каждого одна, и все они в Core, который меняется редко. Абстракции стоят там, где реализаций
много или они приходят снаружи: `IAction`, `ITargetRule`, `IHitModifier`, свойства клеток, `IRandom`.

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

В фазе Main у юнита выполняется первое действие из `Behaviour`, вернувшее `true`.
Если ни одно не выполнилось, юнит в этом тике бездействует.

Действия параметризуются, а не дублируются:
`MeleeAttack(stat<Strength>)`, `MeleeAttack(stat<Agility>)`, `RangedAttack(stat<Power>, {2,5})`.
`stat<T>` сам объявляет требование компонента `T`.

**Проверка компонентов при сборке.** `UnitBuilder::build` собирает требования всех
действий и проверяет, что у юнита есть нужные компоненты. Если `MeleeAttack(stat<Strength>)`
добавили юниту без `Strength`, это `std::logic_error` при сборке
(«Hunter: MeleeAttack requires Strength»), а не падение на каком-нибудь 500-м тике.

**`TurnContext` — узкие фасады**, а не доступ ко всему миру:

```cpp
class TurnContext {
public:
    Tick tick() const;
    const SpatialQuery& query() const; // только чтение
    Targeting& targeting();            // отбор целей с правилами цели
    Interaction& effects();            // урон, лечение, наложение статуса
    MovementService& movement();       // перемещение с проверкой занятости и событиями
    Scheduler& scheduler();            // отложенные эффекты
    void requestRemoval(UnitId);       // удаление не через HP
    IRandom& random();
    EventPublisher& events();          // публикация событий фич
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
  источника S». Периодические эффекты (горение) сами ставят себя на следующий тик.

### 4.5 Смерть и удаление

- Когда HP становится ≤ 0, `UNIT_DIED` публикуется **сразу**.
- Мёртвый юнит больше не действует (даже если ещё не ходил в этом тике), не может
  быть целью и не учитывается в запросах «кто рядом».
- Клетку мёртвый занимает до фазы Cleanup, где он убирается с поля.
- `requestRemoval(unit)` — удаление не через HP (мина после взрыва); тоже в фазе Cleanup.

### 4.6 Конец симуляции

Проверяется в фазе Cleanup. Симуляция заканчивается, если выполнено любое условие:
1. на карте осталось ≤ 1 юнита (считаются все юниты, включая мину);
2. тик «пустой» **и** очередь `Scheduler` пуста.
   «Пустой» тик — ни одного выполненного действия, ни одного выполненного
   отложенного эффекта, ни одного удаления. Действие считается выполненным,
   даже если его эффект равен 0 (лечение под запретом лечения).
   Юниты с полным HP целями лечения не становятся: два Лекаря с полным HP не
   находят целей, тик пустой, симуляция завершается. Бесконечное лечение на 0
   возможно только с раненым юнитом под запретом лечения в радиусе Лекаря —
   такой случай останавливает лимит тиков;
3. достигнут лимит в 30 000 тиков. В stderr — сообщение об остановке по лимиту,
   код выхода 0 (это не ошибка ввода).

**Кто считает активность.** Только Core, в одном месте — `ActivityTracker`. Фичи
ничего не сообщают сами, поэтому правило не ломается от забывчивости:
- `Simulation` отмечает действие, вернувшее `true`, и выполненный отложенный эффект;
- фаза Cleanup отмечает удаление;
- каждый изменяющий метод фасадов `TurnContext` увеличивает счётчик изменений.

Счётчик изменений заодно проверяет контракт `IAction`: если действие вернуло
`false`, а счётчик вырос, в отладочной сборке это `std::logic_error`.

### 4.7 Движение и марш

- Марш — базовое правило задания, поэтому `Movement`, очередь маршей, действие
  `MarchStep` и `MovementService` живут в Core.
- `Movement { speed; deque<Position> marches; }`. Повторный `MARCH` встаёт в очередь.
- `MARCH_STARTED` публикуется, когда марш становится активным: первый — в тике 1,
  следующие — сразу после `MARCH_ENDED` предыдущего, в том же тике.
- Шаг: в любую из 8 соседних клеток, которая уменьшает расстояние до цели и свободна
  для этого юнита. Для юнита с `occupiesCell` клетка должна быть не занята, в том
  числе мёртвым до уборки; юниты без `occupiesCell` занятость игнорируют. Если
  подходящих клеток несколько — случайная. Если нет — `MarchStep` возвращает `false`.
- За ход юнит тратит бюджет `speed`; стоимость входа в клетку дают
  `IMoveCostModifier` клетки (по умолчанию 1). Первый шаг за ход разрешён всегда,
  чтобы дорогая клетка не заблокировала юнита навсегда. На каждую клетку — `UNIT_MOVED`.
- Приход в цель → `MARCH_ENDED`, из очереди берётся следующий марш.
- Порядок «атаковать или идти» в Core не зашит: его задаёт `Behaviour` юнита
  (п. 4.3). `MarchStep` — обычное действие, и фича ставит его на любое место в
  списке. У Мечника и Охотника по заданию атака стоит раньше шага; юнит, у которого
  `MarchStep` первый, сначала идёт и атакует только когда идти некуда или марша нет.
- Марш не отменяется, если в этом тике выполнилось другое действие: юнит продолжит
  его в тике, где до `MarchStep` дойдёт очередь.

### 4.8 Карта и клетки

- `Map` — размер, занятость клеток юнитами с `occupiesCell`, свойства клеток.
- Свойство клетки реализует один или оба интерфейса:

  ```cpp
  class IMoveCostModifier {   // болото, ускорение
  public:
      virtual ~IMoveCostModifier() = default;
      virtual int moveCost(const Unit& unit, int cost) const = 0;
  };

  class IEnterTrigger {       // капкан
  public:
      virtual ~IEnterTrigger() = default;
      virtual void onEnter(const Unit& unit, TurnContext& ctx) = 0;
  };
  ```

  При добавлении свойства `Map` раскладывает его по спискам этих интерфейсов.
- Хуки получают юнита, поэтому летающих можно отсекать по тегу `flying`.
- В Core только механизм и реализация по умолчанию; конкретных клеток пока нет.

### 4.9 Взаимодействия: таргетинг и применение эффекта

Любое воздействие на юнита — урон, лечение, статус — описывается `TargetSpec`:

```cpp
enum class Intent { Hostile, Beneficial };   // против цели или в её пользу
enum class Effect { Damage, Heal, Status };  // что меняется у цели

struct TargetSpec {
    Intent intent;   // обязательные поля без значений по умолчанию:
    Effect effect;   // автор действия обязан выбрать оба
    TagSet tags;     // способ доставки + тип воздействия
    Range  range;
};
```

| Ось | Значения | Кто проверяет |
|---|---|---|
| намерение `Intent` (Core) | `Hostile`, `Beneficial` | правила цели: «ближняя атака запрещена» |
| операция `Effect` (Core) | `Damage`, `Heal`, `Status` | `Targeting` (для `Damage` и `Heal` нужен `Health`); модификаторы: «запрет лечения» |
| способ доставки (теги) | `melee`, `ranged`, `area` | правила цели: иммунитет Ворона к ближнему бою |
| тип воздействия (теги) | `physical`, `magic`, `fire`, `explosion`, `heal` | модификаторы: щиты, резисты, иммунитеты |

- `Intent` и `Effect` — enum в Core: значений мало, на них держатся правила.
  Ось `Intent` не растёт; новая операция (сбивание баффа, отбрасывание) — новое
  значение `Effect`, и правила, проверяющие `Intent`, его не замечают.
- Разработчики фич пользуются именованными конструкторами из
  `Features/Common/TargetSpecs` — те же четыре слова, но оси остаются раздельными:

  ```cpp
  TargetSpec attack(TagSet tags, Range r) { return {Intent::Hostile,    Effect::Damage, tags, r}; }
  TargetSpec heal  (TagSet tags, Range r) { return {Intent::Beneficial, Effect::Heal,   tags, r}; }
  TargetSpec buff  (TagSet tags, Range r) { return {Intent::Beneficial, Effect::Status, tags, r}; }
  TargetSpec debuff(TagSet tags, Range r) { return {Intent::Hostile,    Effect::Status, tags, r}; }
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
  │      └─ модификаторы цели (IHitModifier) по приоритету меняют amount
  │ 3. Результат: изменение Health + событие (UnitAttacked / UnitHealed)
  │ 4. Вторичные эффекты: ctx.effects().applyStatus(target, debuff({fire}, …), Burning{…})
```

```cpp
class ITargetRule {
public:
    virtual ~ITargetRule() = default;
    // nullopt — эту цель таким способом выбрать нельзя
    virtual std::optional<Range> adjust(const TargetSpec& incoming, Range range) const = 0;
};

class IHitModifier {
public:
    virtual ~IHitModifier() = default;
    virtual int priority() const = 0;
    virtual void modify(Hit& hit) = 0;  // может уменьшить amount до 0;
                                        // может менять только своё состояние (остаток щита)
};
```

Пример — правило цели Ворона:

```cpp
std::optional<Range> Flying::adjust(const TargetSpec& in, Range r) const {
    if (in.intent != Intent::Hostile) return r;             // лечение и баффы — без изменений
    if (in.tags.has(tags::Melee))     return std::nullopt;  // ближняя атака запрещена
    if (in.tags.has(tags::Ranged))    return r.shifted(-1); // дистанция стрелка: {2,R} → {1,R-1}
    return r;                                               // взрыв (area) — без изменений
}
```

- `Hit` берёт `Intent` и `Effect` из своего `TargetSpec`; отдельного поля «вид удара»
  нет, поэтому две записи одного и того же не разойдутся.
- Правила и модификаторы берутся из компонентов и статусов **цели**. Поэтому новый
  иммунитет добавляется в цель, а код атакующих не меняется.
- Юнит без `Health` целью `Damage` и `Heal` не бывает: `Targeting` отфильтровывает
  его сам. Поэтому мина «не может быть атакована» без отдельного правила.
- `UNIT_ATTACKED` содержит итоговый урон после модификаторов и HP цели после удара
  (не меньше 0).

### 4.10 Статусы

Статус — компонент юнита со сроком жизни (горение, щит, замедление). Статус может:
- реализовывать `ITargetRule` и/или `IHitModifier`;
- ставить себя в `Scheduler` (периодический урон в фазе Scheduled).

Статус накладывается через `ctx.effects().applyStatus(target, spec, status)`, где
`spec` — `buff(...)` или `debuff(...)`. Поэтому правила цели могут его запретить
(иммунитет к горению — запрет `debuff` с тегом `fire`). Истёкшие статусы снимаются в фазе Cleanup.

### 4.11 События

- `EventBus` в Core не знает конкретных типов событий: `publish<E>(const E&)`,
  подписчики по типу.
- События Core: `MapCreated`, `UnitSpawned`, `UnitMoved`, `MarchStarted`,
  `MarchEnded`, `UnitAttacked`, `UnitHealed`, `UnitDied`.
- Features при необходимости объявляют свои события и публикуют их через
  `ctx.events()`.
- Перевод доменного события в `io::Events` — адаптер в `App/Output`, один файл на
  событие фичи (события Core — в одном файле `CoreEventsOutput`, его меняет
  владелец Core). Адаптер регистрируется сам (`SW_REGISTER_OUTPUT`, п. 5.4).
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
| `IAction::tryExecute` | `false` — не вызывал изменяющих методов `TurnContext` (проверяется в отладочной сборке, п. 4.6). `true` — действие выполнено, даже если эффект равен 0 |
| `IAction::requirements` | перечисляет все компоненты, которые действие читает у `self`; проверяется в `UnitBuilder::build` |
| `ITargetRule::adjust` | чистая функция без побочных эффектов. Явно проверяет нужную ось: правило про атаки проверяет `intent == Hostile`, а не только теги доставки. Правила цели применяются по очереди в порядке добавления, каждое получает дистанцию после предыдущего; `nullopt` любого правила — запрет |
| `IHitModifier::modify` | явно проверяет `effect`: резист к `magic` уменьшает только `Damage`, запрет лечения обнуляет только `Heal`. `amount` не становится отрицательным (Core проверяет). Меняет только `Hit` и своё состояние. Применяются по возрастанию `priority()`, при равенстве — в порядке добавления |
| `IMoveCostModifier::moveCost` | чистая функция; результат ≥ 1 |
| `IEnterTrigger::onEnter` | меняет мир только через `TurnContext` |
| юнит без `Health` | не бывает целью `Effect::Damage` / `Effect::Heal`; любой код Core обязан работать с таким юнитом |
| юнит без `Movement` | никогда не меняет позицию: `MovementService` отказывает в перемещении, кто бы его ни запросил |

## 5. Features

### 5.1 Юниты из задания

```cpp
// Features/Units/Hunter.hpp — ничего не знает про IO
struct HunterSpec {
    UnitId id; Position pos;
    uint32_t hp, agility, strength, range;
};

// Проверяет статы > 0 и range >= 2; возвращает ВСЕ найденные ошибки,
// без номеров строк (их добавляет App)
Result<Unit> makeHunter(const HunterSpec& s) {
    Diagnostics errors;
    requirePositive(errors, "hp", s.hp);
    requirePositive(errors, "agility", s.agility);
    requirePositive(errors, "strength", s.strength);
    if (s.range < 2)
        errors.add("hunter.invalid_range", "range must be >= 2, got " + std::to_string(s.range));
    if (!errors.empty())
        return errors;

    return UnitBuilder(s.id, "Hunter", s.pos)          // occupiesCell = true по умолчанию
        .with<Health>(s.hp).with<Agility>(s.agility).with<Strength>(s.strength)
        .with<Movement>(/*speed*/ 1)
        .action<RangedAttack>(stat<Agility>, Range{2, s.range}, RequireNoAdjacentOccupying{})
        .action<MeleeAttack>(stat<Strength>)
        .action<MarchStep>()
        .build();                                       // проверка требований действий
}
```

```cpp
// App/Bindings/HunterBinding.cpp — единственное место, где встречаются IO и фича
void bindHunter(CommandRegistry& r) {
    r.on<io::SpawnHunter>([](const io::SpawnHunter& c, SetupContext& ctx) {
        return ctx.spawn(features::makeHunter(
            {c.unitId, {c.x, c.y}, c.hp, c.agility, c.strength, c.range}));
    });
}
SW_REGISTER_BINDING(bindHunter);
```

Мечник собирается так же: `SwordsmanSpec` → `makeSwordsman`, behaviour =
[`MeleeAttack(stat<Strength>)`, `MarchStep`].

Проверка спецификации и сборка юнита — в одной функции `makeX`: причина изменения
у них одна — «что такое этот юнит», а разделение дало бы два файла на юнит без выгоды.

- `MeleeAttack` — `attack({melee, physical}, {1,1})`.
- `RangedAttack` — `attack({ranged, physical}, {min, max})`.
- `RequireNoAdjacentOccupying` — выстрел запрещён, если в соседних клетках есть
  живой юнит с `occupiesCell`. Летающие, мины и мёртвые (до уборки) не мешают.

### 5.2 Как ложатся юниты из планов

| Юнит | Сборка (всё в Features) |
|---|---|
| **Башня** | `Health`, `Power`, без `Movement`; `RangedAttack(stat<Power>, {2,5})`. `MARCH` на неё — ошибка валидации |
| **Лекарь** | `Health`, `Spirit`, `Movement(1)`; `Heal(stat<Spirit>)` с `heal({heal}, {0,2})`, `MarchStep` |
| **Ворон** | `occupiesCell(false)`, тег `flying`, `Health`, `Agility`, `Movement(2)`, компонент `Flying : ITargetRule` (только для `Hostile`: `melee` → запрет, `ranged` → дистанция −1; пример в п. 4.9); `MeleeAttack(stat<Agility>)`, `MarchStep` |
| **Мина** | `occupiesCell(false)`, без `Health` (поэтому не бывает целью, п. 4.9), без `Movement` (статична, `MARCH` на неё — ошибка валидации); `MineFuse(power, trigger 2, blast 3)` |

**Мина по тикам.**
- Тик N, Main: `MineFuse` через `ctx.query()` видит юнита на расстоянии 0…2
  (включая летающих) → мина взводится и ставит взрыв в `ctx.scheduler()` на тик N+1.
  Это выполненное действие. Взведённая мина больше не действует.
- Тик N+1, Scheduled: взрыв. `targeting().select(mine, attack({area, explosion}, {0,3}))` →
  `effects().apply` каждой цели (включая летающих: правило `Flying` не трогает `area`) → `UNIT_ATTACKED` с
  attacker = id мины → `requestRemoval(mine)`. Погибшие получают `UNIT_DIED` сразу
  и в Main не ходят.
- Тик N+1, Cleanup: мина и погибшие удаляются.
- Пока взрыв стоит в очереди, симуляция не может закончиться (п. 4.6).

**Лекарь.** Цели — живые юниты с `Health` и HP < MaxHp в радиусе 2, кроме самого
Лекаря. Случайная цель → `effects().apply(Hit{heal({heal}, {0,2}), self, target, spirit})`.
Дистанция 0 возможна: на клетке Лекаря может оказаться юнит, не занимающий клетку.
Правило `Flying` лечение не трогает: у него `Intent::Beneficial`. HP ограничено
MaxHp (равно HP при спавне). Если на цели запрет лечения (`IHitModifier` обнуляет
amount), лечение всё равно считается выполненным действием с суммой 0. Если целей
нет — `MarchStep`. Событие `UNIT_HEALED` добавится в IO вместе с Лекарем.

### 5.3 Как добавить новый юнит

1. `IO/Commands/SpawnX.hpp` — DTO команды (формат ввода).
2. `Features/Units/X.{hpp,cpp}` — `XSpec`, его проверка и сборка юнита
   (`Result<Unit> makeX(const XSpec&)`); при необходимости новые статы, действия,
   теги, статусы в `Features/Common` или рядом с юнитом.
3. `App/Bindings/XBinding.cpp` — перевод `io::SpawnX` → `XSpec`, без логики,
   в конце файла `SW_REGISTER_BINDING(bindX);`.
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

struct BindingRegistrar { explicit BindingRegistrar(BindingFn fn) { bindingList().push_back(fn); } };
struct OutputRegistrar  { explicit OutputRegistrar(OutputFn fn)   { outputList().push_back(fn); } };

#define SW_REGISTER_BINDING(fn) \
    namespace { const ::sw::app::BindingRegistrar SW_UNIQUE_NAME(swBinding_){&fn}; }
#define SW_REGISTER_OUTPUT(fn) \
    namespace { const ::sw::app::OutputRegistrar SW_UNIQUE_NAME(swOutput_){&fn}; }
```

```cpp
// App/Application.cpp
CommandRegistry commands;
for (BindingFn bind : bindingList()) bind(commands);
EventBridge bridge(eventLog);
for (OutputFn bind : outputList()) bind(bridge);
```

Известные риски и как они закрыты:

| Риск | Решение |
|---|---|
| порядок инициализации статиков между файлами | списки — статические переменные внутри функций |
| линкер выбрасывает «неиспользуемые» файлы со статической регистрацией | `App/Bindings` и `App/Output` компилируются прямо в исполняемый файл (или в OBJECT-библиотеку), никогда не в статическую библиотеку; правило записано комментарием в `CMakeLists.txt` |
| порядок регистрации зависит от порядка линковки | `CommandRegistry` хранит команды по имени, `EventBridge` — по типу события; порядок ни на что не влияет |
| две регистрации одной команды или события | ошибка при старте с именем команды / типа события (баг, код выхода 3) |
| глобальное состояние мешает тестам | глобально хранится только список указателей на функции; `CommandRegistry` и `EventBridge` App создаёт сам, тест может собрать свои |
| забыли зарегистрировать адаптер | команда в сценарии даёт понятную ошибку `parse.unknown_command`, событие — не выводится; ловится тестом сценария |

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
случайность, поэтому оба прохода дают одинаковый мир.

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

## 7. Рассмотренные и отклонённые варианты

- **A. Наследование + виртуальные хуки в базовом `Unit`.** Каждая новая механика
  (не занимает клетку, иммунитет, модификатор дистанции, нет HP) добавляет метод в
  базовый класс в Core — владелец Core становится узким местом. Ближний бой
  Мечника, Охотника и Ворона дублируется или уходит в глубокую иерархию.
- **C. Полноценный ECS.** Правило «одно действие за ход, по порядку создания, по
  приоритету» плохо ложится на системы по компонентам: всё равно нужна система
  выбора действия для каждого юнита, то есть вариант B с лишней церемонией.

## 8. Принятые решения по правилам

- Команды — в тике 1, ходы — с тика 2.
- `UNIT_DIED` — сразу; мёртвый не действует; убирается в Cleanup.
- Шаг марша обходит занятую клетку через другую соседнюю, приближающую к цели;
  если такой нет — стоит.
- Конец: ≤ 1 юнита, или пустой тик при пустом `Scheduler`, или 30 000 тиков.
- Расстояние — Чебышёв. Лог в `main.cpp` иллюстративный (на тиках 7–8 выстрел
  на дистанцию 5 при `range 4`).
- Повторный `MARCH` — в очередь.
- Мина считается юнитом; взводится от летающих, взрыв задевает летающих.
- Лекарь лечит только юнитов с неполным HP; лечение под запретом — выполненное
  действие с суммой 0.
- Выстрелу Охотника мешают только живые юниты, занимающие клетки; мёртвый сосед
  до уборки не мешает.
- Лекарь себя не лечит.
- Features не зависят от IO. DTO команд остаются в IO (это формат ввода), перевод
  DTO → спецификация фичи — в `App/Bindings`. Перенос DTO в App отклонён: App
  зависит от Features, и обратная зависимость дала бы цикл.
- Адаптеры команд и событий регистрируются сами (`SW_REGISTER_BINDING`,
  `SW_REGISTER_OUTPUT`), теги объявляются в фичах. Общих списков, которые правят
  все разработчики, нет — нет и постоянных merge-конфликтов. Рассмотренные
  альтернативы: явный отсортированный список (конфликты реже, но остаются) и
  генерация списка CMake по именам файлов (без статиков, но с соглашением об
  именах и логикой в CMake).
- Ограничения Ворона действуют только на атаки: правила цели проверяют
  `Intent::Hostile`, поэтому лечение и баффы Ворона не затрагивают. Намерение
  (`Intent`) и операция (`Effect`) — две раздельные оси, а не один enum
  `{Attack, Heal, Buff, Debuff}`.

## 9. Открытые вопросы

Нет. Все вопросы закрыты, решения — в разделе 8.
