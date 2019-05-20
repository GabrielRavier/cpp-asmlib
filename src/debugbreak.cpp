#include "asmlib.h"
#include <signal.h>

extern "C" void A_DebugBreak()
{
#if defined(__x86_64__) || defined(__i386__)
	__asm__ volatile("int $0x3; nop");
#else
	raise(SIGTRAP);	// Should be a portable equivalent (is at least the same on Linux)
#endif
}
