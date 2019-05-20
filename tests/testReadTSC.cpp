#include "asmlib.h"
#include <iostream>

int main()
{
	ReadTSC();

	auto tsc = ReadTSC();
	tsc = ReadTSC() - tsc;
	std::cout << "ReadTSC takes " << tsc << " clocks\n";
}
