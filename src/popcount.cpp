#include "asmlib.h"
#include <smmintrin.h>

uint32_t popcountSSE42(uint32_t x) __attribute__((target("popcnt")));
uint32_t popcountSSE42(uint32_t x)
{
	return _mm_popcnt_u32(x);
}

uint32_t popcountGeneric(uint32_t x)
{
#if 1
	uint32_t oddBits = x & 0x55555555;
	uint32_t evenBits = (x >> 1) & 0x55555555;

	x = oddBits + evenBits;

	oddBits = x & 0x33333333;
	evenBits = (x >> 2) & 0x33333333;

	x = oddBits + evenBits;

	x += (x >> 4);
	x &= 0xF0F0F0F;

	x += (x >> 8);
	x += (x >> 16);
	return x & 0x3F;
#else
	return __builtin_popcount(x);
#endif
}


static uint32_t popcountCPUDispath(uint32_t x);
static auto popcountDispatch = popcountCPUDispath;

static uint32_t popcountCPUDispath(uint32_t x)
{
	int instructionSet = InstructionSet();
	auto result = popcountGeneric;

	if (instructionSet >= 9)
		result = popcountSSE42;

	popcountDispatch = result;

	return result(x);
}

extern "C" uint32_t A_popcount(uint32_t x)
{
	return popcountDispatch(x);
}
