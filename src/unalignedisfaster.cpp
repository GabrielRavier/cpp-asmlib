#include "asmlib.h"
#include "asmlib-internal.h"

extern "C" int UnalignedIsFaster()
{
	int vendor, family, model;
	CpuType(&vendor, &family, &model);	// Get vendor, family, model

	if (vendor == 1)	// Intel
	{
		// Pentium 1 or older, Old Netburst architecture, earlier than Nehalem or Atom
		if (family <= 6 || family == 0xF || family <= 0x1A || family == 0x1C)
			return asmlibInternal::unalignedIsFasterReturnValues::probablySlower;

		// Nehalem and later except Atom
		return asmlibInternal::unalignedIsFasterReturnValues::probablyFaster;
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
			return asmlibInternal::unalignedIsFasterReturnValues::probablySlower;

		// K10 or later
		return asmlibInternal::unalignedIsFasterReturnValues::probablyFaster;
	}
	else if (vendor == 3)	// VIA
	{
		// Unaligned read is not faster than PALIGNR on VIA Nano 2000 and 3000
		if (family <= 0xF)
			return asmlibInternal::unalignedIsFasterReturnValues::probablySlower;
		// Future versions : unknown
		return asmlibInternal::unalignedIsFasterReturnValues::unknown;
	}
	else
		return asmlibInternal::unalignedIsFasterReturnValues::unknown;	// Unknown
}

extern "C" int Store256BitIsFaster()
{
	// Check AVX support first
	if (InstructionSet() < 11)
		return asmlibInternal::store256FasterReturnValues::thirtyTwoBytesMemoryWriteSlowerOrAVXNotSupported;

	int vendor, family, model;
	CpuType(&vendor, &family, &model);

	if (vendor == 1)	// Intel
	{
		if (family != 6)	// Unknown family, possibly future model
			return asmlibInternal::store256FasterReturnValues::thirtyTwoBytesMemoryWriteFaster;

		if (model <= 0x3A)	// Ivy Bridge or Sandy Bridge
			return asmlibInternal::store256FasterReturnValues::thirtyTwoBytesMemoryWriteSlowerOrAVXNotSupported;

		// Haswell is much faster with 256 bit moves
		return asmlibInternal::store256FasterReturnValues::thirtyTwoBytesMemoryWriteFaster;
	}
	else if (vendor == 2)	// AMD
	{
		if (family > 0x15)	// Family 0x15 : Bulldozer, Piledriver
			return asmlibInternal::store256FasterReturnValues::thirtyTwoBytesMemoryWriteFaster;	// Assume future AMD families are faster

		// Model 1 : Bulldozer is a little slower on 256 bit write
		// Model 2 : Piledriver is terribly slow on 256 bit write
		// Model 0x30 : Steamroller is reasonable on 256 bit write

		if (model <= 0x30)
			return asmlibInternal::store256FasterReturnValues::thirtyTwoBytesMemoryWriteSlowerOrAVXNotSupported;

		return asmlibInternal::store256FasterReturnValues::unknown;	// Later models : Don't know
	}
	else if (vendor == 3)	// VIA
	{
		return asmlibInternal::store256FasterReturnValues::unknown;	// Don't know
	}
	else	// Unknown
		return asmlibInternal::store256FasterReturnValues::unknown;	// Don't know
}
