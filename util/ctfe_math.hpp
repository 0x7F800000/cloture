#pragma once

/*
 * even though the compiler can determine the result of calling standard math functions with constant
 * arguments at compile time it is invalid to do that in constexpr functions,
 * as those functions are not declared constexpr
 */

namespace cloture {
namespace util {
namespace ctfe {
namespace math
{
	using namespace cloture::util::common;

	template<typename T>
	static constexpr bool isNaN(const T n)
	{
		return n != n;
	}
	template<typename T>
	static constexpr bool isFinite(const T n)
	{
		return minValue<T> <= n && n <= maxValue<T>;
	}
	template<typename T> struct fpRaw
	{

	};

	template<> struct fpRaw<float>
	{
		using type = int32;
	};

	template<> struct fpRaw<double>
	{
		using type = int64;
	};

	template<typename T>
	static constexpr typename fpRaw<T>::type rawBits(const T n)
	{
		return -1;
	}
	//from http://www.merlyn.demon.co.uk/js-exact.htm#IEEE
	template<>
	static constexpr int rawBits<float>(const float n)
	{
		if (isNaN(n))
			return 0x7F800001;
		uint32 sign = 0;
		float n2 = n;

		if (sign = ((n + 1.0f/n) < .0f) << 31)
			n2 = -n2;

		if (!n2)
			return sign;

		if (!isFinite(n2))
			return sign | 0x7F800000;

		n2 = static_cast<double>(n2) * (1.0 + 2.9802322387695312e-8);
		uint32 exponent = 127;

		while (exponent < 254 && n2 >= 2.0f)
		{
			exponent++;
			n2 /= 2.0f;
		}
		if (n2 >= 2.0f)
			return sign | 0x7F800000;
		while (exponent > .0f && n2 <  1.0f)
		{
			exponent--;
			n2 *= 2.0f;
		}
		exponent ? n2-- : n2 /= 2.0f;
		const uint32 mantissa = static_cast<uint32>(n2 * static_cast<float>(0x00800000));
		return sign | exponent << 23 | mantissa;
	}

	static_assert(
			rawBits<float>(1.0f) == 0x3f800000,
			"rawBits<float> is incorrect."
			);

	/*
	 * based on rawBits<float>. All that was needed was to change the constants
	 * to work with double
	 */
	template<>
	static constexpr int64 rawBits<double>(const double n)
	{
		if (isNaN(n))
			return 0x7FF0000000000001LL;
		uint64 sign = 0ULL;
		double n2 = n;

		if (sign = static_cast<uint64>((n + 1.0/n) < .0) << 63ULL)
			n2 = -n2;

		if (!n2)
			return sign;

		if (!isFinite(n2))
			return sign | 0x7FF0000000000000LL;

		int64 exponent = 1023ULL;

		while (exponent < 2046 && n2 >= 2.0)
		{
			exponent++;
			n2 /= 2.0;
		}
		if (n2 >= 2.0)
			return sign | 0x7FF0000000000000LL;
		while (exponent > 0LL && n2 < 1.0)
		{
			exponent--;
			n2 *= 2.0;
		}
		exponent ? n2-- : n2 /= 2.0;
		const uint64 mantissa = static_cast<uint64>(n2 * static_cast<double>(1LL << 52LL));
		return sign | exponent << 52LL | mantissa;
	}

	static_assert(
	rawBits<double>(3.14159265358979323846) == 0x400921FB54442D18LL,
	"rawBits<double> is fukt?"
	);

	//http://www.gamedev.net/topic/329991-i-need-floor-function-implementation/
	using arithDefault = double;


	/*
	 * ctfe::math::abs
	 * calculates the absolute value of x
	 * for integral and floating point types.
	 * there is no fabs
	 */
	template<typename T>
	static constexpr T abs(const T x)
	{
		constexpr T ZERO = static_cast<T>(0);
		if(x < ZERO)
			return -x;
		return x;
	}

	/**
	 * ctfe::math::sqrt
	 * calculates the square root of x (dumbass)
	 */
	template<typename T>
	static constexpr T sqrt(const T x)
	{
		constexpr T TWO 	= static_cast<T>(2);
		constexpr T ZERO = static_cast<T>(0);
		T result = x / TWO;
		if(result < ZERO)
			return ZERO;

		#define	__SQRT_LOOP_CONDITION()	static_cast<double>(\
		abs( result - (x / result)) / result	\
) > 0.000000000000001

		#define		__SQRT_ONE_ITERATION()	\
			if(!(__SQRT_LOOP_CONDITION())) break; else result = (result + (x / result)) / TWO

		while(	__SQRT_LOOP_CONDITION()	)
		{
			result = (result + (x / result)) / TWO;
			__SQRT_ONE_ITERATION();
			__SQRT_ONE_ITERATION();
		}
		return result;
	}

	/**
	 * regression tests for sqrt
	 */
	static_assert(
	sqrt<double>(536.0) == 23.15167380558045
	&&
	sqrt<double>(8000.0 * 1336.0) == 3269.250678672408
	&&
	sqrt<double>(1.79769e+308) == 1.3407796239501852e+154);

	/**
	 * 	ctfe::math::hypot
	 * 	calculates hypotenuse of a right-angled triangle with legs x and y
	 */
	template<typename T>
	static constexpr T hypot(const T x, const T y)
	{
		return sqrt<T>(x*x+y*y);
	}

	static_assert(
	hypot<double>(50.0, 119.0) == 129.07749610214788);

	template<typename T>
	static constexpr T ceil(const T x)
	{
		using iType = int64;

		constexpr T ZERO	= static_cast<T>(0);
		constexpr T ONE		= static_cast<T>(1);

		const iType xInt = static_cast<iType>(x);
		if(x < ZERO)
			return static_cast<T>(xInt);
		return static_cast<T>(xInt + ONE);

	}
	template<typename T>
	static constexpr T cbrt(const T x)
	{
		constexpr T FOUR = static_cast<T>(4);
		constexpr T THREE = static_cast<T>(3);
		T result = x / FOUR;

		#define __CBRT_LOOP_CONDITION()		\
			abs(result - (x / result / result)) / result > 0.00000000000001

		while(__CBRT_LOOP_CONDITION())
		{
			result = (result + (x / result / result) + result) / THREE;
		}
		return result;
	}
	template<typename T>
	static constexpr T pow(const T base, const T exponent)
	{
		constexpr T ZERO 	= static_cast<T>(0);
		constexpr T ONE 	= static_cast<T>(1);
		//need the absolute value of the exponent
		const T absExponent = abs(exponent);

		//right now, only integer exponent is supported
		int i = static_cast<int>(absExponent) - 1;

		//base^0 = 1
		if(!i)
			return ONE;

		T result = base;
		while(i)
		{
			result = result * base;
			i--;
		}
		//return the inverse of base^absExponent if exponent was negative
		//this accurately handles negative exponents
		return (	exponent < ZERO		)
		? ONE / result
		: result;
	}
	static_assert(pow<double>(8.5, 6.0) == 377149.515625);

	template<typename T>
	static constexpr T fromRaw(typename fpRaw<T>::type raw)
	{
	}

	template<>
	static constexpr float fromRaw<float>(int raw)
	{
		unsigned sign		= (raw >> 31) & 1;
		signed exponent	= (raw >> 23) & 0xFF;

		signed mantissa 	= raw & 0x007FFFFF;
		//they check for an invalid exponent, but we cant return NaN
		if(exponent != 0)
		{
			exponent -= 127;
			mantissa |= 0x00800000;
		}
		else
			exponent = -126;

		const float f = static_cast<float>(mantissa) *
		pow<float>(2.0f, static_cast<float>(exponent - 23));
		return sign != 0 ? -f : f;
	}
	static_assert(fromRaw<float>(1062668861) == 0.84f);



	template<typename T>
	static constexpr T floor(const T x)
	{
		constexpr T POINTNINE = static_cast<T>(0.9999999999999999);
		constexpr T ZERO = static_cast<T>(0);
		if(x > ZERO)
			return static_cast<int>(x);
		return static_cast<int>(x - POINTNINE);
	}

} //namespace math
}//namespace ctfe
}//namespace util
}//namespace cloture
