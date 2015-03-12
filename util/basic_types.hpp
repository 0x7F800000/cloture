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

	template<typename T> static constexpr T maxValue 		= 0;
	template<> static constexpr real64 	maxValue<real64> 	= 1.79769e+308;
	template<> static constexpr real32 	maxValue<real32> 	= 3.40282e+38f;

	template<typename T> static constexpr T minValue 		= 0;
	template<> static constexpr real64 	minValue<real64> 	= 2.22507e-308;
	template<> static constexpr real32 	minValue<real32> 	= 1.17549e-38f;
	//template<> static constexpr real80 	maxValue<real80>	= (sizeof(real80) > sizeof(double) ? 1.18973e+4932L : maxValue<real64>);

}//namespace common
}//namespace util
}//namespace cloture
