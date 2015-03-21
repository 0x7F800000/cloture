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
	static constexpr auto clotureTypeName()
	{
		return "___NO_VARG___";
	}
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

template<typename T>
static constexpr size_t vaSizeof = sizeof(T);

template<>
static constexpr size_t vaSizeof<___NO_VARG___> = 0;

template<typename T = ___NO_VARG___, typename... others>
struct ___getTypeVArg1
{
	using type = T;
	static constexpr size_t typeSize 		= vaSizeof<type>;
	static constexpr size_t remainingArgs 	= sizeof...(others);
};

template<typename T1= ___NO_VARG___, typename T2 = ___NO_VARG___, typename... others>
struct ___getTypeVArg2
{
	using type = T2;
	static constexpr size_t typeSize 		= vaSizeof<type>;
	static constexpr size_t remainingArgs 	= sizeof...(others);
};

template<typename T1= ___NO_VARG___, typename T2 = ___NO_VARG___,
typename T3 = ___NO_VARG___, typename... others>
struct ___getTypeVArg3
{
	using type = T3;
	static constexpr size_t typeSize 		= vaSizeof<type>;
	static constexpr size_t remainingArgs 	= sizeof...(others);
};

template<typename T1= ___NO_VARG___, typename T2= ___NO_VARG___,
typename T3= ___NO_VARG___, typename T4  = ___NO_VARG___, typename... others>
struct ___getTypeVArg4
{
	using type = T4;
	static constexpr size_t typeSize 		= vaSizeof<type>;
	static constexpr size_t remainingArgs 	= sizeof...(others);
};

template<typename T1= ___NO_VARG___, typename T2= ___NO_VARG___, typename T3= ___NO_VARG___,
typename T4= ___NO_VARG___, typename T5  = ___NO_VARG___, typename... others>
struct ___getTypeVArg5
{
	using type = T5;
	static constexpr size_t typeSize 		= vaSizeof<type>;
	static constexpr size_t remainingArgs 	= sizeof...(others);
};

template<typename T1= ___NO_VARG___, typename T2= ___NO_VARG___, typename T3= ___NO_VARG___,
typename T4= ___NO_VARG___, typename T5= ___NO_VARG___, typename T6  = ___NO_VARG___, typename... others>
struct ___getTypeVArg6
{
	using type = T6;
	static constexpr size_t typeSize 		= vaSizeof<type>;
	static constexpr size_t remainingArgs 	= sizeof...(others);
};

template<typename T1 = ___NO_VARG___, typename T2= ___NO_VARG___, typename T3= ___NO_VARG___,
typename T4= ___NO_VARG___, typename T5= ___NO_VARG___, typename T6= ___NO_VARG___, typename T7 = ___NO_VARG___, typename... others>
struct ___getTypeVArg7
{
	using type = T7;
	static constexpr size_t typeSize 		= vaSizeof<type>;
	static constexpr size_t remainingArgs 	= sizeof...(others);
};

template<typename T1= ___NO_VARG___, typename T2= ___NO_VARG___, typename T3= ___NO_VARG___, typename T4= ___NO_VARG___,
typename T5= ___NO_VARG___, typename T6= ___NO_VARG___, typename T7= ___NO_VARG___, typename T8 = ___NO_VARG___, typename... others>
struct ___getTypeVArg8
{
	using type = T8;
	static constexpr size_t typeSize 		= vaSizeof<type>;
	static constexpr size_t remainingArgs 	= sizeof...(others);
};

template<typename T1= ___NO_VARG___, typename T2= ___NO_VARG___,
typename T3= ___NO_VARG___, typename T4= ___NO_VARG___,
typename T5= ___NO_VARG___, typename T6 = ___NO_VARG___, typename T7= ___NO_VARG___,
typename T8= ___NO_VARG___, typename T9 = ___NO_VARG___, typename... others>
struct ___getTypeVArg9
{
	using type = T9;
	static constexpr size_t typeSize 		= vaSizeof<type>;
	static constexpr size_t remainingArgs 	= sizeof...(others);
};

template<typename T> static constexpr bool isValidVArg = true;
template<> static constexpr bool isValidVArg<___NO_VARG___> = false;

#define		getNthTypename(n, args)		\
typename cloture::util::templates::variadic::___getTypeVArg##n <args...>::type

#define		getNthTypenameSize(n, args)	\
		cloture::util::templates::variadic::___getTypeVArg##n <args...>::typeSize

/*
 * 	container for non-typename elements
 */
template<typename T, T... pack>
struct paramPack
{
	static constexpr size_t size = sizeof...(pack);
	using type = T;
	static constexpr T arr[] =
	{
			pack...
	};
	constexpr paramPack() {}

	constexpr T operator[](size_t index) const
	{
		return arr[index];
	};

	constexpr T& operator[](size_t index)
	{
		return arr[index];
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

