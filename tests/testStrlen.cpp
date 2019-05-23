#include "asmlib.h"
#include <cstddef>
#include <cstring>
#include <iostream>
#include <functional>
#include <vector>

[[noreturn]] static void strlenError(const char *str, size_t correctResult, size_t falseResult)
{
	std::cerr << "Error while computing length of \"" << str << "\" : Expected " << correctResult << ", but got " << falseResult << " !\n";
	std::quick_exit(1);
}

size_t strlen386(const char *str);
size_t strlenSSE2(const char *str);

static auto getAvailableStrlenFunctions()
{
	std::vector<std::function<size_t(const char *)>> result = {strlen386};

	if (InstructionSet() >= 4)
		result.push_back(strlenSSE2);

	return result;
}

int main()
{
	const char *testStrings[] =
	{
		"lol",
		"",
		"test string",
		"azertyuioqdgbjscccccpokagm,;:::::azertyuiopqsdfghjklmwxcvbn",
		"azertyuioqdgbjscccccpokagm,;:::::azertyuiopqsdfghjklmwxcvbnazertyuioqdgbjscccccpokagm,;:::::azertyuiopqsdfghjklmwxcvbnazertyuioqdgbjscccccpokagm,;:::::azertyuiopqsdfghjklmwxcvbnazertyuioqdgbjscccccpokagm,;:::::azertyuiopqsdfghjklmwxcvbnazertyuioqdgbjscccccpokagm,;:::::azertyuiopqsdfghjklmwxcvbnazertyuioqdgbjscccccpokagm,;:::::azertyuiopqsdfghjklmwxcvbnazertyuioqdgbjscccccpokagm,;:::::azertyuiopqsdfghjklmwxcvbnazertyuioqdgbjscccccpokagm,;:::::azertyuiopqsdfghjklmwxcvbnazertyuioqdgbjscccccpokagm,;:::::azertyuiopqsdfghjklmwxcvbnazertyuioqdgbjscccccpokagm,;:::::azertyuiopqsdfghjklmwxcvbnazertyuioqdgbjscccccpokagm,;:::::azertyuiopqsdfghjklmwxcvbnazertyuioqdgbjscccccpokagm,;:::::azertyuiopqsdfghjklmwxcvbnazertyuioqdgbjscccccpokagm,;:::::azertyuiopqsdfghjklmwxcvbnazertyuioqdgbjscccccpokagm,;:::::azertyuiopqsdfghjklmwxcvbnazertyuioqdgbjscccccpokagm,;:::::azertyuiopqsdfghjklmwxcvbnazertyuioqdgbjscccccpokagm,;:::::azertyuiopqsdfghjklmwxcvbnazertyuioqdgbjscccccpokagm,;:::::azertyuiopqsdfghjklmwxcvbnazertyuioqdgbjscccccpokagm,;:::::azertyuiopqsdfghjklmwxcvbnazertyuioqdgbjscccccpokagm,;:::::azertyuiopqsdfghjklmwxcvbnazertyuioqdgbjscccccpokagm,;:::::azertyuiopqsdfghjklmwxcvbnazertyuioqdgbjscccccpokagm,;:::::azertyuiopqsdfghjklmwxcvbnazertyuioqdgbjscccccpokagm,;:::::azertyuiopqsdfghjklmwxcvbnazertyuioqdgbjscccccpokagm,;:::::azertyuiopqsdfghjklmwxcvbnazertyuioqdgbjscccccpokagm,;:::::azertyuiopqsdfghjklmwxcvbnazertyuioqdgbjscccccpokagm,;:::::azertyuiopqsdfghjklmwxcvbnazertyuioqdgbjscccccpokagm,;:::::azertyuiopqsdfghjklmwxcvbnazertyuioqdgbjscccccpokagm,;:::::azertyuiopqsdfghjklmwxcvbnazertyuioqdgbjscccccpokagm,;:::::azertyuiopqsdfghjklmwxcvbnazertyuioqdgbjscccccpokagm,;:::::azertyuiopqsdfghjklmwxcvbnazertyuioqdgbjscccccpokagm,;:::::azertyuiopqsdfghjklmwxcvbnazertyuioqdgbjscccccpokagm,;:::::azertyuiopqsdfghjklmwxcvbnazertyuioqdgbjscccccpokagm,;:::::azertyuiopqsdfghjklmwxcvbnazertyuioqdgbjscccccpokagm,;:::::azertyuiopqsdfghjklmwxcvbnazertyuioqdgbjscccccpokagm,;:::::azertyuiopqsdfghjklmwxcvbnazertyuioqdgbjscccccpokagm,;:::::azertyuiopqsdfghjklmwxcvbn"
	};

	auto strlenFunctions = getAvailableStrlenFunctions();

	for (auto strlenFunction : strlenFunctions)
		for (auto testString : testStrings)
		{
			auto reportedResult = strlenFunction(testString);
			auto correctResult = strlen(testString);

			if (reportedResult != correctResult)
				strlenError(testString, correctResult, reportedResult);
		}
}
