#include "asmlib.h"
#include <signal.h>

extern "C" void A_DebugBreak()
{
#if defined(__x86_64__) || defined(__i386__)
	__asm__ volatile("int $0x3; nop");	// Portable amongst x86 gcc-compliant compliant
#else
#ifdef SIGTRAP
	raise(SIGTRAP);	// Portable across POSIX
#else
	raise(SIGABRT);	// Portable
#endif
#endif
}
