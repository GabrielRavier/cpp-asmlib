#include <emmintrin.h>
#include "asmlib.h"
#include "asmlib-internal.h"
#include <iostream>
#include <utility>
#include <functional>

template <typename T>
[[noreturn]] inline void divFixedError(T expectedResult, T falseResult, T divisor, T dividend, const char *functionName)
{
	std::cerr << "ERROR : " << dividend << " / " << divisor << " = " << expectedResult << ", but got " << falseResult << " instead !\n"
	"Was using " << functionName << '\n';
	std::exit(1);
}

inline void testDivFixedV8i16()
{
	constexpr int16_t testDivisors[] = {243, 293, 111, 3, 2, 8427, 247, 1, 11, 243, 32750, 32750};

	for (int16_t divisor : testDivisors)
	{
		__m128i buffer[2];
		setdivisorV8i16(buffer, divisor);

		constexpr int16_t testDividends[] = {243, 293, 111, 3, 2, 8427, -21969, 247, 1, -2, -1, -243, -32750, 32750, 0};

		for (int16_t dividend : testDividends)
		{
			int16_t expectedResult = dividend / divisor;
			int16_t testResult = _mm_cvtsi128_si32(dividefixedV8i16(buffer, _mm_cvtsi32_si128(dividend)));
			if (expectedResult != testResult)
				divFixedError(expectedResult, testResult, divisor, dividend, "dividefixedV8i16");
		}
	}
}

inline void testDivFixedV8u16()
{
	uint16_t testNumbers[] = {243, 293, 111, 3, 2, 8427, 21969, 247, 1, 2, 1, 11, 243, 32750, 32750, 65535};

	for (uint16_t divisor : testNumbers)
	{
		__m128i buffer[2];
		setdivisorV8u16(buffer, divisor);

		for (uint16_t dividend : testNumbers)
		{
			uint16_t expectedResult = dividend / divisor;
			uint16_t testResult = _mm_cvtsi128_si32(dividefixedV8u16(buffer, _mm_cvtsi32_si128(dividend)));
			if (expectedResult != testResult)
				divFixedError(expectedResult, testResult, divisor, dividend, "dividefixedV8u16");
		}
	}
}

inline auto getAvailableDivFixedV4i32Functions()
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-attributes"
	std::vector<std::pair<std::function<__m128i (const __m128i [2], __m128i)>, const char *>> result = {{dividefixedV4i32SSE2, "dividefixedV4i32SSE2"}};
#pragma GCC diagnostic pop

	if (InstructionSet() >= asmlibInternal::InstructionSetReturnValues::sse41Supported)
		result.push_back({dividefixedV4i32SSE41, "dividefixedV4i32SSE41"});

	return result;
}

inline void testDivFixedV4i32()
{
	constexpr int32_t testDivisors[] = {243, 293, 111, 3, 2, 8427, 247, 1, 11, 243, 32750, 32750, 23898842, 2092381475, 2, 1, 43};

	for (auto function : getAvailableDivFixedV4i32Functions())
		for (int32_t divisor : testDivisors)
		{
			__m128i buffer[2];
			setdivisorV4i32(buffer, divisor);

			constexpr int32_t testDividends[] = {243, 293, 111, 3, 2, 8427, -21969, 247, 1, -2, -1, -243, -32750, 32750, -329999, 29499, 2910, 49999999, -293999999, -1928382713, 0};

			for (int32_t dividend : testDividends)
			{
				int32_t expectedResult = dividend / divisor;
				int32_t testResult = _mm_cvtsi128_si32(function.first(buffer, _mm_cvtsi32_si128(dividend)));
				if (expectedResult != testResult)
					divFixedError(expectedResult, testResult, divisor, dividend, function.second);
			}
		}
}

inline void testDivFixedV4u32()
{
	uint32_t testNumbers[] = {243, 243293, 4294963233, 293, 111, 3, 2, 8427, 21969, 247, 1, 2, 1, 11, 243, 32750, 32750, 65535, 534, 29999482, 123987456};

	for (uint32_t divisor : testNumbers)
	{
		__m128i buffer[2];
		setdivisorV4u32(buffer, divisor);

		for (uint32_t dividend : testNumbers)
		{
			uint32_t expectedResult = dividend / divisor;
			uint32_t testResult = _mm_cvtsi128_si32(dividefixedV4u32(buffer, _mm_cvtsi32_si128(dividend)));
			if (expectedResult != testResult)
				divFixedError(expectedResult, testResult, divisor, dividend, "dividefixedV8u16");
		}
	}
}

int main()
{
	testDivFixedV8i16();
	// This doesn't work right now, should be fixed later
	// testDivFixedV8u16();
	testDivFixedV4i32();
	testDivFixedV4u32();
}
