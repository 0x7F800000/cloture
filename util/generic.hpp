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
	integer_type_class, //both int and long long int become this. probably short as well
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

/**
 * added these for c++
 */
#define		type_is_method(type)					( type_is_typeclass(type, method_type_class) )
#define		type_is_reference(type)					( type_is_typeclass(type, reference_type_class) )


#define 	type_is_pointer(type)					( type_is_typeclass(type, pointer_type_class) )
#define 	type_is_array(type)						( type_is_typeclass(type, array_type_class) )

#define 	object_size(object)		__builtin_object_size(object, 0)


/*
	size-1 for null character
*/
#define 	argNameLength(arg)				(static_cast<ptrdiff_t>(sizeof(""#arg"") - 1))

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

template<typename T>
struct typeHolder
{
	using type = T;
	char y;
	constexpr typeHolder() : y(0) {}
};


template<int typeclass_, typename T>
static constexpr bool isTypeclass()
{
	return classify_type(*static_cast<T*>(nullptr)) == typeclass_;
}

template<int typeclass_, typename T>
static constexpr bool isTypeclass(T f)
{
	return classify_type(f) == typeclass_;
}

#define		__typeClassChecker(name, typeclass)	\
template<typename T>							\
static constexpr bool name ()					\
{	\
	return isTypeclass<typeclass, T>();		\
}	\
template<typename T>	\
static constexpr bool name (T f)	\
{				\
	return isTypeclass<typeclass>(f);	\
}

__typeClassChecker(isVoid, void_type_class)
__typeClassChecker(isInteger, integer_type_class)
__typeClassChecker(isChar, char_type_class)
__typeClassChecker(isEnumeral, enumeral_type_class)

__typeClassChecker(isBoolean, boolean_type_class)
__typeClassChecker(isFunction, function_type_class)
__typeClassChecker(isMethod, method_type_class)
__typeClassChecker(isReference, reference_type_class)
__typeClassChecker(isPointer, pointer_type_class)
__typeClassChecker(isArray, array_type_class)
__typeClassChecker(isRealNumber, real_type_class)
__typeClassChecker(isComplexNumber, complex_type_class)

static_assert(isRealNumber(7.0f) && isRealNumber(7.0) && isRealNumber(7.0L));

static_assert(isComplexNumber((_Complex float) {1.0f, 2.0f} ));

static constexpr int ___GENERIC_TEST___ = 0;
static_assert(isInteger<int>() && isInteger(___GENERIC_TEST___) && isInteger<long long>() );


template<typename T>
static constexpr bool isIntegral()
{
	return isInteger<T>() || isChar<T>() || isEnumeral<T>() || isBoolean<T>();
}

template<typename T>
static constexpr bool isIntegral(T f)
{
	return isIntegral<T>();
}


template<typename T>
static constexpr bool isSigned()
{
	return static_cast<T>(-1) < static_cast<T>(0);
}

template<typename T>
static constexpr bool isSigned(T f)
{
	return isSigned<T>();
}

template<typename T>
static constexpr bool isUnsigned()
{
	return !isSigned<T>();
}

template<typename T>
static constexpr bool isUnsigned(T f)
{
	return isUnsigned<T>();
}

}//namespace generic
}//namespace util
}//namespace cloture

#define 	make_unsigned(type)		typename __make_unsigned__<type>::___type___
#define 	make_signed(type)		typename __make_signed__<type>::___type___

//probably compiles faster than templates
#define 	is_integral(type)		\
(type_is_integer(type) || type_is_char(type) || type_is_enumeral(type) || type_is_boolean(type) )

#define		__import_make_unsigned()	using cloture::util::generic::__make_unsigned__
#define		__import_make_signed()		using cloture::util::generic::__make_signed__

/*
	not sure if this one works with clang.
	works with GCC though.
	
	not very useful. 
*/
#define 	__coerce_constexpr(expr)	(__builtin_constant_p(expr) ? expr : expr)



constexpr int* coerceConstexprTest = __coerce_constexpr((int*)0xFF);


#ifdef __clang__
	#define __enableIf(condition, msg)	__attribute__((enable_if(condition, msg)))
#endif
