#pragma once

namespace cloture::util::routines
{
	namespace general
	{

		template<typename T>
		__forceinline void zeroMemory(T* mem, const common::size32 elements)
		__enableIf(
			alignof(T) % 16  == 0 && sizeof(T) % 16 == 0 && (sizeof(T) * elements) / 16 <= 16, 
			"Enabled if T is SSE-memsetable and unrollable"
		)
		{
			__m128i val = {0, 0};
			__unrollFull
			for(unsigned i = 0; i < (sizeof(T) * elements) / 16; ++i)
				reinterpret_cast<__m128i*>(mem)[i] = val;
		}
		
		template<typename T>
		__forceinline void zeroMemory(T* mem, const common::size32 elements)
		__enableIf(
			alignof(T) % 16 == 0 && sizeof(T) % 16 == 0, 
			"Enabled if T is SSE-memsetable and not unrollable"
		)
		{
			__m128i val = {0, 0};
			for(unsigned i = 0; i < (sizeof(T) * elements) / 16; ++i)
				reinterpret_cast<__m128i*>(mem)[i] = val;
		}
			
		template<typename T>
		__forceinline void zeroMemory(T* mem, const common::size32 elements)
		{
			__builtin_memset(mem, 0, elements * sizeof(T));
		}	
	}
}
