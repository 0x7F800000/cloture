#pragma once


namespace cloture		{
namespace util			{
namespace stream		{

enum class Radix : common::uint8
{
	decimal,
	hex
};

template<void (*output_func)(const char*, ...), typename userDataType = void>
class ostream
{
	Radix rad;
	template<typename TT> struct isUdataVoid
	{
		static constexpr bool value = false;
	};
	template<> struct isUdataVoid<void>
	{
	};

	__if_exists(isUdataVoid<typename userDataType>::value)
	{
		userDataType udata;
	}

public:


	ostream& operator <<(const bool b)
	{
		if(b)
			output_func("true");
		else
			output_func("false");
		return *this;
	}

	ostream& operator <<(Radix radix)
	{
		rad = radix;
		return *this;
	}
	ostream& operator <<(common::uint8 b)
	{
		switch(rad)
		{
		case Radix::decimal:
			output_func("%hhu", b);
			break;
		case Radix::hex:
			output_func("0x%hhX", b);
			break;
		};
		return *this;
	}
	ostream& operator <<(common::uint16 b)
	{
		switch(rad)
		{
		case Radix::decimal:
			output_func("%hu", b);
			break;
		case Radix::hex:
			output_func("0x%hX", b);
			break;
		};
		return *this;
	}
	ostream& operator <<(common::uint32 b)
	{
		switch(rad)
		{
		case Radix::decimal:
			output_func("%iu", b);
			break;
		case Radix::hex:
			output_func("0x%iX", b);
			break;
		};
		return *this;
	}
	ostream& operator <<(common::uint64 b)
	{
		switch(rad)
		{
		case Radix::decimal:
			output_func("%llu", b);
			break;
		case Radix::hex:
			output_func("0x%llX", b);
			break;
		};
		return *this;
	}
	/**
	 * signed int types
	 */
	ostream& operator <<(common::int8 b)
	{
		switch(rad)
		{
		case Radix::decimal:
			output_func("%hhi", b);
			break;
		case Radix::hex:
			output_func("0x%hhX", b);
			break;
		};
		return *this;
	}
	ostream& operator <<(common::int16 b)
	{
		switch(rad)
		{
		case Radix::decimal:
			output_func("%hi", b);
			break;
		case Radix::hex:
			output_func("0x%hX", b);
			break;
		};
		return *this;
	}
	ostream& operator <<(common::int32 b)
	{
		switch(rad)
		{
		case Radix::decimal:
			output_func("%d", b);
			break;
		case Radix::hex:
			output_func("0x%dX", b);
			break;
		};
		return *this;
	}
	ostream& operator <<(common::int64 b)
	{
		switch(rad)
		{
		case Radix::decimal:
			output_func("%lli", b);
			break;
		case Radix::hex:
			output_func("0x%llX", b);
			break;
		};
		return *this;
	}
	ostream& operator <<(const char* b)
	{
		output_func("%s", b);
		return *this;
	}
};//class ostream

}//namespace stream
}//namespace util
}//namespace cloture
