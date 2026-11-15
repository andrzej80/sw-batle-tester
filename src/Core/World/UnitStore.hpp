#pragma once

#include <Core/Common/Types.hpp>
#include <Core/Units/Unit.hpp>
#include <cstddef>
#include <memory>
#include <unordered_map>
#include <vector>

namespace sw::core
{
	class Map;

	// Юниты в порядке создания (SPECIFICATION.md, п. 4.2). Порядок создания — это порядок
	// ходов в фазе Main. Юнит лежит в unique_ptr, поэтому ссылки на него не меняются при
	// добавлении и удалении других юнитов.
	class UnitStore
	{
	public:
		// Повторный id — std::logic_error (ошибку ввода ловит SetupContext раньше)
		Unit& add(Unit unit);

		Unit* find(UnitId id);
		const Unit* find(UnitId id) const;

		// Снимок порядка создания: на нём строится фаза Main
		std::vector<UnitId> orderedIds() const;

		// Все юниты в порядке создания, включая мёртвых до Cleanup
		std::vector<const Unit*> ordered() const;

		size_t size() const
		{
			return _units.size();
		}

		// Cleanup: удаляет мёртвых (HP ≤ 0) и помеченных requestRemoval, освобождает их
		// клетки на карте. Порядок остальных не меняется. Возвращает число удалённых.
		size_t removePending(Map& map);

	private:
		std::vector<std::unique_ptr<Unit>> _units;
		std::unordered_map<UnitId, Unit*> _index;
	};
}
