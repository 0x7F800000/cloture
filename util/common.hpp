#pragma once

namespace cloture//::util::common
{
namespace util
{
namespace common
{
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

}
}
};
