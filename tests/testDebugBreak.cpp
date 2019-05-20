#include "asmlib.h"
#include <cstdlib>
#include <csignal>
#include <iostream>
#include <unistd.h>

void sigtrapHandler(int /*sigNum*/)
{
	std::cout << "A_DebugBreak succesfully raised a SIGTRAP !\n";
	std::quick_exit(0);
}

int main()
{
	signal(SIGTRAP, sigtrapHandler);
	A_DebugBreak();	// Should emit a SIGTRAP

	std::cout << "ERROR : A_DebugBreak did not SIGTRAP !\n";

	return 1;
}
