#pragma once

namespace sw::core
{
	class ComponentRequirements;
	class TurnContext;
	class Unit;

	// Действие юнита (ARCHITECTURE.md, п. 4.3). Behaviour юнита — список действий по
	// приоритету; в фазе Main выполняется первое, вернувшее true.
	//
	// Контракт (п. 4.13):
	// - tryExecute: true — действие выполнено, ход юнита закончен (даже если эффект равен 0);
	//   false — не вызывал изменяющих методов TurnContext;
	// - requirements: перечисляет все компоненты, которые действие читает у self;
	//   проверяется при создании юнита.
	//
	// Состояние действия (например, «мина взведена») хранится в самом объекте действия:
	// у каждого юнита свои экземпляры.
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
