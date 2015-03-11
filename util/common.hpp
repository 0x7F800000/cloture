#pragma once

namespace cloture//::util::common
{
namespace util
{
namespace common
{
	/*
		integer primitive types
	*/
	using uint8		=	unsigned char;
	using int8 		=	signed char;

	using uint16	=	unsigned short;
	using int16 	=	signed short;

	using uint32 	=	unsigned int;
	using int32 	=	signed int;

	using uint64 	=	unsigned __int64;//unsigned long long;
	using int64 	=	__int64;//signed long long;

	/*
		sanity checks
	*/
	static_assert(	sizeof(uint8)	==	1	&&	sizeof(int8)	==	1, "sizeof int8/uint8 ought to be one byte...");
	static_assert(	sizeof(uint16)	==	2	&&	sizeof(int16)	==	2, "sizeof int16/uint16 should be two bytes...");
	static_assert(	sizeof(uint32)	==	4	&&	sizeof(int32)	==	4, "sizeof int32/uint32 should be four bytes...");
	static_assert(	sizeof(uint64)	==	8	&&	sizeof(int64)	==	8, "sizeof int64/uint64 should be eight bytes...");

	/*
		real number types
	*/
	using real32	=	float;
	using real64	=	double;
	using real128	=	long double;

	/**
	 * like sizeof, but for bits instead of bytes
	 * intended for use with basic types
	 */
	template<typename T> constexpr size_t bitSizeof = 	sizeof(T) * 8;

	static_assert
	(
			bitSizeof<char>			==	8
		&&	bitSizeof<short>		==	16
		&&	bitSizeof<int>			==	32
		&&	bitSizeof<long long>	==	64,
		"Apparently bitSizeof is fuckt. Pretty cray."
	);
	/**
	 * popcnt
	 *
	 * counts the number of bits set
	 */
	template<typename T> constexpr
	size_t popcnt(const T inputVal)
	{
		static_assert(is_integral(T), "popcnt requires an integral type.");

		/*
			signed shift would produce inaccurate results
		*/
		make_unsigned(T) val = static_cast<make_unsigned(T)>(inputVal);
		size_t cnt = 0;

		for(size_t i = 0; i < bitSizeof<T>; ++i )
		{
			if(val & 1 != 0)
				++cnt;
			val >>= 1;
		}
		return cnt;
	}

	static_assert
	(
		popcnt<int32>(3) == 2,
		"meta::popcnt is returning inaccurate results..."
	);

	static constexpr ptrdiff_t 	none	=  -1;
	static constexpr size_t 	unone	= (none);
}
}
};
