#include "Registrar.hpp"

#include <stdexcept>

namespace sw::app
{
	std::vector<BindingFn>& bindingList()
	{
		static std::vector<BindingFn> bindings;
		return bindings;
	}

	std::vector<OutputFn>& outputList()
	{
		static std::vector<OutputFn> outputs;
		return outputs;
	}

	BindingRegistrar::BindingRegistrar(BindingFn bind)
	{
		if (bind == nullptr)
		{
			throw std::logic_error("SW_REGISTER_BINDING: null function");
		}
		bindingList().push_back(bind);
	}

	OutputRegistrar::OutputRegistrar(OutputFn bind)
	{
		if (bind == nullptr)
		{
			throw std::logic_error("SW_REGISTER_OUTPUT: null function");
		}
		outputList().push_back(bind);
	}
}
