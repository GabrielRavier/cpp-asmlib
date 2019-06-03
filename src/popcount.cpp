#include "asmlib.h"
#include "asmlib-internal.h"
#include <smmintrin.h>

extern "C" uint32_t popcountSSE42(uint32_t x) __attribute__((target("popcnt")));
extern "C" uint32_t popcountSSE42(uint32_t x)
{
	return _mm_popcnt_u32(x);
}

extern "C" uint32_t popcountGeneric(uint32_t x)
{
#if 1
	uint32_t oddBits = x & 0x55555555;
	uint32_t evenBits = (x >> 1) & 0x55555555;

	x = oddBits + evenBits;

	// Every 2 bits hold the sum of every pair of bits
	oddBits = x & 0x33333333;
	evenBits = (x >> 2) & 0x33333333;

	x = oddBits + evenBits;

	// Every 4 bits hold the sum of every set of 4 bits
	x += (x >> 4);
	x &= 0xF0F0F0F;

	// Every 8 bits hold the sum of every set of 8 bits
	x += (x >> 8);
	x += (x >> 16);
	return x & 0x3F;	// 6 significant bits
#else
	return __builtin_popcount(x);
#endif
}


inline uint32_t popcountCPUDispatch(uint32_t x);
static auto popcountDispatch = popcountCPUDispatch;

inline uint32_t popcountCPUDispatch(uint32_t x)
{
	int instructionSet = InstructionSet();
	auto result = popcountGeneric;

	if (instructionSet >= asmlibInternal::InstructionSetReturnValues::popcntSupported)
		result = popcountSSE42;

	popcountDispatch = result;

	return result(x);
}

extern "C" uint32_t A_popcount(uint32_t x)
{
	return popcountDispatch(x);
}
