#pragma once

#ifdef __clang__
	extern char *gets (char *__s) __wur __attribute_deprecated__;
#endif
#include <stdexcept>


__if_not_exists(ptrdiff_t)
{
	//needed for some files that don't include quakedef.h
	#include "stddef.h"
}

/**
 	 Cloture STL headers.
*/

#if defined(__clang__)
	#define	hasExtendedConstexpr 1
#else
	#define hasExtendedConstexpr 0
#endif

/*
	common compiler extensions
	mostly wraps compiler attributes like __declspec and __attribute__
*/
#include	"../compiler_features.hpp"

/*
	basic types	(uint8, int8, int32, int64, real32, real64, etc)
*/
#include	"basic_types.hpp"

/*
	wrappers for compiler type-checking extensions
	provides macros for type checking. minimal use of templates.
	most compiler-specific extensions used can be easily replaced with templates
	if needed
*/
#include	"generic.hpp"

#include	"va_template.hpp"

/*
	common routines
*/
#include	"common.hpp"


/*
	CTFE stuff for compile-time tricks
*/
#include	"ctfe.hpp"

#include 	"ctfe_math.hpp"
#include	"extended_types.hpp"

/*
	templates/macros for implementing custom type traits that can be used
	for basic compile-time reflection.
	Verry basic reflection.
*/

#include	"reflect.hpp"


/*
	generic integer bitwise operations
	these are wrapped because some architectures provide their own
	bitop instructions.
	such as x86 its powerful bittest instructions and test-and-complement instructions
*/
#include	"bitops.hpp"

/*	miscellaneous floating point tricks	*/
#include	"fpOps.hpp"

#ifndef __QTYPES_INCLUDED
using vec_t = float;
#endif

/*	wrapper for architecture's simd operations	*/
#include	"vector.hpp"

/*	cmath replacement. mostly inline wrappers around __builtin versions	*/
#include	"math.hpp"

/*
 * input-output streams
 */
#include	"io.hpp"

#include 	"endian.hpp"
/*
 * simple wrapper around a c-style string
 */
#include	"CString.hpp"

#include	"pointer.hpp"
