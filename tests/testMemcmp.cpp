#include "asmlib.h"
#include "asmlib-internal.h"
#include <iostream>
#include <vector>
#include <functional>
#include <utility>
#include <algorithm>
#include <cstring>

template <typename T> void dumpArray(std::ostream& os, T array[], size_t arraySize)
{
	auto preservedFlags = os.flags();
	os << std::hex;

	for (size_t i = 0; i < arraySize; ++i)
		os << "0x" << array[i] << ' ';

	os.flags(preservedFlags);
}

[[noreturn]] inline void memcmpError(const void *ptr1, const void *ptr2, size_t size, const char *functionName, int correctResult, int falseResult)
{
	auto preservedFlags = std::cerr.flags();
	std::cerr << std::hex;
	std::cerr << "ERROR : Failed to compare arrays ptr1 (0x" << (uintptr_t)ptr1 << ") and ptr2 (0x" << (uintptr_t)ptr2 << ") of size 0x" << size << " !\n"
	"\n";
	std::cerr.flags(preservedFlags);

	std::cerr << "Got " << falseResult << " when " << correctResult << " was expected !\n"
	"\n"
	"ptr1 : ";
	dumpArray(std::cerr, (const uint8_t *)ptr1, size);
	std::cerr << "\n"
	"\n"
	"ptr2 : ";
	dumpArray(std::cerr, (const uint8_t *)ptr2, size);
	std::cerr << "\n"
	"\n"
	"Function used : " << functionName << '\n';

	std::exit(1);
}

inline auto getAvailableMemcmpFunctions()
{
	std::vector<std::pair<std::function<int(const void *, const void *, size_t)>, const char *>> result = {{memcmp386, "memcmp386"}};

	auto instructionSet = InstructionSet();

	if (instructionSet >= asmlibInternal::InstructionSetReturnValues::sse2Supported)
	{
		result.push_back({memcmpSSE2, "memcmpSSE2"});
		if (instructionSet >= asmlibInternal::InstructionSetReturnValues::avx2Supported)
		{
			result.push_back({memcmpAVX2, "memcmpAVX2"});
			if (instructionSet >= asmlibInternal::InstructionSetReturnValues::avx512FSupported)
			{
				result.push_back({memcmpAVX512F, "memcmpAVX512F"});
				if (instructionSet >= asmlibInternal::InstructionSetReturnValues::avx512BWAvx512DQAvx512VLSupported)
					result.push_back({memcmpAVX512BW, "memcmpAVX512BW"});
			}
		}
	}

	return result;
}

int main()
{
	auto functions = getAvailableMemcmpFunctions();

	for (auto function : functions)
	{
		const std::pair<const char *, size_t> comparedArrays[] = {{(const char []){'a', 'b', 'c'}, 3}, {(const char []){'a', 'b', 'd'}, 3}, {(const char []){1, 53, 53, 22}, 53}, {(const char []){63, 12}, 2}, {(const char []){6, 1, 2, 3}, 4}};

		for (auto ptr1 : comparedArrays)
			for (auto ptr2 : comparedArrays)
			{
				auto size = std::min(ptr1.second, ptr2.second);

				auto correctResult = memcmp(ptr1.first, ptr2.first, size);
				auto testResult = function.first(ptr1.first, ptr2.first, size);

				if (correctResult != testResult)
					memcmpError(ptr1.first, ptr2.first, size, function.second, correctResult, testResult);
			}
	}

	return 0;
}
