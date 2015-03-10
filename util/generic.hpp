#pragma once

#define 	STRINGIFY(x)				#x
#define 	doPragma(x)					_Pragma(STRINGIFY(x))
#define		pushMacro(macroName)		doPragma(push_macro(STRINGIFY(macroName)))
#define		popMacro(macroName)			doPragma(pop_macro(STRINGIFY(macroName)))
#define		__dbgFunctionMacro__(m)		m
#define 	dbgFunctionMacro(m)			static_assert(0, STRINGIFY(__dbgFunctionMacro__(m)))

#define 	mGetFirstVarArg(arg, ...)			arg
#define 	mGetSecondVarArg(unused, arg, ...)	arg
#define 	mShiftVarArgs(unused, ...)			__VA_ARGS__


#define 	dummy_type_ptr(type)		static_cast<type*>(nullptr)

#define 	classify_type(object)		__builtin_classify_type(object)


#define has_nothrow_assign(type)		__has_nothrow_assign(type)
#define has_nothrow_copy(type)			__has_nothrow_copy(type)
#define has_nothrow_constructor(type)	__has_nothrow_constructor(type)
#define	has_trivial_assign(type)		__has_trivial_assign(type)
#define has_trivial_copy(type)			__has_trivial_copy(type)
#define has_trivial_constructor(type)	__has_trivial_constructor(type)
#define has_trivial_destructor(type)	__has_trivial_destructor(type)
#define has_virtual_destructor(type)	__has_virtual_destructor(type)
#define is_abstract(type)				__is_abstract(type)
#define is_base_of(base, derived)		__is_base_of(base, derived)
#define is_class(type)					__is_class(type)
#define is_empty(type)					__is_empty(type)
#define is_enum(type)					__is_enum(type)
#define is_literal_type(type)			__is_literal_type(type)
#define is_pod(type)					__is_pod(type)
#define is_polymorphic(type)			__is_polymorphic(type)
#define is_standard_layout(type)		__is_standard_layout(type)
#define is_trivial(type)				__is_trivial(type)
#define is_union(type)					__is_union(type)
#define underlying_type(type)			__underlying_type(type)

#define	is_trivially_constructible(type, ...)		__is_trivially_constructible(type, ##__VA_ARGS__)
#define is_trivially_assignable(totype, fromtype)	__is_trivially_assignable(totype, fromtype)

/**
	clang-supported msvc extension
*/
#define	is_convertible_to(from, to)	__is_convertible_to(from, to)


#define 	macroError(msg)		doPragma( GCC error STRINGIFY(msg) )

/*
	return values for classify_type
*/
enum
{
	no_type_class = -1,
	void_type_class,
	integer_type_class,
	char_type_class,
	enumeral_type_class,
	boolean_type_class,
	pointer_type_class,
	reference_type_class,
	offset_type_class,
	real_type_class,
	complex_type_class,
	function_type_class,
	method_type_class,
	record_type_class,
	union_type_class,
	array_type_class,
	string_type_class,
	file_type_class,
	lang_type_class
};

#define 	type_is_typeclass(type, typeclass)		(classify_type(*dummy_type_ptr(type)) == typeclass)

#define 	object_is_pointer(object)				(classify_type(object) == pointer_type_class)


#define 	type_is_void(type)						( type_is_typeclass(type, void_type_class) )
#define 	type_is_integer(type)					( type_is_typeclass(type, integer_type_class) )
#define 	type_is_char(type)						( type_is_typeclass(type, char_type_class) )
#define 	type_is_enumeral(type)					( type_is_typeclass(type, enumeral_type_class) )
#define 	type_is_boolean(type)					( type_is_typeclass(type, boolean_type_class) )
#define 	type_is_function(type)					( type_is_typeclass(type, function_type_class) )

#define 	type_is_pointer(type)					( type_is_typeclass(type, pointer_type_class) )
#define 	type_is_array(type)						( type_is_typeclass(type, array_type_class) )

#define 	deref_sizeof(pointer)					(sizeof(__typeof__(*pointer)))

#define		assert_object_is_pointer(object, msg)	({static_assert(object_is_pointer(object), msg)})


#define 	assignable(dest, src)	\
	types_compatible( __typeof__(dest), __typeof__(src) )

#define 	variable_is_type(variable, type)	\
	types_compatible(__typeof__(variable), type)


#define 	type_is_signed(T)			(-static_cast<T>(1) < static_cast<T>(0))

#define 	type_is_unsigned(T)				(!type_is_signed(T))

#define 	elementTypeof(array)		__typeof__(*array)
#define		elementSizeof(array)		sizeof( elementTypeof(array) )
#define 	objectSizeof(object)		__builtin_object_size(object, 0)

/*
	if object's type is object*:
		return the type of object*

	else if object's type is object:
		return the type of &object

	This is only used for first-level references
*/

#define get_structure_pointer_type(object)			\
	__typeof__(object_is_pointer(object) ? object : &object)

#define type_remove_pointer(type)	\
	__typeof__(type_is_pointer(type) ? **dummy_type_ptr(type) : *dummy_type_ptr(type))

#define remove_pointer(type)		type_remove_pointer(type)

static_assert(type_is_pointer(void*), "ruh roh");


/*
	size-1 for null character
*/
#define 	argNameLength(arg)				(static_cast<ptrdiff_t>(sizeof(""#arg"") - 1))
#define 	arrayDiff(elementptr, array)	(reinterpret_cast<const size_t>(elementptr) - reinterpret_cast<const size_t>(&array[0]))

namespace cloture
{
namespace util
{
namespace generic
{
using namespace cloture::util::common;
template<typename T> struct __make_unsigned__
{
	typedef T ___type___;
};

template<> struct __make_unsigned__<int8>
{
	typedef uint8 ___type___;
};

template<> struct __make_unsigned__<int16>
{
	typedef uint16 ___type___;
};

template<> struct __make_unsigned__<int32>
{
	typedef uint32 ___type___;
};

template<> struct __make_unsigned__<int64>
{
	typedef uint64 ___type___;
};

template<typename T> struct __make_signed__
{
	typedef T ___type___;
};

template<> struct __make_signed__<uint8>
{
	typedef int8 ___type___;
};

template<> struct __make_signed__<uint16>
{
	typedef int16 ___type___;
};

template<> struct __make_signed__<uint32>
{
	typedef int32 ___type___;
};

template<> struct __make_signed__<uint64>
{
	typedef int64 ___type___;
};

template<typename T = void> struct __is_integral__
{
	static constexpr bool value = false;
};
template<> struct __is_integral__<int64>
{
	static constexpr bool value = true;
};
template<> struct __is_integral__<uint64>
{
	static constexpr bool value = true;
};

template<typename T> struct __is_float32__
{
	static constexpr bool value = false;
};

template<> struct __is_float32__<float>
{
	static constexpr bool value = true;
};

template<typename T> struct __is_float64__
{
	static constexpr bool value = false;
};

template<> struct __is_float64__<double>
{
	static constexpr bool value = true;
};
}//namespace generic
}//namespace util
}//namespace cloture
#define		type_is_float32(type)		(__is_float32__<type>::value)
#define		type_is_float64(type)		(__is_float64__<type>::value)

#define		object_is_float32(object)	(type_is_float32(__typeof__(object)))
#define		object_is_float64(object)	(type_is_float64(__typeof__(object)))


#define 	make_unsigned(type)		typename __make_unsigned__<type>::___type___
#define 	make_signed(type)		typename __make_signed__<type>::___type___

//probably compiles faster than templates
#define 	is_integral(type)		\
(type_is_integer(type) || type_is_char(type) || type_is_enumeral(type) || type_is_boolean(type) || __is_integral__<type>::value)

/*
	not sure if this one works with clang.
	works with GCC though.
	
	not very useful. 
*/
#define 	__coerce_constexpr(expr)	(__builtin_constant_p(expr) ? expr : expr)

constexpr int* coerceConstexprTest = __coerce_constexpr((int*)0xFF);
