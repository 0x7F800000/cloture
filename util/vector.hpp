#pragma once

#ifndef __EMMINTRIN_H
	#include <emmintrin.h>
#endif


#define		VECTOR_INLINE	__forceinline	

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

	#define		vec__defnXY()	\
	VECTOR_INLINE __pseudopure vec_t __get_x__()  const \
	{\
		return arr[0];\
	}\
	VECTOR_INLINE vec_t __set_x__(const vec_t __x) \
	{\
		return arr[0] = __x;\
	}\
	VECTOR_INLINE __pseudopure vec_t __get_y__() const\
	{\
		return arr[1];\
	}\
	VECTOR_INLINE vec_t __set_y__(const vec_t __y) \
	{\
		return arr[1] = __y;\
	}\
	__msvc_property(true, __get_x__, __set_x__) vec_t x;\
	__msvc_property(true, __get_y__, __set_y__) vec_t y
	
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
		union
		{
			vec_t arr[4];
			__m128 vec;
		};
		using vecType = Vector<vec_t, 2>;
		
		vec__defnOpIndex()
		vec__defnXY();
		
		VECTOR_INLINE Vector() : vec{}
		{
		
		}
		
		VECTOR_INLINE Vector(const vec_t __x, const vec_t __y) : arr{__x, __y, .0f, .0f}
		{ 

		}
		
		VECTOR_INLINE Vector(const __m128 other) : vec(other) {}
		
		
		VECTOR_INLINE __pure vecType operator * (const vec_t scalar) const	
		{ 
			return vecType(arr[0] * scalar, arr[1] * scalar);
		}
		
		VECTOR_INLINE __pure vecType operator / (const vec_t scalar) const	
		{
			return vecType(arr[0] / scalar, arr[1] / scalar);
		}	
		
		VECTOR_INLINE __pure vecType operator + (const vecType& rhs) const	
		{ 
			return vecType(arr[0] + rhs[0], arr[1] + rhs[1]);
		}
		
		VECTOR_INLINE __pure vecType operator - (const vecType& rhs) const	
		{ 
			return vecType(arr[0] - rhs[0], arr[1] - rhs[1]);
		}
		
		VECTOR_INLINE __pure vec_t __get_length__() const	
		{ 
			return static_cast<vec_t>(
				sqrtf(x * x + y * y )
			); 
		}
		
		__msvc_property(false, __get_length__) vec_t length;
		

		VECTOR_INLINE bool operator ==(const vecType& rhs) const 
		{
			return arr[0] == rhs[0] && arr[1] == rhs[1];
		}
		
		VECTOR_INLINE bool operator !=(const vecType& rhs) const 
		{
			return arr[0] != rhs[0] || arr[1] != rhs[1];
		}
		
		VECTOR_INLINE operator bool() const 
		{
			return arr[0] != .0f && arr[1] != .0f;
		}
		
		VECTOR_INLINE explicit operator vec_t*()
		{
			return &arr[0];
		}
		
		VECTOR_INLINE  __pure vecType normalize() const
		{
			auto l = length;
			
			if ( unlikely(l == .0f) )
				return vecType( .0f, .0f );
			l = 1.0f / l;
			return vecType( x *l, y * l );
		
		}
	};
	#define		vec__defnXYZ()	\
		vec__defnXY();	\
		VECTOR_INLINE __pseudopure vec_t __get_z__() const \
		{\
			return arr[2];\
		}\
		VECTOR_INLINE vec_t __set_z__(const vec_t __z) \
		{\
			return arr[2] = __z;\
		}\
		__msvc_property(true, __get_z__, __set_z__) vec_t z
		
	template<> struct Vector<vec_t, 3>
	{
		union
		{
			__m128 vec;
			vec_t arr[4];
		};
		using vecType = Vector<vec_t, 3>;
		
		vec__defnOpIndex()
		vec__defnXYZ();
		
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
			return vecType(arr[0] * scalar, arr[1] * scalar, arr[2] * scalar);
		}
		
		VECTOR_INLINE __pure vecType operator / (const vec_t scalar) const	
		{
			return vecType(arr[0] / scalar, arr[1] / scalar, arr[2] / scalar);
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
			return vecType(arr[0] + rhs[0], arr[1] + rhs[1], arr[2] + rhs[2]);
		}
		
		VECTOR_INLINE __pure vecType operator - (const vecType rhs) const	
		{ 
			return vecType(arr[0] - rhs[0], arr[1] - rhs[1], arr[2] - rhs[2]);
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
	
	#define vec__defnXYZW()		\
		VECTOR_INLINE __pseudopure vec_t __get_w__() const \
		{\
			return arr[3];\
		}\
		VECTOR_INLINE vec_t __set_w__(const vec_t __w) \
		{\
			return arr[3] = __w;\
		}\
		__msvc_property(true, __get_w__, __set_w__) vec_t w
	
	template<> struct Vector<vec_t, 4>
	{
		union
		{
			__m128 vec;
			vec_t arr[4];
		};
		using vecType = Vector<vec_t, 4>;
		vec__defnOpIndex()
		vec__defnXYZW();		
		
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
			return vecType(arr[0] * scalar, arr[1] * scalar, arr[2] * scalar, arr[3] * scalar);
		}
		
		VECTOR_INLINE __pure vecType operator / (const vec_t scalar) const	
		{
			return vecType(arr[0] / scalar, arr[1] / scalar, arr[2] / scalar, arr[3] / scalar);
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
	
	
	using vector2D = Vector<vec_t, 2>;
	using vector3D = Vector<vec_t, 3>;
	using vector4D = Vector<vec_t, 4>;
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
#define		__import_vector2D()		\
		using vector2D = cloture::util::math::vector::vector2D

#define		__import_vector3D()		\
		using vector3D = cloture::util::math::vector::vector3D

#define		__import_vector4D()		\
		using vector4D = cloture::util::math::vector::vector4D

#define		__import_float_vector_types()	\
		__import_vector2D();				\
		__import_vector3D();				\
		__import_vector4D()
