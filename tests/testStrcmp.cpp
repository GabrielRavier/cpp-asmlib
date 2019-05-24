#include "asmlib.h"
#include <iostream>
#include <vector>
#include <functional>
#include <cstring>

[[noreturn]] static void strcmpError(const char *str1, const char *str2, int expectedResult, int falseResult)
{
	std::cerr << "Error when comparing \"" << str1 << "\" and \"" << str2 << "\" : Got " << falseResult << " instead of " << expectedResult << " !\n";
	std::quick_exit(1);
}

int strcmpGeneric(const char *str1, const char *str2);
int strcmpSSE42(const char *str1, const char *str2);

static auto getAvailableStrcmpFunctions()
{
	std::vector<std::function<int(const char *, const char *)>> result = {strcmpGeneric};

	if (InstructionSet() >= 10)
		result.push_back(strcmpSSE42);

	return result;
}

int main()
{
	const char *stringsToCompare[] =
	{
		"lol",
		"aaa",
		"abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrs",
		"abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijkLMNOPQRSTUV",
		"",
		"a",
		"b",
		"aa",
		"ab",
		"aaa",
		"aab",
		"aaaa",
		"aaab",
		"aaaaa",
		"aaaab",
		"aaaaaa",
		"aaaaab",
		"xxx",
		"xxy",
		"x\xf6x",
		"xox",
		"xxx",
		"xxxyyy"
	};

	auto funcs = getAvailableStrcmpFunctions();

	for (auto func : funcs)
		for (auto str1 : stringsToCompare)
			for (auto str2 : stringsToCompare)
			{
				auto reportedResult = func(str1, str2);
				auto expectedResult = strcmp(str1, str2);

				if (reportedResult != expectedResult)
					strcmpError(str1, str2, expectedResult, reportedResult);
			}
}
