#pragma once

/**
	statif (global scope)
		helper template for achieving true static if statements in C++
		if val is false, the struct does not define the alias isTrue within its namespace
		if val is true, the struct defines isTrue 
	
	this is intended for use with __if_exists and __if_not_exists 
*/
template<bool val> struct statif{};
template<> struct statif<true>	{using isTrue = void;};

#define		__static_if(condition)		__if_exists(statif<condition>::isTrue)
#define 	__static_if_not(condition)	__if_not_exists(statif<condition>::isTrue)


namespace cloture
{
namespace util
{
namespace ctfe
{
	using common::unone;
	using common::none;
	/**
		basic pair class
	*/
	template<typename t1, typename t2>
	struct pair
	{
		t1 first;
		t2 second;

		constexpr __pure pair() : first(), second()
		{}

		constexpr __pure pair(t1 f, t2 s) : first(f), second(s)
		{}
	};
#if hasExtendedConstexpr
	/**
	 * The majority of this code I wrote a few months ago when I was attempting to create
	 * a metacompiler using constexpr.
	 * 
	 * it failed miserably, but here some of it LIVES ON
	 * 
	*/
	static constexpr int charToInt(const char c)
	{
		return c - '0';
	}
	
	static constexpr bool isWhitespace(const char c)
	{
		return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f';
	}
	static constexpr bool isIdentifierChar(const char c)
	{
		return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
	}
	static constexpr size_t skipLeadingWhitespace(const char* buffer, const size_t position = 0)
	{
		size_t i = 0;

		if( !isWhitespace( buffer[position] ) )
			return 0;

		for(; isWhitespace(buffer[ i + position ]); ++i )
			if(!buffer[ i + position ])
				return unone;

		return i + position;
	}


	static constexpr size_t whitespaceEnd(const char* buffer)
	{
		size_t i = 0;
		if(isWhitespace(buffer[i]))
		{
			while( isWhitespace( buffer[i] ) )
				++i;
		}
		return i;
	}

	static constexpr size_t nextWhitespaceEnd(const char* buffer, const size_t start = 0)
	{
		size_t i = start;
		while( !isWhitespace(buffer[i]) && buffer[i])
			++i;
		while(isWhitespace(buffer[i]))
			++i;
		return i;
	}
	
	/**
		ctfe::Array
			compile-time array copyable array
			has *all* of the bells and whistles
			all of them
		
		i think there's something wrong with this class. I can't recall what exactly the issue is, but...
		i guess we'll find out!
	*/
	template<typename T, size_t sz = 0>
	class Array
	{
	public:
		T data[sz + 1];
	
		constexpr size_t size() const
		{
			return sz;
		}
		constexpr bool empty() const
		{
			return size() == 0;
		}
		
		
		constexpr Array() : data(){}
	
		constexpr auto& operator[](const size_t index)
		{
			return data[index];
		}
	
		constexpr auto operator[](const size_t index) const
		{
			return data[index];
		}
		constexpr Array(const Array<T, sz>& other) : data()
		{
			for(size_t i = 0; i < sz; ++i)
				data[i]		=		other[i];
		}
	
		constexpr Array(const T* other) : data()
		{
			for(size_t i = 0; i < sz; ++i)
				data[i]		=		other[i];
		}
	
	
	
		constexpr bool operator==(const Array<T, sz>& other) const
		{
			for(size_t i = 0; i < sz; ++i)
			{
				if(other[i] != (*this)[i])
					return false;
			}
			return true;
		}
	
		constexpr bool operator==(const T* const cmp) const
		{
			for(size_t i = 0; i < sz; ++i)
			{
				if(cmp[i] != this->data[i])
					return false;
			}
			return true;
		}
	
		constexpr bool operator!=(const Array<T, sz>& other) const
		{
			return !((*this) == other);
		}
	
		constexpr Array<T, sz>& operator=(const T* const RESTRICT rhs)
		{
			for(size_t i = 0; i < sz; ++i)
				data[i]		=	rhs[i];
			return (*this);
		}
		constexpr Array<T, sz>& operator=(const Array<T, sz>& rhs)
		{
			for(size_t i = 0; i < sz; ++i)
				data[i]		=	rhs[i];
			return (*this);
		}
		explicit constexpr operator T*()		{return data;}
		explicit constexpr operator const T*()	{return data;}
	};

	template<size_t sz>
	class CString : public Array<char, sz>
	{
	public:
		constexpr size_t length() const
		{
			size_t i = 0;
			for(; i < sz && ((*this)[i]) != 0; ++i)	;
			return i;
		}

		explicit constexpr operator const char*() const	{return this->data;}
		constexpr CString() : Array<char, sz>::Array()	{}
		constexpr CString(const char* s) : Array<char, sz>::Array(s){}

		constexpr const char operator[](const size_t idx) const
		{
			return this->data[idx];
		}

		constexpr char& operator[](const size_t idx)
		{
			return this->data[idx];
		}
	};

	template<size_t sz, bool reverse = false>
	class charStream : public CString<sz>
	{

	};

	template<size_t sz>
	class charStream<sz, false> : public CString<sz>
	{
		size_t position;
	public:
		constexpr charStream() : CString<sz>::CString(), position(0)
		{}

		constexpr const bool endReached() const
		{
			return position >= sz;
		}
		constexpr char operator<<(const char c)
		{
			if( !endReached() )
			{
				(*this)[position]	=	c;
				++position;
			}
			return c;
		}

		constexpr char operator>>(char& c)
		{
			c = -1;
			if( !endReached() )
			{
				c	=	(*this)[position];
				++position;
			}
			return c;
		}

		constexpr const bool operator!() const
		{
			return endReached();
		}

		constexpr operator bool() const
		{
			return !endReached();
		}
		constexpr void reset()
		{
			position = 0;
		}
	};


	template<size_t sz>
	class charStream<sz, true> : public CString<sz>
	{
		mutable size_t position;
	public:
		constexpr charStream() : CString<sz>::CString(), position(sz - 1)	{}

		constexpr const bool endReached() const
		{
			return static_cast<ptrdiff_t>( position ) < 0;
		}

		constexpr char operator<<(const char c)
		{
			if( endReached() )
				return c;
			(*this)[position]		=	c;
			--position;
			return c;
		}
		constexpr char operator>>(char& c)
		{
			c = -1;
			if( !endReached() )
			{
				c	=	(*this)[position];
				--position;
			}
			return c;
		}
		constexpr const bool operator!() const
		{
			return endReached();
		}

		constexpr operator bool() const
		{
			return !endReached();
		}
		constexpr void reset()
		{
			position = sz - 1;
		}
	};

	namespace parser
	{
		using namespace cloture::util::common;
		template<typename T> constexpr auto		parse(		const char* text	){	return nullptr;	}
		template<typename T> constexpr bool		parseable(	const char* text	){	return false;	}
		template<typename T> constexpr size_t	skip(		const char* text	){	return unone;	}
	
	
		template<> constexpr auto
			parse<	int32	>(	const char* text	)
		{
			size_t i 		=	0;
			bool negative 	=	false;
			int32 value		=	0;
	
			if(	text[i] == '-'	)
			{
				negative = true;
				++i;
			}
			/*
				read each digit
			*/
			bool firstIteration = true;
			for(char c = 0; 0 <= (c = charToInt( text[i] ) ) && c <= 9; ++i)
			{
				if(!firstIteration)
					value *= 10;
				value += c;
				if(firstIteration)
					firstIteration = false;
			}
	
			return negative ? -value : value;
		}
		static_assert
		(
				parse<int32>("-1000")	==	-1000
			&&	parse<int32>("530395")	== 	530395
			&& parse<int32>("45")		==	45
			&& parse<int32>("20000")	==	20000,
			"parser::parse<int32> is not returning correct results"
		);
	
		template<> constexpr bool
			parseable<real32>(const char* text)
		{
			size_t i = 0;
	
			if( isWhitespace(text[i]) || !text[i])
				return false;
	
			char c = charToInt(text[i]);
			const bool negative		=	text[i] == '-';
	
			/*	not the sign, not a decimal point, not a number	*/
			if(!negative && text[i] != '.' && (0 > c || c > 9))
				return false;
	
			bool foundPoint		=	false;
			size_t numDigits	=	0;
			if(negative)	++i;
			while(true)
			{
				if( text[i] == '.' )
				{
	
					if(foundPoint)	/*	two decimal points? that aint right	*/
						return false;
	
					foundPoint = true;
	
					++i;
					continue;
				}
	
				c = charToInt(text[i]);
	
				if(isWhitespace(text[i]) || (0 > c || c > 9) && !isIdentifierChar(text[i]))
					return numDigits != 0;
	
				++i, ++numDigits;
			}
			return false;
		}
	
		static_assert
		(
				parseable<real32>("-200.056932")
			&&	parseable<real32>("3.141592654")
			&& parseable<real32>("2.718281828")
			&& parseable<real32>("666.666666"),
			"parser::parseable<real32> incorrectly returned false."
		);
	
		template<> constexpr size_t
			skip<	real32	>(	const char* text	)
		{
			if(!parseable<real32>(text))
				return unone;
			size_t i = 0;
			for(; !isWhitespace(text[i]) && (text[i] == '-' || text[i] == '.' || (text[i] >= '0' && text[i] <= '9')); ++i )
				;
			return i;
		}
	
	
	
		template<> constexpr auto
			parse<	real32	>(const char* text)
		{
			size_t i 		=	0;
			bool negative 	=	false;
			real32 integral	=	.0f;
	
			if(text[i] == '-')
			{
				negative = true;
				++i;
			}
			/*
				read the integral part
			*/
			bool firstIteration = true;
	
			for(char c = 0; text[i] != '.' && (0 <= (c = charToInt( text[i] ) ) && c <= 9); ++i)
			{
				if(!firstIteration)
					integral *= 10.0f;
				integral += static_cast<real32>(c);
				if(firstIteration)
					firstIteration = false;
			}
			if(text[i] != '.')
				return negative ? -integral : integral;
	
			const size_t fractionalStart	=	i;
			real32 fractional = .0f;
			++i;
			/*
				seek to the end
			*/
			{
			char c = 0;
			while( 0 <= (c = charToInt(text[i])) && c <= 9)
				++i;
			}
	
			firstIteration = true;
			/*	read the fractional portion	*/
			for(char c = 0; i > fractionalStart; --i)
			{
				c	=	charToInt(text[i]);
				if( 0 > c || c > 9)
					continue;
	
				fractional	+=	static_cast<real32>(c);
				fractional	/=	10.0f;
	
			}
	
			/*	unify the fractional and integral parts	*/
			const real32 value = fractional + integral;
			return negative ? -value : value;
		}
	
		static_assert
		(
				parse<real32>("666.666") 		==	666.666f
			&&	parse<real32>("3.141592654")	== 	3.141592654f
			&& parse<real32>("2.718281828")	==	2.718281828f,
			"parser::parse<real32> is fuckt."
		);
	
		template<int32 value>
		static constexpr size_t calcRepresentationLen()
		{
			size_t totalLength = 0;
	
			if(value < 0)
				++totalLength;
			int32 shifter = value;
			do
			{
				++totalLength;
				shifter /= 10;
			}
			while(shifter);
			return totalLength;
		}
	
		template<int32 val>
		struct toStringHelper_int32
		{
			static constexpr size_t sLen = calcRepresentationLen<val>() + 1;
			using sType = charStream< sLen, false>;
			//static constexpr
			sType b;// = sType();
	
			constexpr toStringHelper_int32()
			{
				int32 i = val;
				if (i >= 0)
				{
					do
					{
						const char c = static_cast<char>('0' + static_cast<char>(i % 10));
						//clang is complaining and thinks this is a bit shift...
						//worked fine in GCC

						//b << c;

						b.operator<<(c);
						i /= 10;
					}
					while (i != 0);
				}
				else
				{
					//b << '-';
					b.operator<<('-');
					do
					{
						const char c = static_cast<char>('0' - (i % 10));
					//	b << c;
						b.operator<<(c);
						i /= 10;
					}
					while (i != 0);
				}
			}
	
			explicit constexpr operator decltype(b) () const	{	return b;	}
		};
		static_assert(calcRepresentationLen<20>() == 2, "woops");

		template<int32 value> static constexpr
		CString<	calcRepresentationLen<value>() + 1	> toString =	toStringHelper_int32<value>().b;

		static_assert(toString<3>[0] == '3',
		"weird");
		static_assert((static_cast<const char*>(toString<40>))[1] == '4', "?");
		static_assert(toString<994>.size() == 4, "uhhh");
		static_assert(toString<1055>[4] == 0, "ruh roh");
	
	}//namespace parser

#endif //#if hasExtendedConstexpr
}//namespace ctfe
}//namespace util
}//namespace cloture
