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


	template<typename T>
	struct isAggregateVector_resolverHelper
	{
	private:
		template<typename TT, bool b>
		struct RESOLVE
		{

		};

		template<typename TT>
		struct RESOLVE<TT, false>
		{
			static constexpr bool value = false;
			using type = TT;
		};

		template<typename TT>
		struct RESOLVE<TT, true>
		{
		private:
			mCreateIdentifierChecker(isAggregateVector);
		public:
			static constexpr bool value = mCallIdentifierChecker(TT, isAggregateVector);
		};

	public:
		static constexpr bool value = RESOLVE<T, generic::isClass<T>()>::value;
	};

	template<typename T>
	static constexpr bool isAggregateVector = isAggregateVector_resolverHelper<T>::value;

	typedef common::int8 	int8x16 	__attribute__((__vector_size__(16)));
	typedef common::uint8 	uint8x16 	__attribute__((__vector_size__(16)));

	typedef common::int16 	int16x8 	__attribute__((__vector_size__(16)));
	typedef common::uint16 	uint16x8 	__attribute__((__vector_size__(16)));

	typedef common::int32 	int32x4 	__attribute__((__vector_size__(16)));
	typedef common::uint32 	uint32x4 	__attribute__((__vector_size__(16)));

	typedef common::real32 	real32x4 	__attribute__((__vector_size__(16)));
	typedef common::real64 	real64x2 	__attribute__((__vector_size__(16)));

	/*
	 * __m64 types
	 */
	typedef common::int8	int8x8 		__attribute__((__vector_size__(8)));
	typedef common::uint8	uint8x8 	__attribute__((__vector_size__(8)));

	typedef common::int16	int16x4 	__attribute__((__vector_size__(8)));
	typedef common::uint16	uint16x4 	__attribute__((__vector_size__(8)));

	typedef common::int32	int32x2 	__attribute__((__vector_size__(8)));
	typedef common::uint32	uint32x2 	__attribute__((__vector_size__(8)));

}

namespace cloture::util::generic
{
	template<typename T>
	static constexpr bool isVector()
	{
		return false;
	}

	template<typename T>
	static constexpr bool isVector(const T in)
	{
		return false;
	}

	#define		__declIsVectorSpecial(type)
}

namespace cloture::util::templates::casts
{
	template<typename T> struct vecResolveNativeType
	{
	private:
		template<typename TT, bool b> struct RESOLVE{};

		template<typename TT>
		struct RESOLVE<TT, false>
		{
			using type = TT;
		};

		template<typename TT>
		struct RESOLVE<TT, true>
		{
		private:
			mCreateTypeResolver(nativeType);
		public:
			using type = mGetTypeResolverTypename(TT, nativeType);
		};
	public:
		using type = typename RESOLVE<T, generic::isClass<T>()>::type;
	};

	#define	_vecFromPrecast(type, tocast)	static_cast<type>(tocast)

	template<bool toTypeIsClass, bool fromTypeIsClass>
	struct vector_cast_picker
	{

	};
	/*
	 * converting between native vector types - easy
	 */
	template<>
	struct vector_cast_picker<false, false>
	{
		template<typename toType, typename FromType>
		__forceinline __pure static constexpr toType doCast(const FromType from)
		{
			return __builtin_convertvector(from, toType);
		}
	};
	/*
	 * 	converting from native vector type to class vector type
	 */
	template<>
	struct vector_cast_picker<true, false>
	{
		template<typename toType, typename FromType>
		__forceinline __pure static constexpr toType doCast(const FromType from)
		{
			using nativeTo = typename vecResolveNativeType<toType>::type;
			return toType(__builtin_convertvector(from, nativeTo));
		}
	};
	/*
	 * converting from class vector type to native vector type
	 */
	template<>
	struct vector_cast_picker<false, true>
	{
		template<typename toType, typename FromType>
		__forceinline __pure static constexpr toType doCast(const FromType from)
		{
			using nativeFrom = typename vecResolveNativeType<FromType>::type;
			return __builtin_convertvector(
				static_cast<nativeFrom>(from),
				toType
			);
		}
	};
	/*
	 * converting from class vector type to class vector type
	 */
	template<>
	struct vector_cast_picker<true, true>
	{
		template<typename toType, typename FromType>
		__forceinline __pure static constexpr toType doCast(const FromType from)
		{
			using nativeFrom 	= typename vecResolveNativeType<FromType>::type;
			using nativeTo 		= typename vecResolveNativeType<toType>::type;
			return toType(__builtin_convertvector(
				static_cast<nativeFrom>(from),
				nativeTo
			));
		}
	};
	/*
	__enableIf(
	!math::vector::isAggregateVector<FromType> && !math::vector::isAggregateVector<toType>,
	"Enabled if both types are not aggregate vector types."
	)*/
	template<typename toType, typename FromType>
	static constexpr toType vector_cast(const FromType from)
	{
	#if 0
		using nativeFrom 	= typename vecResolveNativeType<FromType>::type;
		using nativeTo 		= typename vecResolveNativeType<toType>::type;
		return static_cast<toType>(
				__builtin_convertvector(_vecFromPrecast(nativeFrom, from.vec), nativeTo)
		);
	#else
		constexpr bool toTypeIsClass	= generic::isClass<toType>();
		constexpr bool fromTypeIsClass	= generic::isClass<FromType>();
		using caster = vector_cast_picker<toTypeIsClass, fromTypeIsClass>;
		return caster::doCast<toType, FromType>(from);
	#endif
	}

	#if 0
		template<typename toType, typename FromType>
		__enableIf(
		math::vector::isAggregateVector<FromType> && !math::vector::isAggregateVector<toType>,
		"Enabled if casting from an aggregate vector to a non-aggregate vector."
		)
		static constexpr toType vector_cast(const FromType from)
		{
			using nativeFrom 	= typename vecResolveNativeType<FromType>::type;
			using nativeTo 		= typename vecResolveNativeType<toType>::type;
			return static_cast<toType>(
					__builtin_convertvector(_vecFromPrecast(nativeFrom, from.vec), nativeTo)
			);
		}
	#endif
}

namespace cloture
{
namespace util
{
namespace math
{
namespace vector
{
	using templates::casts::vector_cast;
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
