#include "asmlib.h"

namespace unalignedIsFasterRetVals
{
	constexpr int probablySlower = 0;	// Unaligned read is probably slower than alignment shift
	constexpr int unknown = 1;
	constexpr int probablyFaster = 2;	// Unaligned read is probably faster than alignment shift
}

extern "C" int UnalignedIsFaster()
{
	int vendor, family, model;
	CpuType(&vendor, &family, &model);	// Get vendor, family, model

	int result = 0;
	if (vendor == 1)	// Intel
	{
		// Pentium 1 or older, Old Netburst architecture, earlier than Nehalem or Atom
		if (family <= 6 || family == 0xF || family <= 0x1A || family == 0x1C)
			return unalignedIsFasterRetVals::probablySlower;

		// Nehalem and later except Atom
		return unalignedIsFasterRetVals::probablyFaster;
	}
	else if (vendor == 2)	// AMD
	{
		/* The palignr instruction is slow on AMD Bobcat but fast on Jaguar
		 * K10/Opteron (0x10) : Use unaligned
		 * Bobcat (0x14) : palignr is very slow. Use unaligned
		 * Piledriver (0x15) : Use unaligned
		 * Jaguar (0x16) : palignr is fast. Use aligned (aligned is faster in most cases, but not all)
		 */

		// K8 or earlier or Jaguar
		if (family <= 0x10 || family == 0x16)
			return unalignedIsFasterRetVals::probablySlower;

		// K10 or later
		return unalignedIsFasterRetVals::probablyFaster;
	}
	else if (vendor == 3)	// VIA
	{
		// Unaligned read is not faster than PALIGNR on VIA Nano 2000 and 3000
		if (family <= 0xF)
			return unalignedIsFasterRetVals::probablySlower;
		// Future versions : unknown
		return unalignedIsFasterRetVals::unknown;
	}
	else
		return unalignedIsFasterRetVals::unknown;	// Unknown
}

namespace store256FasterRetVals
{
	constexpr int thirtyTwoBytesMemoryWriteSlowerOrAVXNotSupported = 0;
	constexpr int unknown = 1;
	constexpr int thirtyTwoBytesMemoryWriteFaster = 2;
}

extern "C" int Store256IsFaster()
{
	//
}
