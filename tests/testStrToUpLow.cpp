#include "asmlib.h"
#include <cstddef>
#include <cstring>
#include <iostream>
#include <functional>
#include <vector>
#include <utility>
#include <string>
#include <string_view>

[[noreturn]] static void strToUpLowError(const char *str, std::string_view falseResult, const char *correctResult)
{
	std::cerr << "Error while making \"" << str << "\" upper/lower case : Expected \"" << correctResult << "\", but got \"" << falseResult << "\" !\n";
	std::quick_exit(1);
}

void strtoupperGeneric(char *str);
void strtolowerGeneric(char *str);
void strtoupperSSE42(char *str);
void strtolowerSSE42(char *str);

static auto getAvailableStrToUpLowFunctions()
{
	std::vector<std::pair<std::function<void(char *)>, std::function<void(char *)>>> result = {{strtoupperGeneric, strtolowerGeneric}};

	if (InstructionSet() >= 4)
		result.push_back({strtoupperSSE42, strtolowerSSE42});

	return result;
}

int main()
{
	constexpr const char *testStrings[][3] =
	{
		{"Lol", "LOL", "lol"},
		{"ZodoooappZZZZZZfopkazfDSMKQnnwwwQPMZZ", "ZODOOOAPPZZZZZZFOPKAZFDSMKQNNWWWQPMZZ", "zodoooappzzzzzzfopkazfdsmkqnnwwwqpmzz"}
	};

	auto strToUpLowFunctions = getAvailableStrToUpLowFunctions();

	for (auto strToUpLowFunction : strToUpLowFunctions)
		for (auto testString : testStrings)
		{
			std::string strForUp(testString[0]);
			std::string strForLow(strForUp);

			strToUpLowFunction.first(strForUp.data());
			strToUpLowFunction.second(strForLow.data());

			if (strForUp != testString[1])
				strToUpLowError(testString[0], strForUp, testString[1]);

			if (strForLow != testString[2])
				strToUpLowError(testString[0], strForLow, testString[2]);
		}
}
