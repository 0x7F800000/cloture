/*
	DarkPlaces file system

	Copyright (C) 2003-2005 Mathieu Olivier

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

	See the GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to:

		Free Software Foundation, Inc.
		59 Temple Place - Suite 330
		Boston, MA  02111-1307, USA
*/
#pragma once
#include "quake_constants.hpp"
#include <cstdarg>
// ------ Types ------ //

typedef struct qfile_s qfile_t;

#ifdef WIN32
	typedef __int64		fs_offset_t; ///< 64bit (lots of warnings, and read/write still don't take 64bit on win64)
#else
	typedef long long	fs_offset_t;
#endif



// ------ Variables ------ //

extern char fs_gamedir [MAX_OSPATH];
extern char fs_basedir [MAX_OSPATH];
extern char fs_userdir [MAX_OSPATH];

// list of active game directories (empty if not running a mod)
static constexpr size_t MAX_GAMEDIRS = 16;

extern int fs_numgamedirs;
extern char fs_gamedirs[MAX_GAMEDIRS][MAX_QPATH];
extern const char *const fs_checkgamedir_missing; // "(missing)"

struct gamedir_t
{
	char name[MAX_OSPATH];
	char description[8192];
};

extern gamedir_t *fs_all_gamedirs; // terminated by entry with empty name
extern int fs_all_gamedirs_count;

struct fssearch_t
{
	int numfilenames;
	char **filenames;
	// array of filenames
	char *filenamesbuffer;
};


// ------ Main functions ------ //

// IMPORTANT: the file path is automatically prefixed by the current game directory for
// each file created by FS_WriteFile, or opened in "write" or "append" mode by FS_OpenRealFile

bool		FS_AddPack(const char *pakfile, bool *already_loaded, bool keep_plain_dirs); // already_loaded may be NULL if caller does not care
const char *FS_WhichPack(const char *filename);
void		FS_CreatePath (char *path);
int 		FS_SysOpenFD(const char *filepath, const char *mode, bool nonblocking); // uses absolute path
qfile_t* 	FS_SysOpen (const char* filepath, const char* mode, bool nonblocking); // uses absolute path
qfile_t* 	FS_OpenRealFile (const char* filepath, const char* mode, bool quiet);
qfile_t* 	FS_OpenVirtualFile (const char* filepath, bool quiet);
qfile_t* 	FS_FileFromData (const unsigned char *data, const size_t size, bool quiet);
int 		FS_Close (qfile_t* file);
void 		FS_RemoveOnClose(qfile_t* file);
fs_offset_t FS_Write (qfile_t* file, const void* data, size_t datasize);
fs_offset_t FS_Read (qfile_t* file, void* buffer, size_t buffersize);
int 		FS_Print(qfile_t* file, const char *msg);
int 		FS_Printf(qfile_t* file, const char* format, ...) __printf_type(2);
int 		FS_VPrintf(qfile_t* file, const char* format, va_list ap);
int 		FS_Getc (qfile_t* file);
int 		FS_UnGetc (qfile_t* file, unsigned char c);
int 		FS_Seek (qfile_t* file, fs_offset_t offset, int whence);
fs_offset_t FS_Tell (qfile_t* file);
fs_offset_t FS_FileSize (qfile_t* file);
void 		FS_Purge (qfile_t* file);
const char *FS_FileWithoutPath (const char *in);
const char *FS_FileExtension (const char *in);
int 		FS_CheckNastyPath (const char *path, bool isgamedir);
const char *FS_CheckGameDir(const char *gamedir); // returns NULL if nasty, fs_checkgamedir_missing (exact pointer) if missing
bool 		FS_ChangeGameDirs(int numgamedirs, char gamedirs[][MAX_QPATH], bool complain, bool failmissing);
bool 		FS_IsRegisteredQuakePack(const char *name);
int 		FS_CRCFile(const char *filename, size_t *filesizepointer);
void 		FS_Rescan();

fssearch_t *FS_Search(const char *pattern, int caseinsensitive, int quiet);
void 		FS_FreeSearch(fssearch_t *search);
bool 		FS_WriteFileInBlocks (const char *filename, const void *const *data, const fs_offset_t *len, size_t count);
bool 		FS_WriteFile (const char *filename, const void *data, fs_offset_t len);
unsigned char*
			FS_LoadFile (const char *path, mempool_t *pool, bool quiet, fs_offset_t *filesizepointer);
// ------ Other functions ------ //
void 		FS_StripExtension (const char *in, char *out, size_t size_out);
void 		FS_DefaultExtension (char *path, const char *extension, size_t size_path);
int 		FS_FileType (const char *filename);		// the file can be into a package
int 		FS_SysFileType (const char *filename);		// only look for files outside of packages

bool 		FS_FileExists (const char *filename);		// the file can be into a package
bool 		FS_SysFileExists (const char *filename);	// only look for files outside of packages

void 		FS_mkdir (const char *path);

unsigned char *FS_Deflate(const unsigned char *data, size_t size, size_t *deflated_size, int level, mempool_t *mempool);
unsigned char *FS_Inflate(const unsigned char *data, size_t size, size_t *inflated_size, mempool_t *mempool);

bool 		FS_HasZlib();

void 		FS_Init_SelfPack();
void 		FS_Init();
void 		FS_Shutdown();
void 		FS_Init_Commands();

enum : int
{
	FS_FILETYPE_NONE,
	FS_FILETYPE_FILE,
	FS_FILETYPE_DIRECTORY
};

#define		FS_INLINE		inline

namespace cloture	{
namespace FS		{
	using uint8 = cloture::util::common::uint8;
	enum class FileMode : uint8
	{
		Read,
		Write,
		Append
	};

	struct File
	{
		using offset = fs_offset_t;

		qfile_t* f;

		template<typename T, typename... types> FS_INLINE
		File(T ff);

		//FS_INLINE File(qfile_t* ff) : f(ff)
		//{}



		FS_INLINE int close()
		{
			return FS_Close(f);
		}

		template<typename T>
		FS_INLINE offset write(const T* data, const size_t n)
		{
			return FS_Write(f,
				reinterpret_cast<void*>(data),
				n * sizeof(T)
			);
		}

		template<typename T>
		FS_INLINE offset read(T* buffer, const size_t n)
		{
			return FS_Read(
				f,
				reinterpret_cast<void*>(buffer),
				n * sizeof(T)
			);
		}

		FS_INLINE int print(const char* msg)
		{
			return FS_Print(f, msg);
		}
		FS_INLINE int printf(const char* format, ...) __printf_type(2)
		{
			va_list args;

			va_start(args, format);
				const int result = FS_VPrintf(f, format, args);
			va_end(args);

			return result;
		}

		FS_INLINE int vprintf(const char* format, va_list args)
		{
			return FS_VPrintf(f, format, args);
		}

		FS_INLINE int getc()
		{
			return FS_Getc(f);
		}

		FS_INLINE int unGetc(const uint8 c)
		{
			return FS_UnGetc(f, c);
		}

		FS_INLINE offset tell()
		{
			return FS_Tell(f);
		}

		FS_INLINE int seek(const offset pos, const int whence)
		{
			return FS_Seek(f, pos, whence);
		}

		FS_INLINE offset fileSize()
		{
			return FS_FileSize(f);
		}

		FS_INLINE void purge()
		{
			FS_Purge(f);
		}

		FS_INLINE operator qfile_t*()
		{
			return f;
		}
	};//struct File

	template<typename T> FS_INLINE
	static T* loadFile(
		const char 		*path,
		mempool_t 		*pool,
		bool 			quiet,
		fs_offset_t 	*filesizepointer
	)
	{
		return reinterpret_cast<T*>(FS_LoadFile(path, pool, quiet, filesizepointer));
	}
}//namespace FS
}//namespace cloture
