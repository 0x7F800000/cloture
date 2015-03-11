#pragma once

/**
	statif (global scope)
		helper template for achieving true static if statements in C++
		if val is false, the struct does not define the alias isTrue within its namespace
		if val is true, the struct defines isTrue 
	
	this is intended for use with __if_exists and __if_not_exists 
*/
template<bool val> struct statif{};
template<> struct statif<true>	{using isTrue = void;};

#define		__static_if(condition)		__if_exists(statif<condition>::isTrue)
#define 	__static_if_not(condition)	__if_not_exists(statif<condition>::isTrue)

/**
	choose_expr
	
	GCC extension. sort of like a compile time ternary.
	different from __if_exists and __if_not_exists in that it takes uses a constant expression
	rather than an identifier
	
	on its own it isn't too powerful. combined with __if_exists, __if_not_exists, templates and constexpr functions
	it can be useful
*/
#if HAS_CHOOSE_EXPR
	#define 	choose_expr(const_expr, if_true, if_false)	__builtin_choose_expr(const_expr, if_true, if_false)
#else
	#define		choose_expr(const_expr, if_true, if_false)	((const_expr) ? (if_true) : (if_false))
#endif
/**
	constant_p
	
	GCC extension. returns true if value is known at compile time
	i'm not 100% sure, but I think this can be used in constexpr functions to 
	determine if they are being evaluated at compile time
	
*/
#define		constant_p(value)				__builtin_constant_p(value)

namespace cloture
{
namespace ctfe
{
#if hasExtendedConstexpr
/**
	ctfe::Array
		compile-time array copyable array
		has *all* of the bells and whistles
		all of them
*/
template<typename T, size_t sz = 0>
class Array
{
public:
	T data[sz + 1];

	constexpr size_t __size() const
	{
		return sz;
	}
	constexpr bool empty() const
	{
		return __size() == 0;
	}
	
	__msvc_property(false, __size) size_t 	size;
	__msvc_property(false, empty) bool 		isEmpty;
	
	constexpr Array() : data({}){}

	constexpr auto& operator[](const size_t index)
	{
		return data[index];
	}

	constexpr auto operator[](const size_t index) const
	{
		return data[index];
	}
	constexpr Array(const Array<T, sz>& other) : data({})
	{
		for(size_t i = 0; i < size; ++i)
			data[i]		=		other[i];
	}

	constexpr Array(const T* other) : data({})
	{
		for(size_t i = 0; i < size; ++i)
			data[i]		=		other[i];
	}



	constexpr bool operator==(const Array<T, sz>& other) const
	{
		for(size_t i = 0; i < size; ++i)
		{
			if(other[i] != (*this)[i])
				return false;
		}
		return true;
	}

	constexpr bool operator==(const T* const cmp) const
	{
		for(size_t i = 0; i < size; ++i)
		{
			if(cmp[i] != this->data[i])
				return false;
		}
		return true;
	}

	constexpr bool operator!=(const Array<T, sz>& other) const
	{
		return !((*this) == other);
	}

	constexpr Array<T, sz>& operator=(const T* const RESTRICT rhs)
	{
		for(size_t i = 0; i < size; ++i)
			data[i]		=	rhs[i];
		return (*this);
	}
	constexpr Array<T, sz>& operator=(const Array<T, sz>& rhs)
	{
		for(size_t i = 0; i < size; ++i)
			data[i]		=	rhs[i];
		return (*this);
	}
	explicit constexpr operator T*()		{return data;}
	explicit constexpr operator const T*()	{return data;}
};

#endif //#if hasExtendedConstexpr
}
};
