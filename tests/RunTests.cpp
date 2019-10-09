
#include <iostream>

#include "./command/TestASTParsing.hpp"
#include "./command/InteractiveTestNextTokens.hpp"

/*
g++ -std=c++17 -Wfatal-errors -I../farb/src/core -I../farb/src/interface -I../farb/src/reflection -I../farb/src/serialization -I../farb/src/utils tests/RunTests.cpp ../farb/build/link/farb.a -g && ./a.out;
*/


int main(void)
{
	std::cout << "Beginning Tests" << std::endl;
	
	bool success = Run<
		TestASTParsing,
		InteractiveTestNextTokens>(true);
	
	std::cout << "All Tests Passed" << std::endl;
	if (success) return 0;
	return 1;
}
