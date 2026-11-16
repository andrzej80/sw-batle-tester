#pragma once

namespace sw::core
{
	class ComponentRequirements;
	class TurnContext;
	class Unit;

	// Контракт: tryExecute вернул false — мир не изменён (проверяет Simulation).
	// requirements перечисляет все компоненты self, которые читает действие.
	// Состояние действия хранится в объекте: у каждого юнита свои экземпляры.
	class IAction
	{
	public:
		IAction() = default;
		virtual ~IAction() = default;

		IAction(const IAction&) = delete;
		IAction& operator=(const IAction&) = delete;
		IAction(IAction&&) = delete;
		IAction& operator=(IAction&&) = delete;

		virtual bool tryExecute(const Unit& self, TurnContext& context) = 0;

		virtual void requirements(ComponentRequirements& /*requirements*/) const {}
	};
}
