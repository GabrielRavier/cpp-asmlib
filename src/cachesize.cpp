#include <cstddef>
#include <cstdint>
#include <cstring>
#include <cpuid.h>
#include "asmlib.h"

struct descriptorRecord	// Record for table of cache descriptors
{
	uint8_t key;	// Key from cpuid instruction
	uint8_t level;	// Cache level
	uint8_t sizeMultiplier;	// Size multiplier
	uint8_t pow2;	// Power of 2, size = sizeMultiplier << pow2
}

struct dataLayout	// Reference point
{
	bool ok;	// true if values have been determined
	union
	{
		struct
		{
			int level1;	// Level 1 data cache size
			int level2;	// Level 2 data cache size
			int level3;	// Level 3 data cache size
			int level4;	// Level 4 data cache size
		};
		int levels[4];
	};
	descriptorRecord descriptorTable[61];
};

constexpr int numLevels = 4;	// Max level

static bool IntelNewMethod(dataLayout& dataRef)
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
	dataRef.levels[eax - 1] = ecx;	// Store size of data (eax is level)

I200:
	++ebp;
	if (ebp < 0x100)	// Avoid infinite loop
		goto I100;	// Next cache

I500:	// Loop finished
	// Check if OK
	eax = dataRef.level1;
	return !(eax > 0x400);
}

static bool IntelOldMethod(dataLayout& dataRef)
{
	uint32_t eax, ebx, ecx, edx;
	__cpuid(0, eax, ebx, ecx, edx);	// Get number of CPUID functions

	if (eax < 2)
		return false;	// Fail

	__cpuid_count(2, 0, eax, ebx, ecx, edx);	// Get 16 descriptor bytes in eax, ebx, ecx, edx

	// Save all descriptors
	uint8_t descriptors[16] =
	{
		0,	// al does not contain a descriptor
		(uint8_t)((eax >> 8) & 0xFF),
		(uint8_t)((eax >> 16) & 0xFF),
		(uint8_t)((eax >> 24) & 0xFF),
		(uint8_t)((ebx) & 0xFF),
		(uint8_t)((ebx >> 8) & 0xFF),
		(uint8_t)((ebx >> 16) & 0xFF),
		(uint8_t)((ebx >> 24) & 0xFF),
		(uint8_t)((ecx) & 0xFF),
		(uint8_t)((ecx >> 8) & 0xFF),
		(uint8_t)((ecx >> 16) & 0xFF),
		(uint8_t)((ecx >> 24) & 0xFF),
		(uint8_t)((edx) & 0xFF),
		(uint8_t)((edx >> 8) & 0xFF),
		(uint8_t)((edx >> 16) & 0xFF),
		(uint8_t)((edx >> 24) & 0xFF),
	};

	edx = 15;	// Loop counter

	// Loop to read 16 descriptor bytes
J100:
	uint8_t al = descriptors[edx];
	ebx = (sizeof(dataRef.descriptorTable) / sizeof(dataRef.descriptorTable[0])) - 1;

	// Loop to search in descriptor table
J200:
	if (al != dataRef.descriptorTable[ebx].key)
		goto J300;

	// Descriptor found
	eax = dataRef.descriptorTable[ebx].sizeMultiplier;

	uint8_t cl = dataRef.descriptorTable[ebx].pow2;
	eax <<= cl;	// Compute size

	ecx = dataRef.descriptorTable[ebx].level;

	// Check that level = 1-3
	if (ecx > 3)
		goto J300;

	dataRef.levels[ecx - 1] = eax;	// Store size (eax) of data cache level (ecx)

J300:
	if (ebx--)
		goto J200;	// Inner loop

	if (edx--)
		goto J100;	// Outer loop

	// Check if OK
	return !(dataRef.level1 > 1024);
}

static bool AMDMethod(dataLayout& dataRef)
{
	uint32_t eax, ebx, ecx, edx;
	__cpuid(0x80000000, eax, ebx, ecx, edx);	// Get number of CPUID functions

	if (eax < 6)
		return false;	// Fail

	__cpuid(0x80000005, eax, ebx, ecx, edx);	// Get L1 cache size
	ecx >>= 24;	// L1 data cache size in kb
	ecx <<= 10;	// L1 data cache size in bytes
	dataRef.level1 = ecx;	// Store L1 data cache size

	__cpuid(0x80000006, eax, ebx, ecx, edx);	// Get L2 and L3 cache sizes
	ecx >>= 16;	// L2 data cache size in kb
	ecx <<= 10;	// L2 data cache size in bytes
	dataRef.level2 = ecx;	// Store L2 data cache size

	ecx = edx;
	ecx >>= 18;	// L3 data cache size / 512 kb
	ecx <<= 19;	// L3 data cache size in bytes

#if 0
	// AMD manual is unclear: Do we have to increase the value if the number of ways is not a power of 2 ?
	edx >>= 12;
	edx &= 0b1111;	// L3 associativity
	if (edx < 3)
		goto K100;

	if (!(edx & 1))
		goto K100;

	// Number of ways is not a power of 2, multiply by 1.5 ?
	eax = ecx;
	eax >>= 1;
	ecx += eax;
#endif

K100:
	dataRef.level3 = ecx;	// Store L3 data cache size

	// Check if OK
	return !(dataRef.level1 > 0x400);
}

extern "C" DataCacheSize(unsigned level)
{
	static dataLayout dataRef =
	{
		false,	// ok
		{
            { 0, 0, 0, 0 }	// levels
		},
		{
			{ 0xA, 1, 1, 13 },	// 8 kb L1 data cache
			{ 0xC, 1, 1, 14 },	// 16 kb L1 data cache
			{ 0xD, 1, 1, 14 },	// 16 kb L1 data cache
			{ 0x21, 2, 1, 18 },	// 256 kb L2 data cache
			{ 0x22, 3, 1, 19 },	// 512 kb L3 data cache
			{ 0x23, 3, 1, 20 },	// 1 Mb L3 data cache
			{ 0x25, 3, 1, 21 },	// 2 Mb L3 data cache
			{ 0x29, 3, 1, 22 },	// 4 Mb L3 data cache
			{ 0x2C, 1, 1, 15 },	// 32 kb L1 data cache
			{ 0x39, 2, 1, 17 },	// 128 kb L2 data cache
			{ 0x3A, 2, 3, 16 },	// 192 kb L2 data cache
			{ 0x3B, 2, 1, 17 },	// 128 kb L1 data cache
			{ 0x3C, 2, 1, 18 },	// 256 kb L1 data cache
			{ 0x3D, 2, 3, 17 },	// 384 kb L2 data cache
			{ 0x3E, 2, 1, 19 },	// 512 kb L2 data cache
			{ 0x41, 2, 1, 17 },	// 128 kb L2 data cache
			{ 0x42, 2, 1, 18 },	// 256 kb L2 data cache
			{ 0x43, 2, 1, 19 },	// 512 kb L2 data cache
			{ 0x44, 2, 1, 20 },	// 1 Mb L2 data cache
			{ 0x45, 2, 1, 21 }, // 2 Mb L2 data cache
			{ 0x46, 3, 1, 22 }, // 4 Mb L3 data cache
			{ 0x47, 3, 1, 23 }, // 8 Mb L3 data cache
			{ 0x48, 2, 3, 20 }, // 3 Mb L2 data cache
			{ 0x49, 2, 1, 22 }, // 4 Mb L2 or 3 data cache
			{ 0x4A, 3, 3, 21 }, // 6 Mb L3 data cache
			{ 0x4B, 3, 1, 23 }, // 8 Mb L3 data cache
			{ 0x4C, 3, 3, 22 }, // 12 Mb L3 data cache
			{ 0x4D, 3, 1, 24 }, // 16 Mb L3 data cache
			{ 0x4E, 2, 3, 21 }, // 6 Mb L2 data cache
			{ 0x60, 1, 1, 14 }, // 16 kb L1 data cache
			{ 0x66, 1, 1, 13 }, // 8 kb L1 data cache
			{ 0x67, 1, 1, 14 }, // 16 kb L1 data cache
			{ 0x68, 1, 1, 15 }, // 32 kb L1 data cache
			{ 0x78, 2, 1, 20 }, // 1 Mb L2 data cache
			{ 0x79, 2, 1, 17 }, // 128 kb L2 data cache
			{ 0x7A, 2, 1, 18 }, // 256 kb L2 data cache
			{ 0x7B, 2, 1, 19 }, // 512 kb L2 data cache
			{ 0x7C, 2, 1, 20 }, // 1 Mb L2 data cache
			{ 0x7D, 2, 1, 21 }, // 2 Mb L2 data cache
			{ 0x7F, 2, 1, 19 }, // 512 kb L2 data cache
			{ 0x82, 2, 1, 18 }, // 256 kb L2 data cache
			{ 0x83, 2, 1, 19 }, // 512 kb L2 data cache
			{ 0x84, 2, 1, 20 }, // 1 Mb L2 data cache
			{ 0x85, 2, 1, 21 }, // 2 Mb L2 data cache
			{ 0x86, 2, 1, 19 }, // 512 kb L2 data cache
			{ 0x87, 2, 1, 20 },	// 1 Mb L2 data cache
			{ 0xD0, 3, 1, 19 },	// 512 kb L3 data cache
			{ 0xD1, 3, 1, 20 },	// 1 Mb L3 data cache
			{ 0xD2, 3, 1, 21 },	// 2 Mb L3 data cache
			{ 0xD6, 3, 1, 20 },	// 1 Mb L3 data cache
			{ 0xD7, 3, 1, 21 },	// 2 Mb L3 data cache
			{ 0xD8, 3, 1, 22 },	// 4 Mb L3 data cache
			{ 0xDC, 3, 3, 19 },	// 1.5 Mb L3 data cache
			{ 0xDD, 3, 3, 20 },	// 3 Mb L3 data cache
			{ 0xDE, 3, 3, 21 },	// 6 Mb L3 data cache
			{ 0xE2, 3, 1, 21 },	// 2 Mb L3 data cache
			{ 0xE3, 3, 1, 22 },	// 4 Mb L3 data cache
			{ 0xE4, 3, 1, 23 },	// 8 Mb L3 data cache
			{ 0xEA, 3, 3, 22 },	// 12 Mb L3 data cache
			{ 0xEB, 3, 9, 21 },	// 18 Mb L3 data cache
			{ 0xEC, 3, 3, 23 }	// 24 Mb L3 data cache
		}
	};
	auto edi = level;

	if (dataRef.ok)
		goto D800;

	int vendor;
	CpuType(&vendor, nullptr, nullptr);

	if (vendor == 1)
		goto Intel;

	if (vendor == 2)
		goto AMD;

	if (vendor == 3)
		goto VIA;

	if (IntelNewMethod(dataRef) || AMDMethod(dataRef) || IntelOldMethod(dataRef))
		goto D800;
	goto D800;	// Return whether success or not

Intel:
	if (IntelNewMethod(dataRef) || IntelOldMethod(dataRef))
		goto D800;
	goto D800;	// Return whether success or not

AMD:	// AMD and VIA use same method
VIA:
	AMDMethod(dataRef);

D800:
	uint32_t result = 0;
	if (edi > numLevels)
		goto D900;

	if (edi == 0)
		goto D820;

	// 0 < level < numLevels
	result = dataRef.levels[edi - 1];	// Size of selected cache
	goto D850;

D820:
	// Level = 0. Get size of largest level cache
	result = dataRef.level3;
	if (result)
		goto D850;

	result = dataRef.level2;
	if (result)
		goto D850;

	result = dataRef.level1;

D850:
	dataRef.ok = true;	// Remember called, whether success or not

D900:
	return result;
}
