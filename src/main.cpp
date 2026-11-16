#include <App/Application.hpp>
#include <iostream>

int main(int argc, char** argv)
{
	return sw::app::Application{}.run(argc, argv, std::cerr);
}
