#include "asmlib.h"
#include <cstdint>
#include <iostream>
#include <cstdlib>

template <typename T> [[noreturn]] void divFixedError(T dividend, T divisor, T expectedResult, T falseResult)
{
	std::cerr << "divFixed failed to give correct results !\n"
			<< "Dividend : " << dividend << "\n"
			"Divisor : " << divisor << "\n"
			"Expected result : " << expectedResult << "\n"
			"False result : " << falseResult << '\n';
	std::exit(1);
}

int main()
{
	constexpr int32_t divisorsToTest[] = {23, 1, 6, 93, 2388, 44, 35, 238488};

	for (auto divisor : divisorsToTest)
	{
		int32_t buffer[2];
		setdivisori32(buffer, divisor);

		constexpr int32_t dividendsToTest[] = {4, 7, 1, -1, -45, -23, -52, -923888, 3, 6, 2, 5, 353555, 6444444, 238, 492, 23, 293819};

		for (auto dividend : dividendsToTest)
		{
			auto expectedResult = dividend / divisor;
			auto testResult = dividefixedi32(buffer, dividend);
			if (expectedResult != testResult)
				divFixedError(dividend, divisor, expectedResult, testResult);
		}
	}

	constexpr uint32_t unsignedDivisorsToTest[] = {23, 1, 6, 93, 2388, 44, 35, 238488};

	for (auto divisor : unsignedDivisorsToTest)
	{
		uint32_t buffer[2];
		setdivisoru32(buffer, divisor);

		constexpr uint32_t dividendsToTest[] = {4, 7, 1, 3, 6, 2, 5, 353555, 6444444, 238, 492, 23, 293819};
		for (auto dividend : dividendsToTest)
		{
			auto expectedResult = dividend / divisor;
			auto testResult = dividefixedu32(buffer, dividend);
			if (expectedResult != testResult)
				divFixedError(dividend, divisor, expectedResult, testResult);
		}
	}
}
