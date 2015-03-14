#pragma once

/*
 * 	template/macros for use with variadic templates
 */

namespace cloture		{
namespace util			{
namespace templates		{
namespace variadic		{

struct ___NO_VARG___
{
	static constexpr char filler = 0;

	constexpr operator void*() const
	{
		return nullptr;
	}
	constexpr operator const void*()
	{
		return nullptr;
	}
	constexpr operator const char*() const
	{
		return nullptr;
	}
	constexpr operator const char*()
	{
		return nullptr;
	}
};

template<typename T = ___NO_VARG___, typename... others>
struct ___getTypeVArg1
{
	using type = T;
};

template<typename T1, typename T2 = ___NO_VARG___, typename... others>
struct ___getTypeVArg2
{
	using type = T2;
};

template<typename T1, typename T2, typename T3 = ___NO_VARG___, typename... others>
struct ___getTypeVArg3
{
	using type = T3;
};

template<typename T1, typename T2, typename T3, typename T4  = ___NO_VARG___, typename... others>
struct ___getTypeVArg4
{
	using type = T4;
};

template<typename T1, typename T2, typename T3, typename T4, typename T5  = ___NO_VARG___, typename... others>
struct ___getTypeVArg5
{
	using type = T5;
};

template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6  = ___NO_VARG___, typename... others>
struct ___getTypeVArg6
{
	using type = T6;
};

template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6 , typename T7 = ___NO_VARG___, typename... others>
struct ___getTypeVArg7
{
	using type = T7;
};

template<typename T1, typename T2, typename T3, typename T4,
typename T5, typename T6 , typename T7, typename T8 = ___NO_VARG___, typename... others>
struct ___getTypeVArg8
{
	using type = T8;
};

template<typename T1, typename T2, typename T3, typename T4,
typename T5, typename T6 , typename T7, typename T8, typename T9 = ___NO_VARG___, typename... others>
struct ___getTypeVArg9
{
	using type = T9;
};

template<typename T> static constexpr bool isValidVArg = true;
template<> static constexpr bool isValidVArg<___NO_VARG___> = false;

#define		getNthTypename(n, args)		\
typename cloture::util::templates::variadic::___getTypeVArg##n <args...>::type



/*
 * 	container for non-typename elements
 */
template<typename T, T... pack>
struct paramPack
{
	static constexpr size_t size = sizeof...(pack);
	using type = T;
	static constexpr T packArray[] =
	{
			pack...
	};
	constexpr paramPack() {}

	constexpr T operator[](size_t index) const
	{
		return packArray[index];
	};

	constexpr T& operator[](size_t index)
	{
		return packArray[index];
	}
};
template<>
struct paramPack<void>
{
	static constexpr size_t size = 0;
	using type = void;
};



static constexpr paramPack<size_t, 1, 2, 3, 4, 5> ___PARAM_PACK_TEST___;

static_assert(___PARAM_PACK_TEST___[1] == 2);

}//namespace variadic
}//namespace templates
}//namespace util
}//namespace cloture

