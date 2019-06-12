#include "asmlib.h"
#include "asmlib-internal.h"
#include <iostream>
#include <vector>
#include <functional>
#include <cstring>

[[noreturn]] inline void strstrError(const char *haystack, const char *needle, const char *expectedResult, const char * falseResult)
{
	std::cerr << "Error when searching for \"" << needle << "\" in \"" << haystack << "\" : Got " << falseResult - haystack << " instead of " << expectedResult - haystack << " !\n";
	std::exit(1);
}

inline auto getAvailableStrstrFunctions()
{
	std::vector<std::function<const char *(const char *, const char *)>> result = {(const char *(*)(const char *, const char *))strstrGeneric, (const char *(*)(const char *, const char *))A_strstr};

	if (InstructionSet() >= asmlibInternal::InstructionSetReturnValues::sse42Supported)
		result.push_back((const char *(*)(const char *, const char *))strstrSSE42);

	return result;
}

int main()
{
	const char *testStrings[] =
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

	auto functionsToTest = getAvailableStrstrFunctions();

	for (auto testFunction : functionsToTest)
		for (auto haystack : testStrings)
			for (auto needle : testStrings)
			{
				auto reportedResult = testFunction((char *)haystack, needle);
				auto expectedResult = strstr(haystack, needle);

				if (reportedResult != expectedResult)
					strstrError(haystack, needle, expectedResult, reportedResult);
			}
}
