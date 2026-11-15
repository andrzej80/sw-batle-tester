#include "TestFramework/TestFramework.hpp"

#include <iostream>
#include <string_view>

// sw_tests [фильтр] — выполняет тесты, имя которых содержит фильтр
int main(int argc, char** argv)
{
	if (argc > 2)
	{
		std::cerr << "usage: sw_tests [test-name-filter]\n";
		return 2;
	}
	const std::string_view filter = argc == 2 ? argv[1] : "";
	return sw::test::runAll(filter, std::cout);
}
