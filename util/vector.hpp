#pragma once

#ifndef __EMMINTRIN_H
	#include <emmintrin.h>
#endif

#define		mUse128ForVec2f		0

#define		VECTOR_INLINE	__forceinline	

#define		vecIndexValue(ix)	vec[ix]

#define		vecX()				vecIndexValue(0)
#define		vecY()				vecIndexValue(1)
#define		vecZ()				vecIndexValue(2)
#define		vecW()				vecIndexValue(3)


namespace cloture
{
namespace util
{
namespace math
{
namespace vector
{
	template<typename T, size_t elements> 
	struct Vector
	{
	};
	
	#define vec__defnOpIndex()	\
	VECTOR_INLINE __pseudopure __typeof__(arr[0]) operator [] (const size_t index) const \
	{\
		return arr[index];\
	} \
	VECTOR_INLINE __typeof__(arr[0])& operator [] (const size_t index)\
	{\
		return arr[index];\
	}
	template<> struct Vector<vec_t, 2>
	{
		#if mUse128ForVec2f
			union
			{
				vec_t 	arr[4];
				__m128 	vec;
			};
		#else
			union
			{
				vec_t 	arr[2];
				__m64 	vec;
			};
		#endif

		using nativeType	=	__typeof__(vec);
		using vecType = Vector<vec_t, 2>;
		
		vec__defnOpIndex()
		
		VECTOR_INLINE Vector() : vec{}
		{
		
		}
		
		VECTOR_INLINE Vector(const vec_t __x, const vec_t __y)
		#if mUse128ForVec2f
			: arr{__x, __y, .0f, .0f}
		#else
			: arr{__x, __y}
		#endif
		{ 

		}
		
		VECTOR_INLINE Vector(const nativeType other) : vec(other) {}
		
		
		VECTOR_INLINE __pure vecType operator * (const vec_t scalar) const	
		{ 
			return vecType(vecIndexValue(0) * scalar, vecIndexValue(1) * scalar);
		}
		
		VECTOR_INLINE __pure vecType operator / (const vec_t scalar) const	
		{
			return vecType(vecIndexValue(0) / scalar, vecIndexValue(1) / scalar);
		}	
		
		VECTOR_INLINE __pure vecType operator + (const vecType& rhs) const	
		{ 
			return vecType(vecIndexValue(0) + rhs[0], vecIndexValue(1) + rhs[1]);
		}
		
		VECTOR_INLINE __pure vecType operator - (const vecType& rhs) const	
		{ 
			return vecType(vecIndexValue(0) - rhs[0], vecIndexValue(1) - rhs[1]);
		}
		
		VECTOR_INLINE __pure vec_t length() const
		{ 
			return static_cast<vec_t>(
				sqrtf(vecX() * vecX() + vecY() * vecY() )
			); 
		}


		VECTOR_INLINE bool operator ==(const vecType& rhs) const 
		{
			return vecIndexValue(0) == rhs[0] && vecIndexValue(1) == rhs[1];
		}
		
		VECTOR_INLINE bool operator !=(const vecType& rhs) const 
		{
			return vecIndexValue(0) != rhs[0] || vecIndexValue(1) != rhs[1];
		}
		
		VECTOR_INLINE operator bool() const 
		{
			return vecIndexValue(0) != .0f && vecIndexValue(1) != .0f;
		}
		
		VECTOR_INLINE explicit operator vec_t*()
		{
			return &arr[0];
		}
		
		VECTOR_INLINE  __pure vecType normalize() const
		{
			auto l = length();
			
			if ( unlikely(l == .0f) )
				return vecType( .0f, .0f );
			l = 1.0f / l;
			return vecType( vecX() *l, vecY() * l );
		
		}
	};

	template<> struct Vector<vec_t, 3>
	{
		union
		{
			__m128 vec;
			vec_t arr[4];
		};
		using vecType = Vector<vec_t, 3>;
		
		vec__defnOpIndex()
		
		VECTOR_INLINE Vector() : vec{} {}

		
		VECTOR_INLINE Vector(const vec_t __x, const vec_t __y, const vec_t __z) : arr{__x, __y, __z, 1.0f}
		{
		}
		
		VECTOR_INLINE Vector(const vec_t* RESTRICT const other) : arr{other[0], other[1], other[2], 1.0f}
		{
		}
		
		
		VECTOR_INLINE Vector(const __m128 other) : vec(other) {}

		VECTOR_INLINE __pure vecType operator * (const vec_t scalar) const	
		{ 
			return vecType(arr[0] * scalar, vecIndexValue(1) * scalar, vecIndexValue(2) * scalar);
		}
		
		VECTOR_INLINE __pure vecType operator / (const vec_t scalar) const	
		{
			return vecType(arr[0] / scalar, vecIndexValue(1) / scalar, vecIndexValue(2) / scalar);
		}	
		
		VECTOR_INLINE __pure vecType operator *(const __m128 other) const
		{
			return _mm_mul_ps(vec, other);//vec * other;
		}
		
		VECTOR_INLINE __pure vecType operator /(const __m128 other) const
		{
			return _mm_div_ps(vec, other);//vec / other;
		}
		
		VECTOR_INLINE __pure vecType operator +(const __m128 other) const
		{
			return _mm_add_ps(vec, other);//vec + other;
		}
		VECTOR_INLINE __pure vecType operator -(const __m128 other) const
		{
			return _mm_sub_ps(vec, other);//vec - other;
		}
		
		VECTOR_INLINE __pure vecType operator + (const vecType rhs) const	
		{ 
			return vecType(arr[0] + rhs[0], vecIndexValue(1) + rhs[1], vecIndexValue(2) + rhs[2]);
		}
		
		VECTOR_INLINE __pure vecType operator - (const vecType rhs) const	
		{ 
			return vecType(arr[0] - rhs[0], vecIndexValue(1) - rhs[1], vecIndexValue(2) - rhs[2]);
		}
		
		VECTOR_INLINE __pure vecType abs() const
		{
			const __m128i bits = _mm_castps_si128(vec);
			return _mm_castsi128_ps(
					_mm_and_si128(bits, _mm_set1_epi32(0x7FFFFFFF))
			);
		}
		
		VECTOR_INLINE operator vec_t*()
		{
			return &arr[0];
		}	

		VECTOR_INLINE operator const vec_t*() const
		{
			return &arr[0];
		}
		
		VECTOR_INLINE __pure operator __m128()
		{
			return vec;
		}
		
	};
	
	
	template<> struct Vector<vec_t, 4>
	{
		union
		{
			__m128 vec;
			vec_t arr[4];
		};
		using vecType = Vector<vec_t, 4>;
		vec__defnOpIndex()
		
		VECTOR_INLINE Vector() : vec{} {}

		VECTOR_INLINE Vector(const vec_t __x, const vec_t __y, const vec_t __z, const vec_t __w) : arr{__x, __y, __z, __w}
		{
		}
		
		VECTOR_INLINE Vector(const vec_t* RESTRICT const other) : arr{other[0], other[1], other[2], other[3]}
		{
		}
		VECTOR_INLINE Vector(const __m128 other) : vec(other) {}

		VECTOR_INLINE __pure vecType operator * (const vec_t scalar) const	
		{ 
			return vecType(arr[0] * scalar, vecIndexValue(1) * scalar, vecIndexValue(2) * scalar, vecIndexValue(3) * scalar);
		}
		
		VECTOR_INLINE __pure vecType operator / (const vec_t scalar) const	
		{
			return vecType(arr[0] / scalar, vecIndexValue(1) / scalar, vecIndexValue(2) / scalar, vecIndexValue(3) / scalar);
		}	
		
		VECTOR_INLINE __pure vecType operator *(const __m128 other) const
		{
			return _mm_mul_ps(vec, other);//vec * other;
		}
		
		VECTOR_INLINE __pure vecType operator /(const __m128 other) const
		{
			return _mm_div_ps(vec, other);//vec / other;
		}
		
		VECTOR_INLINE __pure vecType operator +(const __m128 other) const
		{
			return _mm_add_ps(vec, other);//vec + other;
		}
		VECTOR_INLINE __pure vecType operator -(const __m128 other) const
		{
			return _mm_sub_ps(vec, other);//vec - other;
		}
		
		
		VECTOR_INLINE operator vec_t*()
		{
			return &arr[0];
		}	
		
		VECTOR_INLINE operator const vec_t*() const
		{
			return &arr[0];
		}

		VECTOR_INLINE __pure operator __m128()
		{
			return vec;
		}	

	};
	
	
	//using vector2D = Vector<vec_t, 2>;
	//using vector3D = Vector<vec_t, 3>;

	using vector2f = Vector<vec_t, 2>;
	using vector3f = Vector<vec_t, 3>;
	using vector4f = Vector<vec_t, 4>;
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
