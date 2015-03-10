#pragma once

/**
 	 Cloture STL headers.
*/

/*
	common compiler extensions
	mostly wraps compiler attributes like __declspec and __attribute__
*/
#include	"compiler_features.hpp"

/*
	basic types	(uint8, int8, int32, int64, real32, real64, etc)
*/
#include	"util/common.hpp"

/*
	wrappers for compiler type-checking extensions
	provides macros for type checking. minimal use of templates.
	most compiler-specific extensions used can be easily replaced with templates
	if needed
*/
#include	"util/generic.hpp"

/*
	templates/macros for implementing custom type traits that can be used
	for basic compile-time reflection.
	Verry basic reflection.
*/

#include	"util/reflect.hpp"

/*
	CTFE stuff for compile-time tricks
	unfortunately very limited right now due to lack of support for c++14
	constexpr
*/
#include	"util/ctfe.hpp"

/*
	generic integer bitwise operations
	these are wrapped because some architectures provide their own
	bitop instructions.
	such as x86 its powerful bittest instructions and test-and-complement instructions
*/
#include	"util/bitops.hpp"

/*	miscellaneous floating point tricks	*/
#include	"util/fpOps.hpp"

#ifndef __QTYPES_INCLUDED
using vec_t = float;
#endif

/*	wrapper for architecture's simd operations	*/
#include	"util/vector.hpp"

/*	cmath replacement. mostly inline wrappers around __builtin versions	*/
#include	"util/math.hpp"
