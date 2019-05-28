#include "asmlib.h"
#include "asmlib-internal.h"
#include <iostream>
#include <utility>
#include <vector>
#include <functional>

[[noreturn]] static void strCountError(const char *str, size_t actualLength, size_t falseLength)
{
	std::cout << "ERROR : False length returned for \"" << str << "\"!\n"
		"Reported length was " << falseLength << "\n"
		"Actual length was " << actualLength << '\n';
	std::quick_exit(1);
}

static std::vector<std::function<size_t(const char *)>> getUTF8Funcs()
{
	std::vector<std::function<size_t(const char *)>> result = {strcount_UTF8Generic};

	if (InstructionSet() >= 10)
		result.push_back(strcount_UTF8SSE42);

	return result;
}

int main()
{
	constexpr std::pair<const char *, size_t> stringsAndLengths[] =
	{
		{"こんにちは", 5},
		{"test string", 11},
		{"lol", 3},
		{"e󍱌�ӬH趺~ᕮ󴠩󟗻LÉ렸泂aﮆ.󗯙򽡻Ϸ񃪸ߝ弄᳏ø㎐󎐜𴄃𬓣˸ڂ񘵿", 32},
		{"佗Y擀蘇d5oZQ", 9},
		{"ᚠᛇᚻ᛫ᛒᛦᚦ᛫ᚠᚱᚩᚠᚢᚱ᛫ᚠᛁᚱᚪ᛫ᚷᛖᚻᚹᛦᛚᚳᚢᛗ", 29},
		{"ᐊᓕᒍᖅ ᓂᕆᔭᕌᖓᒃᑯ ᓱᕋᙱᑦᑐᓐᓇᖅᑐᖓ ", 24},
		{"�", 2},
		{"κόσμε", 5},
		{"", 0}
	};

	auto funcs = getUTF8Funcs();

	for (auto func : funcs)
	{
		for (auto strAndLen : stringsAndLengths)
		{
			auto maybeTrueResult = func(strAndLen.first);
			if (maybeTrueResult != strAndLen.second)
				strCountError(strAndLen.first, strAndLen.second, maybeTrueResult);
		}
	}
}
