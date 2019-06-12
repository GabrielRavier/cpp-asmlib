#include "asmlib.h"
#include "asmlib-internal.h"
#include <iostream>
#include <vector>
#include <functional>

inline uint32_t naivePopcount(uint32_t x)
{
	uint32_t count = 0;
	for (; x; ++count)
		x &= x - 1;
	return count;
}

inline uint32_t popcountReference(uint32_t x)
{
	if (sizeof(x) == sizeof(unsigned int))
		return __builtin_popcount(x);

	if (sizeof(x) == sizeof(unsigned long))
		return __builtin_popcountl(x);

	if (sizeof(x) == sizeof(unsigned long long))
		return __builtin_popcountll(x);

	return naivePopcount(x);
}

[[noreturn]] inline void popcountError(uint32_t expectedResult, uint32_t falseResult, uint32_t testedVal, const char *funcName)
{
	std::cerr << "ERROR : " << funcName << '(' << testedVal << ") != popcountReference(" << testedVal << ") !\n"
	<< funcName << '(' << testedVal << ") = " << falseResult << "\n"
	"popcountReference(" << testedVal << ") = " << expectedResult << '\n';
	std::exit(1);
}

inline auto getAvailablePopcountFunctions()
{
	std::vector<std::pair<std::function<uint32_t(uint32_t)>, const char *>> availableFuncs{{popcountGeneric, "popcountGeneric"}, {A_popcount, "A_popcount"}};
	if (InstructionSet() >= 9)
		availableFuncs.push_back({popcountSSE42, "popcountSSE42"});

	return availableFuncs;
}

int main()
{

	uint32_t testedVals[] = {1, 0, 54, 23, 64, 87, 0xFFFFFFFF, 0x80000000, 0x23984823, 4921904, 12345, 33, 0xF0F0F0F0, 0x55555555, 0x33333333, 0xF0F0F0F};

	for (auto func : getAvailablePopcountFunctions())
		for (auto i : testedVals)
		{
			auto reportedResult = func.first(i);
			auto expectedResult = popcountReference(i);
			if (reportedResult != expectedResult)
				popcountError(expectedResult, reportedResult, i, func.second);
		}
}
