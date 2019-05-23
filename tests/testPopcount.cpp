#include "asmlib.h"
#include <cassert>
#include <cstdlib>
#include <vector>
#include <functional>

uint32_t popcountSSE42(uint32_t x);
uint32_t popcountGeneric(uint32_t x);

static uint32_t naivePopcount(uint32_t x)
{
	uint32_t count = 0;
	for (; x; ++count)
		x &= x - 1;
	return count;
}

static uint32_t popcountReference(uint32_t x)
{
	if (sizeof(x) == sizeof(unsigned int))
		return __builtin_popcount(x);

	if (sizeof(x) == sizeof(unsigned long))
		return __builtin_popcountl(x);

	if (sizeof(x) == sizeof(unsigned long long))
		return __builtin_popcountll(x);

	return naivePopcount(x);
}

int main()
{
	std::vector<std::function<uint32_t(uint32_t)>> availableFuncs{popcountGeneric};
	if (InstructionSet() >= 9)
		availableFuncs.push_back(popcountGeneric);

	uint32_t testedVals[] = {1, 0, 54, 23, 64, 87, 0xFFFFFFFF, 0x80000000, 0x23984823, 4921904, 12345, 33, 0xF0F0F0F0, 0x55555555, 0x33333333, 0xF0F0F0F};

	for (auto i : testedVals)
		for (auto func : availableFuncs)
			assert(func(i) == popcountReference(i));
}
