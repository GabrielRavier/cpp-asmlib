#include "asmlib.h"
#include "asmlib-internal.h"
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

static auto getAvailableStrlenFunctions()
{
	std::vector<std::function<size_t(const char *)>> result = {strlen386};

	if (InstructionSet() >= 4)
		result.push_back(strlenSSE2);

	return result;
}

int main()
{
	constexpr const char *testStrings[] =
	{
		"lol",
		"",
		"test string",
		"azertyuioqdgbjscccccpokagm,;:::::azertyuiopqsdfghjklmwxcvbn",
		"azertyuioqdgbjscccccpokagm,;:::::azertyuiopqsdfghjklmwxcvbnazertyuioqdgbjscccccpokagm,;:::::azertyuiopqsdfghjklmwxcvbnazertyuioqdgbjscccccpokagm,;:::::azertyuiopqsdfghjklmwxcvbnazertyuioqdgbjscccccpokagm,;:::::azertyuiopqsdfghjklmwxcvbnazertyuioqdgbjscccccpokagm,;:::::azertyuiopqsdfghjklmwxcvbnazertyuioqdgbjscccccpokagm,;:::::azertyuiopqsdfghjklmwxcvbnazertyuioqdgbjscccccpokagm,;:::::azertyuiopqsdfghjklmwxcvbnazertyuioqdgbjscccccpokagm,;:::::azertyuiopqsdfghjklmwxcvbnazertyuioqdgbjscccccpokagm,;:::::azertyuiopqsdfghjklmwxcvbnazertyuioqdgbjscccccpokagm,;:::::azertyuiopqsdfghjklmwxcvbnazertyuioqdgbjscccccpokagm,;:::::azertyuiopqsdfghjklmwxcvbnazertyuioqdgbjscccccpokagm,;:::::azertyuiopqsdfghjklmwxcvbnazertyuioqdgbjscccccpokagm,;:::::azertyuiopqsdfghjklmwxcvbnazertyuioqdgbjscccccpokagm,;:::::azertyuiopqsdfghjklmwxcvbnazertyuioqdgbjscccccpokagm,;:::::azertyuiopqsdfghjklmwxcvbnazertyuioqdgbjscccccpokagm,;:::::azertyuiopqsdfghjklmwxcvbnazertyuioqdgbjscccccpokagm,;:::::azertyuiopqsdfghjklmwxcvbnazertyuioqdgbjscccccpokagm,;:::::azertyuiopqsdfghjklmwxcvbnazertyuioqdgbjscccccpokagm,;:::::azertyuiopqsdfghjklmwxcvbnazertyuioqdgbjscccccpokagm,;:::::azertyuiopqsdfghjklmwxcvbnazertyuioqdgbjscccccpokagm,;:::::azertyuiopqsdfghjklmwxcvbnazertyuioqdgbjscccccpokagm,;:::::azertyuiopqsdfghjklmwxcvbnazertyuioqdgbjscccccpokagm,;:::::azertyuiopqsdfghjklmwxcvbnazertyuioqdgbjscccccpokagm,;:::::azertyuiopqsdfghjklmwxcvbnazertyuioqdgbjscccccpokagm,;:::::azertyuiopqsdfghjklmwxcvbnazertyuioqdgbjscccccpokagm,;:::::azertyuiopqsdfghjklmwxcvbnazertyuioqdgbjscccccpokagm,;:::::azertyuiopqsdfghjklmwxcvbnazertyuioqdgbjscccccpokagm,;:::::azertyuiopqsdfghjklmwxcvbnazertyuioqdgbjscccccpokagm,;:::::azertyuiopqsdfghjklmwxcvbnazertyuioqdgbjscccccpokagm,;:::::azertyuiopqsdfghjklmwxcvbnazertyuioqdgbjscccccpokagm,;:::::azertyuiopqsdfghjklmwxcvbnazertyuioqdgbjscccccpokagm,;:::::azertyuiopqsdfghjklmwxcvbnazertyuioqdgbjscccccpokagm,;:::::azertyuiopqsdfghjklmwxcvbnazertyuioqdgbjscccccpokagm,;:::::azertyuiopqsdfghjklmwxcvbnazertyuioqdgbjscccccpokagm,;:::::azertyuiopqsdfghjklmwxcvbn"
	};

	auto strlenFunctions = getAvailableStrlenFunctions();

	for (auto strlenFunction : strlenFunctions)
	{
		for (auto testString : testStrings)
		{
			auto reportedResult = strlenFunction(testString);
			auto correctResult = strlen(testString);

			if (reportedResult != correctResult)
				strlenError(testString, correctResult, reportedResult);
		}

		// Test a random string but byte by byte
		constexpr const char *testStr = "azertyuiopqsdfghjklmwxcvbn1234567890, the brown fox jumps over the lazy dog, lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec nec dolor interdum, finibus ligula nec, accumsan lectus. Duis ac porta odio. Vestibulum eu tincidunt nisi, eget suscipit mi. Mauris nisl velit, fringilla a vestibulum vel, consequat et enim. Interdum et malesuada dfames ac ante ipsum primis in faucibus. Donec at est et lorem venenatis semper. Suspendisse suscipit lobortis turpis, sed fringilla neque eleifend et. Curabitur imperdiet mauris accumsam commodo consequat. Curabitur ac libero suscipit tellus faucibus suscipit tellus faucibus suscipit. Vivamus congue semper enim, vel sodales tortor ullamcorper ut. Curabitur tortor dolor, faucibus id diam nec, tincidunt imperdiet magna. Nunc id tristique velit. Curabitur eu dignissim mauris. Class aptent taciti sociosqu ad litora torquent per conubia nostra, per inceptos himenaeos.";
		const size_t testStrLen = strlen(testStr);

		for (size_t i = 0; i < testStrLen; ++i)
		{
			auto reportedResult = strlenFunction(testStr + i);
			auto correctResult = strlen(testStr + i);
			if (reportedResult != correctResult)
				strlenError(testStr + i, correctResult, reportedResult);
		}
	}
}
