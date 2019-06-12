#include "asmlib.h"
#include "asmlib-internal.h"
#include <iostream>
#include <functional>
#include <vector>
#include <tuple>

inline std::vector<std::function<size_t(const char *, const char *)>> getAvailableStrCountInSetFunctions()
{
	std::vector<std::function<size_t(const char *, const char *)>> result = {strCountInSetGeneric, strCountInSet};

	if (InstructionSet() >= 9)
		result.push_back(strCountInSetSSE42);

	return result;
}

[[noreturn]] inline void strCountInSetError(size_t expectedResult, size_t falseResult, const char *testStr, const char *testSet)
{
	std::cerr << "ERROR : strCountInSet found " << falseResult << " elements of \"" << testSet << "\" in \"" << testStr << "\" when it should have found " << expectedResult << " !\n";
	std::exit(1);
}

int main()
{
	std::tuple<const char *, const char *, size_t> testVals[] =
	{
		{"aaaa", "abeozpe", 4},
		{"dzpaokdpoiiiiii", "dizp", 11},
		{"gfy", "f", 1},
		{"sample string", "sng", 4},
		{"azertyuiopqsdfghjklmwxcvbn", "iejzoslr", 8},
		{"really long str azertyuiopqsdfghjklmwxcvbnazertyuiopqsdfghjklmwxcvbnazertyuiopqsdfghjklmwxcvbnazertyuiopqsdfghjklmwxcvbnazertyuiopqsdfghjklmwxcvbnazertyuiopqsdfghjklmwxcvbnazertyuiopqsdfghjklmwxcvbnazertyuiopqsdfghjklmwxcvbnazertyuiopqsdfghjklmwxcvbnazertyuiopqsdfghjklmwxcvbnazertyuiopqsdfghjklmwxcvbnazertyuiopqsdfghjklmwxcvbnazertyuiopqsdfghjklmwxcvbnazertyuiopqsdfghjklmwxcvbnazertyuiopqsdfghjklmwxcvbnazertyuiopqsdfghjklmwxcvbnazertyuiopqsdfghjklmwxcvbnazertyuiopqsdfghjklmwxcvbnazertyuiopqsdfghjklmwxcvbnazertyuiopqsdfghjklmwxcvbn", "aijerlz", (20 * 7) + 5 + 1 + 1}
	};

	auto funcs = getAvailableStrCountInSetFunctions();

	for (auto func : funcs)
	{
		for (auto testVal : testVals)
		{
			auto [testStr, testSet, expectedResult] = testVal;

			auto reportedResult = func(testStr, testSet);

			if (expectedResult != reportedResult)
				strCountInSetError(expectedResult, reportedResult, testStr, testSet);
		}
	}
}
