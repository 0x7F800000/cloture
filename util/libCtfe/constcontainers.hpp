#pragma once

namespace cloture::util::ctfe
{
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

	template <char... characters>
	static constexpr auto //CString<sizeof...(characters)>
	operator "" _constexprString() noexcept
	{
		constexpr char temp[] = {characters...};
		return CString<sizeof...(characters)>(temp);
	}

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
		size_t position;
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
}
