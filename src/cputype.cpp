#include "asmlib.h"
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

extern "C" void CpuType(int *pVendor, int *pFamily, int *pModel)
{
	int vendor = 0;
	int family = 0;
	int model = 0;

	if (!isCPUIDSupported())
		goto end;	// CPUID not supported

	uint32_t eax, ebx, ecx, edx;
	__cpuid(0, eax, ebx, ecx, edx);	// Get number of CPUID functions

	// Get vendor
	// ecx : Last 4 characters of vendor string
	// ebx : First 4 characters of vendor string
	if (ecx == 0x6C65746E)	// 'ntel' ('GenuineIntel')
		vendor |= 1;
	else if (ecx == 0x444D4163)	// 'cAMD' ('AuthenticAMD')
		vendor |= 2;
	else if (ebx == 0x746E6543 || ebx == 0x20414956)	// 'Cent' ('CentaurHauls'), 'VIA ' ('VIA VIA VIA ')
		vendor |= 3;
	else if (ebx == 0x69727943)	// 'Cyri' ('CyrixInstead')
		vendor |= 4;
	else if (ebx == 0x4778654E)	// 'NexG' ('NexGenDriven')
		vendor |= 5;

	// Other, do nothing

	if (eax == 0)
		goto end;	// Function 1 not supported

	// Get family and model
	__cpuid(1, eax, ebx, ecx, edx);

	// Left : Family, Right : Extended family
	family = ((eax >> 8) & 0xF) + ((eax >> 20) & 0xFF);	// Family + Extended family
	// Left : Model, Right : Extended model
	model = ((eax >> 4) & 0xF) | ((eax >> 12) & 0xF0);	// Model | Extended model

end:
	if (pVendor)
		*pVendor = vendor;

	if (pFamily)
		*pFamily = family;

	if (pModel)
		*pModel = model;

	return;
}
