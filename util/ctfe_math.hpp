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

	template<typename T> constexpr T positiveInf = 0;
	template<> constexpr float positiveInf<float> = 1.0f / 0.0f;

	template<typename T>
	static constexpr bool isNaN(const T n)
	{
		return n != n;
	}
	template<typename T>
	static constexpr bool isFinite(const T n)
	{
		return minValue<T> <= n && value <= maxValue<T>;
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
	static constexpr fpRaw<T>::type rawBits(const T n)
	{
		return -1;
	}

	template<>
	static constexpr int rawBits(const float n)
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


	//http://www.gamedev.net/topic/329991-i-need-floor-function-implementation/
	using arithDefault = double;


	constexpr uint32 SngFwd(const uint32 sign, const uint32 exponent, const float mt)
	{
		const uint32 b = static_cast<uint32>(8388608.0 * static_cast<double>(mt) + .5);
		return
		(0xFF & b)
		|
		(0xFF00 & b)
		|
		((0x7F0000 & b)
		|
		((exponent & 1) << 23))
		|
		((sign << 31) | ((exponent >> 1) << 24));
	}

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
		//ehehehehe
		//shit
		int i = static_cast<int>(exponent)- 1;
		T result = base;
		while(i)
		{
			result = result * base;
			i--;
		}
		return result;
	}
	static_assert(pow<double>(8.5, 6.0) == 377149.515625);


} //namespace math
}//namespace ctfe
}//namespace util
}//namespace cloture
