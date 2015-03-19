#pragma once

#define		mUse128ForVec2f		1
#define		mNoVectorDefault	0

#define		VECTOR_INLINE		__forceinline

#define		vecIndexValue(ix)	vec[ix]

#define		vecX()				vecIndexValue(0)
#define		vecY()				vecIndexValue(1)
#define		vecZ()				vecIndexValue(2)
#define		vecW()				vecIndexValue(3)

#define		mVecPure			VECTOR_INLINE __pure
#define		mVecPureOp			VECTOR_INLINE __pure vecType operator
namespace cloture
{
namespace util
{
namespace math
{
namespace vector
{
	#if defined(__i386__) || defined(__x86_64__)

		#if defined(__SSE__)
			#include "vector_x86.hpp"
		#else
			#error Uhhh, SSE is unavailable...
		#endif

	#elif defined(__ARM_ARCH)

		#if defined(__ARM_NEON__)
			#include "vector_arm.hpp"
		#else
			#error Architecture is ARM, but neon is unavailable.
		#endif

	#else

		#error Generic version of vector classes unimplemented.

	#endif
} //namespace vector
} //namespace math
} //namespace util
}

/**
 * 	these macros are for use within structures/functions in header files
 * 	never use these at global scope in header files
 * 	you can use these in source files, but why bother when you can just
 * 	import the namespace?
 */
#define		__import_vector2f()		\
		using vector2f = cloture::util::math::vector::vector2f

#define		__import_vector3f()		\
		using vector3f = cloture::util::math::vector::vector3f

#define		__import_vector4f()		\
		using vector4f = cloture::util::math::vector::vector4f

#define		__import_float_vector_types()	\
		__import_vector2f();				\
		__import_vector3f();				\
		__import_vector4f()
