/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public Licensem64
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// quakedef.h -- primary header for client

#pragma once

#define		disableProfiling	1

#define USE_QUAKEC
#ifdef __APPLE__
# include <TargetConditionals.h>
#endif



#include "stddef.h"
#if defined(__INTEL_COMPILER)
#undef _MSVC_VER
#undef _MSC_VER
#endif
#include <sys/types.h>
#include <ctype.h>
#include <math.h>
#include <string.h>
#define strrchr(s, i)	__builtin_strrchr(s, i)
#define strchr(s, i)	__builtin_strchr(s, i)
#define memchr(v, i, u)	__builtin_memchr(v, i, u)
#define strstr(s, s1)	__builtin_strstr(s, s1)
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <setjmp.h>

#include "qtypes.h"

/*	cloture util headers	*/
#include "util/stl.hpp"

extern const char *buildstring;
extern char engineversion[128];
#include "quake_constants.hpp"

#include "zone.h"
#include "fs.h"
#include "cloture_common.h"
#include "cvar.h"
#include "bspfile.h"
#include "sys.h"
#include "vid.h"
#include "mathlib.h"

#include "r_textures.h"

#include "crypto.h"
#include "draw.h"
#include "screen.h"
#include "netconn.h"
#include "protocol.h"
#include "cmd.h"
#include "sbar.h"
#include "sound.h"
#include "model_shared.h"
#include "world.hpp"
#include "client.h"
#include "render.h"
#include "progs.h"
#include "progsvm.h"
#include "server.h"

#include "input.h"
#include "keys.h"
#include "console.h"
//#ifdef CONFIG_MENU
#include "menu.h"
//#endif
#include "csprogs.h"

extern bool noclip_anglehack;

extern cvar_t developer;
extern cvar_t developer_extra;
extern cvar_t developer_insane;
extern cvar_t developer_loadfile;
extern cvar_t developer_loading;

extern cvar_t sessionid;

#define STARTCONFIGFILENAME "quake.rc"
#define CONFIGFILENAME "config.cfg"

/* Preprocessor macros to identify platform
    DP_OS_NAME 	- "friendly" name of the OS, for humans to read
    DP_OS_STR	- "identifier" of the OS, more suited for code to use
    DP_ARCH_STR	- "identifier" of the processor architecture
 */
#if defined(__ANDROID__) /* must come first because it also defines linux */
# define DP_OS_NAME		"Android"
# define DP_OS_STR		"android"
# define USE_GLES2		1
# define LINK_TO_ZLIB	1
# define LINK_TO_LIBVORBIS 1
# define DP_MOBILETOUCH	1
# define DP_FREETYPE_STATIC 1
#elif TARGET_OS_IPHONE /* must come first because it also defines MACOSX */
# define DP_OS_NAME		"iPhoneOS"
# define DP_OS_STR		"iphoneos"
# define USE_GLES2		1
# define LINK_TO_ZLIB	1
# define LINK_TO_LIBVORBIS 1
# define DP_MOBILETOUCH	1
# define DP_FREETYPE_STATIC 1
#elif defined(__linux__)
# define DP_OS_NAME		"Linux"
# define DP_OS_STR		"linux"
#elif defined(_WIN64)
# define DP_OS_NAME		"Windows64"
# define DP_OS_STR		"win64"
#elif defined(WIN32)
# define DP_OS_NAME		"Windows"
# define DP_OS_STR		"win32"
#elif defined(__FreeBSD__)
# define DP_OS_NAME		"FreeBSD"
# define DP_OS_STR		"freebsd"
#elif defined(__NetBSD__)
# define DP_OS_NAME		"NetBSD"
# define DP_OS_STR		"netbsd"
#elif defined(__OpenBSD__)
# define DP_OS_NAME		"OpenBSD"
# define DP_OS_STR		"openbsd"
#elif defined(MACOSX)
# define DP_OS_NAME		"Mac OS X"
# define DP_OS_STR		"osx"
#elif defined(__MORPHOS__)
# define DP_OS_NAME		"MorphOS"
# define DP_OS_STR		"morphos"
#else
# define DP_OS_NAME		"Unknown"
# define DP_OS_STR		"unknown"
#endif

#if defined(__GNUC__)
# if defined(__i386__)
#  define DP_ARCH_STR		"686"
#  define SSE_POSSIBLE
#  ifdef __SSE__
#   define SSE_PRESENT
#  endif
#  ifdef __SSE2__
#   define SSE2_PRESENT
#  endif
# elif defined(__x86_64__)
#  define DP_ARCH_STR		"x86_64"
#  define SSE_PRESENT
#  define SSE2_PRESENT
# elif defined(__powerpc__)
#  define DP_ARCH_STR		"ppc"
# endif
#elif defined(_WIN64)
# define DP_ARCH_STR		"x86_64"
# define SSE_PRESENT
# define SSE2_PRESENT
#elif defined(WIN32)
# define DP_ARCH_STR		"x86"
# define SSE_POSSIBLE
#endif

#ifdef SSE_PRESENT
# define SSE_POSSIBLE
#endif

#ifdef NO_SSE
# undef SSE_PRESENT
# undef SSE_POSSIBLE
# undef SSE2_PRESENT
#endif

#ifdef SSE_POSSIBLE
// runtime detection of SSE/SSE2 capabilities for x86
bool Sys_HaveSSE();
bool Sys_HaveSSE2();
#else
#define Sys_HaveSSE() false
#define Sys_HaveSSE2() false
#endif

#include "glquake.h"

#include "palette.h"

/// incremented every frame, never reset
extern int host_framecount;
/// not bounded in any way, changed at start of every frame, never reset
extern double realtime;
/// equal to Sys_DirtyTime() at the start of this host frame
extern double host_dirtytime;

void Host_InitCommands();
void Host_Main();
void Host_Shutdown();
void Host_StartVideo();
void Host_Error(const char *error, ...) DP_FUNC_PRINTF(1) DP_FUNC_NORETURN;
void Host_Quit_f();
void Host_ClientCommands(const char *fmt, ...) DP_FUNC_PRINTF(1);
void Host_ShutdownServer();
void Host_Reconnect_f();
void Host_NoOperation_f();
void Host_LockSession();
void Host_UnlockSession();

void Host_AbortCurrentFrame();

/// skill level for currently loaded level (in case the user changes the cvar while the level is running, this reflects the level actually in use)
extern int current_skill;

//
// chase
//
extern cvar_t chase_active;
extern cvar_t cl_viewmodel_scale;

void Chase_Init();
void Chase_Reset();
void Chase_Update();

void fractalnoise(unsigned char *noise, int size, int startgrid);
void fractalnoisequick(unsigned char *noise, int size, int startgrid);
float noise4f(float x, float y, float z, float w);

void Sys_Shared_Init();

// Flag in size field of demos to indicate a client->server packet. Demo
// playback will ignore this, but it may be useful to make DP sniff packets to
// debug protocol exploits.
#define DEMOMSG_CLIENT_TO_SERVER 0x80000000

// In Quake, any char in 0..32 counts as whitespace
//#define ISWHITESPACE(ch) ((unsigned char) ch <= (unsigned char) ' ')
#define ISWHITESPACE(ch) (!(ch) || (ch) == ' ' || (ch) == '\t' || (ch) == '\r' || (ch) == '\n')

// This also includes extended characters, and ALL control chars
#define ISWHITESPACEORCONTROL(ch) ((signed char) (ch) <= (signed char) ' ')


#ifdef PRVM_64
#define FLOAT_IS_TRUE_FOR_INT(x) ((x) & 0x7FFFFFFFFFFFFFFF) // also match "negative zero" doubles of value 0x8000000000000000
#define FLOAT_LOSSLESS_FORMAT "%.17g"
#define VECTOR_LOSSLESS_FORMAT "%.17g %.17g %.17g"
#else
#define FLOAT_IS_TRUE_FOR_INT(x) ((x) & 0x7FFFFFFF) // also match "negative zero" floats of value 0x80000000
#define FLOAT_LOSSLESS_FORMAT "%.9g"
#define VECTOR_LOSSLESS_FORMAT "%.9g %.9g %.9g"
#endif

// originally this was _MSC_VER
// but here we want to test the system libc, which on win32 is borked, and NOT the compiler
#ifdef WIN32
#define INT_LOSSLESS_FORMAT_SIZE "I64"
#define INT_LOSSLESS_FORMAT_CONVERT_S(x) ((__int64)(x))
#define INT_LOSSLESS_FORMAT_CONVERT_U(x) ((unsigned __int64)(x))
#else
#define INT_LOSSLESS_FORMAT_SIZE "j"
#define INT_LOSSLESS_FORMAT_CONVERT_S(x) ((intmax_t)(x))
#define INT_LOSSLESS_FORMAT_CONVERT_U(x) ((uintmax_t)(x))
#endif



#ifdef __cplusplus
	#ifdef NULL
		#undef NULL
	#endif
	#define		NULL nullptr
#endif
