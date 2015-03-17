#pragma once


#if defined(__i386__) || defined(__x86_64__)

	#if		defined(__AVX__) && !defined(__AVX512F__)
		#define		SIMD_ALIGN	32

	#elif	!defined(__AVX__)
		#define		SIMD_ALIGN	16

	#elif	defined(__AVX512F__)
		#define		SIMD_ALIGN	64
	#endif

#elif defined(__ARM_ARCH) && defined(__ARM_NEON__)
	#define SIMD_ALIGN	16
#else
	#define SIMD_ALIGN	16
#endif
