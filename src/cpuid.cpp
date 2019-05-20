#include "asmlib.h"
#include <cpuid.h>

extern "C" void cpuid_ex(int abcd[4], int eax, int ecx)
{
	__cpuid_count(eax, ecx, abcd[0], abcd[1], abcd[2], abcd[3]);
}
