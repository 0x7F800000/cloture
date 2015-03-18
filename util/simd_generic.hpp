#pragma once


#if defined(__i386__) || defined(__x86_64__)

	#if		defined(__AVX__) && !defined(__AVX512F__)
		#define		mSimdAlign			32
		#define		mNativeVectorSize	mSimdAlign
	#elif	!defined(__AVX__)
		#define		mSimdAlign			16
		#define		mNativeVectorSize	mSimdAlign
	#elif	defined(__AVX512F__)
		#define		mSimdAlign			64
		#define		mNativeVectorSize	mSimdAlign
	#endif

#elif defined(__ARM_ARCH) && defined(__ARM_NEON__)

	#define 	mSimdAlign			16
	#define		mNativeVectorSize	mSimdAlign

#else

	#define 	mSimdAlign			16
	#define		mNativeVectorSize	mSimdAlign

#endif

#define		mDebugVectorOps		1
