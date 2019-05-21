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

static void WriteDigit(char *str, uint8_t num)
{
	if (num > 10)
		num += 'A' - 10;	// A - F
	else
		num += '0';	// 0 - 9

	*str = num;
}

static char *WriteHex(char *str, uint8_t num)
{
	WriteDigit(str, (num >> 4) & 0xF);	// Most significant digit first
	WriteDigit(str + 1, num & 0xF);	// Next digit
	return str + 2;
}

static char *processorNameNoCPUID(char *buf)
{
	return strcpy(buf, "80386 or 80486");
}

extern "C" char *ProcessorName()
{
	static char NameBuffer[0x50] = {0};	// Static buffer to contain name
	char *pName = NameBuffer;

	if (!isCPUIDSupported())
		return processorNameNoCPUID(NameBuffer);

	uint32_t eax, ebx, ecx, edx;
	__cpuid(0, eax, ebx, ecx, edx);
	if (eax < 1)
		return processorNameNoCPUID(NameBuffer);

	// Identifiable
	__cpuid(0x80000000, eax, ebx, ecx, edx);

	if (eax <= 0x80000004)	// Test if extended vendor string available
	{
		// No extended vendor string, get short vendor string

		__cpuid(0, eax, ebx, ecx, edx);
		*(uint32_t *)pName = ebx;	// Store short vendor string
		*(uint32_t *)(pName + 4) = edx;
		*(uint32_t *)(pName + 8) = ecx;
		pName[12] = '\0';	// Terminate string
	}
	else
	{
		// Extended vendor string available

		__cpuid(0x80000002, eax, ebx, ecx, edx);	// 16 bytes of extended vendor string
		*(uint32_t *)pName = eax;
		*(uint32_t *)(pName + 4) = ebx;
		*(uint32_t *)(pName + 8) = ecx;
		*(uint32_t *)(pName + 12) = edx;

		__cpuid(0x80000003, eax, ebx, ecx, edx);	// 16 more bytes
		*(uint32_t *)(pName + 16) = eax;
		*(uint32_t *)(pName + 20) = ebx;
		*(uint32_t *)(pName + 24) = ecx;
		*(uint32_t *)(pName + 28) = edx;

		__cpuid(0x80000004, eax, ebx, ecx, edx);	// 16 more bytes
		*(uint32_t *)(pName + 32) = eax;
		*(uint32_t *)(pName + 36) = ebx;
		*(uint32_t *)(pName + 40) = ecx;
		*(uint32_t *)(pName + 44) = edx;
	}

	// Get family and model
	strcat(pName, " Family ");
	pName += strlen(pName);

	__cpuid(1, eax, ebx, ecx, edx);
	auto family = (eax >> 8) & 0xF;
	auto extendedFamily = (eax >> 20) & 0xFF;
	pName = WriteHex(pName, family + extendedFamily);

	constexpr char hModel[] = "H Model ";
	strcpy(pName, hModel);
	pName += sizeof(hModel) - 1;

	auto model = (ebx >> 4) & 0xF;
	auto extendedModel = (ebx >> 12) & 0xF0;
	pName = WriteHex(pName, model | extendedModel);

	*pName = 'H';
	++pName;
	*pName = '\0';

	return NameBuffer;
}
