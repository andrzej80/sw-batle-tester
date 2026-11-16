Входные данные теста `layer_boundaries_reports_all_violations` (CMakeLists.txt):
маленькое дерево `src/` с разрешёнными включениями и ровно тремя нарушениями границ
слоёв. Файлы не компилируются — включаемые `Fake.hpp` намеренно не существуют.

| Файл | Включение | Ожидание |
|---|---|---|
| `Core/Allowed.hpp` | свой слой (`"Helper.hpp"`, `<Core/Helper.hpp>`), стандартная библиотека | разрешено |
| `Core/BadCore.hpp` | `<IO/…>` | нарушение: Core включает IO |
| `Features/AllowedFeature.hpp` | `<Core/…>` | разрешено |
| `Features/BadFeature.hpp` | `"../App/…"` (относительный путь) | нарушение: Features включает App |
| `IO/BadIo.hpp` | `<Core/…>` | нарушение: IO включает Core |
| `App/AnyApp.hpp` | все слои | разрешено |
| `main.cpp` | все слои | разрешено: точка сборки приложения |
