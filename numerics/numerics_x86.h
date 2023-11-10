// Don't include this file directly, it is included by numerics.h

/* 
   ============================================================================
		INSTRUCTION SET EXTENSIONS
   ============================================================================
*/

#define CPU_HAS_F16C         __builtin_cpu_supports("f16c")
#define CPU_HAS_AVX          __builtin_cpu_supports("avx")
#define CPU_HAS_AVX2         __builtin_cpu_supports("avx2")
#define CPU_HAS_AVX512F      __builtin_cpu_supports("avx512f")
#define CPU_HAS_AVX512VNNI   __builtin_cpu_supports("avx512vnni")
#define CPU_HAS_AVX512BF16   __builtin_cpu_supports("avx512bf16")
#define CPU_HAS_AVX512FP16   __builtin_cpu_supports("avx512fp16")

#define SYSV_ABI __attribute__((sysv_abi))

NUMERICS_API  void f32_to_f16_x86f16c (uint16_t *dst, float *src, int64_t count);
NUMERICS_API  void f16_to_f32_x86f16c (float *dst, uint16_t *src, int64_t count);

NUMERICS_API  void f32_to_f16_x86_dispatch (uint16_t *dst, float *src, int64_t count);
NUMERICS_API  void f16_to_f32_x86_dispatch (float *dst, uint16_t *src, int64_t count);

extern void SYSV_ABI issue_cpuid(unsigned registers[static 4], unsigned eax, unsigned ecx);

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
#ifdef NUMERICS_ARCH_IMPLEMENTATION

asm(
".global issue_cpuid            \n"
"issue_cpuid:                   \n"
"	pushq	%rbx            \n"
"	movl	%esi, %eax      \n"
"	movl	%edx, %ecx      \n"
"	cpuid                   \n"
"	movl 	%eax, (%rdi)    \n"
"	movl	%ebx, 4(%rdi)   \n"
"	movl 	%ecx, 8(%rdi)   \n"
"	movl 	%edx, 12(%rdi)  \n"
"	popq	%rbx            \n"
"       ret                     \n"
);




/*
	x86_64 processors with the F16C instruction set extension have hardware conversions.
*/

#include <immintrin.h>

NUMERICS_API void __attribute__((target("f16c,avx"))) 
f32_to_f16_x86f16c (uint16_t *dst, float *src, int64_t count)
{
	int64_t i = 0;
	for (; i < round_down(count, 8); i+=8) {
		__m128i f16vec = _mm256_cvtps_ph(_mm256_loadu_ps(src+i), _MM_FROUND_TO_NEAREST_INT);
		_mm_storeu_si128((__m128i*)(dst+i), f16vec);
	}

	for (; i < count; i++) {
		__m128i f16 = _mm_cvtps_ph(_mm_set_ss(src[i]), _MM_FROUND_TO_NEAREST_INT);
		dst[i] = 0xffff & _mm_cvtsi128_si32(f16);
	}

	_mm256_zeroupper();
	return;
}

NUMERICS_API void __attribute__((target("f16c,avx"))) 
f16_to_f32_x86f16c (float *dst, uint16_t *src, int64_t count)
{
	int i = 0;
	for (; i < round_down(count, 8); i+=8) {
		__m256  f32vec = _mm256_cvtph_ps(_mm_loadu_si128((__m128i*)(src+i)));
		_mm256_storeu_ps(dst+i, f32vec);
	}

	for (; i < count; i++) {
		__m128  f32 = _mm_cvtph_ps(_mm_cvtsi32_si128(src[i]));
		_mm_store_ss(dst+i, f32);
	}

	_mm256_zeroupper();
	return;
}

NUMERICS_API void  
f32_to_f16_x86_dispatch (uint16_t *dst, float *src, int64_t count)
{
	if(CPU_HAS_F16C && CPU_HAS_AVX)
		f32_to_f16_x86f16c(dst,src,count);
	else
		f32_to_f16_c(dst,src,count);
}

NUMERICS_API void  
f16_to_f32_x86_dispatch (float *dst, uint16_t *src, int64_t count)
{
	if(CPU_HAS_F16C && CPU_HAS_AVX)
		f16_to_f32_x86f16c(dst,src,count);
	else
		f16_to_f32_c(dst,src,count);
}

// If we're compiling with -mf16c and -mavx, then directly call the relevant version. 
// Otherwise, do a run-time dispatch.
#if defined(__F16C__) && defined(__AVX__)
#define f32_to_f16(dst,src,count) f32_to_f16_x86f16c((dst),(src),(count))
#define f16_to_f32(dst,src,count) f16_to_f32_x86f16c((dst),(src),(count))
#else 
#define f32_to_f16(dst,src,count) f32_to_f16_x86_dispatch((dst),(src),(count))
#define f16_to_f32(dst,src,count) f16_to_f32_x86_dispatch((dst),(src),(count))
#endif

#endif
