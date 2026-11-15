#include "TypeName.hpp"

#include <cstdlib>
#include <memory>

#if __has_include(<cxxabi.h>)
	#include <cxxabi.h>
	#define SW_HAS_CXXABI 1
#endif

namespace sw::core
{
	std::string typeName(const std::type_info& type)
	{
#ifdef SW_HAS_CXXABI
		int status = 0;
		const std::unique_ptr<char, decltype(&std::free)> demangled(
			abi::__cxa_demangle(type.name(), nullptr, nullptr, &status), &std::free);
		if (status == 0 && demangled)
		{
			return demangled.get();
		}
#endif
		return type.name();
	}
}
