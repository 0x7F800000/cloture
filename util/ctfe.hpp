#pragma once


/**
 * The majority of this code I wrote a few months ago when I was attempting to create
 * a metacompiler using constexpr.
 *
 * it failed miserably, but here some of it LIVES ON
 *
*/
namespace cloture::util::ctfe
{
	using common::unone;
	using common::none;


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

	static constexpr size_t cstrlen(const char* s)
	{
		size_t i = 0;
		for(; s[i]; ++i)
			;
		return i;
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

} //namespace cloture::util::ctfe

#define constantCString(s)		\
cloture::util::ctfe::CString<cloture::util::ctfe::cstrlen(s)>::CString<cloture::util::ctfe::cstrlen(s)>(s)


#include 	"libCtfe/constcontainers.hpp"
#include 	"libCtfe/constparse.hpp"
#include 	"libCtfe/constmath.hpp"
#include 	"libCtfe/constcrc.hpp"
