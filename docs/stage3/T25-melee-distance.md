# T25. Дистанция ближней атаки — S

[← Этап 3](README.md)

- **Сделать:** у `MeleeAttack` — параметр дистанции `Distance` со значением по умолчанию
  `distance(1, 1)`: `MeleeAttack(ValueSource damage, Distance distance = distance(1, 1))`.
  Нужен Ворону (`{0,1}`, [Raven.md](../units/Raven.md)) — «новый вариант способности —
  параметр, а не флаг» (src/Features/README.md). Мечник и Охотник не меняются.
- **Правки существующих файлов:** `src/Features/Common/Attacks.{hpp,cpp}`; спец. 9.2 —
  строка `MeleeAttack`.
- **Зависит от:** —.
- **Тесты:**
  - `MeleeAttack` с `distance(0, 1)` бьёт юнита на своей клетке (тестовый юнит без
    `occupiesCell`) и в соседней;
  - по умолчанию поведение прежнее: юнита на своей клетке не бьёт;
  - `distance` со `stat<T>` объявляет требование компонента.
- **Изменения спецификации:** уточнение — спец. 9.2 (дистанция `MeleeAttack` задаётся
  параметром, по умолчанию `{1,1}`); требующих согласования нет.
- **Готово:** все прежние тесты и эталоны сценариев проходят без правок;
  `git diff main -- src/Core` пуст.
- **Итог (выполнена):** `MeleeAttack(ValueSource damage, Distance reach = distance(1, 1))`;
  параметр назван `reach`, а не `distance`, — иначе значение по умолчанию сослалось бы на
  сам параметр, а не на функцию `distance(...)`. Дистанция вычисляется при каждом ударе,
  её `stat<T>` объявляются в `requirements`. Три новых теста в
  `tests/Features/Common/FeaturesCommonTests.cpp`; сборка clang 15/18 с `-Werror`, ASan +
  UBSan, clang-tidy-18 и clang-format-18 — без замечаний.
