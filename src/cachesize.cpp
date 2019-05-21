#include <cstddef>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <cpuid.h>
#include "asmlib.h"

struct descriptorRecord	// Record for table of cache descriptors
{
	uint8_t key;	// Key from cpuid instruction
	uint8_t level;	// Cache level
	uint8_t sizeMultiplier;	// Size multiplier
	uint8_t pow2;	// Power of 2, size = sizeMultiplier << pow2
};

struct dataLayout	// Reference point
{
	bool ok;	// true if values have been determined
	union
	{
		struct
		{
			size_t level1;	// Level 1 data cache size
			size_t level2;	// Level 2 data cache size
			size_t level3;	// Level 3 data cache size
			size_t level4;	// Level 4 data cache size
		};
		size_t levels[4];
	};
	descriptorRecord descriptorTable[61];
};

constexpr size_t numLevels = 4;	// Max level

static bool IntelNewMethod(dataLayout& dataRef)
{
	uint32_t eax, ebx, ecx, edx;
	__cpuid(0, eax, ebx, ecx, edx);	// Get number of CPUID functions
	if (eax < 4)
		return false;	// Fail

	for (size_t i = 0; i < 0x100; ++i)
	{
		__cpuid_count(4, i, eax, ebx, ecx, edx);	// Get cache parameters

		// Check cache type to determine whether there are remaining caches
		auto cacheType = eax & 0b11111;
		if (!cacheType)	// No more caches
			return !(dataRef.level1 < 0x400);	// Check if ok

		if (cacheType == 2)
			continue;	// Code cache, ignore

		++ecx;

		ecx = (int)ecx * ((ebx >> 22) + 1);	// Ways

		ecx = (int)ecx * (((ebx >> 12) & 0b1111111111) + 1);	// Partitions

		ebx = (ebx & 0b111111111111) + 1;	// Line size
		size_t calculatedCacheSize = (ssize_t)ecx * ebx; // Calculated cache size

		eax = (eax >> 5) & 0b111;	// Cache level

		eax = std::min((size_t)eax, numLevels);
		dataRef.levels[eax - 1] = calculatedCacheSize;	// Store size of data (eax is level)
	}

	return !(dataRef.level1 < 0x400);
}

static bool IntelOldMethod(dataLayout& dataRef)
{
	uint32_t eax, ebx, ecx, edx;
	__cpuid(0, eax, ebx, ecx, edx);	// Get number of CPUID functions

	if (eax < 2)
		return false;	// Fail

	__cpuid_count(2, 0, eax, ebx, ecx, edx);	// Get 16 descriptor bytes in eax, ebx, ecx, edx

	// Save all descriptors
	union
	{
		uint32_t descriptors32[4];
		uint8_t descriptors[16];
	};
	eax = (eax >> 8) << 8;	// Clear lowest 8 bytes (al does not contain a descriptor)
	descriptors32[0] = eax;
	descriptors32[1] = ebx;
	descriptors32[2] = ecx;
	descriptors32[3] = edx;

	// Loop to read 16 descriptor bytes
	for (uint32_t i = 0; i < 16; ++i)
	{
		auto currentDescriptor = descriptors[i];
		constexpr auto descriptorTblCnt = (sizeof(dataRef.descriptorTable) / sizeof(dataRef.descriptorTable[0]));

		// Search in descriptor table
		auto begin = &dataRef.descriptorTable[0];
		auto end = dataRef.descriptorTable + descriptorTblCnt;
		auto descriptor = std::find_if(begin, end,
		[&currentDescriptor] (descriptorRecord& record)
		{
			return record.key == currentDescriptor;
		});

		if (descriptor == end)
			continue;	// Descriptor not found

		// Descriptor found
		size_t cacheSize = descriptor->sizeMultiplier;
		cacheSize <<= descriptor->pow2;

		auto tmpLvl = descriptor->level;
		if (tmpLvl > 3)
			continue;
		dataRef.levels[tmpLvl - 1] = cacheSize;
	}

	// Check if OK
	return !(dataRef.level1 < 0x400);
}

static bool AMDMethod(dataLayout& dataRef)
{
	uint32_t eax, ebx, ecx, edx;
	__cpuid(0x80000000, eax, ebx, ecx, edx);	// Get number of CPUID functions

	if (eax < 6)
		return false;	// Fail

	__cpuid(0x80000005, eax, ebx, ecx, edx);	// Get L1 cache size
	auto l1DataCacheSize = (ecx >> 24) << 10; // L1 data cache size in bytes
	dataRef.level1 = l1DataCacheSize;	// Store L1 data cache size

	__cpuid(0x80000006, eax, ebx, ecx, edx);	// Get L2 and L3 cache sizes
	auto l2DataCacheSize = (ecx >> 16) << 10; // L2 data cache size in bytes
	dataRef.level2 = l2DataCacheSize;	// Store L2 data cache size

	auto l3DataCacheSize = (edx >> 18) << 19; // L3 data cache size in bytes

#if 0
	// AMD manual is unclear: Do we have to increase the value if the number of ways is not a power of 2 ?
	edx = (edx >> 12) & 0b1111; // L3 associativity
	if (edx < 3)
		goto K100;

	if (!(edx & 1))
		goto K100;

	// Number of ways is not a power of 2, multiply by 1.5 ?
	l3DataCacheSize += l3DataCacheSize >> 1;

K100:

#endif

	dataRef.level3 = l3DataCacheSize;	// Store L3 data cache size

	// Check if OK
	return !(dataRef.level1 < 0x400);
}

extern "C" size_t DataCacheSize(int level)
{
	static dataLayout dataRef =
	{
		false,	// Ok
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

	if (!(dataRef.ok))
	{
		// Cache values haven't already been determined
		int vendor;
		CpuType(&vendor, nullptr, nullptr);

		if (vendor == 1)	// Intel
		{
			if (!IntelNewMethod(dataRef))
				IntelOldMethod(dataRef);
		}
		else if (vendor == 2 || vendor == 3) // AMD or VIA
		{
			AMDMethod(dataRef);
		}
		else	// Other
		{
			if (!IntelNewMethod(dataRef))
				if (!AMDMethod(dataRef))
					IntelOldMethod(dataRef);
		}
	}

	uint32_t result = 0;
	size_t lvl = level;
	if (lvl > numLevels)
		return result;

	if (lvl == 0)
	{
		// Level = 0. Get size of largest level cache
		result = dataRef.level3;
		if (result)
			goto finish;

		result = dataRef.level2;
		if (result)
			goto finish;

		result = dataRef.level1;
		goto finish;
	}

	// 0 < level < numLevels
	result = dataRef.levels[lvl - 1];	// Size of selected cache

finish:
	dataRef.ok = true;	// Remember called, whether success or not
	return result;
}
