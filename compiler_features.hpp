#pragma once

//moved this here from qtypes.h
#if defined(__GNUC__) || defined(__clang__) || (defined(_MSC_VER) && _MSC_VER >= 1400)
	#define RESTRICT __restrict
#else
	#define RESTRICT
#endif

#if !defined(__clang__)
	#define	HAS_CHOOSE_EXPR	0
#else
	#define HAS_CHOOSE_EXPR 1
#endif

#if defined(__GNUC__) && (__GNUC__ > 2)
#define DP_FUNC_PRINTF(n) __attribute__ ((format (printf, n, n+1)))
#define DP_FUNC_PURE      __attribute__ ((pure))
#define DP_FUNC_NORETURN  __attribute__ ((noreturn))
#else
#define DP_FUNC_PRINTF(n)
#define DP_FUNC_PURE
#define DP_FUNC_NORETURN
#endif

#if defined(__GNUC__) || defined(__clang__) || defined(__INTEL_COMPILER)
	#define		__forceinline							__attribute__((always_inline))

	#define		__noinline								__attribute__((noinline))
	#define 	__flatten								__attribute__((flatten))

	#define		__noreturn								__attribute__((noreturn))

	#define		__pure									__attribute__((__const__))
	#define		__pseudopure							__attribute__((pure))

	#define		__align(alignment)						__attribute__((aligned(alignment)))
	#define		__returns_aligned(alignment)			__attribute__((assume_aligned(alignment)))
	#define		__nonull_args(...)						__attribute__((nonnull(__VA_ARGS__)))
	#define		__returns_nonull 						__attribute__((returns_nonnull))

	#define		unreachable()							__builtin_unreachable()

	#ifdef		__GNUC__
		#define 	__assume(x)								if(!(x))	unreachable()
	#endif

	#define 	__assume_aligned(ptr,alignment)			__builtin_assume_aligned(ptr, alignment)
	#define		prefetch(addr, ...)						__builtin_prefetch(addr, ##__VA_ARGS__)
	#define 	clear_cache(begin, end)					__builtin___clear_cache(begin, end)


	#define 	__offsetOf(type, member)				__builtin_offsetof (type, member)
	#define 	likely(x)								__builtin_expect(!!(x), true)
	#define 	unlikely(x)								__builtin_expect(!!(x), false)

#else
	#ifndef _MSVC_VER
		#define	__forceinline	inline
		#define	__noinline
		#define	__noreturn	
		#define	__pure	
		#define	__assume(x)
		#define	__align(a)	
	#else
		#define	__noreturn		__declspec(noreturn)
		#define	__noinline		__declspec(noinline)
		#define	__pure			__declspec(noalias)
		#define	__align(a)		__declspec( align( a))
	#endif
	#define	__pseudopure	
	#define unlikely(x)		(x)
	#define likely(x)		(x)
	#define __flatten
	#define	__returns_nonull 
	#define	__nonull_args(...)
	#define	unreachable()
	#define __assume_aligned(ptr,alignment)		(ptr)
	#define	prefetch(addr, ...)	
	#define clear_cache(begin, end)	
	#define __offsetof(type, member)	offsetof(type, member)
#endif

#if defined(__clang__) || defined(_MSVC_VER) || defined(__INTEL_COMPILER)
	#define	__novtbl	__declspec(novtable)
	#define	__msvc_property_true(gt, st)			__declspec( property(get = gt, put = st))
	#define	__msvc_property_false(gt, ...)			__declspec( property(get = gt))

	#define	__expand_msvc_property__(call, gt, ...)	call(gt, __VA_ARGS__)

	#define	__msvc_property(readWrite, gt, ...)		__expand_msvc_property__(__msvc_property_##readWrite, gt, __VA_ARGS__)
	#define	__returns_unaliased						__declspec(noalias)
	#define	__naked									__declspec(naked)
#else
	#define	__novtbl	
	#define	__msvc_property(readWrite, gt, ...)	
	#define	__returns_unaliased			
#endif

#if defined(__clang__)
	#define		vectorizeLoop	_Pragma("clang loop vectorize(enable)")
#elif defined(__INTEL_COMPILER)
	#define		vectorizeLoop	_Pragma("simd")
#else
	#define		vectorizeLoop
#endif
