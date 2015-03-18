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
// Z_zone.c

#include "quakedef.h"
#include "thread.h"

#ifdef WIN32
#include <windows.h>
#include <winbase.h>
#else
#include <unistd.h>
#endif

#ifdef _MSC_VER
#include <vadefs.h>
#else
#include <stdint.h>
#endif

using namespace cloture;
using namespace util;
using namespace common;
using engine::cvars::CVar;

using namespace engine::memory;

#define		__memErrorProc	__cold __noinline __noreturn

#if mDebugMemory
	#define		mNameAndLine	filename, fileline
#else
	#define		mNameAndLine
#endif

#if mDebugMemory
	#define		mNameAndLineArgs	const char* filename, int fileline
#else
	#define		mNameAndLineArgs
#endif


#define MEMHEADER_SENTINEL_FOR_ADDRESS(p) ((sentinel_seed ^ (unsigned int) (uintptr_t) (p)) + sentinel_seed)
#define MEMHEADER_SENTINEL_FOR_ADDRESS_SEED(p, seed) \
((seed ^ (unsigned int) (uintptr_t) (p)) + seed)

unsigned int sentinel_seed;


void *mem_mutex = nullptr;

// divVerent: enables file backed malloc using mmap to conserve swap space (instead of malloc)
#ifndef FILE_BACKED_MALLOC
# define FILE_BACKED_MALLOC 0
#endif

// LordHavoc: enables our own low-level allocator (instead of malloc)
#ifndef MEMCLUMPING
# define MEMCLUMPING 0
#endif
#ifndef MEMCLUMPING_FREECLUMPS
# define MEMCLUMPING_FREECLUMPS 0
#endif

#if MEMCLUMPING
	// smallest unit we care about is this many bytes
	#define MEMUNIT 128
	// try to do 32MB clumps, but overhead eats into this
	#ifndef MEMWANTCLUMPSIZE
		#define MEMWANTCLUMPSIZE (1<<27)
	#endif
	// give malloc padding so we can't waste most of a page at the end
	#define MEMCLUMPSIZE (MEMWANTCLUMPSIZE - MEMWANTCLUMPSIZE/MEMUNIT/32 - 128)
	#define MEMBITS (MEMCLUMPSIZE / MEMUNIT)
	#define MEMBITINTS (MEMBITS / 32)

	typedef struct memclump_s
	{
		// contents of the clump
		unsigned char block[MEMCLUMPSIZE];
		// should always be MEMCLUMP_SENTINEL
		unsigned int sentinel1;
		// if a bit is on, it means that the MEMUNIT bytes it represents are
		// allocated, otherwise free
		unsigned int bits[MEMBITINTS];
		// should always be MEMCLUMP_SENTINEL
		unsigned int sentinel2;
		// if this drops to 0, the clump is freed
		size_t blocksinuse;
		// largest block of memory available (this is reset to an optimistic
		// number when anything is freed, and updated when alloc fails the clump)
		size_t largestavailable;
		// next clump in the chain
		struct memclump_s *chain;
	}
	memclump_t;

	#if MEMCLUMPING == 2
		static memclump_t masterclump;
	#endif
	static memclump_t *clumpchain = nullptr;
#endif

#if mDebugMemory
	CVar developer_memory =
	{
		0,
		"developer_memory",
		"0",
		"prints debugging information about memory allocations"
	};
	CVar developer_memorydebug =
	{
		0,
		"developer_memorydebug",
		"0",
		"enables memory corruption checks (very slow)"
	};

	CVar sys_memsize_physical =
	{
			CVAR_READONLY,
			"sys_memsize_physical",
			"",
			"physical memory size in MB (or empty if unknown)"
	};

	CVar sys_memsize_virtual =
	{
		CVAR_READONLY,
		"sys_memsize_virtual",
		"",
		"virtual memory size in MB (or empty if unknown)"
	};
#endif //#if mDebugMemory
static mempool_t *poolchain = nullptr;

void Mem_PrintStats();
void Mem_PrintList(size_t minallocationsize);

#if FILE_BACKED_MALLOC
	#include <stdlib.h>
	#include <sys/mman.h>
	typedef struct mmap_data_s
	{
		size_t len;
	}
	mmap_data_t;
	static void *mmap_malloc(size_t size)
	{
		char vabuf[MAX_OSPATH + 1];
		char *tmpdir = getenv("TEMP");
		mmap_data_t *data;
		int fd;
		size += sizeof(mmap_data_t); // waste block
		dpsnprintf(vabuf, sizeof(vabuf), "%s/darkplaces.XXXXXX", tmpdir ? tmpdir : "/tmp");
		fd = mkstemp(vabuf);
		if(fd < 0)
			return nullptr;
		ftruncate(fd, size);
		data = (unsigned char *) mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_NORESERVE, fd, 0);
		close(fd);
		unlink(vabuf);
		if(!data)
			return nullptr;
		data->len = size;
		return (void *) (data + 1);
	}
	static void mmap_free(void *mem)
	{
		mmap_data_t *data;
		if(!mem)
			return;
		data = ((mmap_data_t *) mem) - 1;
		munmap(data, data->len);
	}
	#define malloc mmap_malloc
	#define free mmap_free
#endif

#if MEMCLUMPING != 2
	// some platforms have a malloc that returns NULL but succeeds later
	// (Windows growing its swapfile for example)
	static void *attempt_malloc(size_t size)
	{
		// try for half a second or so
		unsigned int attempts = 500;
		while (attempts--)
		{
			void* base = reinterpret_cast<void*>(malloc(size));
			if (base)
				return base;
			Sys_Sleep(1000);
		}
		return nullptr;
	}
#endif

#if MEMCLUMPING
	static memclump_t *Clump_NewClump()
	{
		memclump_t **clumpchainpointer;
		memclump_t *clump;

		#if MEMCLUMPING == 2
			if (clumpchain)
				return nullptr;
			clump = &masterclump;
		#else
			clump = (memclump_t*)attempt_malloc(sizeof(memclump_t));
			if (!clump)
				return nullptr;
		#endif

		// initialize clump
		if (developer_memorydebug.integer)
			memset(clump, 0xEF, sizeof(*clump));
		clump->sentinel1 = MEMHEADER_SENTINEL_FOR_ADDRESS(&clump->sentinel1);
		memset(clump->bits, 0, sizeof(clump->bits));
		clump->sentinel2 = MEMHEADER_SENTINEL_FOR_ADDRESS(&clump->sentinel2);
		clump->blocksinuse = 0;
		clump->largestavailable = 0;
		clump->chain = nullptr;

		// link clump into chain
		for (clumpchainpointer = &clumpchain;*clumpchainpointer;clumpchainpointer = &(*clumpchainpointer)->chain)
			;
		*clumpchainpointer = clump;

		return clump;
	}
#endif

// low level clumping functions, all other memory functions use these
static void *Clump_AllocBlock(size_t size)
{
	unsigned char *base;
	#if MEMCLUMPING
		if (size <= MEMCLUMPSIZE)
		{
			int index;
			unsigned int bit;
			unsigned int needbits;
			unsigned int startbit;
			unsigned int endbit;
			unsigned int needints;
			int startindex;
			int endindex;
			unsigned int value;
			unsigned int mask;
			unsigned int *array;
			memclump_t **clumpchainpointer;
			memclump_t *clump;
			needbits = (size + MEMUNIT - 1) / MEMUNIT;
			needints = (needbits+31)>>5;
			for (clumpchainpointer = &clumpchain;;clumpchainpointer = &(*clumpchainpointer)->chain)
			{
				clump = *clumpchainpointer;
				if (!clump)
				{
					clump = Clump_NewClump();
					if (!clump)
						return nullptr;
				}
				if (clump->sentinel1 != MEMHEADER_SENTINEL_FOR_ADDRESS(&clump->sentinel1))
					Sys_Error("Clump_AllocBlock: trashed sentinel1\n");
				if (clump->sentinel2 != MEMHEADER_SENTINEL_FOR_ADDRESS(&clump->sentinel2))
					Sys_Error("Clump_AllocBlock: trashed sentinel2\n");
				startbit = 0;
				endbit = startbit + needbits;
				array = clump->bits;
				// do as fast a search as possible, even if it means crude alignment
				if (needbits >= 32)
				{
					// large allocations are aligned to large boundaries
					// furthermore, they are allocated downward from the top...
					endindex = MEMBITINTS;
					startindex = endindex - needints;
					index = endindex;
					while (--index >= startindex)
					{
						if (array[index])
						{
							endindex = index;
							startindex = endindex - needints;
							if (startindex < 0)
								goto nofreeblock;
						}
					}
					startbit = startindex*32;
					goto foundblock;
				}
				else
				{
					// search for a multi-bit gap in a single int
					// (not dealing with the cases that cross two ints)
					mask = (1<<needbits)-1;
					endbit = 32-needbits;
					bit = endbit;
					for (index = 0;index < MEMBITINTS;index++)
					{
						value = array[index];
						if (value != 0xFFFFFFFFu)
						{
							// there may be room in this one...
							for (bit = 0;bit < endbit;bit++)
							{
								if (!(value & (mask<<bit)))
								{
									startbit = index*32+bit;
									goto foundblock;
								}
							}
						}
					}
					goto nofreeblock;
				}
	foundblock:
				endbit = startbit + needbits;
				// mark this range as used
				// TODO: optimize
				for (bit = startbit;bit < endbit;bit++)
					if (clump->bits[bit>>5] & (1<<(bit & 31)))
						Sys_Error("Clump_AllocBlock: internal error (%i needbits)\n", needbits);
				for (bit = startbit;bit < endbit;bit++)
					clump->bits[bit>>5] |= (1<<(bit & 31));
				clump->blocksinuse += needbits;
				base = clump->block + startbit * MEMUNIT;
				if (developer_memorydebug.integer)
					memset(base, 0xBF, needbits * MEMUNIT);
				return base;
	nofreeblock:
				;
			}
			// never reached
			return nullptr;
		}
		// too big, allocate it directly
	#endif //#if MEMCLUMPING
	#if MEMCLUMPING == 2
		return nullptr;
	#else
		base = reinterpret_cast<uint8*>(attempt_malloc(size));
		#if mDebugMemory
			if (unlikely(developer_memorydebug.integer) && base)
				memset(base, 0xAF, size);
		#endif
		return base;
	#endif
}
static void Clump_FreeBlock(void *base, size_t size)
{
	#if MEMCLUMPING
		unsigned int needbits;
		unsigned int startbit;
		unsigned int endbit;
		unsigned int bit;
		memclump_t **clumpchainpointer;
		memclump_t *clump;
		unsigned char *start = (unsigned char *)base;
		for (clumpchainpointer = &clumpchain;(clump = *clumpchainpointer);clumpchainpointer = &(*clumpchainpointer)->chain)
		{
			if (start >= clump->block && start < clump->block + MEMCLUMPSIZE)
			{
				if (clump->sentinel1 != MEMHEADER_SENTINEL_FOR_ADDRESS(&clump->sentinel1))
					Sys_Error("Clump_FreeBlock: trashed sentinel1\n");
				if (clump->sentinel2 != MEMHEADER_SENTINEL_FOR_ADDRESS(&clump->sentinel2))
					Sys_Error("Clump_FreeBlock: trashed sentinel2\n");
				if (start + size > clump->block + MEMCLUMPSIZE)
					Sys_Error("Clump_FreeBlock: block overrun\n");
				// the block belongs to this clump, clear the range
				needbits = (size + MEMUNIT - 1) / MEMUNIT;
				startbit = (start - clump->block) / MEMUNIT;
				endbit = startbit + needbits;
				// first verify all bits are set, otherwise this may be misaligned or a double free
				for (bit = startbit;bit < endbit;bit++)
					if ((clump->bits[bit>>5] & (1<<(bit & 31))) == 0)
						Sys_Error("Clump_FreeBlock: double free\n");
				for (bit = startbit;bit < endbit;bit++)
					clump->bits[bit>>5] &= ~(1<<(bit & 31));
				clump->blocksinuse -= needbits;
				memset(base, 0xFF, needbits * MEMUNIT);
				// if all has been freed, free the clump itself
				if (clump->blocksinuse == 0)
				{
					*clumpchainpointer = clump->chain;
					if (developer_memorydebug.integer)
						memset(clump, 0xFF, sizeof(*clump));
				#if MEMCLUMPING != 2
								free(clump);
				#endif
				}
				return;
			}
		}
		// does not belong to any known chunk...  assume it was a direct allocation
	#endif //#if MEMCLUMPING
	#if MEMCLUMPING != 2
		memset(base, 0xFF, size);
		free(base);
	#endif
}


__memErrorProc
#if mDebugMemory
static void memAllocNullPool(const char* filename, const int fileline)
{
	Sys_Error("Mem_Alloc: pool == NULL (alloc at %s:%i)", filename, fileline);
}
#else
static void memAllocNullPool()
{
	Sys_Error("Mem_Alloc: pool == NULL (alloc at (unknown))");
}
#endif

__memErrorProc
static void memAllocOutOfMemory(
#if mDebugMemory
const char* filename, const int fileline
#endif
)
{
	Mem_PrintList(0);
	Mem_PrintStats();
	Mem_PrintList(1<<30);
	Mem_PrintStats();
	Sys_Error(
	#if mDebugMemory
			"Mem_Alloc: out of memory (alloc at %s:%i)", filename, fileline
	#else
			"Mem_Alloc: out of memory (alloc at (unknown))"
	#endif
	);
}

#if mDebugMemory
__cold __noinline
static void memAllocPrintDebug(mempool_t *pool, allocsize_t size, const char* filename, const int fileline)
{

	Con_DPrintf("Mem_Alloc: pool %s, file %s:%i, size %i bytes\n", pool->name, filename, fileline, (int)size);
}
#endif

__hot
void *_Mem_Alloc(mempool_t *pool, void *olddata, allocsize_t size
#if mDebugMemory
, const char *filename, int fileline
#endif
)
{
	using sentinelType	=	typeof(sentinel_seed);
	static constexpr size32 headerSize		= sizeof(memheader_t);
	static constexpr size32 sentinelSize 	= sizeof(sentinelType);
	static constexpr size32 poolAlign32		= poolAllocAlign;
	static constexpr size32 poolPad32		= poolAllocPad;
	memheader_t *oldmem;
/*
 * 	static constexpr size_t poolAlignFactor = headerSize + (poolAllocAlign-1);
	static constexpr size_t poolAlignMask	= ~(poolAllocAlign-1);

	memheader_t *mem = reinterpret_cast<memheader_t*>(
	((reinterpret_cast<size_t>(base) + poolAlignFactor) & poolAlignMask) - headerSize
	);
 */
	size += (poolPad32 - 1);
	size &= ~(poolPad32 - 1);

	if (size <= 0)
	{
		if (olddata)
		{
			#if mDebugMemory
				_Mem_Free(olddata, filename, fileline);
			#else
				_Mem_Free(olddata);
			#endif
		}
		return nullptr;
	}

	if (pool == nullptr)
	{
		if(likely(olddata))
			pool = reinterpret_cast<memheader_t *>(reinterpret_cast<uint8*>(olddata) - headerSize)->pool;
		else
		{
			#if mDebugMemory
				memAllocNullPool(filename, fileline);
			#else
				memAllocNullPool();
			#endif
		}
	}

	if (mem_mutex != nullptr)
		Thread_LockMutex(mem_mutex);

	#if mDebugMemory
		if (unlikely(developer_memory.integer))
			memAllocPrintDebug(pool, size, filename, fileline);
	#endif

	pool->totalsize += size;
	uint8* base = ({
		const allocsize_t realsize = poolAlign32 + headerSize + size + sentinelSize;
		pool->realsize += realsize;
		reinterpret_cast<uint8*>(Clump_AllocBlock(realsize));
	});

	if (unlikely(base == nullptr))
	{
		#if mDebugMemory
			memAllocOutOfMemory(filename, fileline);
		#else
			memAllocOutOfMemory();
		#endif
	}
	static constexpr size_t poolAlignFactor = headerSize + (poolAllocAlign-1);
	static constexpr size_t poolAlignMask	= ~(poolAllocAlign-1);

	memheader_t *mem = reinterpret_cast<memheader_t*>(
	((reinterpret_cast<size_t>(base) + poolAlignFactor) & poolAlignMask) - headerSize
	);
	mem->baseaddress	= reinterpret_cast<void*>(base);
	#if mDebugMemory
		mem->filename		= filename;
		mem->fileline		= fileline;
	#endif
	mem->size			= size;
	mem->pool			= pool;

	// calculate sentinels (detects buffer overruns, in a way that is hard to exploit)
	{
		const sentinelType localSentinelSeed = sentinel_seed;

		const sentinelType sentinel1 =
				MEMHEADER_SENTINEL_FOR_ADDRESS_SEED(
				&mem->sentinel,
				localSentinelSeed
				);
		const sentinelType sentinel2 =
				MEMHEADER_SENTINEL_FOR_ADDRESS_SEED(
				reinterpret_cast<uint8*>( mem ) + headerSize + size,
				localSentinelSeed
				);

		mem->sentinel = sentinel1;
		__builtin_memcpy(reinterpret_cast<uint8*>(mem) + headerSize + size, &sentinel2, sentinelSize);
	}
	// append to head of list
	mem->next	= pool->chain;
	mem->prev	= nullptr;
	pool->chain = mem;
	if (mem->next)
		mem->next->prev = mem;

	if (mem_mutex)
		Thread_UnlockMutex(mem_mutex);

	// copy the shared portion in the case of a realloc, then memset the rest
	allocsize_t sharedsize = 0;
	allocsize_t remainsize = size;
	if (olddata)
	{
		memheader_t *oldmem = reinterpret_cast<memheader_t*>(olddata) - 1;
		sharedsize = min(oldmem->size, size);

		memcpy(
			reinterpret_cast<void *>( reinterpret_cast<uint8*>(mem) + headerSize),
			olddata,
			sharedsize
		);
		remainsize -= sharedsize;

		_Mem_Free(olddata
		#if mDebugMemory
				, filename, fileline
		#endif
		);
	}

	void* result = reinterpret_cast<void*>(reinterpret_cast<uint8*>(mem) + headerSize);

	__builtin_memset(reinterpret_cast<uint8*>(result) + sharedsize, 0, remainsize);
	return result;
}

#if	mDebugMemory
	#define		mNameHeaderAndLineArgs		memheader_t* mem, const char* filename, int fileline
#else
	#define		mNameHeaderAndLineArgs
#endif

__memErrorProc
static void freeBlockTrashedHeadSentinel(mNameHeaderAndLineArgs)
{
#if mDebugMemory
	Sys_Error("Mem_Free: trashed head sentinel (alloc at %s:%i, free at %s:%i)", mem->filename, mem->fileline, filename, fileline);
#else
	Sys_Error("Mem_Free: trashed head sentinel (alloc at (unknown), free at (unknown))");
#endif
}

__memErrorProc
static void freeBlockTrashedTailSentinel(mNameHeaderAndLineArgs)
{
	#if mDebugMemory
		Sys_Error("Mem_Free: trashed tail sentinel (alloc at %s:%i, free at %s:%i)", mem->filename, mem->fileline, filename, fileline);
	#else
		Sys_Error("Mem_Free: trashed tail sentinel (alloc at (unknown), free at (unknown))");
	#endif
}

__memErrorProc
static void freeBlockDoubleFree(mNameAndLineArgs)
{
	#if mDebugMemory
		Sys_Error("_Mem_FreeBlock: not allocated or double freed (free at %s:%i)", filename, fileline);
	#else
		Sys_Error("_Mem_FreeBlock: not allocated or double freed (free at (unknown):(unknown))");
	#endif
}

// only used by _Mem_Free and _Mem_FreePool
static inline void _Mem_FreeBlock(memheader_t *mem
#if mDebugMemory
, const char *filename, int fileline
#endif
)
{
	mempool_t *pool;
	allocsize_t size;
	allocsize_t realsize;
	unsigned int sentinel1;
	unsigned int sentinel2;

	// check sentinels (detects buffer overruns, in a way that is hard to exploit)
	sentinel1 = MEMHEADER_SENTINEL_FOR_ADDRESS(&mem->sentinel);
	sentinel2 = MEMHEADER_SENTINEL_FOR_ADDRESS((unsigned char *) mem + sizeof(memheader_t) + mem->size);
	if (unlikely(mem->sentinel != sentinel1))
	{
		#if mDebugMemory
			freeBlockTrashedHeadSentinel(mem, filename, fileline);
		#else
			freeBlockTrashedHeadSentinel();
		#endif
	}
	if (unlikely(memcmp((unsigned char *) mem + sizeof(memheader_t) + mem->size, &sentinel2, sizeof(sentinel2))))
	{
		#if mDebugMemory
			freeBlockTrashedTailSentinel(mem, filename, fileline);
		#else
			freeBlockTrashedTailSentinel();
		#endif
	}
	pool = mem->pool;
	#if mDebugMemory
		if (developer_memory.integer)
			Con_DPrintf("Mem_Free: pool %s, alloc %s:%i, free %s:%i, size %i bytes\n", pool->name, mem->filename, mem->fileline, filename, fileline, (int)(mem->size));
	#endif
	// unlink memheader from doubly linked list
	if (
		unlikely(
				(mem->prev ? mem->prev->next != mem : pool->chain != mem) || (mem->next && mem->next->prev != mem)
		)
	)
	{
		#if mDebugMemory
			freeBlockDoubleFree(filename, fileline);
		#else
			freeBlockDoubleFree();
		#endif
	}
	if (mem_mutex)
		Thread_LockMutex(mem_mutex);
	if (mem->prev)
		mem->prev->next = mem->next;
	else
		pool->chain = mem->next;
	if (mem->next)
		mem->next->prev = mem->prev;
	// memheader has been unlinked, do the actual free now
	size = mem->size;
	realsize = sizeof(memheader_t) + size + sizeof(sentinel2);
	pool->totalsize -= size;
	pool->realsize -= realsize;
	Clump_FreeBlock(mem->baseaddress, realsize);
	if (mem_mutex)
		Thread_UnlockMutex(mem_mutex);
}

__noinline  __cold
static void memFreeNullData(
#if mDebugMemory
		const char* filename,
		int fileline
#endif
)
{
	#if mDebugMemory
		Con_DPrintf("Mem_Free: data == NULL (called at %s:%i)\n", filename, fileline);
	#else
		Con_DPrintf("Mem_Free: data == NULL (called at (unknown):(unknown))\n");
	#endif
}

void _Mem_Free(void *data
#if mDebugMemory
, const char *filename, int fileline
#endif
)
{
	if (unlikely(data == nullptr))
	{
		#if mDebugMemory
			memFreeNullData(filename, fileline)
		#else
			memFreeNullData();
		#endif

		return;
	}

	#if mDebugMemory
		if (unlikely(developer_memorydebug.integer))
		{
			//_Mem_CheckSentinelsGlobal(filename, fileline);
			if (!Mem_IsAllocated(nullptr, data))
				Sys_Error("Mem_Free: data is not allocated (called at %s:%i)", filename, fileline);
		}
	#endif

	_Mem_FreeBlock((memheader_t *)((unsigned char *) data - sizeof(memheader_t))
	#if mDebugMemory
			, filename, fileline
	#endif
	);
}

__memErrorProc
static void memAllocPoolOutOfMemory(
#if mDebugMemory
const char* filename, const int fileline
#endif
)
{
	Mem_PrintList(0);
	Mem_PrintStats();
	Mem_PrintList(1<<30);
	Mem_PrintStats();
	Sys_Error(
	#if mDebugMemory
			"Mem_AllocPool: out of memory (alloc at %s:%i)", filename, fileline
	#else
			"Mem_AllocPool: out of memory (allocpool at (unknown):(unknown))"
	#endif
	);
}
mempool_t *_Mem_AllocPool(const char *name, int flags, mempool_t *parent
#if mDebugMemory
, const char *filename, int fileline
#endif
)
{
	mempool_t *pool;
	#if mDebugMemory
		if (unlikely(developer_memorydebug.integer))
			_Mem_CheckSentinelsGlobal(filename, fileline);
	#endif
	pool = (mempool_t *)Clump_AllocBlock(sizeof(mempool_t));
	if (unlikely(pool == nullptr))
	{
		memAllocPoolOutOfMemory(
		#if mDebugMemory
				filename, fileline
		#endif
		);
	}
	memset(pool, 0, sizeof(mempool_t));
	pool->sentinel1 = MEMHEADER_SENTINEL_FOR_ADDRESS(&pool->sentinel1);
	pool->sentinel2 = MEMHEADER_SENTINEL_FOR_ADDRESS(&pool->sentinel2);
	#if mDebugMemory
		pool->filename = filename;
		pool->fileline = fileline;
	#endif
	pool->flags = flags;
	pool->chain = nullptr;
	pool->totalsize = 0;
	pool->realsize = sizeof(mempool_t);
	strlcpy (pool->name, name, sizeof (pool->name));
	pool->parent = parent;
	pool->next = poolchain;
	poolchain = pool;
	return pool;
}

__memErrorProc
static void freePoolDoubleFree(mNameAndLineArgs)
{
#if mDebugMemory
	Sys_Error("Mem_FreePool: pool already free (freepool at %s:%i)", filename, fileline);
#else
	Sys_Error("Mem_FreePool: pool already free (freepool at (unknown):(unknown))");
#endif
}

#if mDebugMemory
	#define		mPoolNameAndLineArgs		mempool_t* pool, const char *filename, int fileline
#else
	#define		mPoolNameAndLineArgs
#endif

__memErrorProc
static void freePoolTrashedSentinel1(mPoolNameAndLineArgs)
{
#if mDebugMemory
	Sys_Error("Mem_FreePool: trashed pool sentinel 1 (allocpool at %s:%i, freepool at %s:%i)", pool->filename, pool->fileline, filename, fileline);
#else
	Sys_Error("Mem_FreePool: trashed pool sentinel 1 (allocpool at (unknown):(unknown), freepool at (unknown):(unknown))");
#endif
}

__memErrorProc
static void freePoolTrashedSentinel2(mPoolNameAndLineArgs)
{
#if mDebugMemory
	Sys_Error("Mem_FreePool: trashed pool sentinel 2 (allocpool at %s:%i, freepool at %s:%i)", pool->filename, pool->fileline, filename, fileline);
#else
	Sys_Error("Mem_FreePool: trashed pool sentinel 2 (allocpool at (unknown):(unknown), freepool at (unknown):(unknown))");
#endif
}

void _Mem_FreePool(mempool_t **poolpointer
#if mDebugMemory
, const char *filename, int fileline
#endif
)
{
	mempool_t *pool = *poolpointer;
	mempool_t **chainaddress, *iter, *temp;
	#if mDebugMemory
		if (developer_memorydebug.integer)
			_Mem_CheckSentinelsGlobal(filename, fileline);
	#endif
	if (unlikely(pool == nullptr))
		return;

	// unlink pool from chain
	for (
			chainaddress = &poolchain;
			*chainaddress && *chainaddress != pool;
			chainaddress = &((*chainaddress)->next)
		)
		;

	if (unlikely(*chainaddress != pool))
		freePoolDoubleFree(mNameAndLine);
	if (unlikely(pool->sentinel1 != MEMHEADER_SENTINEL_FOR_ADDRESS(&pool->sentinel1)))
	{
		freePoolTrashedSentinel1(
		#if mDebugMemory
				pool, filename, fileline
		#endif
		);
	}
	if (unlikely(pool->sentinel2 != MEMHEADER_SENTINEL_FOR_ADDRESS(&pool->sentinel2)))
	{
		freePoolTrashedSentinel2(
		#if mDebugMemory
				pool, filename, fileline
		#endif
		);
	}
	*chainaddress = pool->next;

	// free memory owned by the pool
	while (pool->chain)
	{
		_Mem_FreeBlock(pool->chain
		#if mDebugMemory
				, filename, fileline
		#endif
		);
	}
	// free child pools, too
	for(iter = poolchain; iter; temp = iter = iter->next)
	{
		if(iter->parent == pool)
		{
			_Mem_FreePool(&temp
			#if mDebugMemory
					, filename, fileline
			#endif
			);
		}
	}
	// free the pool itself
	Clump_FreeBlock(pool, sizeof(*pool));

	*poolpointer = nullptr;

}

__memErrorProc
static void emptyPoolNullPool(mNameAndLineArgs)
{
#if mDebugMemory
	Sys_Error("Mem_EmptyPool: pool == NULL (emptypool at %s:%i)", filename, fileline);
#else
	Sys_Error("Mem_EmptyPool: pool == NULL (emptypool at (unknown):(unknown))");
#endif
}


__memErrorProc
static void emptyPoolTrashedSentinel1(mPoolNameAndLineArgs)
{
#if mDebugMemory
	Sys_Error("Mem_EmptyPool: trashed pool sentinel 1 (allocpool at %s:%i, freepool at %s:%i)", pool->filename, pool->fileline, filename, fileline);
#else
	Sys_Error("Mem_EmptyPool: trashed pool sentinel 1 (allocpool at (unknown):(unknown), freepool at (unknown):(unknown))");
#endif
}

__memErrorProc
static void emptyPoolTrashedSentinel2(mPoolNameAndLineArgs)
{
#if mDebugMemory
	Sys_Error("Mem_EmptyPool: trashed pool sentinel 2 (allocpool at %s:%i, freepool at %s:%i)", pool->filename, pool->fileline, filename, fileline);
#else
	Sys_Error("Mem_EmptyPool: trashed pool sentinel 2 (allocpool at (unknown):(unknown), freepool at (unknown):(unknown))");
#endif
}
void _Mem_EmptyPool(mempool_t *pool
#if mDebugMemory
, const char *filename, int fileline
#endif
)
{
	mempool_t *chainaddress;

	#if mDebugMemory
		if (developer_memorydebug.integer)
		{
			//_Mem_CheckSentinelsGlobal(filename, fileline);
			// check if this pool is in the poolchain
			for (chainaddress = poolchain;chainaddress;chainaddress = chainaddress->next)
				if (chainaddress == pool)
					break;
			if (!chainaddress)
				Sys_Error("Mem_EmptyPool: pool is already free (emptypool at %s:%i)", filename, fileline);
		}
	#endif
	if (unlikely(pool == nullptr))
	{
		emptyPoolNullPool(
			#if mDebugMemory
				filename, fileline
			#endif
		);
	}
	if (unlikely(pool->sentinel1 != MEMHEADER_SENTINEL_FOR_ADDRESS(&pool->sentinel1)))
	{
		emptyPoolTrashedSentinel1(
		#if mDebugMemory
				pool, filename, fileline
		#endif
		);
	}
	if (unlikely(pool->sentinel2 != MEMHEADER_SENTINEL_FOR_ADDRESS(&pool->sentinel2)))
	{
		emptyPoolTrashedSentinel2(
		#if mDebugMemory
				pool, filename, fileline
		#endif
		);
	}
	// free memory owned by the pool
	while (pool->chain)
	{
		_Mem_FreeBlock(pool->chain
		#if mDebugMemory
				, filename, fileline
		#endif
		);
	}
	// empty child pools, too
	for(chainaddress = poolchain; chainaddress; chainaddress = chainaddress->next)
	{
		if(chainaddress->parent == pool)
		{
			_Mem_EmptyPool(chainaddress
			#if mDebugMemory
					, filename, fileline
			#endif
			);
		}
	}

}

__memErrorProc
static void checkSentinelsNullData(mNameAndLineArgs)
{
#if mDebugMemory
	Sys_Error("Mem_CheckSentinels: data == NULL (sentinel check at %s:%i)", filename, fileline);
#else
	Sys_Error("Mem_CheckSentinels: data == NULL (sentinel check at (unknown):(unknown))");
#endif
}

void _Mem_CheckSentinels(void *data
#if mDebugMemory
, const char *filename, int fileline
#endif
)
{
	memheader_t *mem;
	unsigned int sentinel1;
	unsigned int sentinel2;

	if (unlikely(data == nullptr))
	{
		checkSentinelsNullData(
		#if mDebugMemory
				filename, fileline
		#endif
		);
	}
	mem = (memheader_t *)((unsigned char *) data - sizeof(memheader_t));
	sentinel1 = MEMHEADER_SENTINEL_FOR_ADDRESS(&mem->sentinel);
	sentinel2 = MEMHEADER_SENTINEL_FOR_ADDRESS((unsigned char *) mem + sizeof(memheader_t) + mem->size);
	if (unlikely(mem->sentinel != sentinel1))
	{
		freeBlockTrashedHeadSentinel(
		#if mDebugMemory
				mem, filename, fileline
		#endif
		);//
	}
	if (unlikely(memcmp((unsigned char *) mem + sizeof(memheader_t) + mem->size, &sentinel2, sizeof(sentinel2))))
	{
		freeBlockTrashedTailSentinel(
		#if mDebugMemory
				mem, filename, fileline
		#endif
		);
	}
}

#if MEMCLUMPING
static void _Mem_CheckClumpSentinels(memclump_t *clump
	#if mDebugMemory
	, const char *filename, int fileline
	#endif
)
{
	// this isn't really very useful
	if (clump->sentinel1 != MEMHEADER_SENTINEL_FOR_ADDRESS(&clump->sentinel1))
		Sys_Error("Mem_CheckClumpSentinels: trashed sentinel 1 (sentinel check at %s:%i)", filename, fileline);
	if (clump->sentinel2 != MEMHEADER_SENTINEL_FOR_ADDRESS(&clump->sentinel2))
		Sys_Error("Mem_CheckClumpSentinels: trashed sentinel 2 (sentinel check at %s:%i)", filename, fileline);
}
#endif

__memErrorProc
static void checkSentinelsGlobalTrashedSentinel1(mPoolNameAndLineArgs)
{
#if mDebugMemory
	Sys_Error("Mem_CheckSentinelsGlobal: trashed pool sentinel 1 (allocpool at %s:%i, sentinel check at %s:%i)", pool->filename, pool->fileline, filename, fileline);
#else
	Sys_Error("Mem_CheckSentinelsGlobal: trashed pool sentinel 1 (allocpool at (unknown):(unknown), sentinel check at (unknown):(unknown))");
#endif
}

__memErrorProc
static void checkSentinelsGlobalTrashedSentinel2(mPoolNameAndLineArgs)
{
#if mDebugMemory
	Sys_Error("Mem_CheckSentinelsGlobal: trashed pool sentinel 2 (allocpool at %s:%i, sentinel check at %s:%i)", pool->filename, pool->fileline, filename, fileline);
#else
	Sys_Error("Mem_CheckSentinelsGlobal: trashed pool sentinel 2 (allocpool at (unknown):(unknown), sentinel check at (unknown):(unknown))");
#endif
}

void _Mem_CheckSentinelsGlobal(mNameAndLineArgs)
{
	memheader_t *mem;
#if MEMCLUMPING
	memclump_t *clump;
#endif
	mempool_t *pool;
	for (pool = poolchain;pool;pool = pool->next)
	{
		if (unlikely(pool->sentinel1 != MEMHEADER_SENTINEL_FOR_ADDRESS(&pool->sentinel1)))
		{
			checkSentinelsGlobalTrashedSentinel1(
			#if mDebugMemory
					pool, filename, line
			#endif
			);
		}
		if (unlikely(pool->sentinel2 != MEMHEADER_SENTINEL_FOR_ADDRESS(&pool->sentinel2)))
		{
			checkSentinelsGlobalTrashedSentinel2(
			#if mDebugMemory
					pool, filename, line
			#endif
			);
		}
	}
	for (pool = poolchain;pool;pool = pool->next)
	{
		for (mem = pool->chain;mem;mem = mem->next)
		{
			_Mem_CheckSentinels((void *)((unsigned char *) mem + sizeof(memheader_t))
			#if mDebugMemory
					, filename, fileline
			#endif
			);
		}
	}
#if MEMCLUMPING
	for (pool = poolchain;pool;pool = pool->next)
	{
		for (clump = clumpchain;clump;clump = clump->chain)
		{
			_Mem_CheckClumpSentinels(clump
			#if mDebugMemory
					, filename, fileline
			#endif
			);
		}
	}
#endif
}

bool Mem_IsAllocated(mempool_t *pool, void *data)
{
	memheader_t *header;
	memheader_t *target;

	if (pool)
	{
		// search only one pool
		target = (memheader_t *)((unsigned char *) data - sizeof(memheader_t));
		for( header = pool->chain ; header ; header = header->next )
			if( header == target )
				return true;
	}
	else
	{
		// search all pools
		for (pool = poolchain;pool;pool = pool->next)
			if (Mem_IsAllocated(pool, data))
				return true;
	}
	return false;
}

void Mem_ExpandableArray_NewArray(memexpandablearray_t *l, mempool_t *mempool, size_t recordsize, int numrecordsperarray)
{
	memset(l, 0, sizeof(*l));
	l->mempool = mempool;
	l->recordsize = recordsize;
	l->numrecordsperarray = numrecordsperarray;
}

void Mem_ExpandableArray_FreeArray(memexpandablearray_t *l)
{
	size_t i;
	if (l->maxarrays)
	{
		for (i = 0;i != l->numarrays;i++)
			Mem_Free(l->arrays[i].data);
		Mem_Free(l->arrays);
	}
	memset(l, 0, sizeof(*l));
}

void *Mem_ExpandableArray_AllocRecord(memexpandablearray_t *l)
{
	size_t i, j;
	for (i = 0;;i++)
	{
		if (i == l->numarrays)
		{
			if (l->numarrays == l->maxarrays)
			{
				memexpandablearray_array_t *oldarrays = l->arrays;
				l->maxarrays = max(l->maxarrays * 2, 128);
				l->arrays = (memexpandablearray_array_t*) Mem_Alloc(l->mempool, l->maxarrays * sizeof(*l->arrays));
				if (oldarrays)
				{
					memcpy(l->arrays, oldarrays, l->numarrays * sizeof(*l->arrays));
					Mem_Free(oldarrays);
				}
			}
			l->arrays[i].numflaggedrecords = 0;
			l->arrays[i].data = (unsigned char *) Mem_Alloc(l->mempool, (l->recordsize + 1) * l->numrecordsperarray);
			l->arrays[i].allocflags = l->arrays[i].data + l->recordsize * l->numrecordsperarray;
			l->numarrays++;
		}
		if (l->arrays[i].numflaggedrecords < l->numrecordsperarray)
		{
			for (j = 0;j < l->numrecordsperarray;j++)
			{
				if (!l->arrays[i].allocflags[j])
				{
					l->arrays[i].allocflags[j] = true;
					l->arrays[i].numflaggedrecords++;
					memset(l->arrays[i].data + l->recordsize * j, 0, l->recordsize);
					return (void *)(l->arrays[i].data + l->recordsize * j);
				}
			}
		}
	}
}

/*****************************************************************************
 * IF YOU EDIT THIS:
 * If this function was to change the size of the "expandable" array, you have
 * to update r_shadow.c
 * Just do a search for "range =", R_ShadowClearWorldLights would be the first
 * function to look at. (And also seems like the only one?) You  might have to
 * move the  call to Mem_ExpandableArray_IndexRange  back into for(...) loop's
 * condition
 */
void Mem_ExpandableArray_FreeRecord(memexpandablearray_t *l, void *record) // const!
{
	size_t i, j;
	unsigned char *p = (unsigned char *)record;
	for (i = 0;i != l->numarrays;i++)
	{
		if (p >= l->arrays[i].data && p < (l->arrays[i].data + l->recordsize * l->numrecordsperarray))
		{
			j = (p - l->arrays[i].data) / l->recordsize;
			if (unlikely(p != l->arrays[i].data + j * l->recordsize))
				Sys_Error("Mem_ExpandableArray_FreeRecord: no such record %p\n", p);
			if (unlikely(!l->arrays[i].allocflags[j]))
				Sys_Error("Mem_ExpandableArray_FreeRecord: record %p is already free!\n", p);
			l->arrays[i].allocflags[j] = false;
			l->arrays[i].numflaggedrecords--;
			return;
		}
	}
}

size_t Mem_ExpandableArray_IndexRange(const memexpandablearray_t *l)
{
	size_t i, j, k, end = 0;
	for (i = 0;i < l->numarrays;i++)
	{
		for (j = 0, k = 0;k < l->arrays[i].numflaggedrecords;j++)
		{
			if (l->arrays[i].allocflags[j])
			{
				end = l->numrecordsperarray * i + j + 1;
				k++;
			}
		}
	}
	return end;
}

void *Mem_ExpandableArray_RecordAtIndex(const memexpandablearray_t *l, size_t index)
{
	size_t i, j;
	i = index / l->numrecordsperarray;
	j = index % l->numrecordsperarray;
	if (i >= l->numarrays || !l->arrays[i].allocflags[j])
		return nullptr;
	return (void *)(l->arrays[i].data + j * l->recordsize);
}


// used for temporary memory allocations around the engine, not for longterm
// storage, if anything in this pool stays allocated during gameplay, it is
// considered a leak
mempool_t *tempmempool;
// only for zone
mempool_t *zonemempool;

void Mem_PrintStats()
{
	size_t count = 0, size = 0, realsize = 0;
	mempool_t *pool;
	memheader_t *mem;
	Mem_CheckSentinelsGlobal();
	for (pool = poolchain;pool;pool = pool->next)
	{
		count++;
		size += pool->totalsize;
		realsize += pool->realsize;
	}
	Con_Printf("%lu memory pools, totalling %lu bytes (%.3fMB)\n", (unsigned long)count, (unsigned long)size, size / 1048576.0);
	Con_Printf("total allocated size: %lu bytes (%.3fMB)\n", (unsigned long)realsize, realsize / 1048576.0);
	for (pool = poolchain;pool;pool = pool->next)
	{
		if ((pool->flags & POOLFLAG_TEMP) && pool->chain)
		{
			Con_Printf("Memory pool %p has sprung a leak totalling %lu bytes (%.3fMB)!  Listing contents...\n", (void *)pool, (unsigned long)pool->totalsize, pool->totalsize / 1048576.0);
			#if mDebugMemory
			for (mem = pool->chain;mem;mem = mem->next)
				Con_Printf("%10lu bytes allocated at %s:%i\n", (unsigned long)mem->size, mem->filename, mem->fileline);
			#endif
		}
	}
}

void Mem_PrintList(size_t minallocationsize)
{
	mempool_t *pool;
	memheader_t *mem;
	Mem_CheckSentinelsGlobal();
	Con_Print("memory pool list:\n"
	           "size    name\n");
	for (pool = poolchain;pool;pool = pool->next)
	{
		Con_Printf("%10luk (%10luk actual) %s (%+li byte change) %s\n", (unsigned long) ((pool->totalsize + 1023) / 1024), (unsigned long)((pool->realsize + 1023) / 1024), pool->name, (long)(pool->totalsize - pool->lastchecksize), (pool->flags & POOLFLAG_TEMP) ? "TEMP" : "");
		pool->lastchecksize = pool->totalsize;
		for (mem = pool->chain;mem;mem = mem->next)
		{
			#if mDebugMemory
			if (mem->size >= minallocationsize)
				Con_Printf("%10lu bytes allocated at %s:%i\n", (unsigned long)mem->size, mem->filename, mem->fileline);
			#endif
		}
	}
}

static void MemList_f()
{
	switch(Cmd_Argc())
	{
	case 1:
		Mem_PrintList(1<<30);
		Mem_PrintStats();
		break;
	case 2:
		Mem_PrintList(atoi(Cmd_Argv(1)) * 1024);
		Mem_PrintStats();
		break;
	default:
		Con_Print("MemList_f: unrecognized options\nusage: memlist [all]\n");
		break;
	}
}

static void MemStats_f()
{
	Mem_CheckSentinelsGlobal();
	R_TextureStats_Print(false, false, true);
	GL_Mesh_ListVBOs(false);
	Mem_PrintStats();
}


char* Mem_strdup (mempool_t *pool, const char* s)
{
	char* p;
	size_t sz;
	if (s == nullptr)
		return nullptr;
	sz = strlen (s) + 1;
	p = (char*)Mem_Alloc (pool, sz);
	strlcpy (p, s, sz);
	return p;
}

/*
========================
Memory_Init
========================
*/
void Memory_Init ()
{
	volatile static union {unsigned short s;unsigned char b[2];} u;
	u.s = 0x100;

	assert(mem_bigendian == u.b[0] != 0);

	sentinel_seed = rand();
	poolchain = nullptr;
	tempmempool = Mem_AllocPool("Temporary Memory", POOLFLAG_TEMP, nullptr);
	zonemempool = Mem_AllocPool("Zone", 0, nullptr);

	if (Thread_HasThreads())
		mem_mutex = Thread_CreateMutex();
}

void Memory_Shutdown ()
{
	if (mem_mutex)
		Thread_DestroyMutex(mem_mutex);
	mem_mutex = nullptr;
}

#if mDebugMemory
	void Memory_Init_Commands ()
	{
		Cmd_AddCommand ("memstats", MemStats_f, "prints memory system statistics");
		Cmd_AddCommand ("memlist", MemList_f, "prints memory pool information (or if used as memlist 5 lists individual allocations of 5K or larger, 0 lists all allocations)");
		Cvar_RegisterVariable (&developer_memory);
		Cvar_RegisterVariable (&developer_memorydebug);
		Cvar_RegisterVariable (&sys_memsize_physical);
		Cvar_RegisterVariable (&sys_memsize_virtual);

	#if defined(WIN32)
	#ifdef _WIN64
		{
			MEMORYSTATUSEX status;
			// first guess
			Cvar_SetValueQuick(&sys_memsize_virtual, 8388608);
			// then improve
			status.dwLength = sizeof(status);
			if(GlobalMemoryStatusEx(&status))
			{
				Cvar_SetValueQuick(&sys_memsize_physical, status.ullTotalPhys / 1048576.0);
				Cvar_SetValueQuick(&sys_memsize_virtual, min(sys_memsize_virtual.value, status.ullTotalVirtual / 1048576.0));
			}
		}
	#else
		{
			MEMORYSTATUS status;
			// first guess
			Cvar_SetValueQuick(&sys_memsize_virtual, 2048);
			// then improve
			status.dwLength = sizeof(status);
			GlobalMemoryStatus(&status);
			Cvar_SetValueQuick(&sys_memsize_physical, status.dwTotalPhys / 1048576.0);
			Cvar_SetValueQuick(&sys_memsize_virtual, min(sys_memsize_virtual.value, status.dwTotalVirtual / 1048576.0));
		}
	#endif
	#else
		{
			// first guess
			Cvar_SetValueQuick(&sys_memsize_virtual, (sizeof(void*) == 4) ? 2048 : 268435456);
			// then improve
			{
				// Linux, and BSD with linprocfs mounted
				FILE *f = fopen("/proc/meminfo", "r");
				if(f)
				{
					static char buf[1024];
					while(fgets(buf, sizeof(buf), f))
					{
						const char *p = buf;
						if(!COM_ParseToken_Console(&p))
							continue;
						if(!strcmp(com_token, "MemTotal:"))
						{
							if(!COM_ParseToken_Console(&p))
								continue;
							Cvar_SetValueQuick(&sys_memsize_physical, atof(com_token) / 1024.0);
						}
						if(!strcmp(com_token, "SwapTotal:"))
						{
							if(!COM_ParseToken_Console(&p))
								continue;
							Cvar_SetValueQuick(&sys_memsize_virtual, min(sys_memsize_virtual.value , atof(com_token) / 1024.0 + sys_memsize_physical.value));
						}
					}
					fclose(f);
				}
			}
		}
	#endif
	}
#endif //#if mDebugMemory

void* operator new(size_t sz)
{
	return malloc(sz);
}

void operator delete(void* v)
{
	free(v);
}
void* operator new[](size_t sz)
{
	return malloc(sz);
}

void operator delete[](void* v)
{
	free(v);
}

extern "C" void __cxa_pure_virtual()
{
	exit(1);
}//

//#if 0
void* __gxx_personality_v0;

extern "C" void* __cxa_begin_catch(void* exceptionObject)
{
	exit(1);
	return nullptr;
}
extern "C" void __clang_call_terminate(void* )
{
	exit(1);
}



extern "C" void __cxa_rethrow()
{
	exit(1);
}
extern "C" void __cxa_call_unexpected(void*)
{
	exit(1);
}

namespace std
{
	/*[[noreturn]] void terminate()
	{
		exit(1);
	}*/
	void __throw_bad_alloc(){exit(1);}
	void __throw_length_error(char const*)
	{
		exit(1);
	}
};

namespace __cxxabiv1
{
	class __si_class_type_info
	{
		virtual ~__si_class_type_info()
		{
			exit(1);
		}
	};
};

//#endif
