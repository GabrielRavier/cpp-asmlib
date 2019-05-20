#include "asmlib.h"
#include <cstdint>
#include <cpuid.h>
#include <x86intrin.h>

uint64_t ReadTSC()
{
	int eax, ebx, ecx, edx;
	__cpuid(0, eax, ebx, ecx, edx);
	auto result = __rdtsc();
	__cpuid(0, eax, ebx, ecx, edx);
	return result;
}
