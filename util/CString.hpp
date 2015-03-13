#pragma once

namespace cloture
{
namespace util
{
namespace string
{
	class ceString
	{
		const char* s;
	public:
		using charType = __typeof__(*s);
		constexpr __pseudopure inline ceString(const char* ss) noexcept : s(ss)
		{}

		constexpr __pseudopure inline charType operator[](const size_t index) const noexcept
		{
			return s[index];
		}
		constexpr __pseudopure inline charType& operator[](const size_t index) noexcept
		{
			return s[index];
		}
		__pseudopure inline bool operator ==(ceString other) const noexcept
		{
			return __builtin_strcmp(s, other.s) == 0;
		}
		__pseudopure inline bool operator !=(ceString other) const noexcept
		{
			return !((*this) == other);
		}
		explicit constexpr __pseudopure inline operator bool() const noexcept
		{
			return s != nullptr;
		}
		constexpr __pseudopure inline bool operator !() const noexcept
		{
			return !(bool(*this));
		}

		constexpr __pseudopure inline operator const char*() noexcept
		{
			return s;
		}

		constexpr inline void swap(ceString& other) noexcept
		{
			const char* original = s;
			s = other.s;
			other.s = original;
		}

		constexpr __pseudopure size_t length() const noexcept
		{
			size_t i = 0;
			for(; s[i]; ++i)
				;
			return i;
		}
	};//class ceString

}//namespace string
}//namespace util
}//namespace cloture
