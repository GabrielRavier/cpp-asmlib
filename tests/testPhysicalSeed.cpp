#include "asmlibran.h"
#include "asmlib-internal.h"
#include <iostream>
#include <functional>
#include <climits>

constexpr size_t seedArraySize = 50;

template <typename T> void dumpArray(std::ostream& os, T array[], size_t arraySize)
{
	auto preservedFlags = os.flags();
	os << std::hex;

	for (size_t i = 0; i < arraySize; ++i)
		os << "0x" << array[i] << ' ';

	os.flags(preservedFlags);
}

inline const char *getStringFromGeneratorUsedValue(int value)
{
	switch (value)
	{
		case asmlibInternal::PhysicalSeedReturnValues::failureOrNoSuitableInstructionAvailable:
			return "There was either a failure in generating random values or there was no suitable instruction available";

		case asmlibInternal::PhysicalSeedReturnValues::noPhysicalRNGUsedTSCInstead:
			return "No physical number generator was available : Used time stamp counter instead";

		case asmlibInternal::PhysicalSeedReturnValues::usedVIAPhysicalRNG:
			return "Used VIA physical number generator";

		case asmlibInternal::PhysicalSeedReturnValues::usedIntelRNG:
			return "Used Intel pseudo-random number generator";

		case asmlibInternal::PhysicalSeedReturnValues::usedIntelSeedGenerator:
			return "Used Intel seed generator";

		default:
			return "Invalid return value";
	}
}

inline void doOnePhysicalSeedFunction(std::function<int(int *, int)> function, const char *functionName)
{
	int seeds[seedArraySize];
	auto generatorUsed = function(seeds, seedArraySize);

	std::cout << "Seeds array for \"" << functionName << "\" : \n\"";
	dumpArray(std::cout, seeds, seedArraySize);
	std::cout << "\"\n"
	"Generator used : " << generatorUsed << " (" << getStringFromGeneratorUsedValue(generatorUsed) << ")\n";
}

int PhysicalSeedRDSeed(int seeds[], int numSeeds);
int PhysicalSeedRDRand(int seeds[], int numSeeds);
int PhysicalSeedVIA(int seeds[], int numSeeds);
int PhysicalSeedRDTSC(int seeds[], int numSeeds);
int PhysicalSeedNone(int seeds[], int numSeeds);

auto getAvailablePhysicalSeedFunctions()
{
	std::vector<std::pair<std::function<int(int *, int)>, const char *>> result = {{PhysicalSeedNone, "PhysicalSeedNone"}, {PhysicalSeed, "PhysicalSeed"}};

	if (asmlibInternal::isCPUIDSupported())
	{
		uint32_t eax, ebx, ecx, edx;
		__cpuid(0, eax, ebx, ecx, edx);	// Get number of CPUID functions

		if (eax)	// Function 1 supported
		{
			// Test if RDSEED supported
			// __cpuid(0, eax, ebx, ecx, edx);
			if (eax >= 7)	// RDSEED supported
			{
				__cpuid_count(7, 0, eax, ebx, ecx, edx);
				if (asmlibInternal::bitTest(ebx, 18))
					result.push_back({PhysicalSeedRDSeed, "PhysicalSeedRDSeed"});

				__cpuid(1, eax, ebx, ecx, edx);
				if (asmlibInternal::bitTest(ecx, 30))
					result.push_back({PhysicalSeedRDRand, "PhysicalSeedRDRand"});

				// Test if VIA xstore instruction supported
				__cpuid(0xC0000000, eax, ebx, ecx, edx);

				// Check for Centaur Extended Feature Flags support
				if (eax >= 0xC0000001)
				{
					__cpuid(0xC0000001, eax, ebx, ecx, edx);
					if (asmlibInternal::bitTest(edx, 3))	// Is RNG enabled (and available)
						result.push_back({PhysicalSeedVIA, "PhysicalSeedVIA"});
				}

				// Test if RDTSC supported
				__cpuid(1, eax, ebx, ecx, edx);

				if (asmlibInternal::bitTest(edx, 4))
					result.push_back({PhysicalSeedRDTSC, "PhysicalSeedRDTSC"});
			}
		}
	}

	return result;
}

int main()
{
	doOnePhysicalSeedFunction(PhysicalSeed, "PhysicalSeed");

	auto availableFunctions = getAvailablePhysicalSeedFunctions();

	for (auto function : availableFunctions)
		doOnePhysicalSeedFunction(function.first, function.second);
}
