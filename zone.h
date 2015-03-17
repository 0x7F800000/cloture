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

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#pragma once

#if defined(__i386__) || defined(__x86_64__)

	#if		defined(__AVX__) && !defined(__AVX512F__)
		#define		SIMD_ALIGN	32

	#elif	!defined(__AVX__)
		#define		SIMD_ALIGN	16

	#elif	defined(__AVX512F__)
		#define		SIMD_ALIGN	64
	#endif

#elif defined(__ARM_ARCH) && defined(__ARM_NEON__)
	#define SIMD_ALIGN	16
#else
	#define SIMD_ALIGN	16
#endif

#define		mDebugMemory	0

static constexpr bool mem_bigendian = false;


// div0: heap overflow detection paranoia
#define MEMPARANOIA 0

constexpr size_t POOLNAMESIZE = 128;

// if set this pool will be printed in memlist reports
#define POOLFLAG_TEMP 1


namespace cloture::engine
{
namespace cvars
{
	struct CVar;
}
namespace memory
{
	using allocsize_t	=	cloture::util::common::size32;
	static constexpr size_t poolAllocAlign	=	SIMD_ALIGN;
	struct Header
	{
		// address returned by Chunk_Alloc (may be significantly before this header to satisify alignment)
		void *baseaddress;
		// next and previous memheaders in chain belonging to pool
		struct Header *next;
		struct Header *prev;
		// pool this memheader belongs to
		struct Pool *pool;
		// size of the memory after the header (excluding header and sentinel2)
		allocsize_t size;
#if mDebugMemory
		// file name and line where Mem_Alloc was called
		const char *filename;
		int fileline;
#endif
		// should always be equal to MEMHEADER_SENTINEL_FOR_ADDRESS()
		unsigned int sentinel;
		// immediately followed by data, which is followed by another copy of mem_sentinel[]
	};

	struct Pool
	{
		// should always be MEMPOOL_SENTINEL
		unsigned int sentinel1;
		// chain of individual memory allocations
		struct Header *chain;
		// POOLFLAG_*
		int flags;
		// total memory allocated in this pool (inside memheaders)
		allocsize_t totalsize;
		// total memory allocated in this pool (actual malloc total)
		allocsize_t realsize;
		// updated each time the pool is displayed by memlist, shows change from previous time (unless pool was freed)
		allocsize_t lastchecksize;
		// linked into global mempool list
		struct Pool *next;
		// parent object (used for nested memory pools)
		struct Pool *parent;
#if mDebugMemory
		// file name and line where Mem_AllocPool was called
		const char *filename;
		int fileline;
#endif
		// name of the pool
		char name[POOLNAMESIZE];
		// should always be MEMPOOL_SENTINEL
		unsigned int sentinel2;
		template<typename T> inline T* alloc(const allocsize_t n);
		template<typename T> inline T* realloc(T* data, const allocsize_t n);
	};
}
}

#define	cvar_s	cloture::engine::cvars::CVar

using memheader_s = cloture::engine::memory::Header;
using memheader_t = memheader_s;
using mempool_s = cloture::engine::memory::Pool;
using mempool_t = mempool_s;

#if mDebugMemory
	#define Mem_Alloc(pool,size) 				_Mem_Alloc(pool, NULL, size, __FILE__, __LINE__)
	#define Mem_Realloc(pool,data,size) 		_Mem_Alloc(pool, data, size, __FILE__, __LINE__)
	#define Mem_Free(mem) 						_Mem_Free(mem, __FILE__, __LINE__)
	#define Mem_CheckSentinels(data)			_Mem_CheckSentinels(data, __FILE__, __LINE__)
	#define Mem_CheckSentinelsGlobal() 			_Mem_CheckSentinelsGlobal(__FILE__, __LINE__)
	#define Mem_AllocPool(name, flags, parent) 	_Mem_AllocPool(name, flags, parent, __FILE__, __LINE__)
	#define Mem_FreePool(pool) 					_Mem_FreePool(pool, __FILE__, __LINE__)
	#define Mem_EmptyPool(pool) 				_Mem_EmptyPool(pool, __FILE__, __LINE__)
#else
	#define Mem_Alloc(pool,size) 				_Mem_Alloc(pool, NULL, size)
	#define Mem_Realloc(pool,data,size) 		_Mem_Alloc(pool, data, size)
	#define Mem_Free(mem) 						_Mem_Free(mem)
	#define Mem_CheckSentinels(data) 			_Mem_CheckSentinels(data)
	#define Mem_CheckSentinelsGlobal() 			_Mem_CheckSentinelsGlobal()
	#define Mem_AllocPool(name, flags, parent) 	_Mem_AllocPool(name, flags, parent)
	#define Mem_FreePool(pool) 					_Mem_FreePool(pool)
	#define Mem_EmptyPool(pool) 				_Mem_EmptyPool(pool)
#endif

__returns_aligned(cloture::engine::memory::poolAllocAlign) __returns_nonull __returns_unaliased
void *_Mem_Alloc(
	mempool_t 								*pool,
	void 									*data,
	cloture::engine::memory::allocsize_t 	size
#if mDebugMemory
	,
	const char 								*filename,
	int 									fileline
#endif
);

void _Mem_Free(
	void 			*data
#if mDebugMemory
	,
	const char 		*filename,
	int 			fileline
#endif
);

__returns_unaliased
mempool_t *_Mem_AllocPool(
	const char 		*name,
	int 			flags,
	mempool_t 		*parent
#if mDebugMemory
	,
	const char 		*filename,
	int 			fileline
#endif
);

void _Mem_FreePool(
	mempool_t **pool
#if mDebugMemory
	,
	const char 		*filename,
	int 			fileline
#endif
);
void _Mem_EmptyPool(
	mempool_t *pool
#if mDebugMemory
	,
	const char 		*filename,
	int 			fileline
#endif
);
void _Mem_CheckSentinels(
	void *data
#if mDebugMemory
	,
	const char 		*filename,
	int 			fileline
#endif
);
void _Mem_CheckSentinelsGlobal(
#if mDebugMemory
	const char 		*filename,
	int 			fileline
#endif
);
// if pool is NULL this searches ALL pools for the allocation
bool Mem_IsAllocated(mempool_t *pool, void *data);

char* Mem_strdup (mempool_t *pool, const char* s);

typedef struct memexpandablearray_array_s
{
	unsigned char *data;
	unsigned char *allocflags;
	size_t numflaggedrecords;
}
memexpandablearray_array_t;

typedef struct memexpandablearray_s
{
	mempool_t *mempool;
	size_t recordsize;
	size_t numrecordsperarray;
	size_t numarrays;
	size_t maxarrays;
	memexpandablearray_array_t *arrays;
}
memexpandablearray_t;

void Mem_ExpandableArray_NewArray(memexpandablearray_t *l, mempool_t *mempool, size_t recordsize, int numrecordsperarray);
void Mem_ExpandableArray_FreeArray(memexpandablearray_t *l);
void *Mem_ExpandableArray_AllocRecord(memexpandablearray_t *l);
void Mem_ExpandableArray_FreeRecord(memexpandablearray_t *l, void *record);
size_t Mem_ExpandableArray_IndexRange(const memexpandablearray_t *l) DP_FUNC_PURE;
void *Mem_ExpandableArray_RecordAtIndex(const memexpandablearray_t *l, size_t index) DP_FUNC_PURE;

// used for temporary allocations
extern mempool_t *tempmempool;

void Memory_Init ();
void Memory_Shutdown ();
#if mDebugMemory
	void Memory_Init_Commands ();
#else
	#define Memory_Init_Commands()	static_cast<void>(0)
#endif
extern mempool_t *zonemempool;
#define Z_Malloc(size) 		Mem_Alloc(zonemempool,size)
#define Z_Free(data) 		Mem_Free(data)

#if mDebugMemory
	extern struct cvar_s developer_memory;
	extern struct cvar_s developer_memorydebug;
#endif

namespace cloture::engine
{


namespace memory
{
	
	template<typename T> inline void dealloc(T* mem)
	{
		Mem_Free(reinterpret_cast<void*>(mem));
	}

	template<typename T>
	__returns_aligned(poolAllocAlign) __returns_nonull __returns_unaliased
	inline T* Pool::alloc(const allocsize_t n)
	{
		return reinterpret_cast<T*>(Mem_Alloc(this, n * sizeof(T)));
	}

	template<typename T>
	__returns_aligned(poolAllocAlign) __returns_nonull __nonull_args(2) __returns_unaliased
	inline T* Pool::realloc(T* data, const allocsize_t n)
	{
		return reinterpret_cast<T*>(Mem_Realloc(this, reinterpret_cast<void*>(data), n * sizeof(T)));
	}

	template<>
	__returns_aligned(poolAllocAlign) __returns_nonull __nonull_args(2) __returns_unaliased
	inline void* Pool::realloc<void>(void* data, const allocsize_t n)
	{
		return reinterpret_cast<void*>(Mem_Realloc(this, data, n));
	}


}
}
