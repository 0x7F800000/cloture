#pragma once

#define		__QTYPES_INCLUDED	1

#ifndef NULL
	#define NULL nullptr
#endif


// up / down
static constexpr cloture::util::common::size32 PITCH 	= 0;
//#define	PITCH	0

// left / right
//#define	YAW		1
static constexpr cloture::util::common::size32 YAW 		= 1;
// fall over
//#define	ROLL	2
static constexpr cloture::util::common::size32 ROLL 	= 2;


using prvm_vec_t	= float;
using prvm_int_t	= int;
using prvm_uint_t	= unsigned int;

using prvm_vec3_t = prvm_vec_t[3];

using vec_t = float;

using vec3_t = vec_t[3];
using vec4_t = vec_t[4];
using vec5_t = vec_t[5];
using vec6_t = vec_t[6];
using vec7_t = vec_t[7];
using vec8_t = vec_t[8];
