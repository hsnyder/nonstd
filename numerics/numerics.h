// SUPPORTING MULTIPLE ARCHITECTURES AND INSTRUCTION SETS
//
// A numerics library needs to access special hardware instructions that only exist on
// some systems. On the one hand there are different architectures: code for different
// architecutres is always compiled into separate binaries. On the other hand there 
// are instruction set extensions, small variants of the same architecture (e.g. avx).
// How will we address these differences? Well, for architectures, it's kind of easy,
// we'll put architecture specific code in separate files and conditionally include
// them from here. What about for instruction set extensions? ISEs can be detected at
// compile time (e.g. the user used the -mavx flag) or at run time (e.g. via `cpuid`). 
// We'll be ambitious and support both. How? For each function we want to offer,
// e.g. f32_to_f16, we'll have several versions, like f32_to_f16_c for a pure c 
// implementation, f32_to_f16_x86, etc. There may be sub-varieties of x86 to account
// for different instruction set extensions. We'll let f32_to_f16 be a macro. That
// macro gets defined to one of the following things (not all need to be supported
// for all functions):
//
// - if the arch isn't supported, the macro invokes the generic pure C version 
// - if compile time flags were used to enable ISEs, directly invoke that version
// - otherwise, call a dispatcher that will detect supported ISEs at runtime

#ifndef NUMERICS_H
#define NUMERICS_H

#ifndef NUMERICS_API
#define NUMERICS_API
#endif

#include <stdint.h>
#include <limits.h>
#include <float.h>

#define LARGEST_VALUE(x) _Generic((x), \
	int8_t: INT8_MAX,\
	uint8_t: UINT8_MAX,\
	int16_t: INT16_MAX,\
	uint16_t: UINT16_MAX,\
	int32_t: INT32_MAX,\
	uint32_t: UINT32_MAX,\
	int64_t: INT64_MAX,\
	uint64_t: UINT64_MAX,\
	float: FLT_MAX,\
	double: DBL_MAX)

#define SMALLEST_VALUE(x) _Generic((x), \
	int8_t: INT8_MIN,\
	uint8_t: 0,\
	int16_t: INT16_MIN,\
	uint16_t: 0,\
	int32_t: INT32_MIN,\
	uint32_t: 0,\
	int64_t: INT64_MIN,\
	uint64_t: 0,\
	float: (-FLT_MAX),\
	double: (-DBL_MAX))


/*
   ============================================================================
		16-BIT FLOATING POINT
   ============================================================================
*/


NUMERICS_API  void f32_to_f16_c (uint16_t *dst, float *src, int64_t count);
NUMERICS_API  void f16_to_f32_c (float *dst, uint16_t *src, int64_t count);

NUMERICS_API  float    f16_to_f32_c_single (uint16_t f16);
NUMERICS_API  uint16_t f32_to_f16_c_single (float f32);




/*
   ============================================================================
		SOME BASIC ARRAY FUNCTIONS
	
	These are non-optimized convenience functions which support arrays
	of many different numeric types. 
   ============================================================================
*/

#include <stdbool.h>

#define MULTITYPE_INTEGER

#define NAME(x) x ## b
#define TYPE int8_t
#include "numerics_multitype.h"
#undef TYPE
#undef NAME
#define NAME(x) x ## s
#define TYPE int16_t
#include "numerics_multitype.h"
#undef TYPE
#undef NAME
#define NAME(x) x ## i
#define TYPE int32_t
#include "numerics_multitype.h"
#undef TYPE
#undef NAME
#define NAME(x) x ## l
#define TYPE int64_t
#include "numerics_multitype.h"
#undef TYPE
#undef NAME

#define MULTITYPE_UNSIGNED
#define NAME(x) x ## B
#define TYPE uint8_t
#include "numerics_multitype.h"
#undef TYPE
#undef NAME
#define NAME(x) x ## S
#define TYPE uint16_t
#include "numerics_multitype.h"
#undef TYPE
#undef NAME
#define NAME(x) x ## I
#define TYPE uint32_t
#include "numerics_multitype.h"
#undef TYPE
#undef NAME
#define NAME(x) x ## L
#define TYPE uint64_t
#include "numerics_multitype.h"
#undef TYPE
#undef NAME
#undef MULTITYPE_UNSIGNED

#undef MULTITYPE_INTEGER

#define MULTITYPE_FP
#define NAME(x) x ## f
#define TYPE float
#include "numerics_multitype.h"
#undef TYPE
#undef NAME
#define NAME(x) x ## d
#define TYPE double
#include "numerics_multitype.h"
#undef TYPE
#undef NAME
#undef MULTITYPE_FP



/*
   ============================================================================
		INCLUDE ARCHITECTURE-SPECIFIC HEADER
   ============================================================================
*/

// Include the file for the relevant architecture. If we're compiling on an unsupported
// architecture, then we need to define all of our functions to the generic c version.
#if defined(__x86_64__) 
#include "numerics_x86.h"
#else

#define f32_to_f16(dst,src,count) f32_to_f16_c((dst),(src),(count))
#define f16_to_f32(dst,src,count) f16_to_f32_c((dst),(src),(count))

#endif

#endif
/* 
   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   ----------------------------------------------------------------------------
   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

   		END OF HEADER SECTION

		Implementation follows

   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   ----------------------------------------------------------------------------
   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
#ifdef NUMERICS_IMPLEMENTATION

#include <string.h>
#include <math.h>

NUMERICS_API float 
f16_to_f32_c_single (uint16_t f16)
{
        // fp16: 1 sign bit, 5 exponent bits, 10 fraction bits
        uint32_t sign = f16 >> 15;
        uint32_t exponent = (f16 >> 10) & 0x1F;
        uint32_t fraction = (f16 & 0x3FF);

        // fp32: 1 sign bit, 8 exponent bits, 23 fraciton bits
        uint32_t f32 = 0;

        // zero
        if (exponent == 0 && fraction == 0) {
                f32 = (sign << 31);
        }

        // denormal (in fp16)
        else if (exponent == 0) {
                exponent = 127 - 14;
                while ((fraction & (1 << 10)) == 0)
                {
                        exponent--;
                        fraction <<= 1;
                }
                fraction &= 0x3FF;
                f32 = (sign << 31) | (exponent << 23) | (fraction << 13);
        }

        // inf or nan
        else if (exponent == 0x1F) {
                f32 = (sign << 31) | (0xFF << 23) | (fraction << 13);
        }

        // normal number
        else {
                f32 = (sign << 31) | ((exponent + (127-15)) << 23) | (fraction << 13);
        }

        float f32_return = 0;
        memcpy(&f32_return, &f32, sizeof(f32_return));
        return f32_return;
}


NUMERICS_API uint16_t 
f32_to_f16_c_single (float f32)
{
        uint32_t u32 = 0;
        memcpy(&u32, &f32, sizeof(u32));

        uint16_t exponent  =  (u32 >> 23) & 0xff;
        uint32_t mantissa  =  u32 & (0x7fffff);

        uint16_t f16 = 0;
        f16 |= (u32 >> 16) & 0x8000;

        if (exponent == 0xff)  { // nan or inf

                return f16 | (mantissa ? 0x7e00 : 0x7c00);

        } else if (exponent >= 0x8f) { // overflow

                return f16 | 0x7c00;

        } else if (exponent >= 0x71) { // normalized

                return f16 | (((exponent - 0x70) << 10) | (mantissa >> 13));

        } else if (exponent >= 0x67) { // denormal

                return f16 | ((mantissa | 0x800000) >> (0x7e - exponent));

        } else {

                return f16;

        }
}

NUMERICS_API void  
f32_to_f16_c (uint16_t *dst, float *src, int64_t count)
{
	for (int64_t i = 0; i < count; i++)
		dst[i] = f32_to_f16_c_single(src[i]);
}

NUMERICS_API void  
f16_to_f32_c (float *dst, uint16_t *src, int64_t count)
{
	for (int64_t i = 0; i < count; i++)
		dst[i] = f16_to_f32_c_single(src[i]);
}



///////////////////////////////////////////////////////////////////////////////
//	INCLUDE DEFINITIONS FOR MULTITYPE ARRAY FUNCTIONS
//
//	Yeah it's unfortunate that this is in here twice, but it's not worth
//	an extra file just to avoid the duplication.
//
#define NUMERICS_MULTITYPE_IMPLEMENTATION

#define MULTITYPE_INTEGER

#define NAME(x) x ## b
#define TYPE int8_t
#include "numerics_multitype.h"
#undef TYPE
#undef NAME
#define NAME(x) x ## s
#define TYPE int16_t
#include "numerics_multitype.h"
#undef TYPE
#undef NAME
#define NAME(x) x ## i
#define TYPE int32_t
#include "numerics_multitype.h"
#undef TYPE
#undef NAME
#define NAME(x) x ## l
#define TYPE int64_t
#include "numerics_multitype.h"
#undef TYPE
#undef NAME

#define MULTITYPE_UNSIGNED
#define NAME(x) x ## B
#define TYPE uint8_t
#include "numerics_multitype.h"
#undef TYPE
#undef NAME
#define NAME(x) x ## S
#define TYPE uint16_t
#include "numerics_multitype.h"
#undef TYPE
#undef NAME
#define NAME(x) x ## I
#define TYPE uint32_t
#include "numerics_multitype.h"
#undef TYPE
#undef NAME
#define NAME(x) x ## L
#define TYPE uint64_t
#include "numerics_multitype.h"
#undef TYPE
#undef NAME
#undef MULTITYPE_UNSIGNED

#undef MULTITYPE_INTEGER

#define MULTITYPE_FP
#define NAME(x) x ## f
#define TYPE float
#include "numerics_multitype.h"
#undef TYPE
#undef NAME
#define NAME(x) x ## d
#define TYPE double
#include "numerics_multitype.h"
#undef TYPE
#undef NAME
#undef MULTITYPE_FP



///////////////////////////////////////////////////////////////////////////////
//	INCLUDE ARCHITECTURE-SPECIFIC DEFINITIONS
//
#define NUMERICS_ARCH_IMPLEMENTATION


//#define round_up_pow2(x,n) (((x)+((n)-1))&(~((n)-1)))

static int64_t 
round_up (int64_t value, int64_t to)
{
	int64_t mod = value%to;	
	if(mod) return value+to-(value%to);
	return value;
}

static int64_t 
round_down (int64_t value, int64_t to)
{
	return (value/to)*to;
}

#if defined(__x86_64__) 
#include "numerics_x86.h"
#endif

#endif
