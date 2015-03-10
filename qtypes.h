#pragma once

#define		__QTYPES_INCLUDED	1

[[deprecated]] typedef bool qboolean;//using qboolean = bool;
#ifndef NULL
	#define NULL nullptr
#endif

#ifndef FALSE
	#define FALSE false
	#define TRUE true
#endif

// up / down
#define	PITCH	0

// left / right
#define	YAW		1

// fall over
#define	ROLL	2


// LordHavoc: upgrade the prvm to double precision for better time values
// LordHavoc: to be enabled when bugs are worked out...
//#define PRVM_64
#ifdef PRVM_64
	using prvm_vec_t 	= double;
	using prvm_int_t 	= long long;
	using prvm_uint_t 	= unsigned long long;
#else
	using prvm_vec_t	= float;
	using prvm_int_t	= int;
	using prvm_uint_t	= unsigned int;
#endif
typedef prvm_vec_t prvm_vec3_t[3];

#ifdef VEC_64
	using vec_t = double;
#else
	using vec_t = float;
#endif

typedef vec_t vec2_t[2];
typedef vec_t vec3_t[3];
typedef vec_t vec4_t[4];
typedef vec_t vec5_t[5];
typedef vec_t vec6_t[6];
typedef vec_t vec7_t[7];
typedef vec_t vec8_t[8];
