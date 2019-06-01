#include "asmlibran.h"
#include "asmlib-internal.h"
#include <iostream>
#include <cstdint>

using namespace asmlibInternal::randomNumberGenerators;

[[noreturn]] inline void motherSizeError()
{
	std::cerr << "ERROR : sizeof(mother::internals) != MOTHER_BUFFERSIZE !";
	std::cerr.flush();
	std::quick_exit(1);
}

[[noreturn]] inline void motherCheckError(uint32_t asmResult, uint32_t cppResult, uint32_t staticResult, size_t i)
{
	std::cerr << "ERROR : On " << i << "th iteration of checking mother RNG generator results, got : \n"
	"asmResult : " << asmResult << "\n"
	"staticResult : " << staticResult << "\n"
	"When " << cppResult << " was expected !";
	std::cerr.flush();

	std::quick_exit(1);
}

class CRandomMother                    // Encapsulate random number generator
{
public:
	void RandomInit(int seed)          // Initialization
	{
		int i;
		uint32_t s = seed;
		// make random numbers and put them into the buffer
		for (i = 0; i < 5; i++)
		{
			s = s * 29943829 - 1;
			x[i] = s;
		}
		// randomize some more
		for (i = 0; i < 19; i++) BRandom();
	}

	int IRandom(int min, int max)      // Get integer random number in desired interval
	{
		// Output random integer in the interval min <= x <= max
		// Relative error on frequencies < 2^-32
		if (max <= min)
		{
			if (max == min)
				return min;
			else
				return 0x80000000;
		}
		// Assume 64 bit integers supported. Use multiply and shift method
		uint32_t interval;                  // Length of interval
		uint64_t longran;                   // Random bits * interval
		uint32_t iran;                      // Longran / 2^32

		interval = (uint32_t)(max - min + 1);
		longran  = (uint64_t)BRandom() * interval;
		iran = (uint32_t)(longran >> 32);
		// Convert back to signed and return result
		return (int32_t)iran + min;
	}

	double Random()                    // Get floating point random number
	{
		return (double)BRandom() * (1. / (65536.*65536.));
	}

	uint32_t BRandom()                 // Output random bits
	{
		uint64_t sum;
		sum = (uint64_t)2111111111ULL * (uint64_t)x[3] +
		      (uint64_t)1492 * (uint64_t)(x[2]) +
		      (uint64_t)1776 * (uint64_t)(x[1]) +
		      (uint64_t)5115 * (uint64_t)(x[0]) +
		      (uint64_t)x[4];
		x[3] = x[2];  x[2] = x[1];  x[1] = x[0];
		x[4] = (uint32_t)(sum >> 32);                  // Carry
		x[0] = (uint32_t)sum;                          // Low 32 bits of sum
		return x[0];
	}

	CRandomMother(int seed)             // Constructor
	{
		RandomInit(seed);
	}

protected:
	uint32_t x[5];                      // History buffer
};

int main()
{
	if (sizeof(mother::internals) != MOTHER_BUFFERSIZE)
		motherSizeError();

	int seed;
	PhysicalSeed(&seed, 1);

	CRandomMotherA motherAsm(0);
	CRandomMother motherCpp(0);
	motherAsm.RandomInit(seed);
	motherCpp.RandomInit(seed);
	MotherRandomInit(seed);

	for (size_t i = 0; i < 10000; ++i)
	{
		auto asmResult = motherAsm.BRandom();
		auto staticResult = MotherBRandom();
		auto cppResult = motherCpp.BRandom();

		if (cppResult != asmResult || cppResult != staticResult)
			motherCheckError(asmResult, cppResult, staticResult, i);
	}
}
