#pragma once

// Most credit for this goes to compiler-rt int_endianness.h

#if defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) && defined(__ORDER_LITTLE_ENDIAN__)

// Built-in endianness definitions are provided in GCC-compliant compilers
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define ASMLIB_LITTLE_ENDIAN 0
#define ASMLIB_BIG_ENDIAN 1
#elif __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define ASMLIB_LITTLE_ENDIAN 1
#define ASMLIB_BIG_ENDIAN 0
#endif

#else // Not GCC-compliant

#if defined(__SVR4) && defined(__sun)
#include <sys/byteorder.h>

#if defined(_BIG_ENDIAN)
#define ASMLIB_LITTLE_ENDIAN 0
#define ASMLIB_BIG_ENDIAN 1
#elif defined(_LITTLE_ENDIAN)
#define ASMLIB_LITTLE_ENDIAN 1
#define ASMLIB_BIG_ENDIAN 0
#else
#error "Unknown endianness"
#endif

#endif

#if defined(__FreeBSD) || defined(__NetBSD__) || defined(__DragonFly__) || defined(__minix)
#include <sys/endian.h>

#if _BYTE_ORDER == _BIG_ENDIAN
#define ASMLIB_LITTLE_ENDIAN 0
#define ASMLIB_BIG_ENDIAN 1
#elif _BYTE_ORDER == _LITTLE_ENDIAN
#define ASMLIB_LITTLE_ENDIAN 1
#define ASMLIB_BIG_ENDIAN 0
#endif

#endif

#if defined(__OpenBSD__)
#include <machine/endian.h>

#if _BYTE_ORDER == _BIG_ENDIAN
#define ASMLIB_LITTLE_ENDIAN 0
#define ASMLIB_BIG_ENDIAN 1
#elif _BYTE_ORDER == _LITTLE_ENDIAN
#define ASMLIB_LITTLE_ENDIAN 1
#define ASMLIB_BIG_ENDIAN 0
#endif

#endif

// GCC/Clang on Mac OS X automatically sets __BIG_ENDIAN__/__LITTLE_ENDIAN
#if defined(__APPLE__) || defined(__ellcc__)

#ifdef __BIG_ENDIAN__
#if __BIG_ENDIAN__
#define ASMLIB_LITTLE_ENDIAN 0
#define ASMLIB_BIG_ENDIAN 1
#endif
#endif

#ifdef __LITTLE_ENDIAN
#if __LITTLE_ENDIAN
#define ASMLIB_LITTLE_ENDIAN 1
#define ASMLIB_BIG_ENDIAN 0
#endif
#endif

#endif

#if defined(_WIN32)

#define ASMLIB_LITTLE_ENDIAN 1
#define ASMLIB_BIG_ENDIAN 0

#endif

#endif

#if !defined(ASMLIB_LITTLE_ENDIAN) || !defined(ASMLIB_BIG_ENDIAN)
#error Could not determine endian
#endif


