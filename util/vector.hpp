#pragma once

#define		mUse128ForVec2f		1
#define		mNoVectorDefault	0


#define		mVecInline			__forceinline
#define		mVecCall			constexpr mVecInline
#define		vecIndexValue(ix)	vec[ix]

#define		vecX()				vecIndexValue(0)
#define		vecY()				vecIndexValue(1)
#define		vecZ()				vecIndexValue(2)
#define		vecW()				vecIndexValue(3)

#define		mVecPure			mVecInline __pure
#define		mVecPureCtfe		mVecCall __pure
#define		mVecPureOp			mVecInline __pure vecType operator

#define		mMarkAggregateVec()	static constexpr bool isAggregateVector = true
//using isAggregateVector = bool

namespace cloture::util::math::vector
{
	mCreateIdentifierChecker(isAggregateVector);

	template<typename T>
	static constexpr bool isAggregateVector = mCallIdentifierChecker(T, isAggregateVector);
}

namespace cloture::util::templates::casts
{
	mCreateTypeResolver(nativeType);


	template<typename toType, typename FromType>
	__enableIf(
	!math::vector::isAggregateVector<FromType> && !math::vector::isAggregateVector<toType>,
	"Enabled if both types are not aggregate vector types."
	)
	static constexpr FromType vector_cast(const FromType from)
	{
		using nativeFrom 	= mGetTypeResolverTypename(FromType, nativeType);
		using nativeTo 		= mGetTypeResolverTypename(toType, nativeType);
		return static_cast<toType>(
				__builtin_convertvector(static_cast<nativeFrom>(from), nativeTo)
		);
	}

	template<typename toType, typename FromType>
	__enableIf(
	math::vector::isAggregateVector<FromType> && !math::vector::isAggregateVector<toType>,
	"Enabled if casting from an aggregate vector to a non-aggregate vector."
	)
	static constexpr FromType vector_cast(const FromType from)
	{
		using nativeFrom 	= mGetTypeResolverTypename(FromType, nativeType);
		using nativeTo 		= mGetTypeResolverTypename(toType, nativeType);
		return static_cast<toType>(
				__builtin_convertvector(static_cast<nativeFrom>(from.vec1), nativeTo)
		);
	}
}

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
			#include "libVector/vector_x86.hpp"
		#else
			#warning "Building for x86 with __SSE__ not defined... Choosing generic vector implementation."
			#include "libVector/vector_generic.hpp"
		#endif

	#elif defined(__ARM_ARCH)

		#if defined(__ARM_NEON__)
			#include "libVector/vector_arm.hpp"
		#else
			#warning "Architecture is arm, but __ARM_NEON__ is not defined. Using generic vector classes."
			#include "libVector/vector_generic.hpp"
		#endif

	#else
		#include "libVector/vector_generic.hpp"
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
