#pragma once

#define		BITOPS_FIXED	1

#if BITOPS_FIXED

extern "C"
{
	unsigned char _bittest(int const *a, int b);
	unsigned char _bittest64(__int64 const *a, __int64 b);


	unsigned char _bittestandcomplement(long *a, long b);
	unsigned char _bittestandcomplement64(__int64 *a, __int64 b);
};
#pragma intrinsic(_bittest)
#pragma intrinsic(_bittest64)
#pragma intrinsic(_bittestandcomplement)
#pragma intrinsic(_bittestandcomplement64)

#else
#define		_bittest(a, b)		(*(a)) & (1 << (b))
#define		_bittest64(a, b)	(*(a)) & (1 << (b))
#endif




namespace cloture
{
namespace util
{
namespace bitwise
{
	using namespace cloture::util::common;
	
	template<typename T> __pure inline bool test( const register T *u, const register __typeof__(*u) b){}
	
	template<> __pure inline bool test( const register uint32* u, const register uint32 b)
	{
		volatile register bool result __asm__("al");
		//volatile register bool carry __asm__("cfee");
		__asm
		{
			bt u, b
			setalc
		}
		return result;
	}
	
	template<> __pure inline bool test(const register int32 *u, const register int32 b)
	{

		return (*u & (1 << b)) != 0;
	}

	template<> __pure inline bool test( const register uint64 *u, const register uint64 b)
	{
		return 0;//_bittest64(&u, b);
	}
	template<> __pure inline bool test( const register int64 *u, const register int64 b)
	{
		return 0;//_bittest64(&u, b);
	}
#if BITOPS_FIXED
	template<typename T> __pure inline bool testAndComplement( const  T u, const __typeof__(*u) b){}

	template<> __pure inline bool testAndComplement(uint32* u, const uint32 b)
	{
		return _bittestandcomplement(u, b);
	}

	template<> __pure inline bool testAndComplement(int32* u, const int32 b)
	{
		return _bittestandcomplement(u, b);
	}

	template<> __pure inline bool testAndComplement(uint64* u, const uint64 b)
	{
		return _bittestandcomplement64(u, b);
	}
	template<> __pure inline bool testAndComplement(int64* u, const int64 b)
	{
		return _bittestandcomplement64(u, b);
	}
#endif
}
}
};
