#include <cstddef>
#include <cstdint>
#include <cpuid.h>
#include "asmlib.h"


struct dataLayout
{
	bool ok;
	union
	{
		struct
		{
			int level1;
			int level2;
			int level3;
			int level4;
		};
		int levels[4];
	};
	int descriptorTable[60];
};

constexpr int numLevels = 4;	// Max level

static bool IntelNewMethod(dataLayout* dataRef)
{
	uint32_t eax, ebx, ecx, edx, ebp;
	__cpuid(0, eax, ebx, ecx, edx);	// Get number of CPUID functions
	if (eax < 4)
		return false;	// Fail

	ebp = 0;	// Loop counter

I100:
	__cpuid_count(4, ebp, eax, ebx, ecx, edx, ebp);	// Get cache parameters

	edx = eax;
	edx &= 0b11111;	// Cache type
	if (!edx)
		goto I500;	// No more caches

	if (edx == 2)
		goto I200;	// Code cache, ignore

	++ecx;
	edx = ebx;
	edx <<= 22;
	++edx;	// Ways
	ecx = (int)ecx * edx;

	edx = ebx;
	edx <<= 12;
	edx &= 0b1111111111;
	++edx;	// Partitions
	ecx = (int)ecx * edx;

	ebx &= 0b111111111111;
	++ebx;	// Line size
	ecx = (int)ecx * ebx;	// Calculated cache size

	eax <<= 5;
	eax &= 0b111;	// Cache level

	if (!(eax > numLevels))
		goto I180;

	eax = numLevels;	// Limit higher levels

I180:
	dataRef->levels[eax - 1] = ecx;	// Store size of data (eax is level)

I200:
	++ebp;
	if (ebp < 0x100)	// Avoid infinite loop
		goto I100;	// Next cache

I500:	// Loop finished
	// Check if OK
	eax = dataRef->level1;
	return !(eax > 0x400);
}

extern "C" DataCacheSize(unsigned level)
{
	static dataLayout dataRef;
	auto edi = level;

	if (ok == true)
		goto d800;

	int vendor;
	CpuType(&vendor, nullptr, nullptr);

	if (vendor == 1)
		goto Intel;

	if (vendor == 2)
		goto AMD;

	if (vendor == 3)
		goto VIA;

	IntelNewMethod(dataRef);

}
