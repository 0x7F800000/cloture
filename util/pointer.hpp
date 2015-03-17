#pragma once


namespace cloture	{
namespace util		{
namespace pointers	{

#define		WRAPPED_PTR_INLINE	inline

/*
 * 	basic pointer-wrapping template
 *
 */
template<typename T>
class wrapped_ptr
{
	T* ptr;
public:

	constexpr WRAPPED_PTR_INLINE wrapped_ptr(T* p) : ptr(p)
	{}

	//__enableIf(is_class(T), "operator-> is provided only for class pointers.")
	constexpr WRAPPED_PTR_INLINE T* operator ->()
	{
		return ptr;
	}
	constexpr WRAPPED_PTR_INLINE const T* operator ->() const
	{
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
