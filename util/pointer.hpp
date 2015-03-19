#pragma once


namespace cloture	{
namespace util		{
namespace pointers	{

#define		WRAPPED_PTR_INLINE	inline

/*
 * 	basic pointer-wrapping template
 *
 */
template<typename T, __printf_type(1) __noreturn void (*onError)(const char*, ...) = nullptr>
class wrapped_ptr
{
	T* ptr;
public:

	template<__printf_type(1) __noreturn void (*onError_)(const char*, ...) = nullptr>
	__noinline __cold
	static void PtrNullError()
	{
		onError_("INTERNAL ERROR: Attempted to dereference NULL pointer in wrapped_ptr.");
	}

	template<> __noinline __cold static void PtrNullError<nullptr>(){}

	constexpr WRAPPED_PTR_INLINE wrapped_ptr(T* p) : ptr(p)
	{}

	constexpr WRAPPED_PTR_INLINE T* operator ->()
	{
		#if mCheckPointerNull
			if(unlikely(ptr == nullptr))
				PtrNullError<onError>();
		#endif
		return ptr;
	}
	constexpr WRAPPED_PTR_INLINE const T* operator ->() const
	{
		#if mCheckPointerNull
			if(unlikely(ptr == nullptr))
				PtrNullError<onError>();
		#endif
		return ptr;
	}

	__pseudopure constexpr WRAPPED_PTR_INLINE bool operator ==(const T* const other) const
	{
		return ptr == other;
	}

	__pseudopure constexpr WRAPPED_PTR_INLINE bool operator !=(const T* const other) const
	{
		return ptr != other;
	}


	__pseudopure constexpr WRAPPED_PTR_INLINE explicit operator bool() const
	{
		return ptr != nullptr;
	}

	__pseudopure constexpr WRAPPED_PTR_INLINE bool operator !() const
	{
		return ptr == nullptr;
	}

	__pseudopure constexpr WRAPPED_PTR_INLINE explicit operator T*()
	{
		return ptr;
	}

	__pseudopure constexpr WRAPPED_PTR_INLINE T* getPtr()
	{
		return ptr;
	}

};//class wrapped_ptr

} //namespace pointers
} //namespace util
} //namespace cloture
