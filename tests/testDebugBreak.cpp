#include "asmlib.h"
#include <cstdlib>
#include <csignal>
#include <iostream>
#include <unistd.h>

inline void sigtrapHandler(int /* sigNum */)
{
	std::cout << "A_DebugBreak successfully raised a SIGTRAP !\n";
	std::exit(0);
}

inline void sigabrtHandler(int /* sigNum */)
{
	std::cout << "A_DebugBreak successfully raised a SIGABRT !\n";
	std::exit(0);
}

int main()
{
	signal(SIGTRAP, sigtrapHandler);
	signal(SIGABRT, sigabrtHandler);
	A_DebugBreak();	// Should emit a SIGTRAP

	std::cerr << "ERROR : A_DebugBreak did not raise SIGTRAP or SIGABRT !\n";
	std::exit(1);
}
