#include "asmlib.h"
#include <cstdint>
#include <cstring>
#include <cpuid.h>

static bool isCPUIDSupported()
{
	#if __i386__
	int __cpuid_supported;

	__asm("  pushfl\n"
	"  popl   %%eax\n"
	"  movl   %%eax,%%ecx\n"
	"  xorl   $0x00200000,%%eax\n"
	"  pushl  %%eax\n"
	"  popfl\n"
	"  pushfl\n"
	"  popl   %%eax\n"
	"  movl   $0,%0\n"
	"  cmpl   %%eax,%%ecx\n"
	"  je     1f\n"
	"  movl   $1,%0\n"
	"1:"
	: "=r" (__cpuid_supported) : : "eax", "ecx");
	if (!__cpuid_supported)
		return false;
	return true;
	#else
	// x86-64
	return true;
	#endif
}

extern "C" char *ProcessorName()
{
	static char NameBuffer[0x50];	// Static buffer to contain name

	if (!isCPUIDSupported())
		goto NOID;

	uint32_t eax, ebx, ecx, edx;
	__cpuid(0, eax, ebx, ecx, edx);
	if (eax >= 1)
		goto IDENTIFYABLE;

NOID:
	// Processor has no CPUID
	strcpy(NameBuffer, "80386 or 80386");
	goto PNEND;

IDENTIFYABLE:
	__cpuid(0x80000000, eax, ebx, ecx, edx);

	if (eax <= 0x80000004)	// Test if extended vendor string available
		goto noExtVendorString;

	__cpuid(0x80000002, eax, ebx, ecx, edx);	// 16 bytes of extended vendor string
	*(uint32_t *)NameBuffer = eax;
	*(uint32_t *)(NameBuffer + 4) = ebx;
	*(uint32_t *)(NameBuffer + 8) = ecx;
	*(uint32_t *)(NameBuffer + 12) = edx;

	__cpuid(0x80000003, eax, ebx, ecx, edx);	// 16 more bytes
	*(uint32_t *)(NameBuffer + 16) = eax;
	*(uint32_t *)(NameBuffer + 20) = ebx;
	*(uint32_t *)(NameBuffer + 24) = ecx;
	*(uint32_t *)(NameBuffer + 28) = edx;

	__cpuid(0x80000004, eax, ebx, ecx, edx);	// 16 more bytes
	*(uint32_t *)(NameBuffer + 32) = eax;
	*(uint32_t *)(NameBuffer + 36) = ebx;
	*(uint32_t *)(NameBuffer + 40) = ecx;
	*(uint32_t *)(NameBuffer + 44) = edx;
	goto getFamilyAndModel;

noExtVendorString:
	// No extended vendor string, get short vendor string
	__cpuid(0, eax, ebx, ecx, edx);
}
