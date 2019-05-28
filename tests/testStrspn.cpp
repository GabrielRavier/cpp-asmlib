#include "asmlib.h"
#include "asmlib-internal.h"
#include <iostream>
#include <functional>
#include <utility>
#include <vector>
#include <string>
#include <cstring>

[[noreturn]] inline void strspnError(size_t expectedResult, size_t falseResult, const char *string, const char *set, const char *functionName, const char *stdlibFuncName)
{
	auto parametersString = std::string("(\"") + string + "\", \"" + set + "\")";
	std::cout << "ERROR : " << stdlibFuncName << parametersString << " != " << functionName << parametersString << " !\n"
	<< stdlibFuncName << parametersString << " = " << expectedResult << '\n'
	<< functionName << parametersString << " = " << falseResult << '\n';
	std::quick_exit(1);
}

inline auto getAvailableStrspnFunctions()
{
	std::vector<std::pair<std::pair<std::function<size_t(const char *, const char *)>, const char *>, std::pair<std::function<size_t(const char *, const char *)>, const char *>>> result = {{{strspnGeneric, "strspnGeneric"}, {strcspnGeneric, "strcspnGeneric"}}};

	if (InstructionSet() >= asmlibInternal::InstructionSetReturnValues::sse42Supported)
		result.push_back({{strspnSSE42, "strspnSSE42"}, {strcspnSSE42, "strcspnSSE42"}});

	return result;
}

int main()
{
	const char *testStrings[] =
	{
		"",
		"lol",
		"aaa",
		"a",
		"b",
		"poppppo",
		"test string",
		"lorep ipsum",
		"blargo",
		"azertyuiopqsdfghjklmwxcvbn",
		"qwertyuiopasdfghjklzxcvbnm"
	};

	for (auto function : getAvailableStrspnFunctions())
		for (auto string : testStrings)
			for (auto set : testStrings)
			{
				auto expectedStrspnResult = strspn(string, set);
				auto expectedStrcspnResult = strcspn(string, set);
				auto reportedStrspnResult = function.first.first(string, set);
				auto reportedStrcspnResult = function.second.first(string, set);

				if (expectedStrspnResult != reportedStrspnResult)
					strspnError(expectedStrspnResult, reportedStrcspnResult, string, set, function.first.second, "strspn");

				if (expectedStrcspnResult != reportedStrcspnResult)
					strspnError(expectedStrcspnResult, reportedStrcspnResult, string, set, function.second.second, "strcspn");
			}
}
