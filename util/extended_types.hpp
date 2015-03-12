#pragma once

namespace cloture	{
namespace util		{
namespace common	{

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

	/*
	 * todo: use some ctfe:: functions in here to clean this up
	 * maybe make this ctfe::parser::parse<uint128>
	 */
	constexpr uint128 toU128(const char* str)
	{
		constexpr uint128 ZERO 		= static_cast<uint128>(0);
		constexpr uint128 TEN 		= static_cast<uint128>(10);
		constexpr uint128 SIXTEEN	= static_cast<uint128>(16);
		constexpr uint128 SIGN		= static_cast<uint128>(0x8000000000000000ULL) << 64;

		constexpr char digitSeperator = '\'';

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

				if(c == digitSeperator)
					continue;

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
				if(c == digitSeperator)
					continue;
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
				if(c == digitSeperator)
					continue;
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

	#include	"quad.hpp"

}//namespace common
}//namespace util
}//namespace cloture
