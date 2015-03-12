#pragma once

namespace cloture {
namespace util    {
namespace common  {
	/*
	integer primitive types
	*/
	using uint8 = unsigned char;
	using int8 = signed char;
	using uint16 = unsigned short;
	using int16 = signed short;
	using uint32 = unsigned int;
	using int32 = signed int;
	using uint64 = unsigned __int64;//unsigned long long;
	using int64 = __int64;//signed long long;

	#if defined(__clang__) || defined(__GNUC__) || defined(__INTEL_COMPILER)
		#define	compilerSupportsInt128	1
	#else
		#define	compilerSupportsInt128	0
	#endif

	#if compilerSupportsInt128 && defined(__x86_64__)
		using int128	= __int128;
		using uint128	= unsigned __int128;
	#else
		/*
		 * include some header here containing a class to emulate __int128
		 */
	#endif

	constexpr uint128 toU128(const char* str)
	{
		constexpr uint128 ZERO 		= static_cast<uint128>(0);
		constexpr uint128 TEN 		= static_cast<uint128>(10);
		constexpr uint128 SIXTEEN	= static_cast<uint128>(16);
		constexpr uint128 SIGN		= static_cast<uint128>(0x8000000000000000ULL) << 64;
		uint128 result = ZERO;
		size_t pos = 0;
		//hexadecimal literal
		if(str[0] == '0' && (str[1] == 'x' || str[1] == 'X'))
		{
			const char* s = &str[2];
			while(true)
			{
				const unsigned char c = static_cast<unsigned char>(s[pos++]);
				if(!c)
					return result;

				uint128 digit = ZERO;

				if(c >= '0' && c <= '9')
					digit = static_cast<uint128>(c - '0');
				else if(c >= 'a' && c <= 'z')
					digit = static_cast<uint128>(c - ('a' - 10));
				else
					digit = static_cast<uint128>(c - ('A' - 10));
				result *= SIXTEEN;
				result += digit;
			}
		}
		//binary literal
		else if(str[0] == '0' && (str[1] == 'b' || str[1] == 'B'))
		{
			const char* s = &str[2];

			while(true)
			{
				const char c = s[pos++];
				if(!c)
					return result;
				const uint128 bit = static_cast<uint128>(c - '0');
				result <<= 1;
				result |= bit;
			}
		}
		//decimal literal
		else
		{
			const bool negative = str[0] == '-';

			if(negative || str[0] == '+')
				++pos;

			bool first = true;
			for(char c = 0; 0 <= (c = str[pos] - '0' ) && c <= 9; ++pos)
			{
				if(!first)
					result *= TEN;
				uint128 c128 = static_cast<uint128>(c);
				result += c128;
				if(first)
					first = false;
			}

			return negative ? result | SIGN : result;
		}
	}

	constexpr uint128 operator"" _u128(const char *str)
	{
		return toU128(str);
	}
	constexpr int128 operator"" _i128(const char *str)
	{
		return static_cast<int128>(toU128(str));
	}

	static_assert(
	0x7ffeffffffffffffffffffffffffffff_u128 ==
	((static_cast<uint128>(0x7ffeffffffffffffULL) << 64)
	|
	static_cast<uint128>(0xffffffffffffffffULL)),
	"hexadecimal int128/uint128 literals are fuckt"
	);
	/*
	sanity checks
	*/
	static_assert( sizeof(uint8) == 1	&& sizeof(int8) == 1, "sizeof int8/uint8 ought to be one byte...");
	static_assert( sizeof(uint16) == 2	&& sizeof(int16) == 2, "sizeof int16/uint16 should be two bytes...");
	static_assert( sizeof(uint32) == 4	&& sizeof(int32) == 4, "sizeof int32/uint32 should be four bytes...");
	static_assert( sizeof(uint64) == 8	&& sizeof(int64) == 8, "sizeof int64/uint64 should be eight bytes...");
	/*
	real number types
	*/
	using real32 = float;
	using real64 = double;
	using real80 = long double;


	static constexpr ptrdiff_t 	none	=  -1;
	static constexpr size_t 	unone	= (none);

	template<typename T> static constexpr T maxValue 	= 0;

	template<> static constexpr real64 	maxValue<real64> 	= 1.79769e+308;
	template<> static constexpr real32 	maxValue<real32> 	= 3.40282e+38f;
	template<> static constexpr real80 	maxValue<real80>	= (sizeof(real80) > sizeof(double) ? 1.18973e+4932L : maxValue<real64>);

}//namespace common
}//namespace util
}//namespace cloture