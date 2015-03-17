
#include "../quakedef.h"

#include "newvm.hpp"
#include "../libcurl.h"
#include <time.h>

#include "../cl_collision.h"

#include "../csprogs.h"
#include "../ft2.h"
#include "../mdfour.h"

using namespace cloture;
using namespace util;
//import basic types
using namespace common;

//import dbgout/con
using console::con;
using console::dbgout;

using engine::cvars::CVar;
using namespace engine;

using server::Server;
using client::Client;
using memory::Pool;

namespace cloture::engine::vm
{
	void warn(const Program prog, const char *fmt, ...)
	{
		va_list argptr;
		char msg[MAX_INPUTLINE];

		#if !qcDisabled
			static double recursive = -1;
		#endif
		va_start(argptr, fmt);
		dpvsnprintf(msg, sizeof(msg), fmt, argptr);
		va_end(argptr);

		Con_Print(msg);

		#if !qcDisabled
			if(prvm_backtraceforwarnings.integer == 0 || recursive == realtime)
				return;

			recursive = realtime;


			PRVM_PrintState(prog, 0);

			recursive = -1;
		#endif

	}
	enum getStringErr : uint8
	{
		negativeOffset,
		invalidTempOffset,
		invalidZoneOffsetFreed,
		invalidZoneOffset,
		invalidConstantOffset
	};

	static const char* nullVMString = "";
	[[gnu::cold]]
	__noinline const char* getStringError(const Program prog, const int num, const getStringErr err)
	{
		switch(err)
		{
		case negativeOffset:
			vm::warn(prog, "vm::GetString: Invalid string offset (%i < 0)\n", num);
			break;
		case invalidTempOffset:
			vm::warn(prog, "vm::GetString: Invalid temp-string offset (%i >= %i prog->tempstringsbuf.cursize)\n", num, prog->tempstringsbuf.cursize);
			break;
		case invalidZoneOffsetFreed:
			vm::warn(prog, "vm::GetString: Invalid zone-string offset (%i has been freed)\n", num);
			break;
		case invalidZoneOffset:
			vm::warn(prog, "vm::GetString: Invalid zone-string offset (%i >= %i)\n", num, prog->numknownstrings);
			break;
		case invalidConstantOffset:
			vm::warn(prog, "vm::GetString: Invalid constant-string offset (%i >= %i prog->stringssize)\n", num, prog->stringssize);
			break;
		};
		return nullVMString;
	}
#define		mVMGetString(prog, _num, doOnError, preAssumes)														\
	({																										\
	__label__ end;																							\
	unsigned int num = _num;																				\
	const char* str;																						\
	auto onError = getStringError;																			\
	preAssumes();																							\
	if (unlikely(static_cast<int>(num) < 0))																\
		doOnError(onError(prog, num, negativeOffset));														\
	{																										\
		const int stringsSize = prog->stringssize;															\
		__assume(stringsSize > 0);																			\
		if (num < stringsSize)																				\
		{																									\
			__assume(static_cast<int>(static_cast<size_t>(num)) == num);									\
			str = prog->strings + num;																		\
			goto end;																						\
		}																									\
		if (static_cast<int>(num) <= stringsSize + prog->tempstringsbuf.maxsize)							\
		{																									\
			num = static_cast<int>(num) - stringsSize;														\
			if (likely(static_cast<int>(num) < prog->tempstringsbuf.cursize))								\
			{																								\
				str = reinterpret_cast<const char*>(prog->tempstringsbuf.data) + static_cast<int>(num); 	\
				goto end;																					\
			}																								\
			else																							\
				doOnError(onError(prog, num, invalidTempOffset));											\
		}																									\
	}																										\
	if (likely(num & PRVM_KNOWNSTRINGBASE))																	\
	{																										\
		num = static_cast<int>(num) - PRVM_KNOWNSTRINGBASE;													\
		if (likely(static_cast<int>(num) >= 0 && static_cast<int>(num) < prog->numknownstrings))			\
		{																									\
			__assume(static_cast<int>(static_cast<size_t>(num)) == num);									\
			const char* s = prog->knownstrings[num];														\
			if (unlikely(!s))																				\
				doOnError(onError(prog, num, invalidZoneOffsetFreed));										\
			str = s;																						\
			goto end;																						\
		}																									\
		else																								\
			doOnError(onError(prog, num, invalidZoneOffset));												\
	}																										\
	else																									\
		doOnError(onError(prog, num, invalidConstantOffset));												\
end:;																								\
		str;																								\
	})

/*
 * PRVM_GetString, rewritten to make it more inlineable
 *
 * all of the unlikely error conditions have been marked unlikely, and their error-handling code
 * has been moved to __noinline functions
 *
 */
[[gnu::used]] [[gnu::hot]]
__forceinline static const char *getString(Program prog, const unsigned int __num)
{
	#if 0
	auto onError = getStringError;
	if (unlikely(static_cast<int>(num) < 0))
		return onError(prog, num, negativeOffset);
	{
		const int stringsSize = prog->stringssize;

		__assume(stringsSize > 0);
		if (num < stringsSize)
		{
			__assume(static_cast<int>(static_cast<size_t>(num)) == num);

			return prog->strings + num;	// constant string from progs.dat
		}


		if (static_cast<int>(num) <= stringsSize + prog->tempstringsbuf.maxsize)
		{
			// tempstring returned by engine to QC (becomes invalid after returning to engine)
			num = static_cast<int>(num) - stringsSize;
			if (likely(static_cast<int>(num) < prog->tempstringsbuf.cursize))
				return reinterpret_cast<const char*>(prog->tempstringsbuf.data)
				+ static_cast<int>(num); //this static cast might be unnecessary and produces a sign extension to 64 bits on x86-64
			else
				return onError(prog, num, invalidTempOffset);
		}
	}

	register int knownStringBase = PRVM_KNOWNSTRINGBASE;

	if (likely(num & knownStringBase))
	{
		num = static_cast<int>(num) - knownStringBase;
		if (likely(static_cast<int>(num) >= 0 && static_cast<int>(num) < prog->numknownstrings))
		{
			__assume(static_cast<int>(static_cast<size_t>(num)) == num);
			const char* s = prog->knownstrings[num];
			if (unlikely(!s))
				return onError(prog, num, invalidZoneOffsetFreed);

			return s;
		}
		else
			return onError(prog, num, invalidZoneOffset);
	}
	else
		return onError(prog, num, invalidConstantOffset);

	#else
	#define mNullPreassume()
	#define mGetStringOnError(x)	return x;

	return mVMGetString(prog, __num, mGetStringOnError, mNullPreassume);

	#endif
}


[[gnu::used]] [[gnu::hot]]
ddef_t *findField (Program prog, const char *name)
{

	{
		const unsigned int numFieldDefs 	= prog->numfielddefs;
		ddef_t* RESTRICT const fieldDefs 	= prog->fielddefs;
		for (unsigned int i = 0; likely(i < numFieldDefs); i++)
		{
			ddef_t* RESTRICT const def = &fieldDefs[i];

			#define		mFindFieldOnError(x)	\
			{		\
				x;	\
				continue;	\
			}

			#define		mFindFieldPreAssumes()	\
					__assume(static_cast<int>(num) >= 0)
			const char* string = mVMGetString(prog, i,mFindFieldOnError, mFindFieldPreAssumes);
			if (unlikely(	!strcmp(	string, name)))
				return def;
		}
	}
	return nullptr;
}

[[gnu::used]] [[gnu::hot]]
ddef_t *findGlobal (Program prog, const char *name)
{
	{
		const unsigned int numGlobalDefs	= prog->numglobaldefs;
		ddef_t* RESTRICT const globalDefs	= prog->globaldefs;
		for (unsigned int i = 0; likely(i < numGlobalDefs); i++)
		{
			ddef_t* RESTRICT const def = &globalDefs[i];
			const char* defName = mVMGetString(prog, i,mFindFieldOnError, mFindFieldPreAssumes);
			if (unlikely(!strcmp(defName, name))	)
				return def;
		}
	}
	return nullptr;
}

[[gnu::used]] [[gnu::hot]]
mfunction_t *findFunction (Program prog, const char *name)
{
	{
		const unsigned int numFunctions				= prog->numfunctions;
		mfunction_t* RESTRICT const progFunctions	= prog->functions;
		for (unsigned int i = 0; likely(i < numFunctions); i++)
		{
			mfunction_t* RESTRICT const func = &progFunctions[i];
			const char* defName = mVMGetString(prog, i, mFindFieldOnError, mFindFieldPreAssumes);
			if (	unlikely(	!strcmp(defName, name)	)	)
				return func;
		}
	}
	return nullptr;
}


//============================================================================
// mempool handling

/*
	OOPS. this one is only called once. what a waste of time
*/
void alloc(Program prog)
{
	// reserve space for the null entity aka world
	// check bound of max_edicts

	const uint32 maxEdicts =
			prog->max_edicts = bound(1 + prog->reserved_edicts, prog->max_edicts, prog->limit_edicts);

	prog->num_edicts = bound(1 + prog->reserved_edicts, prog->num_edicts, static_cast<int>(maxEdicts));

	// edictprivate_size has to be min as big prvm_edict_private_t
	const uint32 edictPrivateSize =
	prog->edictprivate_size =
			max(
				prog->edictprivate_size,
				static_cast<int>(sizeof(prvm_edict_private_t))
			);

	{
		Pool* RESTRICT const progsPool = prog->progs_mempool;

		// alloc edicts
		prog->edicts 			= progsPool->alloc<prvm_edict_t>(prog->limit_edicts);

		// alloc edict private space
		prog->edictprivate 		= progsPool->alloc<uint8>(maxEdicts * edictPrivateSize);
		// alloc edict fields
		prog->edictsfields 		= progsPool->alloc<prvm_vec_t>(
					(prog->entityfieldsarea = prog->entityfields * maxEdicts)
				);

	}
	// set edict pointers
	{
		//dereference fields used in the loop
		Edict* RESTRICT const localEdicts 	= prog->edicts;
		uint8* localEdictPrivate			= reinterpret_cast<uint8*>(prog->edictprivate);

		const uint32 localEntityFields		= prog->entityfields;
		prvm_vec_t* localEdictsFields		= prog->edictsfields;

		for(uint32 i = 0; i < maxEdicts; i++)
		{
			localEdicts[i].priv.required =
					reinterpret_cast<prvm_edict_private_t *>(localEdictPrivate + i * edictPrivateSize);
			localEdicts[i].fields.fp = localEdictsFields + i * localEntityFields;
		}

	}
}

/*
===============
PRVM_MEM_IncreaseEdicts
===============
*/
void increaseEdicts(Program prog)
{
	if(prog->max_edicts >= prog->limit_edicts)
		return;

	prog->begin_increase_edicts(prog.getPtr());

	// increase edicts
	const size32 maxEdicts =
			prog->max_edicts = min(prog->max_edicts + 256, prog->limit_edicts);

	prog->entityfieldsarea = prog->entityfields * maxEdicts;
	const size32 edictPrivateSize = prog->edictprivate_size;
	{
		Pool* RESTRICT const progsPool = prog->progs_mempool;

		prog->edictsfields = progsPool->realloc< prvm_vec_t >(prog->edictsfields, prog->entityfieldsarea);
		prog->edictprivate = progsPool->realloc< void >(prog->edictprivate,
				maxEdicts * edictPrivateSize);
	}
	{
		//dereference fields used in the loop
		Edict* RESTRICT const localEdicts 	= prog->edicts;
		uint8* localEdictPrivate			= reinterpret_cast<uint8*>(prog->edictprivate);

		const size32 localEntityFields		= prog->entityfields;
		prvm_vec_t* localEdictsFields		= prog->edictsfields;

		//set e and v pointers
		for(size32 i = 0; i < maxEdicts; i++)
		{
			localEdicts[i].priv.required  =
					reinterpret_cast<prvm_edict_private_t *>
					(reinterpret_cast<uint8*>(localEdictPrivate) + i * edictPrivateSize);
			localEdicts[i].fields.fp = localEdictsFields + i * localEntityFields;
		}
	}
	prog->end_increase_edicts(prog.getPtr());
}

void clearEdict(Program prog, Edict *e)
{
	memset(e->fields.fp, 0, prog->entityfields * sizeof(prvm_vec_t));
	e->priv.required->free = false;

	// AK: Let the init_edict function determine if something needs to be initialized
	prog->init_edict(prog.getPtr(), e);
}


/*
=================
PRVM_ED_CanAlloc

Returns if this particular edict could get allocated by PRVM_ED_Alloc
=================
*/

bool canAllocEdict(Program prog, Edict *e)
{
	if(!e->priv.required->free)
		return false;
	//if(prvm_reuseedicts_always_allow == realtime)
	//	return true;
	if(realtime <= e->priv.required->freetime + 0.1)
		return false; // never allow reuse in same frame (causes networking trouble)
	if(e->priv.required->freetime < prog->starttime + 2)
		return true;
	if(realtime > e->priv.required->freetime + 1)
		return true;
	return false; // entity slot still blocked because the entity was freed less than one second ago
}

/*
=================
PRVM_ED_Alloc

Either finds a free edict, or allocates a new one.
Try to avoid reusing an entity that was recently freed, because it
can cause the client to think the entity morphed into something else
instead of being removed and recreated, which can cause interpolated
angles and bad trails.
=================
*/
Edict *allocEdict(Program prog)
{
	Edict* e;
	size32 i;
	Edict* RESTRICT const localEdicts = prog->edicts;
	{

		const size32 numEdicts = prog->num_edicts;
		for (i = prog->reserved_edicts + 1; i < numEdicts; i++)
		{
			e = &localEdicts[i];
			if(vm::canAllocEdict(prog, e))
			{
				vm::clearEdict (prog, e);
				e->priv.required->allocation_origin = nullptr;
				return e;
			}
		}
	}
	if (unlikely(i == prog->limit_edicts))
		prog->error_cmd("%s: allocEdict: no free edicts", prog->name);

	if (unlikely(++prog->num_edicts >= prog->max_edicts))
		vm::increaseEdicts(prog);

	e = &localEdicts[i];
	vm::clearEdict(prog, e);

	e->priv.required->allocation_origin = nullptr;

	return e;
}

void freeEdict(Program prog, Edict *ed)
{
	// dont delete the null entity (world) or reserved edicts
	if (unlikely(ed - prog->edicts <= prog->reserved_edicts))
		return;

	prog->free_edict(prog.getPtr(), ed);

	ed->priv.required->free = true;
	ed->priv.required->freetime = realtime;
	if(unlikely(ed->priv.required->allocation_origin != nullptr))
	{
		Mem_Free((char *)ed->priv.required->allocation_origin);
		ed->priv.required->allocation_origin = nullptr;
	}
}
}//namespace cloture::vm


using namespace vm;
using namespace math::vector;


extern CVar prvm_backtraceforwarnings;


/*
=================
VM_bprint

broadcast print to everyone on server

bprint(...[string])
=================
*/
void vm::bprint(Program prog, const char* msg)
{
	if(unlikely(!sv.active))
	{
		vm::warn(prog, "vm::bprint: game is not server(%s) !\n",prog->name);
		return;
	}
	SV_BroadcastPrint(msg);
}

/*
=================
VM_sprint (menu & client but only if server.active == true)

single print to a specific client

sprint(float clientnum,...[string])
=================
*/
void vm::sprint(Program prog, const ptrdiff_t clientnum, const char* s)
{

	if (unlikely(!sv.active  || clientnum < 0 || clientnum >= svs.maxclients || !svs.clients[clientnum].active))
	{
		vm::warn(prog, "vm::sprint: %s: invalid client or server is not active !\n", prog->name);
		return;
	}

	Client* client = &svs.clients[clientnum];
	if (!client->netconnection)
		return;

	MSG_WriteChar(&client->netconnection->message,svc_print);
	MSG_WriteString(&client->netconnection->message, s);
}

/*
=================
VM_centerprint

single print to the screen

centerprint(value)
=================
*/
void vm::centerprint(const Program prog, const char* msg)
{
	SCR_CenterPrint(msg);
}

/*
=================
VM_normalize

vector normalize(vector)
=================
*/
vector3D vm::normalize(const Program prog, const vector3D vec)
{

	double f = VectorLength2(vec);
	vector3D newvalue;
	if (f)
	{
		f = 1.0 / sqrt(f);
		VectorScale(vec, f, newvalue);
	}
	else
		VectorClear(newvalue);
	return newvalue;
}

/*
=================
VM_vlen

scalar vlen(vector)
=================
*/
float vm::vlen(const Program prog, vector3D v)
{
	return VectorLength(v);
}

/*
=================
VM_vectoyaw

float vectoyaw(vector)
=================
*/
float vm::vectoyaw(const Program prog, const vector3D vec)
{
	float	yaw;

	if (vec[1] == .0f && vec[0] == .0f)
		yaw = .0f;
	else
	{
		yaw = static_cast<int>(atan2(vec[1], vec[0]) * 180 / M_PI);
		if (yaw < .0f)
			yaw += 360.0f; //normalize
	}

	return yaw;
}


/*
=================
VM_vectoangles

vector vectoangles(vector[, vector])
=================
*/
vector3D vm::vectoangles(const Program prog, const vector3D parm0)
{
	vector3D result, forward;

	VectorCopy(parm0, forward);
	AnglesFromVectors(result, forward, nullptr, true);
	return result;
}

vector3D vm::vectoangles(const Program prog, const vector3D parm0, const vector3D parm1)
{
	vector3D result, forward, up;

	VectorCopy(parm0, forward);

	VectorCopy(parm1, up);
	AnglesFromVectors(result, forward, up, true);
	return result;
}
/*
=================
VM_random

Returns a number from 0<= num < 1

float random()
=================
*/
float vm::random(Program prog)
{
	return lhrandom(0, 1);
}

/*
=========
VM_localsound

localsound(string sample)
=========
*/
bool vm::localsound(const Program prog, const char* s)
{

	if(unlikely(!S_LocalSound (s)))
	{
		vm::warn(prog, "vm::localsound: Failed to play %s for %s !\n", s, prog->name);
		return false;
	}

	return true;
}

/*
=================
VM_break

break()
=================
*/
void vm::_break(Program prog)
{
	prog->error_cmd("%s: break statement", prog->name);
}

//============================================================================

/*
=================
VM_localcmd

Sends text over to the client's execution buffer

[localcmd (string, ...) or]
cmd (string, ...)
=================
*/
void vm::localcmd(const Program prog, const char* cmd)
{
	Cbuf_AddText(cmd);
}

static inline bool PRVM_Cvar_ReadOk(const char *string)
{
	CVar *cvar = CVar::find(string);
	return cvar != nullptr && (cvar->flags & CVAR_PRIVATE) == 0;
}

/*
=================
VM_cvar

float cvar (string)
=================
*/
float vm::cvar(const Program prog, const char* s)
{
	return PRVM_Cvar_ReadOk(s) ? CVar::variableValue(s) : .0f;
}

/*
=================
VM_cvar

float cvar_type (string)
float CVAR_TYPEFLAG_EXISTS = 1;
float CVAR_TYPEFLAG_SAVED = 2;
float CVAR_TYPEFLAG_PRIVATE = 4;
float CVAR_TYPEFLAG_ENGINE = 8;
float CVAR_TYPEFLAG_HASDESCRIPTION = 16;
float CVAR_TYPEFLAG_READONLY = 32;
=================
*/
decltype(CVar::flags) vm::cvar_type(const Program prog, const char* s)
{
	CVar *cvar = CVar::find(s);

	if(cvar == nullptr)
		return 0; // CVAR_TYPE_NONE
	return 1 | cvar->flags;
}


/*
=========
VM_spawn

entity spawn()
=========
*/

Edict* vm::spawn(Program prog)
{
	Edict	*ed = vm::allocEdict(prog);
	return ed;
}

/*
=========
VM_remove

remove(entity e)
=========
*/

void vm::remove(Program prog, Edict* e)
{
	if( unlikely(PRVM_NUM_FOR_EDICT(e) <= prog->reserved_edicts) )
	{
		if (unlikely(developer.integer > 0))
			vm::warn(prog, "vm::remove: tried to remove the null entity or a reserved entity!\n" );
	}
	else if( unlikely( e->priv.required->free ))
	{
		if (unlikely(developer.integer > 0))
			vm::warn(prog, "vm::remove: tried to remove an already freed entity!\n" );
	}
	else
		vm::freeEdict(prog, e);
}

/*
=========
VM_find

entity	find(entity start, .string field, string match)
=========
*/

Edict* vm::find(Program prog, Edict* start, const char* field, const char* match )
{
	Edict* RESTRICT const localEdicts = prog->edicts;

	const uint32 f = vm::findField(prog, field)->ofs;

	{
		uint32 e = vm::edictToIndex(prog, start);
		const uint32 numEdicts = prog->num_edicts;


		for (e++ ; e < numEdicts ; e++)
		{
			Edict* edict = &localEdicts[e];
			if (edict->priv.required->free)
				continue;
			/*
			 * so field (.string) is actually an index into e->fields.ip
			 * it gets the string, then compares it to match
			 * need to emulate that
			 */
			const char *t = vm::getString(prog, edict->fields.ip[f]);

			if (unlikely(!strcmp(t, match)))
				return edict;
		}
	}
	//return world
	return &localEdicts[0];
}

#if 0
/*
=========
VM_findfloat

  entity	findfloat(entity start, .float field, float match)
  entity	findentity(entity start, .entity field, entity match)
=========
*/
Edict* vm::findfloat(Program prog, Edict* start, const char* field, const float match)
{
	size32 e = PRVM_NUM_FOR_EDICT(start);
	const size32 f = vm::findField(prog, field);

	{
		const size32 numEdicts = prog->num_edicts;
		for (e++; e < numEdicts; e++)
		{
			Edict *RESTRICT const ed = PRVM_EDICT_NUM(e);
			if (ed->priv.required->free)
				continue;
			if (unlikely(PRVM_E_FLOAT(ed,f) == match))
				return ed;
		}
	}
	return &prog->edicts[0];
}

/*
=========
VM_findchain

entity	findchain(.string field, string match)
=========
*/
// chained search for strings in entity fields
// entity(.string field, string match) findchain = #402;
void VM_findchain(Program prog)
{
	int		i;
	int		f;
	const char	*s, *t;
	prvm_edict_t	*ent, *chain;
	int chainfield;

	VM_SAFEPARMCOUNTRANGE(2,3,VM_findchain);

	if(prog->argc == 3)
		chainfield = PRVM_G_INT(OFS_PARM2);
	else
		chainfield = prog->fieldoffsets.chain;
	if (chainfield < 0)
		prog->error_cmd("VM_findchain: %s doesnt have the specified chain field !", prog->name);

	chain = prog->edicts;

	f = PRVM_G_INT(OFS_PARM0);
	s = PRVM_G_STRING(OFS_PARM1);

	// LordHavoc: apparently BloodMage does a find(world, weaponmodel, "") and
	// expects it to find all the monsters, so we must be careful to support
	// searching for ""

	ent = PRVM_NEXT_EDICT(prog->edicts);
	for (i = 1;i < prog->num_edicts;i++, ent = PRVM_NEXT_EDICT(ent))
	{
		prog->xfunction->builtinsprofile++;
		if (ent->priv.required->free)
			continue;
		t = PRVM_E_STRING(ent,f);
		if (!t)
			t = "";
		if (strcmp(t,s))
			continue;

		PRVM_EDICTFIELDEDICT(ent,chainfield) = PRVM_NUM_FOR_EDICT(chain);
		chain = ent;
	}

	VM_RETURN_EDICT(chain);
}

/*
=========
VM_findchainfloat

entity	findchainfloat(.string field, float match)
entity	findchainentity(.string field, entity match)
=========
*/
// LordHavoc: chained search for float, int, and entity reference fields
// entity(.string field, float match) findchainfloat = #403;
void VM_findchainfloat(Program prog)
{
	int		i;
	int		f;
	float	s;
	prvm_edict_t	*ent, *chain;
	int chainfield;

	VM_SAFEPARMCOUNTRANGE(2, 3, VM_findchainfloat);

	if(prog->argc == 3)
		chainfield = PRVM_G_INT(OFS_PARM2);
	else
		chainfield = prog->fieldoffsets.chain;
	if (chainfield < 0)
		prog->error_cmd("VM_findchain: %s doesnt have the specified chain field !", prog->name);

	chain = (prvm_edict_t *)prog->edicts;

	f = PRVM_G_INT(OFS_PARM0);
	s = PRVM_G_FLOAT(OFS_PARM1);

	ent = PRVM_NEXT_EDICT(prog->edicts);
	for (i = 1;i < prog->num_edicts;i++, ent = PRVM_NEXT_EDICT(ent))
	{
		prog->xfunction->builtinsprofile++;
		if (ent->priv.required->free)
			continue;
		if (PRVM_E_FLOAT(ent,f) != s)
			continue;

		PRVM_EDICTFIELDEDICT(ent,chainfield) = PRVM_EDICT_TO_PROG(chain);
		chain = ent;
	}

	VM_RETURN_EDICT(chain);
}

/*
========================
VM_findflags

entity	findflags(entity start, .float field, float match)
========================
*/
// LordHavoc: search for flags in float fields
void VM_findflags(Program prog)
{
	prvm_int_t	e;
	prvm_int_t	f;
	prvm_int_t	s;
	prvm_edict_t	*ed;

	VM_SAFEPARMCOUNT(3, VM_findflags);


	e = PRVM_G_EDICTNUM(OFS_PARM0);
	f = PRVM_G_INT(OFS_PARM1);
	s = (prvm_int_t)PRVM_G_FLOAT(OFS_PARM2);

	for (e++ ; e < prog->num_edicts ; e++)
	{
		prog->xfunction->builtinsprofile++;
		ed = PRVM_EDICT_NUM(e);
		if (ed->priv.required->free)
			continue;
		if (!PRVM_E_FLOAT(ed,f))
			continue;
		if ((prvm_int_t)PRVM_E_FLOAT(ed,f) & s)
		{
			VM_RETURN_EDICT(ed);
			return;
		}
	}

	VM_RETURN_EDICT(prog->edicts);
}

/*
========================
VM_findchainflags

entity	findchainflags(.float field, float match)
========================
*/
// LordHavoc: chained search for flags in float fields
void vm::findChainFlags(Program prog)
{
	prvm_int_t		i;
	prvm_int_t		f;
	prvm_int_t		s;
	prvm_edict_t	*ent, *chain;
	int chainfield;

	VM_SAFEPARMCOUNTRANGE(2, 3, VM_findchainflags);

	if(prog->argc == 3)
		chainfield = PRVM_G_INT(OFS_PARM2);
	else
		chainfield = prog->fieldoffsets.chain;
	if (chainfield < 0)
		prog->error_cmd("VM_findchain: %s doesnt have the specified chain field !", prog->name);

	chain = (prvm_edict_t *)prog->edicts;

	f = PRVM_G_INT(OFS_PARM0);
	s = (prvm_int_t)PRVM_G_FLOAT(OFS_PARM1);

	ent = PRVM_NEXT_EDICT(prog->edicts);
	for (i = 1;i < prog->num_edicts;i++, ent = PRVM_NEXT_EDICT(ent))
	{
		prog->xfunction->builtinsprofile++;
		if (ent->priv.required->free)
			continue;
		if (!PRVM_E_FLOAT(ent,f))
			continue;
		if (!((prvm_int_t)PRVM_E_FLOAT(ent,f) & s))
			continue;

		PRVM_EDICTFIELDEDICT(ent,chainfield) = PRVM_EDICT_TO_PROG(chain);
		chain = ent;
	}

	VM_RETURN_EDICT(chain);
}

/*
=========
VM_precache_sound

string	precache_sound (string sample)
=========
*/
void vm::precacheSound(Program prog, const char* sample)
{
	if(likely(snd_initialized.integer) && unlikely(!S_PrecacheSound(sample, true, true)))
		vm::warn(prog, "vm::precacheSound: Failed to load %s for %s\n", sample, prog->name);
}

/*
=============
VM_nextent

entity	nextent(entity)
=============
*/
Edict* vm::nextEntity(Program prog, size32 i)
{
	const size32 numEdicts 				= prog->num_edicts;
	Edict* RESTRICT const localEdicts 	= prog->edicts;
	while (true)
	{
		if (unlikely(++i == numEdicts))
			return &localEdicts[0];
		Edict * RESTRICT const ent = &localEdicts[i];
		if (!ent->priv.required->free)
			return ent;
	}
}

//=============================================================================

/*
==============
VM_changelevel
server and menu

changelevel(string map)
==============
*/
void vm::changeLevel(const Program prog, const char* map)
{
	char vabuf[1024];

	if(unlikely(!sv.active))
	{
		vm::warn(prog, "VM_changelevel: game is not server (%s)\n", prog->name);
		return;
	}

	// make sure we don't issue two changelevels
	if (unlikely(svs.changelevel_issued)))
		return;
	svs.changelevel_issued = true;

	Cbuf_AddText(
			va(vabuf, sizeof(vabuf), "changelevel %s\n", map)
	);
}

/*
=================
VM_randomvec

Returns a vector of length < 1 and > 0

vector randomvec()
=================
*/
vector3D vm::randomVec(Program prog)
{
	vector3D temp;
	VectorRandom(temp);
	return temp;
}

//=============================================================================

/*
=========
VM_registercvar

float	registercvar (string name, string value[, float flags])
=========
*/
void VM_registercvar(Program prog)
{
	const char *name, *value;
	int	flags;

	VM_SAFEPARMCOUNTRANGE(2, 3, VM_registercvar);

	name = PRVM_G_STRING(OFS_PARM0);
	value = PRVM_G_STRING(OFS_PARM1);
	flags = prog->argc >= 3 ? (int)PRVM_G_FLOAT(OFS_PARM2) : 0;
	PRVM_G_FLOAT(OFS_RETURN) = 0;

	if(flags > CVAR_MAXFLAGSVAL)
		return;

// first check to see if it has already been defined
	if (Cvar_FindVar (name))
		return;

// check for overlap with a command
	if (Cmd_Exists (name))
	{
		VM_Warning(prog, "VM_registercvar: %s is a command\n", name);
		return;
	}

	Cvar_Get(name, value, flags, nullptr);

	PRVM_G_FLOAT(OFS_RETURN) = 1; // success
}


// KrimZon - DP_QC_ENTITYDATA
/*
=========
VM_numentityfields

float() numentityfields
Return the number of entity fields - NOT offsets
=========
*/
void VM_numentityfields(Program prog)
{
	PRVM_G_FLOAT(OFS_RETURN) = prog->numfielddefs;
}

// KrimZon - DP_QC_ENTITYDATA
/*
=========
VM_entityfieldname

string(float fieldnum) entityfieldname
Return name of the specified field as a string, or empty if the field is invalid (warning)
=========
*/
void VM_entityfieldname(Program prog)
{
	ddef_t *d;
	int i = (int)PRVM_G_FLOAT(OFS_PARM0);

	if (i < 0 || i >= prog->numfielddefs)
	{
		VM_Warning(prog, "VM_entityfieldname: %s: field index out of bounds\n", prog->name);
		PRVM_G_INT(OFS_RETURN) = PRVM_SetTempString(prog, "");
		return;
	}

	d = &prog->fielddefs[i];
	PRVM_G_INT(OFS_RETURN) = d->s_name; // presuming that s_name points to a string already
}

// KrimZon - DP_QC_ENTITYDATA
/*
=========
VM_entityfieldtype

float(float fieldnum) entityfieldtype
=========
*/
void VM_entityfieldtype(Program prog)
{
	ddef_t *d;
	int i = (int)PRVM_G_FLOAT(OFS_PARM0);

	if (i < 0 || i >= prog->numfielddefs)
	{
		VM_Warning(prog, "VM_entityfieldtype: %s: field index out of bounds\n", prog->name);
		PRVM_G_FLOAT(OFS_RETURN) = -1.0;
		return;
	}

	d = &prog->fielddefs[i];
	PRVM_G_FLOAT(OFS_RETURN) = (prvm_vec_t)d->type;
}

// KrimZon - DP_QC_ENTITYDATA
/*
=========
VM_getentityfieldstring

string(float fieldnum, entity ent) getentityfieldstring
=========
*/
void VM_getentityfieldstring(Program prog)
{
	// put the data into a string
	ddef_t *d;
	int type, j;
	prvm_eval_t *val;
	prvm_edict_t * ent;
	int i = (int)PRVM_G_FLOAT(OFS_PARM0);
	char valuebuf[MAX_INPUTLINE];

	if (i < 0 || i >= prog->numfielddefs)
	{
        VM_Warning(prog, "VM_entityfielddata: %s: field index out of bounds\n", prog->name);
		PRVM_G_INT(OFS_RETURN) = PRVM_SetTempString(prog, "");
		return;
	}

	d = &prog->fielddefs[i];

	// get the entity
	ent = PRVM_G_EDICT(OFS_PARM1);
	if(ent->priv.required->free)
	{
		PRVM_G_INT(OFS_RETURN) = PRVM_SetTempString(prog, "");
		VM_Warning(prog, "VM_entityfielddata: %s: entity %i is free !\n", prog->name, PRVM_NUM_FOR_EDICT(ent));
		return;
	}
	val = (prvm_eval_t *)(ent->fields.fp + d->ofs);

	// if it's 0 or blank, return an empty string
	type = d->type & ~DEF_SAVEGLOBAL;
	for (j=0 ; j<prvm_type_size[type] ; j++)
		if (val->ivector[j])
			break;
	if (j == prvm_type_size[type])
	{
		PRVM_G_INT(OFS_RETURN) = PRVM_SetTempString(prog, "");
		return;
	}

	PRVM_G_INT(OFS_RETURN) = PRVM_SetTempString(prog, PRVM_UglyValueString(prog, (etype_t)d->type, val, valuebuf, sizeof(valuebuf)));
}

// KrimZon - DP_QC_ENTITYDATA
/*
=========
VM_putentityfieldstring

float(float fieldnum, entity ent, string s) putentityfieldstring
=========
*/
void VM_putentityfieldstring(Program prog)
{
	ddef_t *d;
	prvm_edict_t * ent;
	int i = (int)PRVM_G_FLOAT(OFS_PARM0);

	if (i < 0 || i >= prog->numfielddefs)
	{
        VM_Warning(prog, "VM_entityfielddata: %s: field index out of bounds\n", prog->name);
		PRVM_G_FLOAT(OFS_RETURN) = 0.0f;
		return;
	}

	d = &prog->fielddefs[i];

	// get the entity
	ent = PRVM_G_EDICT(OFS_PARM1);
	if(ent->priv.required->free)
	{
		VM_Warning(prog, "VM_entityfielddata: %s: entity %i is free !\n", prog->name, PRVM_NUM_FOR_EDICT(ent));
		PRVM_G_FLOAT(OFS_RETURN) = 0.0f;
		return;
	}

	// parse the string into the value
	PRVM_G_FLOAT(OFS_RETURN) = ( PRVM_ED_ParseEpair(prog, ent, d, PRVM_G_STRING(OFS_PARM2), false) ) ? 1.0f : 0.0f;
}


// DRESK - Decolorized String
/*
=========
VM_strdecolorize

string	strdecolorize(string s)
=========
*/
// string (string s) strdecolorize = #472; // returns the passed in string with color codes stripped
void VM_strdecolorize(Program prog)
{
	char szNewString[VM_STRINGTEMP_LENGTH];
	const char *szString;

	// Prepare Strings
	VM_SAFEPARMCOUNT(1,VM_strdecolorize);
	szString = PRVM_G_STRING(OFS_PARM0);
	COM_StringDecolorize(szString, 0, szNewString, sizeof(szNewString), TRUE);
	PRVM_G_INT(OFS_RETURN) = PRVM_SetTempString(prog, szNewString);
}

// DRESK - String Length (not counting color codes)
/*
=========
VM_strlennocol

float	strlennocol(string s)
=========
*/
// float(string s) strlennocol = #471; // returns how many characters are in a string not including color codes
// For example, ^2Dresk returns a length of 5
void VM_strlennocol(Program prog)
{
	const char *szString;
	int nCnt;

	VM_SAFEPARMCOUNT(1,VM_strlennocol);

	szString = PRVM_G_STRING(OFS_PARM0);

	//nCnt = (int)COM_StringLengthNoColors(szString, 0, nullptr);
	nCnt = (int)u8_COM_StringLengthNoColors(szString, 0, nullptr);

	PRVM_G_FLOAT(OFS_RETURN) = nCnt;
}

// DRESK - String to Uppercase and Lowercase
/*
=========
VM_strtolower

string	strtolower(string s)
=========
*/
// string (string s) strtolower = #480; // returns passed in string in lowercase form
void VM_strtolower(Program prog)
{
	char szNewString[VM_STRINGTEMP_LENGTH];
	const char *szString;

	// Prepare Strings
	VM_SAFEPARMCOUNT(1,VM_strtolower);
	szString = PRVM_G_STRING(OFS_PARM0);

	COM_ToLowerString(szString, szNewString, sizeof(szNewString) );

	PRVM_G_INT(OFS_RETURN) = PRVM_SetTempString(prog, szNewString);
}

/*
=========
VM_strtoupper

string	strtoupper(string s)
=========
*/
// string (string s) strtoupper = #481; // returns passed in string in uppercase form
void VM_strtoupper(Program prog)
{
	char szNewString[VM_STRINGTEMP_LENGTH];
	const char *szString;

	// Prepare Strings
	VM_SAFEPARMCOUNT(1,VM_strtoupper);
	szString = PRVM_G_STRING(OFS_PARM0);

	COM_ToUpperString(szString, szNewString, sizeof(szNewString) );

	PRVM_G_INT(OFS_RETURN) = PRVM_SetTempString(prog, szNewString);
}


/*
=========
VM_strreplace

string(string search, string replace, string subject) strreplace = #484;
=========
*/
// replaces all occurrences of search with replace in the string subject, and returns the result
void VM_strreplace(Program prog)
{
	int i, j, si;
	const char *search, *replace, *subject;
	char string[VM_STRINGTEMP_LENGTH];
	int search_len, replace_len, subject_len;

	VM_SAFEPARMCOUNT(3,VM_strreplace);

	search = PRVM_G_STRING(OFS_PARM0);
	replace = PRVM_G_STRING(OFS_PARM1);
	subject = PRVM_G_STRING(OFS_PARM2);

	search_len = (int)strlen(search);
	replace_len = (int)strlen(replace);
	subject_len = (int)strlen(subject);

	si = 0;
	for (i = 0; i <= subject_len - search_len; i++)
	{
		for (j = 0; j < search_len; j++) // thus, i+j < subject_len
			if (subject[i+j] != search[j])
				break;
		if (j == search_len)
		{
			// NOTE: if search_len == 0, we always hit THIS case, and never the other
			// found it at offset 'i'
			for (j = 0; j < replace_len && si < (int)sizeof(string) - 1; j++)
				string[si++] = replace[j];
			if(search_len > 0)
			{
				i += search_len - 1;
			}
			else
			{
				// the above would subtract 1 from i... so we
				// don't do that, but instead output the next
				// char
				if (si < (int)sizeof(string) - 1)
					string[si++] = subject[i];
			}
		}
		else
		{
			// in THIS case, we know search_len > 0, thus i < subject_len
			// not found
			if (si < (int)sizeof(string) - 1)
				string[si++] = subject[i];
		}
	}
	// remaining chars (these cannot match)
	for (; i < subject_len; i++)
		if (si < (int)sizeof(string) - 1)
			string[si++] = subject[i];
	string[si] = '\0';

	PRVM_G_INT(OFS_RETURN) = PRVM_SetTempString(prog, string);
}

/*
=========
VM_strireplace

string(string search, string replace, string subject) strireplace = #485;
=========
*/
// case-insensitive version of strreplace
void VM_strireplace(Program prog)
{
	int i, j, si;
	const char *search, *replace, *subject;
	char string[VM_STRINGTEMP_LENGTH];
	int search_len, replace_len, subject_len;

	VM_SAFEPARMCOUNT(3,VM_strreplace);

	search = PRVM_G_STRING(OFS_PARM0);
	replace = PRVM_G_STRING(OFS_PARM1);
	subject = PRVM_G_STRING(OFS_PARM2);

	search_len = (int)strlen(search);
	replace_len = (int)strlen(replace);
	subject_len = (int)strlen(subject);

	si = 0;
	for (i = 0; i <= subject_len - search_len; i++)
	{
		for (j = 0; j < search_len; j++) // thus, i+j < subject_len
			if (tolower(subject[i+j]) != tolower(search[j]))
				break;
		if (j == search_len)
		{
			// NOTE: if search_len == 0, we always hit THIS case, and never the other
			// found it at offset 'i'
			for (j = 0; j < replace_len && si < (int)sizeof(string) - 1; j++)
				string[si++] = replace[j];
			if(search_len > 0)
			{
				i += search_len - 1;
			}
			else
			{
				// the above would subtract 1 from i... so we
				// don't do that, but instead output the next
				// char
				if (si < (int)sizeof(string) - 1)
					string[si++] = subject[i];
			}
		}
		else
		{
			// in THIS case, we know search_len > 0, thus i < subject_len
			// not found
			if (si < (int)sizeof(string) - 1)
				string[si++] = subject[i];
		}
	}
	// remaining chars (these cannot match)
	for (; i < subject_len; i++)
		if (si < (int)sizeof(string) - 1)
			string[si++] = subject[i];
	string[si] = '\0';

	PRVM_G_INT(OFS_RETURN) = PRVM_SetTempString(prog, string);
}

/*
=========
VM_stov

vector	stov(string s)
=========
*/
//vector(string s) stov = #117; // returns vector value from a string
void VM_stov(Program prog)
{
	char string[VM_STRINGTEMP_LENGTH];

	VM_SAFEPARMCOUNT(1,VM_stov);

	VM_VarString(prog, 0, string, sizeof(string));
	Math_atov(string, PRVM_G_VECTOR(OFS_RETURN));
}

/*
=========
VM_strzone

string	strzone(string s)
=========
*/
//string(string s, ...) strzone = #118; // makes a copy of a string into the string zone and returns it, this is often used to keep around a tempstring for longer periods of time (tempstrings are replaced often)
void VM_strzone(Program prog)
{
	char *out;
	char string[VM_STRINGTEMP_LENGTH];
	size_t alloclen;

	VM_SAFEPARMCOUNT(1,VM_strzone);

	VM_VarString(prog, 0, string, sizeof(string));
	alloclen = strlen(string) + 1;
	PRVM_G_INT(OFS_RETURN) = PRVM_AllocString(prog, alloclen, &out);
	memcpy(out, string, alloclen);
}

/*
=========
VM_strunzone

strunzone(string s)
=========
*/
//void(string s) strunzone = #119; // removes a copy of a string from the string zone (you can not use that string again or it may crash!!!)
void VM_strunzone(Program prog)
{
	VM_SAFEPARMCOUNT(1,VM_strunzone);
	PRVM_FreeString(prog, PRVM_G_INT(OFS_PARM0));
}

/*
=========
VM_command (used by client and menu)

clientcommand(float client, string s) (for client and menu)
=========
*/
//void(entity e, string s) clientcommand = #440; // executes a command string as if it came from the specified client
//this function originally written by KrimZon, made shorter by LordHavoc
void VM_clcommand (Program prog)
{
	client_t *temp_client;
	int i;

	VM_SAFEPARMCOUNT(2,VM_clcommand);

	i = (int)PRVM_G_FLOAT(OFS_PARM0);
	if (!sv.active  || i < 0 || i >= svs.maxclients || !svs.clients[i].active)
	{
		VM_Warning(prog, "VM_clientcommand: %s: invalid client/server is not active !\n", prog->name);
		return;
	}

	temp_client = host_client;
	host_client = svs.clients + i;
	Cmd_ExecuteString (PRVM_G_STRING(OFS_PARM1), src_client, true);
	host_client = temp_client;
}


/*
=========
VM_tokenize

float tokenize(string s)
=========
*/
//float(string s) tokenize = #441; // takes apart a string into individal words (access them with argv), returns how many
//this function originally written by KrimZon, made shorter by LordHavoc
//20040203: rewritten by LordHavoc (no longer uses allocations)
static int num_tokens = 0;
static int tokens[VM_STRINGTEMP_LENGTH / 2];
static int tokens_startpos[VM_STRINGTEMP_LENGTH / 2];
static int tokens_endpos[VM_STRINGTEMP_LENGTH / 2];
static char tokenize_string[VM_STRINGTEMP_LENGTH];
void VM_tokenize (Program prog)
{
	const char *p;

	VM_SAFEPARMCOUNT(1,VM_tokenize);

	strlcpy(tokenize_string, PRVM_G_STRING(OFS_PARM0), sizeof(tokenize_string));
	p = tokenize_string;

	num_tokens = 0;
	for(;;)
	{
		if (num_tokens >= (int)(sizeof(tokens)/sizeof(tokens[0])))
			break;

		// skip whitespace here to find token start pos
		while(*p && ISWHITESPACE(*p))
			++p;

		tokens_startpos[num_tokens] = p - tokenize_string;
		if(!COM_ParseToken_VM_Tokenize(&p, false))
			break;
		tokens_endpos[num_tokens] = p - tokenize_string;
		tokens[num_tokens] = PRVM_SetTempString(prog, com_token);
		++num_tokens;
	}

	PRVM_G_FLOAT(OFS_RETURN) = num_tokens;
}

//float(string s) tokenize = #514; // takes apart a string into individal words (access them with argv), returns how many
void VM_tokenize_console (Program prog)
{
	const char *p;

	VM_SAFEPARMCOUNT(1,VM_tokenize);

	strlcpy(tokenize_string, PRVM_G_STRING(OFS_PARM0), sizeof(tokenize_string));
	p = tokenize_string;

	num_tokens = 0;
	for(;;)
	{
		if (num_tokens >= (int)(sizeof(tokens)/sizeof(tokens[0])))
			break;

		// skip whitespace here to find token start pos
		while(*p && ISWHITESPACE(*p))
			++p;

		tokens_startpos[num_tokens] = p - tokenize_string;
		if(!COM_ParseToken_Console(&p))
			break;
		tokens_endpos[num_tokens] = p - tokenize_string;
		tokens[num_tokens] = PRVM_SetTempString(prog, com_token);
		++num_tokens;
	}

	PRVM_G_FLOAT(OFS_RETURN) = num_tokens;
}

/*
=========
VM_tokenizebyseparator

float tokenizebyseparator(string s, string separator1, ...)
=========
*/
//float(string s, string separator1, ...) tokenizebyseparator = #479; // takes apart a string into individal words (access them with argv), returns how many
//this function returns the token preceding each instance of a separator (of
//which there can be multiple), and the text following the last separator
//useful for parsing certain kinds of data like IP addresses
//example:
//numnumbers = tokenizebyseparator("10.1.2.3", ".");
//returns 4 and the tokens "10" "1" "2" "3".
void VM_tokenizebyseparator (Program prog)
{
	int j, k;
	int numseparators;
	int separatorlen[7];
	const char *separators[7];
	const char *p, *p0;
	const char *token;
	char tokentext[MAX_INPUTLINE];

	VM_SAFEPARMCOUNTRANGE(2, 8,VM_tokenizebyseparator);

	strlcpy(tokenize_string, PRVM_G_STRING(OFS_PARM0), sizeof(tokenize_string));
	p = tokenize_string;

	numseparators = 0;
	for (j = 1;j < prog->argc;j++)
	{
		// skip any blank separator strings
		const char *s = PRVM_G_STRING(OFS_PARM0+j*3);
		if (!s[0])
			continue;
		separators[numseparators] = s;
		separatorlen[numseparators] = (int)strlen(s);
		numseparators++;
	}

	num_tokens = 0;
	j = 0;

	while (num_tokens < (int)(sizeof(tokens)/sizeof(tokens[0])))
	{
		token = tokentext + j;
		tokens_startpos[num_tokens] = p - tokenize_string;
		p0 = p;
		while (*p)
		{
			for (k = 0;k < numseparators;k++)
			{
				if (!strncmp(p, separators[k], separatorlen[k]))
				{
					p += separatorlen[k];
					break;
				}
			}
			if (k < numseparators)
				break;
			if (j < (int)sizeof(tokentext)-1)
				tokentext[j++] = *p;
			p++;
			p0 = p;
		}
		tokens_endpos[num_tokens] = p0 - tokenize_string;
		if (j >= (int)sizeof(tokentext))
			break;
		tokentext[j++] = 0;
		tokens[num_tokens++] = PRVM_SetTempString(prog, token);
		if (!*p)
			break;
	}

	PRVM_G_FLOAT(OFS_RETURN) = num_tokens;
}

//string(float n) argv = #442; // returns a word from the tokenized string (returns nothing for an invalid index)
//this function originally written by KrimZon, made shorter by LordHavoc
void VM_argv (Program prog)
{
	int token_num;

	VM_SAFEPARMCOUNT(1,VM_argv);

	token_num = (int)PRVM_G_FLOAT(OFS_PARM0);

	if(token_num < 0)
		token_num += num_tokens;

	if (token_num >= 0 && token_num < num_tokens)
		PRVM_G_INT(OFS_RETURN) = tokens[token_num];
	else
		PRVM_G_INT(OFS_RETURN) = OFS_NULL;
}

//float(float n) argv_start_index = #515; // returns the start index of a token
void VM_argv_start_index (Program prog)
{
	int token_num;

	VM_SAFEPARMCOUNT(1,VM_argv);

	token_num = (int)PRVM_G_FLOAT(OFS_PARM0);

	if(token_num < 0)
		token_num += num_tokens;

	if (token_num >= 0 && token_num < num_tokens)
		PRVM_G_FLOAT(OFS_RETURN) = tokens_startpos[token_num];
	else
		PRVM_G_FLOAT(OFS_RETURN) = -1;
}

//float(float n) argv_end_index = #516; // returns the end index of a token
void VM_argv_end_index (Program prog)
{
	int token_num;

	VM_SAFEPARMCOUNT(1,VM_argv);

	token_num = (int)PRVM_G_FLOAT(OFS_PARM0);

	if(token_num < 0)
		token_num += num_tokens;

	if (token_num >= 0 && token_num < num_tokens)
		PRVM_G_FLOAT(OFS_RETURN) = tokens_endpos[token_num];
	else
		PRVM_G_FLOAT(OFS_RETURN) = -1;
}

/*
=========
VM_isserver

float	isserver()
=========
*/
void VM_isserver(Program prog)
{
	VM_SAFEPARMCOUNT(0,VM_serverstate);

	PRVM_G_FLOAT(OFS_RETURN) = sv.active;
}

/*
=========
VM_clientcount

float	clientcount()
=========
*/
void VM_clientcount(Program prog)
{
	VM_SAFEPARMCOUNT(0,VM_clientcount);

	PRVM_G_FLOAT(OFS_RETURN) = svs.maxclients;
}

/*
=========
VM_clientstate

float	clientstate()
=========
*/
void VM_clientstate(Program prog)
{
	VM_SAFEPARMCOUNT(0,VM_clientstate);


	switch( cls.state ) {
		case ca_uninitialized:
		case ca_dedicated:
			PRVM_G_FLOAT(OFS_RETURN) = 0;
			break;
		case ca_disconnected:
			PRVM_G_FLOAT(OFS_RETURN) = 1;
			break;
		case ca_connected:
			PRVM_G_FLOAT(OFS_RETURN) = 2;
			break;
		default:
			// should never be reached!
			break;
	}
}

/*
=========
VM_gettime

float	gettime(Program prog)
=========
*/
#ifdef CONFIG_CD
float CDAudio_GetPosition(void);
#endif
void VM_gettime(Program prog)
{
	int timer_index;

	VM_SAFEPARMCOUNTRANGE(0,1,VM_gettime);

	if(prog->argc == 0)
	{
		PRVM_G_FLOAT(OFS_RETURN) = (prvm_vec_t) realtime;
	}
	else
	{
		timer_index = (int) PRVM_G_FLOAT(OFS_PARM0);
		switch(timer_index)
		{
			case 0: // GETTIME_FRAMESTART
				PRVM_G_FLOAT(OFS_RETURN) = realtime;
				break;
			case 1: // GETTIME_REALTIME
				PRVM_G_FLOAT(OFS_RETURN) = Sys_DirtyTime();
				break;
			case 2: // GETTIME_HIRES
				PRVM_G_FLOAT(OFS_RETURN) = (Sys_DirtyTime() - host_dirtytime);
				break;
			case 3: // GETTIME_UPTIME
				PRVM_G_FLOAT(OFS_RETURN) = realtime;
				break;
#ifdef CONFIG_CD
			case 4: // GETTIME_CDTRACK
				PRVM_G_FLOAT(OFS_RETURN) = CDAudio_GetPosition();
				break;
#endif
			default:
				VM_Warning(prog, "VM_gettime: %s: unsupported timer specified, returning realtime\n", prog->name);
				PRVM_G_FLOAT(OFS_RETURN) = realtime;
				break;
		}
	}
}

/*
=========
VM_getsoundtime

float	getsoundtime(Program prog)
=========
*/

void VM_getsoundtime (Program prog)
{
	int entnum, entchannel;
	VM_SAFEPARMCOUNT(2,VM_getsoundtime);

	if (prog == SVVM_prog)
		entnum = PRVM_NUM_FOR_EDICT(PRVM_G_EDICT(OFS_PARM0));
	else if (prog == CLVM_prog)
		entnum = MAX_EDICTS + PRVM_NUM_FOR_EDICT(PRVM_G_EDICT(OFS_PARM0));
	else
	{
		VM_Warning(prog, "VM_getsoundtime: %s: not supported on this progs\n", prog->name);
		PRVM_G_FLOAT(OFS_RETURN) = -1;
		return;
	}
	entchannel = (int)PRVM_G_FLOAT(OFS_PARM1);
	entchannel = CHAN_USER2ENGINE(entchannel);
	if (!IS_CHAN(entchannel))
		VM_Warning(prog, "VM_getsoundtime: %s: bad channel %i\n", prog->name, entchannel);
	PRVM_G_FLOAT(OFS_RETURN) = (prvm_vec_t)S_GetEntChannelPosition(entnum, entchannel);
}

/*
=========
VM_GetSoundLen

string	soundlength (string sample)
=========
*/
void VM_soundlength (Program prog)
{
	const char *s;

	VM_SAFEPARMCOUNT(1, VM_soundlength);

	s = PRVM_G_STRING(OFS_PARM0);
	PRVM_G_FLOAT(OFS_RETURN) = S_SoundLength(s);
}

/*
=========
VM_loadfromdata

loadfromdata(string data)
=========
*/
void VM_loadfromdata(Program prog)
{
	VM_SAFEPARMCOUNT(1,VM_loadentsfromfile);

	PRVM_ED_LoadFromFile(prog, PRVM_G_STRING(OFS_PARM0));
}

/*
========================
VM_parseentitydata

parseentitydata(entity ent, string data)
========================
*/
void VM_parseentitydata(Program prog)
{
	prvm_edict_t *ent;
	const char *data;

	VM_SAFEPARMCOUNT(2, VM_parseentitydata);

	// get edict and test it
	ent = PRVM_G_EDICT(OFS_PARM0);
	if (ent->priv.required->free)
		prog->error_cmd("VM_parseentitydata: %s: Can only set already spawned entities (entity %i is free)!", prog->name, PRVM_NUM_FOR_EDICT(ent));

	data = PRVM_G_STRING(OFS_PARM1);

	// parse the opening brace
	if (!COM_ParseToken_Simple(&data, false, false, true) || com_token[0] != '{' )
		prog->error_cmd("VM_parseentitydata: %s: Couldn't parse entity data:\n%s", prog->name, data );

	PRVM_ED_ParseEdict (prog, data, ent);
}


//=============================================================================
// Draw builtins (client & menu)

/*
=========
VM_iscachedpic

float	iscachedpic(string pic)
=========
*/
void VM_iscachedpic(Program prog)
{
	VM_SAFEPARMCOUNT(1,VM_iscachedpic);

	// drawq hasnt such a function, thus always return true
	PRVM_G_FLOAT(OFS_RETURN) = false;
}

/*
=========
VM_precache_pic

string	precache_pic(string pic)
=========
*/
#define PRECACHE_PIC_FROMWAD 1 /* FTEQW, not supported here */
#define PRECACHE_PIC_NOTPERSISTENT 2
//#define PRECACHE_PIC_NOCLAMP 4
#define PRECACHE_PIC_MIPMAP 8
void VM_precache_pic(Program prog)
{
	const char	*s;
	int flags = 0;

	VM_SAFEPARMCOUNTRANGE(1, 2, VM_precache_pic);

	s = PRVM_G_STRING(OFS_PARM0);
	PRVM_G_INT(OFS_RETURN) = PRVM_G_INT(OFS_PARM0);
	VM_CheckEmptyString(prog, s);

	if(prog->argc >= 2)
	{
		int f = PRVM_G_FLOAT(OFS_PARM1);
		if(f & PRECACHE_PIC_NOTPERSISTENT)
			flags |= CACHEPICFLAG_NOTPERSISTENT;
		//if(f & PRECACHE_PIC_NOCLAMP)
		//	flags |= CACHEPICFLAG_NOCLAMP;
		if(f & PRECACHE_PIC_MIPMAP)
			flags |= CACHEPICFLAG_MIPMAP;
	}

	// AK Draw_CachePic is supposed to always return a valid pointer
	if( Draw_CachePic_Flags(s, flags)->tex == r_texture_notexture )
		PRVM_G_INT(OFS_RETURN) = OFS_NULL;
}

/*
=========
VM_freepic

freepic(string s)
=========
*/
void VM_freepic(Program prog)
{
	const char *s;

	VM_SAFEPARMCOUNT(1,VM_freepic);

	s = PRVM_G_STRING(OFS_PARM0);
	VM_CheckEmptyString(prog, s);

	Draw_FreePic(s);
}

void getdrawfontscale(Program prog, float *sx, float *sy)
{
	vec3_t v;
	*sx = *sy = 1;
	VectorCopy(PRVM_drawglobalvector(drawfontscale), v);
	if(VectorLength2(v) > 0)
	{
		*sx = v[0];
		*sy = v[1];
	}
}

static dp_font_t *getdrawfont(Program prog)
{
	int f = (int) PRVM_drawglobalfloat(drawfont);
	if(f < 0 || f >= dp_fonts.maxsize)
		return FONT_DEFAULT;
	return &dp_fonts.f[f];
}

/*
=========
VM_drawcharacter

float	drawcharacter(vector position, float character, vector scale, vector rgb, float alpha, float flag)
=========
*/
void VM_drawcharacter(Program prog)
{
	prvm_vec_t *pos,*scale,*rgb;
	char   character;
	int flag;
	float sx, sy;
	VM_SAFEPARMCOUNT(6,VM_drawcharacter);

	character = (char) PRVM_G_FLOAT(OFS_PARM1);
	if(character == 0)
	{
		PRVM_G_FLOAT(OFS_RETURN) = -1;
		VM_Warning(prog, "VM_drawcharacter: %s passed null character !\n",prog->name);
		return;
	}

	pos = PRVM_G_VECTOR(OFS_PARM0);
	scale = PRVM_G_VECTOR(OFS_PARM2);
	rgb = PRVM_G_VECTOR(OFS_PARM3);
	flag = (int)PRVM_G_FLOAT(OFS_PARM5);

	if(flag < DRAWFLAG_NORMAL || flag >=DRAWFLAG_NUMFLAGS)
	{
		PRVM_G_FLOAT(OFS_RETURN) = -2;
		VM_Warning(prog, "VM_drawcharacter: %s: wrong DRAWFLAG %i !\n",prog->name,flag);
		return;
	}

	if(pos[2] || scale[2])
		VM_Warning(prog, "VM_drawcharacter: z value%c from %s discarded\n",(pos[2] && scale[2]) ? 's' : 0,((pos[2] && scale[2]) ? "pos and scale" : (pos[2] ? "pos" : "scale")));

	if(!scale[0] || !scale[1])
	{
		PRVM_G_FLOAT(OFS_RETURN) = -3;
		VM_Warning(prog, "VM_drawcharacter: scale %s is null !\n", (scale[0] == 0) ? ((scale[1] == 0) ? "x and y" : "x") : "y");
		return;
	}

	getdrawfontscale(prog, &sx, &sy);
	DrawQ_String_Scale(pos[0], pos[1], &character, 1, scale[0], scale[1], sx, sy, rgb[0], rgb[1], rgb[2], PRVM_G_FLOAT(OFS_PARM4), flag, nullptr, true, getdrawfont(prog));
	PRVM_G_FLOAT(OFS_RETURN) = 1;
}

/*
=========
VM_drawstring

float	drawstring(vector position, string text, vector scale, vector rgb, float alpha[, float flag])
=========
*/
void VM_drawstring(Program prog)
{
	prvm_vec_t *pos,*scale,*rgb;
	const char  *string;
	int flag = 0;
	float sx, sy;
	VM_SAFEPARMCOUNTRANGE(5,6,VM_drawstring);

	string = PRVM_G_STRING(OFS_PARM1);
	pos = PRVM_G_VECTOR(OFS_PARM0);
	scale = PRVM_G_VECTOR(OFS_PARM2);
	rgb = PRVM_G_VECTOR(OFS_PARM3);
	if (prog->argc >= 6)
		flag = (int)PRVM_G_FLOAT(OFS_PARM5);

	if(flag < DRAWFLAG_NORMAL || flag >=DRAWFLAG_NUMFLAGS)
	{
		PRVM_G_FLOAT(OFS_RETURN) = -2;
		VM_Warning(prog, "VM_drawstring: %s: wrong DRAWFLAG %i !\n",prog->name,flag);
		return;
	}

	if(!scale[0] || !scale[1])
	{
		PRVM_G_FLOAT(OFS_RETURN) = -3;
		VM_Warning(prog, "VM_drawstring: scale %s is null !\n", (scale[0] == 0) ? ((scale[1] == 0) ? "x and y" : "x") : "y");
		return;
	}

	if(pos[2] || scale[2])
		VM_Warning(prog, "VM_drawstring: z value%s from %s discarded\n",(pos[2] && scale[2]) ? "s" : " ",((pos[2] && scale[2]) ? "pos and scale" : (pos[2] ? "pos" : "scale")));

	getdrawfontscale(prog, &sx, &sy);
	DrawQ_String_Scale(pos[0], pos[1], string, 0, scale[0], scale[1], sx, sy, rgb[0], rgb[1], rgb[2], PRVM_G_FLOAT(OFS_PARM4), flag, nullptr, true, getdrawfont(prog));
	//Font_DrawString(pos[0], pos[1], string, 0, scale[0], scale[1], rgb[0], rgb[1], rgb[2], PRVM_G_FLOAT(OFS_PARM4), flag, nullptr, true);
	PRVM_G_FLOAT(OFS_RETURN) = 1;
}

/*
=========
VM_drawcolorcodedstring

float	drawcolorcodedstring(vector position, string text, vector scale, float alpha, float flag)
/
float	drawcolorcodedstring(vector position, string text, vector scale, vector rgb, float alpha, float flag)
=========
*/
void VM_drawcolorcodedstring(Program prog)
{
	prvm_vec_t *pos, *scale;
	const char  *string;
	int flag;
	vec3_t rgb;
	float sx, sy, alpha;

	VM_SAFEPARMCOUNTRANGE(5,6,VM_drawcolorcodedstring);

	if (prog->argc == 6) // full 6 parms, like normal drawstring
	{
		pos = PRVM_G_VECTOR(OFS_PARM0);
		string = PRVM_G_STRING(OFS_PARM1);
		scale = PRVM_G_VECTOR(OFS_PARM2);
		VectorCopy(PRVM_G_VECTOR(OFS_PARM3), rgb);
		alpha = PRVM_G_FLOAT(OFS_PARM4);
		flag = (int)PRVM_G_FLOAT(OFS_PARM5);
	}
	else
	{
		pos = PRVM_G_VECTOR(OFS_PARM0);
		string = PRVM_G_STRING(OFS_PARM1);
		scale = PRVM_G_VECTOR(OFS_PARM2);
		rgb[0] = 1.0;
		rgb[1] = 1.0;
		rgb[2] = 1.0;
		alpha = PRVM_G_FLOAT(OFS_PARM3);
		flag = (int)PRVM_G_FLOAT(OFS_PARM4);
	}

	if(flag < DRAWFLAG_NORMAL || flag >= DRAWFLAG_NUMFLAGS)
	{
		PRVM_G_FLOAT(OFS_RETURN) = -2;
		VM_Warning(prog, "VM_drawcolorcodedstring: %s: wrong DRAWFLAG %i !\n",prog->name,flag);
		return;
	}

	if(!scale[0] || !scale[1])
	{
		PRVM_G_FLOAT(OFS_RETURN) = -3;
		VM_Warning(prog, "VM_drawcolorcodedstring: scale %s is null !\n", (scale[0] == 0) ? ((scale[1] == 0) ? "x and y" : "x") : "y");
		return;
	}

	if(pos[2] || scale[2])
		VM_Warning(prog, "VM_drawcolorcodedstring: z value%s from %s discarded\n",(pos[2] && scale[2]) ? "s" : " ",((pos[2] && scale[2]) ? "pos and scale" : (pos[2] ? "pos" : "scale")));

	getdrawfontscale(prog, &sx, &sy);
	DrawQ_String_Scale(pos[0], pos[1], string, 0, scale[0], scale[1], sx, sy, rgb[0], rgb[1], rgb[2], alpha, flag, nullptr, false, getdrawfont(prog));
	if (prog->argc == 6) // also return vector of last color
		VectorCopy(DrawQ_Color, PRVM_G_VECTOR(OFS_RETURN));
	else
		PRVM_G_FLOAT(OFS_RETURN) = 1;
}
/*
=========
VM_stringwidth

float	stringwidth(string text, float allowColorCodes, float size)
=========
*/
void VM_stringwidth(Program prog)
{
	const char  *string;
	vec2_t szv;
	float mult; // sz is intended font size so we can later add freetype support, mult is font size multiplier in pixels per character cell
	int colors;
	float sx, sy;
	size_t maxlen = 0;
	VM_SAFEPARMCOUNTRANGE(2,3,VM_drawstring);

	getdrawfontscale(prog, &sx, &sy);
	if(prog->argc == 3)
	{
		Vector2Copy(PRVM_G_VECTOR(OFS_PARM2), szv);
		mult = 1;
	}
	else
	{
		// we want the width for 8x8 font size, divided by 8
		Vector2Set(szv, 8, 8);
		mult = 0.125;
		// to make sure snapping is turned off, ALWAYS use a nontrivial scale in this case
		if(sx >= 0.9 && sx <= 1.1)
		{
			mult *= 2;
			sx /= 2;
			sy /= 2;
		}
	}

	string = PRVM_G_STRING(OFS_PARM0);
	colors = (int)PRVM_G_FLOAT(OFS_PARM1);

	PRVM_G_FLOAT(OFS_RETURN) = DrawQ_TextWidth_UntilWidth_TrackColors_Scale(string, &maxlen, szv[0], szv[1], sx, sy, nullptr, !colors, getdrawfont(prog), 1000000000) * mult;
/*
	if(prog->argc == 3)
	{
		mult = sz = PRVM_G_FLOAT(OFS_PARM2);
	}
	else
	{
		sz = 8;
		mult = 1;
	}

	string = PRVM_G_STRING(OFS_PARM0);
	colors = (int)PRVM_G_FLOAT(OFS_PARM1);

	PRVM_G_FLOAT(OFS_RETURN) = DrawQ_TextWidth(string, 0, !colors, getdrawfont()) * mult; // 1x1 characters, don't actually draw
*/
}

/*
=========
VM_findfont

float findfont(string s)
=========
*/

static float getdrawfontnum(const char *fontname)
{
	int i;

	for(i = 0; i < dp_fonts.maxsize; ++i)
		if(!strcmp(dp_fonts.f[i].title, fontname))
			return i;
	return -1;
}

void VM_findfont(Program prog)
{
	VM_SAFEPARMCOUNT(1,VM_findfont);
	PRVM_G_FLOAT(OFS_RETURN) = getdrawfontnum(PRVM_G_STRING(OFS_PARM0));
}

/*
=========
VM_loadfont

float loadfont(string fontname, string fontmaps, string sizes, float slot)
=========
*/

void VM_loadfont(Program prog)
{
	const char *fontname, *filelist, *sizes, *c, *cm;
	char mainfont[MAX_QPATH];
	int i, numsizes;
	float sz, scale, voffset;
	dp_font_t *f;

	VM_SAFEPARMCOUNTRANGE(3,6,VM_loadfont);

	fontname = PRVM_G_STRING(OFS_PARM0);
	if (!fontname[0])
		fontname = "default";

	filelist = PRVM_G_STRING(OFS_PARM1);
	if (!filelist[0])
		filelist = "gfx/conchars";

	sizes = PRVM_G_STRING(OFS_PARM2);
	if (!sizes[0])
		sizes = "10";

	// find a font
	f = nullptr;
	if (prog->argc >= 4)
	{
		i = PRVM_G_FLOAT(OFS_PARM3);
		if (i >= 0 && i < dp_fonts.maxsize)
		{
			f = &dp_fonts.f[i];
			strlcpy(f->title, fontname, sizeof(f->title)); // replace name
		}
	}
	if (!f)
		f = FindFont(fontname, true);
	if (!f)
	{
		PRVM_G_FLOAT(OFS_RETURN) = -1;
		return; // something go wrong
	}

	memset(f->fallbacks, 0, sizeof(f->fallbacks));
	memset(f->fallback_faces, 0, sizeof(f->fallback_faces));

	// first font is handled "normally"
	c = strchr(filelist, ':');
	cm = strchr(filelist, ',');
	if(c && (!cm || c < cm))
		f->req_face = atoi(c+1);
	else
	{
		f->req_face = 0;
		c = cm;
	}
	if(!c || (c - filelist) > MAX_QPATH)
		strlcpy(mainfont, filelist, sizeof(mainfont));
	else
	{
		memcpy(mainfont, filelist, c - filelist);
		mainfont[c - filelist] = 0;
	}

	// handle fallbacks
	for(i = 0; i < MAX_FONT_FALLBACKS; ++i)
	{
		c = strchr(filelist, ',');
		if(!c)
			break;
		filelist = c + 1;
		if(!*filelist)
			break;
		c = strchr(filelist, ':');
		cm = strchr(filelist, ',');
		if(c && (!cm || c < cm))
			f->fallback_faces[i] = atoi(c+1);
		else
		{
			f->fallback_faces[i] = 0; // f->req_face; could make it stick to the default-font's face index
			c = cm;
		}
		if(!c || (c-filelist) > MAX_QPATH)
		{
			strlcpy(f->fallbacks[i], filelist, sizeof(mainfont));
		}
		else
		{
			memcpy(f->fallbacks[i], filelist, c - filelist);
			f->fallbacks[i][c - filelist] = 0;
		}
	}

	// handle sizes
	for(i = 0; i < MAX_FONT_SIZES; ++i)
		f->req_sizes[i] = -1;
	for (numsizes = 0,c = sizes;;)
	{
		if (!COM_ParseToken_VM_Tokenize(&c, 0))
			break;
		sz = atof(com_token);
		// detect crap size
		if (sz < 0.001f || sz > 1000.0f)
		{
			VM_Warning(prog, "VM_loadfont: crap size %s", com_token);
			continue;
		}
		// check overflow
		if (numsizes == MAX_FONT_SIZES)
		{
			VM_Warning(prog, "VM_loadfont: MAX_FONT_SIZES = %i exceeded", MAX_FONT_SIZES);
			break;
		}
		f->req_sizes[numsizes] = sz;
		numsizes++;
	}

	// additional scale/hoffset parms
	scale = 1;
	voffset = 0;
	if (prog->argc >= 5)
	{
		scale = PRVM_G_FLOAT(OFS_PARM4);
		if (scale <= 0)
			scale = 1;
	}
	if (prog->argc >= 6)
		voffset = PRVM_G_FLOAT(OFS_PARM5);

	// load
	LoadFont(true, mainfont, f, scale, voffset);

	// return index of loaded font
	PRVM_G_FLOAT(OFS_RETURN) = (f - dp_fonts.f);
}

/*
=========
VM_drawpic

float	drawpic(vector position, string pic, vector size, vector rgb, float alpha, float flag)
=========
*/
void VM_drawpic(Program prog)
{
	const char *picname;
	prvm_vec_t *size, *pos, *rgb;
	int flag = 0;

	VM_SAFEPARMCOUNTRANGE(5,6,VM_drawpic);

	picname = PRVM_G_STRING(OFS_PARM1);
	VM_CheckEmptyString(prog, picname);

	// is pic cached ? no function yet for that
	if(!1)
	{
		PRVM_G_FLOAT(OFS_RETURN) = -4;
		VM_Warning(prog, "VM_drawpic: %s: %s not cached !\n", prog->name, picname);
		return;
	}

	pos = PRVM_G_VECTOR(OFS_PARM0);
	size = PRVM_G_VECTOR(OFS_PARM2);
	rgb = PRVM_G_VECTOR(OFS_PARM3);
	if (prog->argc >= 6)
		flag = (int) PRVM_G_FLOAT(OFS_PARM5);

	if(flag < DRAWFLAG_NORMAL || flag >=DRAWFLAG_NUMFLAGS)
	{
		PRVM_G_FLOAT(OFS_RETURN) = -2;
		VM_Warning(prog, "VM_drawpic: %s: wrong DRAWFLAG %i !\n",prog->name,flag);
		return;
	}

	if(pos[2] || size[2])
		VM_Warning(prog, "VM_drawpic: z value%s from %s discarded\n",(pos[2] && size[2]) ? "s" : " ",((pos[2] && size[2]) ? "pos and size" : (pos[2] ? "pos" : "size")));

	DrawQ_Pic(pos[0], pos[1], Draw_CachePic_Flags (picname, CACHEPICFLAG_NOTPERSISTENT), size[0], size[1], rgb[0], rgb[1], rgb[2], PRVM_G_FLOAT(OFS_PARM4), flag);
	PRVM_G_FLOAT(OFS_RETURN) = 1;
}
/*
=========
VM_drawrotpic

float	drawrotpic(vector position, string pic, vector size, vector org, float angle, vector rgb, float alpha, float flag)
=========
*/
void VM_drawrotpic(Program prog)
{
	const char *picname;
	prvm_vec_t *size, *pos, *org, *rgb;
	int flag;

	VM_SAFEPARMCOUNT(8,VM_drawrotpic);

	picname = PRVM_G_STRING(OFS_PARM1);
	VM_CheckEmptyString(prog, picname);

	// is pic cached ? no function yet for that
	if(!1)
	{
		PRVM_G_FLOAT(OFS_RETURN) = -4;
		VM_Warning(prog, "VM_drawrotpic: %s: %s not cached !\n", prog->name, picname);
		return;
	}

	pos = PRVM_G_VECTOR(OFS_PARM0);
	size = PRVM_G_VECTOR(OFS_PARM2);
	org = PRVM_G_VECTOR(OFS_PARM3);
	rgb = PRVM_G_VECTOR(OFS_PARM5);
	flag = (int) PRVM_G_FLOAT(OFS_PARM7);

	if(flag < DRAWFLAG_NORMAL || flag >=DRAWFLAG_NUMFLAGS)
	{
		PRVM_G_FLOAT(OFS_RETURN) = -2;
		VM_Warning(prog, "VM_drawrotpic: %s: wrong DRAWFLAG %i !\n",prog->name,flag);
		return;
	}

	if(pos[2] || size[2] || org[2])
		VM_Warning(prog, "VM_drawrotpic: z value from pos/size/org discarded\n");

	DrawQ_RotPic(pos[0], pos[1], Draw_CachePic_Flags(picname, CACHEPICFLAG_NOTPERSISTENT), size[0], size[1], org[0], org[1], PRVM_G_FLOAT(OFS_PARM4), rgb[0], rgb[1], rgb[2], PRVM_G_FLOAT(OFS_PARM6), flag);
	PRVM_G_FLOAT(OFS_RETURN) = 1;
}
/*
=========
VM_drawsubpic

float	drawsubpic(vector position, vector size, string pic, vector srcPos, vector srcSize, vector rgb, float alpha, float flag)

=========
*/
void VM_drawsubpic(Program prog)
{
	const char *picname;
	prvm_vec_t *size, *pos, *rgb, *srcPos, *srcSize, alpha;
	int flag;

	VM_SAFEPARMCOUNT(8,VM_drawsubpic);

	picname = PRVM_G_STRING(OFS_PARM2);
	VM_CheckEmptyString(prog, picname);

	// is pic cached ? no function yet for that
	if(!1)
	{
		PRVM_G_FLOAT(OFS_RETURN) = -4;
		VM_Warning(prog, "VM_drawsubpic: %s: %s not cached !\n", prog->name, picname);
		return;
	}

	pos = PRVM_G_VECTOR(OFS_PARM0);
	size = PRVM_G_VECTOR(OFS_PARM1);
	srcPos = PRVM_G_VECTOR(OFS_PARM3);
	srcSize = PRVM_G_VECTOR(OFS_PARM4);
	rgb = PRVM_G_VECTOR(OFS_PARM5);
	alpha = PRVM_G_FLOAT(OFS_PARM6);
	flag = (int) PRVM_G_FLOAT(OFS_PARM7);

	if(flag < DRAWFLAG_NORMAL || flag >=DRAWFLAG_NUMFLAGS)
	{
		PRVM_G_FLOAT(OFS_RETURN) = -2;
		VM_Warning(prog, "VM_drawsubpic: %s: wrong DRAWFLAG %i !\n",prog->name,flag);
		return;
	}

	if(pos[2] || size[2])
		VM_Warning(prog, "VM_drawsubpic: z value%s from %s discarded\n",(pos[2] && size[2]) ? "s" : " ",((pos[2] && size[2]) ? "pos and size" : (pos[2] ? "pos" : "size")));

	DrawQ_SuperPic(pos[0], pos[1], Draw_CachePic_Flags (picname, CACHEPICFLAG_NOTPERSISTENT),
		size[0], size[1],
		srcPos[0],              srcPos[1],              rgb[0], rgb[1], rgb[2], alpha,
		srcPos[0] + srcSize[0], srcPos[1],              rgb[0], rgb[1], rgb[2], alpha,
		srcPos[0],              srcPos[1] + srcSize[1], rgb[0], rgb[1], rgb[2], alpha,
		srcPos[0] + srcSize[0], srcPos[1] + srcSize[1], rgb[0], rgb[1], rgb[2], alpha,
		flag);
	PRVM_G_FLOAT(OFS_RETURN) = 1;
}

/*
=========
VM_drawfill

float drawfill(vector position, vector size, vector rgb, float alpha, float flag)
=========
*/
void VM_drawfill(Program prog)
{
	prvm_vec_t *size, *pos, *rgb;
	int flag;

	VM_SAFEPARMCOUNT(5,VM_drawfill);


	pos = PRVM_G_VECTOR(OFS_PARM0);
	size = PRVM_G_VECTOR(OFS_PARM1);
	rgb = PRVM_G_VECTOR(OFS_PARM2);
	flag = (int) PRVM_G_FLOAT(OFS_PARM4);

	if(flag < DRAWFLAG_NORMAL || flag >=DRAWFLAG_NUMFLAGS)
	{
		PRVM_G_FLOAT(OFS_RETURN) = -2;
		VM_Warning(prog, "VM_drawfill: %s: wrong DRAWFLAG %i !\n",prog->name,flag);
		return;
	}

	if(pos[2] || size[2])
		VM_Warning(prog, "VM_drawfill: z value%s from %s discarded\n",(pos[2] && size[2]) ? "s" : " ",((pos[2] && size[2]) ? "pos and size" : (pos[2] ? "pos" : "size")));

	DrawQ_Fill(pos[0], pos[1], size[0], size[1], rgb[0], rgb[1], rgb[2], PRVM_G_FLOAT(OFS_PARM3), flag);
	PRVM_G_FLOAT(OFS_RETURN) = 1;
}

/*
=========
VM_drawsetcliparea

drawsetcliparea(float x, float y, float width, float height)
=========
*/
void VM_drawsetcliparea(Program prog)
{
	float x,y,w,h;
	VM_SAFEPARMCOUNT(4,VM_drawsetcliparea);

	x = bound(0, PRVM_G_FLOAT(OFS_PARM0), vid_conwidth.integer);
	y = bound(0, PRVM_G_FLOAT(OFS_PARM1), vid_conheight.integer);
	w = bound(0, PRVM_G_FLOAT(OFS_PARM2) + PRVM_G_FLOAT(OFS_PARM0) - x, (vid_conwidth.integer  - x));
	h = bound(0, PRVM_G_FLOAT(OFS_PARM3) + PRVM_G_FLOAT(OFS_PARM1) - y, (vid_conheight.integer - y));

	DrawQ_SetClipArea(x, y, w, h);
}

/*
=========
VM_drawresetcliparea

drawresetcliparea()
=========
*/
void VM_drawresetcliparea(Program prog)
{
	VM_SAFEPARMCOUNT(0,VM_drawresetcliparea);

	DrawQ_ResetClipArea();
}

/*
=========
VM_getimagesize

vector	getimagesize(string pic)
=========
*/
void VM_getimagesize(Program prog)
{
	const char *p;
	cachepic_t *pic;

	VM_SAFEPARMCOUNT(1,VM_getimagesize);

	p = PRVM_G_STRING(OFS_PARM0);
	VM_CheckEmptyString(prog, p);

	pic = Draw_CachePic_Flags (p, CACHEPICFLAG_NOTPERSISTENT);
	if( pic->tex == r_texture_notexture )
	{
		PRVM_G_VECTOR(OFS_RETURN)[0] = 0;
		PRVM_G_VECTOR(OFS_RETURN)[1] = 0;
	}
	else
	{
		PRVM_G_VECTOR(OFS_RETURN)[0] = pic->width;
		PRVM_G_VECTOR(OFS_RETURN)[1] = pic->height;
	}
	PRVM_G_VECTOR(OFS_RETURN)[2] = 0;
}

/*
=========
VM_keynumtostring

string keynumtostring(float keynum)
=========
*/
void VM_keynumtostring (Program prog)
{
	char tinystr[2];
	VM_SAFEPARMCOUNT(1, VM_keynumtostring);

	PRVM_G_INT(OFS_RETURN) = PRVM_SetTempString(prog, Key_KeynumToString((int)PRVM_G_FLOAT(OFS_PARM0), tinystr, sizeof(tinystr)));
}

/*
=========
VM_findkeysforcommand

string	findkeysforcommand(string command, float bindmap)

the returned string is an altstring
=========
*/
#define FKFC_NUMKEYS 5
void M_FindKeysForCommand(const char *command, int *keys);
void VM_findkeysforcommand(Program prog)
{
	const char *cmd;
	char ret[VM_STRINGTEMP_LENGTH];
	int keys[FKFC_NUMKEYS];
	int i;
	int bindmap;
	char vabuf[1024];

	VM_SAFEPARMCOUNTRANGE(1, 2, VM_findkeysforcommand);

	cmd = PRVM_G_STRING(OFS_PARM0);
	if(prog->argc == 2)
		bindmap = bound(-1, PRVM_G_FLOAT(OFS_PARM1), MAX_BINDMAPS-1);
	else
		bindmap = 0; // consistent to "bind"

	VM_CheckEmptyString(prog, cmd);

	Key_FindKeysForCommand(cmd, keys, FKFC_NUMKEYS, bindmap);

	ret[0] = 0;
	for(i = 0; i < FKFC_NUMKEYS; i++)
		strlcat(ret, va(vabuf, sizeof(vabuf), " \'%i\'", keys[i]), sizeof(ret));

	PRVM_G_INT(OFS_RETURN) = PRVM_SetTempString(prog, ret);
}

/*
=========
VM_stringtokeynum

float stringtokeynum(string key)
=========
*/
void VM_stringtokeynum (Program prog)
{
	VM_SAFEPARMCOUNT( 1, VM_keynumtostring );

	PRVM_G_FLOAT(OFS_RETURN) = Key_StringToKeynum(PRVM_G_STRING(OFS_PARM0));
}

/*
=========
VM_getkeybind

string getkeybind(float key, float bindmap)
=========
*/
void VM_getkeybind (Program prog)
{
	int bindmap;
	VM_SAFEPARMCOUNTRANGE(1, 2, VM_CL_getkeybind);
	if(prog->argc == 2)
		bindmap = bound(-1, PRVM_G_FLOAT(OFS_PARM1), MAX_BINDMAPS-1);
	else
		bindmap = 0; // consistent to "bind"

	PRVM_G_INT(OFS_RETURN) = PRVM_SetTempString(prog, Key_GetBind((int)PRVM_G_FLOAT(OFS_PARM0), bindmap));
}

/*
=========
VM_setkeybind

float setkeybind(float key, string cmd, float bindmap)
=========
*/
void VM_setkeybind (Program prog)
{
	int bindmap;
	VM_SAFEPARMCOUNTRANGE(2, 3, VM_CL_setkeybind);
	if(prog->argc == 3)
		bindmap = bound(-1, PRVM_G_FLOAT(OFS_PARM2), MAX_BINDMAPS-1);
	else
		bindmap = 0; // consistent to "bind"

	PRVM_G_FLOAT(OFS_RETURN) = 0;
	if(Key_SetBinding((int)PRVM_G_FLOAT(OFS_PARM0), bindmap, PRVM_G_STRING(OFS_PARM1)))
		PRVM_G_FLOAT(OFS_RETURN) = 1;
}

/*
=========
VM_getbindmap

vector getbindmaps()
=========
*/
void VM_getbindmaps (Program prog)
{
	int fg, bg;
	VM_SAFEPARMCOUNT(0, VM_CL_getbindmap);
	Key_GetBindMap(&fg, &bg);
	PRVM_G_VECTOR(OFS_RETURN)[0] = fg;
	PRVM_G_VECTOR(OFS_RETURN)[1] = bg;
	PRVM_G_VECTOR(OFS_RETURN)[2] = 0;
}

/*
=========
VM_setbindmap

float setbindmaps(vector bindmap)
=========
*/
void VM_setbindmaps (Program prog)
{
	VM_SAFEPARMCOUNT(1, VM_CL_setbindmap);
	PRVM_G_FLOAT(OFS_RETURN) = 0;
	if(PRVM_G_VECTOR(OFS_PARM0)[2] == 0)
		if(Key_SetBindMap((int)PRVM_G_VECTOR(OFS_PARM0)[0], (int)PRVM_G_VECTOR(OFS_PARM0)[1]))
			PRVM_G_FLOAT(OFS_RETURN) = 1;
}

// CL_Video interface functions

/*
========================
VM_cin_open

float cin_open(string file, string name)
========================
*/
void VM_cin_open(Program prog)
{
	const char *file;
	const char *name;

	VM_SAFEPARMCOUNT( 2, VM_cin_open );

	file = PRVM_G_STRING( OFS_PARM0 );
	name = PRVM_G_STRING( OFS_PARM1 );

	VM_CheckEmptyString(prog,  file );
    VM_CheckEmptyString(prog,  name );

	if( CL_OpenVideo( file, name, MENUOWNER, "" ) )
		PRVM_G_FLOAT( OFS_RETURN ) = 1;
	else
		PRVM_G_FLOAT( OFS_RETURN ) = 0;
}

/*
========================
VM_cin_close

void cin_close(string name)
========================
*/
void VM_cin_close(Program prog)
{
	const char *name;

	VM_SAFEPARMCOUNT( 1, VM_cin_close );

	name = PRVM_G_STRING( OFS_PARM0 );
	VM_CheckEmptyString(prog,  name );

	CL_CloseVideo( CL_GetVideoByName( name ) );
}

/*
========================
VM_cin_setstate
void cin_setstate(string name, float type)
========================
*/
void VM_cin_setstate(Program prog)
{
	const char *name;
	clvideostate_t 	state;
	clvideo_t		*video;

	VM_SAFEPARMCOUNT( 2, VM_cin_netstate );

	name = PRVM_G_STRING( OFS_PARM0 );
	VM_CheckEmptyString(prog,  name );

	state = (clvideostate_t)((int)PRVM_G_FLOAT( OFS_PARM1 ));

	video = CL_GetVideoByName( name );
	if( video && state > CLVIDEO_UNUSED && state < CLVIDEO_STATECOUNT )
		CL_SetVideoState( video, state );
}

/*
========================
VM_cin_getstate

float cin_getstate(string name)
========================
*/
void VM_cin_getstate(Program prog)
{
	const char *name;
	clvideo_t		*video;

	VM_SAFEPARMCOUNT( 1, VM_cin_getstate );

	name = PRVM_G_STRING( OFS_PARM0 );
	VM_CheckEmptyString(prog,  name );

	video = CL_GetVideoByName( name );
	if( video )
		PRVM_G_FLOAT( OFS_RETURN ) = (int)video->state;
	else
		PRVM_G_FLOAT( OFS_RETURN ) = 0;
}

/*
========================
VM_cin_restart

void cin_restart(string name)
========================
*/
void VM_cin_restart(Program prog)
{
	const char *name;
	clvideo_t		*video;

	VM_SAFEPARMCOUNT( 1, VM_cin_restart );

	name = PRVM_G_STRING( OFS_PARM0 );
	VM_CheckEmptyString(prog,  name );

	video = CL_GetVideoByName( name );
	if( video )
		CL_RestartVideo( video );
}


/*
==============
VM_makevectors

Writes new values for v_forward, v_up, and v_right based on angles
void makevectors(vector angle)
==============
*/
void VM_makevectors (Program prog)
{
	vec3_t angles, forward, right, up;
	VM_SAFEPARMCOUNT(1, VM_makevectors);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), angles);
	AngleVectors(angles, forward, right, up);
	VectorCopy(forward, PRVM_gameglobalvector(v_forward));
	VectorCopy(right, PRVM_gameglobalvector(v_right));
	VectorCopy(up, PRVM_gameglobalvector(v_up));
}

/*
==============
VM_vectorvectors

Writes new values for v_forward, v_up, and v_right based on the given forward vector
vectorvectors(vector)
==============
*/
void VM_vectorvectors (Program prog)
{
	vec3_t forward, right, up;
	VM_SAFEPARMCOUNT(1, VM_vectorvectors);
	VectorNormalize2(PRVM_G_VECTOR(OFS_PARM0), forward);
	VectorVectors(forward, right, up);
	VectorCopy(forward, PRVM_gameglobalvector(v_forward));
	VectorCopy(right, PRVM_gameglobalvector(v_right));
	VectorCopy(up, PRVM_gameglobalvector(v_up));
}

/*
========================
VM_drawline

void drawline(float width, vector pos1, vector pos2, vector rgb, float alpha, float flags)
========================
*/
void VM_drawline (Program prog)
{
	prvm_vec_t	*c1, *c2, *rgb;
	float	alpha, width;
	unsigned char	flags;

	VM_SAFEPARMCOUNT(6, VM_drawline);
	width	= PRVM_G_FLOAT(OFS_PARM0);
	c1		= PRVM_G_VECTOR(OFS_PARM1);
	c2		= PRVM_G_VECTOR(OFS_PARM2);
	rgb		= PRVM_G_VECTOR(OFS_PARM3);
	alpha	= PRVM_G_FLOAT(OFS_PARM4);
	flags	= (int)PRVM_G_FLOAT(OFS_PARM5);
	DrawQ_Line(width, c1[0], c1[1], c2[0], c2[1], rgb[0], rgb[1], rgb[2], alpha, flags);
}

// float(float number, float quantity) bitshift (EXT_BITSHIFT)
void VM_bitshift (Program prog)
{
	prvm_int_t n1, n2;
	VM_SAFEPARMCOUNT(2, VM_bitshift);

	n1 = (prvm_int_t)fabs((prvm_vec_t)((prvm_int_t)PRVM_G_FLOAT(OFS_PARM0)));
	n2 = (prvm_int_t)PRVM_G_FLOAT(OFS_PARM1);
	if(!n1)
		PRVM_G_FLOAT(OFS_RETURN) = n1;
	else
	if(n2 < 0)
		PRVM_G_FLOAT(OFS_RETURN) = (n1 >> -n2);
	else
		PRVM_G_FLOAT(OFS_RETURN) = (n1 << n2);
}




//=============

/*
==============
VM_changeyaw

This was a major timewaster in progs, so it was converted to C
==============
*/
void VM_changeyaw (Program prog)
{
	prvm_edict_t		*ent;
	float		ideal, current, move, speed;

	// this is called (VERY HACKISHLY) by VM_SV_MoveToGoal, so it can not use any
	// parameters because they are the parameters to VM_SV_MoveToGoal, not this
	//VM_SAFEPARMCOUNT(0, VM_changeyaw);

	ent = PRVM_PROG_TO_EDICT(PRVM_gameglobaledict(self));
	if (ent == prog->edicts)
	{
		VM_Warning(prog, "changeyaw: can not modify world entity\n");
		return;
	}
	if (ent->priv.server->free)
	{
		VM_Warning(prog, "changeyaw: can not modify free entity\n");
		return;
	}
	current = PRVM_gameedictvector(ent, angles)[1];
	current = ANGLEMOD(current);
	ideal = PRVM_gameedictfloat(ent, ideal_yaw);
	speed = PRVM_gameedictfloat(ent, yaw_speed);

	if (current == ideal)
		return;
	move = ideal - current;
	if (ideal > current)
	{
		if (move >= 180)
			move = move - 360;
	}
	else
	{
		if (move <= -180)
			move = move + 360;
	}
	if (move > 0)
	{
		if (move > speed)
			move = speed;
	}
	else
	{
		if (move < -speed)
			move = -speed;
	}

	current += move;
	PRVM_gameedictvector(ent, angles)[1] = ANGLEMOD(current);
}

/*
==============
VM_changepitch
==============
*/
void VM_changepitch (Program prog)
{
	prvm_edict_t		*ent;
	float		ideal, current, move, speed;

	VM_SAFEPARMCOUNT(1, VM_changepitch);

	ent = PRVM_G_EDICT(OFS_PARM0);
	if (ent == prog->edicts)
	{
		VM_Warning(prog, "changepitch: can not modify world entity\n");
		return;
	}
	if (ent->priv.server->free)
	{
		VM_Warning(prog, "changepitch: can not modify free entity\n");
		return;
	}
	current = PRVM_gameedictvector(ent, angles)[0];
	current = ANGLEMOD(current);
	ideal = PRVM_gameedictfloat(ent, idealpitch);
	speed = PRVM_gameedictfloat(ent, pitch_speed);

	if (current == ideal)
		return;
	move = ideal - current;
	if (ideal > current)
	{
		if (move >= 180)
			move = move - 360;
	}
	else
	{
		if (move <= -180)
			move = move + 360;
	}
	if (move > 0)
	{
		if (move > speed)
			move = speed;
	}
	else
	{
		if (move < -speed)
			move = -speed;
	}

	current += move;
	PRVM_gameedictvector(ent, angles)[0] = ANGLEMOD(current);
}


void VM_uncolorstring (Program prog)
{
	char szNewString[VM_STRINGTEMP_LENGTH];
	const char *szString;

	// Prepare Strings
	VM_SAFEPARMCOUNT(1, VM_uncolorstring);
	szString = PRVM_G_STRING(OFS_PARM0);
	COM_StringDecolorize(szString, 0, szNewString, sizeof(szNewString), TRUE);
	PRVM_G_INT(OFS_RETURN) = PRVM_SetTempString(prog, szNewString);

}

// #226 string(string info, string key, string value, ...) infoadd (FTE_STRINGS)
//uses qw style \key\value strings
void VM_infoadd (Program prog)
{
	const char *info, *key;
	char value[VM_STRINGTEMP_LENGTH];
	char temp[VM_STRINGTEMP_LENGTH];

	VM_SAFEPARMCOUNTRANGE(2, 8, VM_infoadd);
	info = PRVM_G_STRING(OFS_PARM0);
	key = PRVM_G_STRING(OFS_PARM1);
	VM_VarString(prog, 2, value, sizeof(value));

	strlcpy(temp, info, VM_STRINGTEMP_LENGTH);

	InfoString_SetValue(temp, VM_STRINGTEMP_LENGTH, key, value);

	PRVM_G_INT(OFS_RETURN) = PRVM_SetTempString(prog, temp);
}

// #227 string(string info, string key) infoget (FTE_STRINGS)
//uses qw style \key\value strings
void VM_infoget (Program prog)
{
	const char *info;
	const char *key;
	char value[VM_STRINGTEMP_LENGTH];

	VM_SAFEPARMCOUNT(2, VM_infoget);
	info = PRVM_G_STRING(OFS_PARM0);
	key = PRVM_G_STRING(OFS_PARM1);

	InfoString_GetValue(info, key, value, VM_STRINGTEMP_LENGTH);

	PRVM_G_INT(OFS_RETURN) = PRVM_SetTempString(prog, value);
}

// #494 float(float caseinsensitive, string s, ...) crc16
void VM_crc16(Program prog)
{
	float insensitive;
	char s[VM_STRINGTEMP_LENGTH];
	VM_SAFEPARMCOUNTRANGE(2, 8, VM_crc16);
	insensitive = PRVM_G_FLOAT(OFS_PARM0);
	VM_VarString(prog, 1, s, sizeof(s));
	PRVM_G_FLOAT(OFS_RETURN) = (unsigned short) ((insensitive ? CRC_Block_CaseInsensitive : CRC_Block) ((unsigned char *) s, strlen(s)));
}

// #639 float(string digest, string data, ...) digest_hex
void VM_digest_hex(Program prog)
{
	const char *digest;

	char out[32];
	char outhex[65];
	int outlen;

	char s[VM_STRINGTEMP_LENGTH];
	int len;

	VM_SAFEPARMCOUNTRANGE(2, 8, VM_digest_hex);
	digest = PRVM_G_STRING(OFS_PARM0);
	if(!digest)
		digest = "";
	VM_VarString(prog, 1, s, sizeof(s));
	len = (int)strlen(s);

	outlen = 0;

	if(!strcmp(digest, "MD4"))
	{
		outlen = 16;
		mdfour((unsigned char *) out, (unsigned char *) s, len);
	}
	else if(!strcmp(digest, "SHA256") && Crypto_Available())
	{
		outlen = 32;
		sha256((unsigned char *) out, (unsigned char *) s, len);
	}
	// no warning needed on mismatch - we return string_null to QC

	if(outlen)
	{
		int i;
		static const char *hexmap = "0123456789abcdef";
		for(i = 0; i < outlen; ++i)
		{
			outhex[2*i]   = hexmap[(out[i] >> 4) & 15];
			outhex[2*i+1] = hexmap[(out[i] >> 0) & 15];
		}
		outhex[2*i] = 0;
		PRVM_G_INT(OFS_RETURN) = PRVM_SetTempString(prog, outhex);
	}
	else
		PRVM_G_INT(OFS_RETURN) = 0;
}

void VM_wasfreed (Program prog)
{
	VM_SAFEPARMCOUNT(1, VM_wasfreed);
	PRVM_G_FLOAT(OFS_RETURN) = PRVM_G_EDICT(OFS_PARM0)->priv.required->free;
}

void VM_SetTraceGlobals(Program prog, const trace_t *trace)
{
	PRVM_gameglobalfloat(trace_allsolid) = trace->allsolid;
	PRVM_gameglobalfloat(trace_startsolid) = trace->startsolid;
	PRVM_gameglobalfloat(trace_fraction) = trace->fraction;
	PRVM_gameglobalfloat(trace_inwater) = trace->inwater;
	PRVM_gameglobalfloat(trace_inopen) = trace->inopen;
	VectorCopy(trace->endpos, PRVM_gameglobalvector(trace_endpos));
	VectorCopy(trace->plane.normal, PRVM_gameglobalvector(trace_plane_normal));
	PRVM_gameglobalfloat(trace_plane_dist) = trace->plane.dist;
	PRVM_gameglobaledict(trace_ent) = PRVM_EDICT_TO_PROG(trace->ent ? trace->ent : prog->edicts);
	PRVM_gameglobalfloat(trace_dpstartcontents) = trace->startsupercontents;
	PRVM_gameglobalfloat(trace_dphitcontents) = trace->hitsupercontents;
	PRVM_gameglobalfloat(trace_dphitq3surfaceflags) = trace->hitq3surfaceflags;
	PRVM_gameglobalstring(trace_dphittexturename) = trace->hittexture ? PRVM_SetTempString(prog, trace->hittexture->name) : 0;
}

void VM_ClearTraceGlobals(Program prog)
{
	// clean up all trace globals when leaving the VM (anti-triggerbot safeguard)
	PRVM_gameglobalfloat(trace_allsolid) = 0;
	PRVM_gameglobalfloat(trace_startsolid) = 0;
	PRVM_gameglobalfloat(trace_fraction) = 0;
	PRVM_gameglobalfloat(trace_inwater) = 0;
	PRVM_gameglobalfloat(trace_inopen) = 0;
	VectorClear(PRVM_gameglobalvector(trace_endpos));
	VectorClear(PRVM_gameglobalvector(trace_plane_normal));
	PRVM_gameglobalfloat(trace_plane_dist) = 0;
	PRVM_gameglobaledict(trace_ent) = PRVM_EDICT_TO_PROG(prog->edicts);
	PRVM_gameglobalfloat(trace_dpstartcontents) = 0;
	PRVM_gameglobalfloat(trace_dphitcontents) = 0;
	PRVM_gameglobalfloat(trace_dphitq3surfaceflags) = 0;
	PRVM_gameglobalstring(trace_dphittexturename) = 0;
}

//=============

void VM_Cmd_Init(Program prog)
{
	// only init the stuff for the current prog
	VM_Files_Init(prog);
	VM_Search_Init(prog);
}

void VM_Cmd_Reset(Program prog)
{
	CL_PurgeOwner( MENUOWNER );
	VM_Search_Reset(prog);
	VM_Files_CloseAll(prog);
}

// #510 string(string input, ...) uri_escape (DP_QC_URI_ESCAPE)
// does URI escaping on a string (replace evil stuff by %AB escapes)
void VM_uri_escape (Program prog)
{
	char src[VM_STRINGTEMP_LENGTH];
	char dest[VM_STRINGTEMP_LENGTH];
	char *p, *q;
	static const char *hex = "0123456789ABCDEF";

	VM_SAFEPARMCOUNTRANGE(1, 8, VM_uri_escape);
	VM_VarString(prog, 0, src, sizeof(src));

	for(p = src, q = dest; *p && q < dest + sizeof(dest) - 3; ++p)
	{
		if((*p >= 'A' && *p <= 'Z')
			|| (*p >= 'a' && *p <= 'z')
			|| (*p >= '0' && *p <= '9')
			|| (*p == '-')  || (*p == '_') || (*p == '.')
			|| (*p == '!')  || (*p == '~')
			|| (*p == '\'') || (*p == '(') || (*p == ')'))
			*q++ = *p;
		else
		{
			*q++ = '%';
			*q++ = hex[(*(unsigned char *)p >> 4) & 0xF];
			*q++ = hex[ *(unsigned char *)p       & 0xF];
		}
	}
	*q++ = 0;

	PRVM_G_INT(OFS_RETURN) = PRVM_SetTempString(prog, dest);
}

// #510 string(string input, ...) uri_unescape (DP_QC_URI_ESCAPE)
// does URI unescaping on a string (get back the evil stuff)
void VM_uri_unescape (Program prog)
{
	char src[VM_STRINGTEMP_LENGTH];
	char dest[VM_STRINGTEMP_LENGTH];
	char *p, *q;
	int hi, lo;

	VM_SAFEPARMCOUNTRANGE(1, 8, VM_uri_unescape);
	VM_VarString(prog, 0, src, sizeof(src));

	for(p = src, q = dest; *p; ) // no need to check size, because unescape can't expand
	{
		if(*p == '%')
		{
			if(p[1] >= '0' && p[1] <= '9')
				hi = p[1] - '0';
			else if(p[1] >= 'a' && p[1] <= 'f')
				hi = p[1] - 'a' + 10;
			else if(p[1] >= 'A' && p[1] <= 'F')
				hi = p[1] - 'A' + 10;
			else
				goto nohex;
			if(p[2] >= '0' && p[2] <= '9')
				lo = p[2] - '0';
			else if(p[2] >= 'a' && p[2] <= 'f')
				lo = p[2] - 'a' + 10;
			else if(p[2] >= 'A' && p[2] <= 'F')
				lo = p[2] - 'A' + 10;
			else
				goto nohex;
			if(hi != 0 || lo != 0) // don't unescape NUL bytes
				*q++ = (char) (hi * 0x10 + lo);
			p += 3;
			continue;
		}

nohex:
		// otherwise:
		*q++ = *p++;
	}
	*q++ = 0;

	PRVM_G_INT(OFS_RETURN) = PRVM_SetTempString(prog, dest);
}

// #502 string(string filename) whichpack (DP_QC_WHICHPACK)
// returns the name of the pack containing a file, or "" if it is not in any pack (but local or non-existant)
void VM_whichpack (Program prog)
{
	const char *fn, *pack;

	VM_SAFEPARMCOUNT(1, VM_whichpack);
	fn = PRVM_G_STRING(OFS_PARM0);
	pack = FS_WhichPack(fn);

	PRVM_G_INT(OFS_RETURN) = pack ? PRVM_SetTempString(prog, pack) : 0;
}

typedef struct
{
	Program prog;
	double starttime;
	float id;
	char buffer[MAX_INPUTLINE];
	char posttype[128];
	unsigned char *postdata; // free when uri_to_prog_t is freed
	size_t postlen;
	char *sigdata; // free when uri_to_prog_t is freed
	size_t siglen;
}
uri_to_prog_t;

void uri_to_string_callback(int status, size_t length_received, unsigned char *buffer, void *cbdata)
{
	Program prog;
	uri_to_prog_t *handle = (uri_to_prog_t *) cbdata;

	prog = handle->prog;
	if(!prog->loaded)
	{
		// curl reply came too late... so just drop it
		if(handle->postdata)
			Z_Free(handle->postdata);
		if(handle->sigdata)
			Z_Free(handle->sigdata);
		Z_Free(handle);
		return;
	}

	if((prog->starttime == handle->starttime) && (PRVM_allfunction(URI_Get_Callback)))
	{
		if(length_received >= sizeof(handle->buffer))
			length_received = sizeof(handle->buffer) - 1;
		handle->buffer[length_received] = 0;

		PRVM_G_FLOAT(OFS_PARM0) = handle->id;
		PRVM_G_FLOAT(OFS_PARM1) = status;
		PRVM_G_INT(OFS_PARM2) = PRVM_SetTempString(prog, handle->buffer);
		prog->ExecuteProgram(prog, PRVM_allfunction(URI_Get_Callback), "QC function URI_Get_Callback is missing");
	}

	if(handle->postdata)
		Z_Free(handle->postdata);
	if(handle->sigdata)
		Z_Free(handle->sigdata);
	Z_Free(handle);
}

// uri_get() gets content from an URL and calls a callback "uri_get_callback" with it set as string; an unique ID of the transfer is returned
// returns 1 on success, and then calls the callback with the ID, 0 or the HTTP status code, and the received data in a string
void VM_uri_get (Program prog)
{
	const char *url;
	float id;
	bool ret;
	uri_to_prog_t *handle;
	const char *posttype = nullptr;
	const char *postseparator = nullptr;
	int poststringbuffer = -1;
	int postkeyid = -1;
	const char *query_string = nullptr;
	size_t lq;

	if(!PRVM_allfunction(URI_Get_Callback))
		prog->error_cmd("uri_get called by %s without URI_Get_Callback defined", prog->name);

	VM_SAFEPARMCOUNTRANGE(2, 6, VM_uri_get);

	url = PRVM_G_STRING(OFS_PARM0);
	id = PRVM_G_FLOAT(OFS_PARM1);
	if(prog->argc >= 3)
		posttype = PRVM_G_STRING(OFS_PARM2);
	if(prog->argc >= 4)
		postseparator = PRVM_G_STRING(OFS_PARM3);
	if(prog->argc >= 5)
		poststringbuffer = PRVM_G_FLOAT(OFS_PARM4);
	if(prog->argc >= 6)
		postkeyid = PRVM_G_FLOAT(OFS_PARM5);
	handle = (uri_to_prog_t *) Z_Malloc(sizeof(*handle)); // this can't be the prog's mem pool, as curl may call the callback later!

	query_string = strchr(url, '?');
	if(query_string)
		++query_string;
	lq = query_string ? strlen(query_string) : 0;

	handle->prog = prog;
	handle->starttime = prog->starttime;
	handle->id = id;
	if(postseparator && posttype && *posttype)
	{
		size_t l = strlen(postseparator);
		if(poststringbuffer >= 0)
		{
			size_t ltotal;
			int i;
			// "implode"
			prvm_stringbuffer_t *stringbuffer;
			stringbuffer = (prvm_stringbuffer_t *)Mem_ExpandableArray_RecordAtIndex(&prog->stringbuffersarray, poststringbuffer);
			if(!stringbuffer)
			{
				VM_Warning(prog, "uri_get: invalid buffer %i used in %s\n", (int)PRVM_G_FLOAT(OFS_PARM0), prog->name);
				return;
			}
			ltotal = 0;
			for(i = 0;i < stringbuffer->num_strings;i++)
			{
				if(i > 0)
					ltotal += l;
				if(stringbuffer->strings[i])
					ltotal += strlen(stringbuffer->strings[i]);
			}
			handle->postdata = (unsigned char *)Z_Malloc(ltotal + 1 + lq);
			handle->postlen = ltotal;
			ltotal = 0;
			for(i = 0;i < stringbuffer->num_strings;i++)
			{
				if(i > 0)
				{
					memcpy(handle->postdata + ltotal, postseparator, l);
					ltotal += l;
				}
				if(stringbuffer->strings[i])
				{
					memcpy(handle->postdata + ltotal, stringbuffer->strings[i], strlen(stringbuffer->strings[i]));
					ltotal += strlen(stringbuffer->strings[i]);
				}
			}
			if(ltotal != handle->postlen)
				prog->error_cmd("%s: string buffer content size mismatch, possible overrun", prog->name);
		}
		else
		{
			handle->postdata = (unsigned char *)Z_Malloc(l + 1 + lq);
			handle->postlen = l;
			memcpy(handle->postdata, postseparator, l);
		}
		handle->postdata[handle->postlen] = 0;
		if(query_string)
			memcpy(handle->postdata + handle->postlen + 1, query_string, lq);
		if(postkeyid >= 0)
		{
			// POST: we sign postdata \0 query string
			size_t ll;
			handle->sigdata = (char *)Z_Malloc(8192);
			strlcpy(handle->sigdata, "X-D0-Blind-ID-Detached-Signature: ", 8192);
			l = strlen(handle->sigdata);
			handle->siglen = Crypto_SignDataDetached(handle->postdata, handle->postlen + 1 + lq, postkeyid, handle->sigdata + l, 8192 - l);
			if(!handle->siglen)
			{
				Z_Free(handle->sigdata);
				handle->sigdata = nullptr;
				goto out1;
			}
			ll = base64_encode((unsigned char *) (handle->sigdata + l), handle->siglen, 8192 - l - 1);
			if(!ll)
			{
				Z_Free(handle->sigdata);
				handle->sigdata = nullptr;
				goto out1;
			}
			handle->siglen = l + ll;
			handle->sigdata[handle->siglen] = 0;
		}
out1:
		strlcpy(handle->posttype, posttype, sizeof(handle->posttype));
		ret = Curl_Begin_ToMemory_POST(url, handle->sigdata, 0, handle->posttype, handle->postdata, handle->postlen, (unsigned char *) handle->buffer, sizeof(handle->buffer), uri_to_string_callback, handle);
	}
	else
	{
		if(postkeyid >= 0 && query_string)
		{
			// GET: we sign JUST the query string
			size_t l, ll;
			handle->sigdata = (char *)Z_Malloc(8192);
			strlcpy(handle->sigdata, "X-D0-Blind-ID-Detached-Signature: ", 8192);
			l = strlen(handle->sigdata);
			handle->siglen = Crypto_SignDataDetached(query_string, lq, postkeyid, handle->sigdata + l, 8192 - l);
			if(!handle->siglen)
			{
				Z_Free(handle->sigdata);
				handle->sigdata = nullptr;
				goto out2;
			}
			ll = base64_encode((unsigned char *) (handle->sigdata + l), handle->siglen, 8192 - l - 1);
			if(!ll)
			{
				Z_Free(handle->sigdata);
				handle->sigdata = nullptr;
				goto out2;
			}
			handle->siglen = l + ll;
			handle->sigdata[handle->siglen] = 0;
		}
out2:
		handle->postdata = nullptr;
		handle->postlen = 0;
		ret = Curl_Begin_ToMemory_POST(url, handle->sigdata, 0, nullptr, nullptr, 0, (unsigned char *) handle->buffer, sizeof(handle->buffer), uri_to_string_callback, handle);
	}
	if(ret)
	{
		PRVM_G_INT(OFS_RETURN) = 1;
	}
	else
	{
		if(handle->postdata)
			Z_Free(handle->postdata);
		if(handle->sigdata)
			Z_Free(handle->sigdata);
		Z_Free(handle);
		PRVM_G_INT(OFS_RETURN) = 0;
	}
}

void VM_netaddress_resolve (Program prog)
{
	const char *ip;
	char normalized[128];
	int port;
	lhnetaddress_t addr;

	VM_SAFEPARMCOUNTRANGE(1, 2, VM_netaddress_resolve);

	ip = PRVM_G_STRING(OFS_PARM0);
	port = 0;
	if(prog->argc > 1)
		port = (int) PRVM_G_FLOAT(OFS_PARM1);

	if(LHNETADDRESS_FromString(&addr, ip, port) && LHNETADDRESS_ToString(&addr, normalized, sizeof(normalized), prog->argc > 1))
		PRVM_G_INT(OFS_RETURN) = PRVM_SetTempString(prog, normalized);
	else
		PRVM_G_INT(OFS_RETURN) = PRVM_SetTempString(prog, "");
}

//string(Program prog) getextresponse = #624; // returns the next extResponse packet that was sent to this client
void VM_CL_getextresponse (Program prog)
{
	VM_SAFEPARMCOUNT(0,VM_argv);

	if (cl_net_extresponse_count <= 0)
		PRVM_G_INT(OFS_RETURN) = OFS_NULL;
	else
	{
		int first;
		--cl_net_extresponse_count;
		first = (cl_net_extresponse_last + NET_EXTRESPONSE_MAX - cl_net_extresponse_count) % NET_EXTRESPONSE_MAX;
		PRVM_G_INT(OFS_RETURN) = PRVM_SetTempString(prog, cl_net_extresponse[first]);
	}
}

void VM_SV_getextresponse (Program prog)
{
	VM_SAFEPARMCOUNT(0,VM_argv);

	if (sv_net_extresponse_count <= 0)
		PRVM_G_INT(OFS_RETURN) = OFS_NULL;
	else
	{
		int first;
		--sv_net_extresponse_count;
		first = (sv_net_extresponse_last + NET_EXTRESPONSE_MAX - sv_net_extresponse_count) % NET_EXTRESPONSE_MAX;
		PRVM_G_INT(OFS_RETURN) = PRVM_SetTempString(prog, sv_net_extresponse[first]);
	}
}

/*
=========
Common functions between menu.dat and clsprogs
=========
*/

//#349 float() isdemo
void VM_CL_isdemo (Program prog)
{
	VM_SAFEPARMCOUNT(0, VM_CL_isdemo);
	PRVM_G_FLOAT(OFS_RETURN) = cls.demoplayback;
}

//#355 float() videoplaying
void VM_CL_videoplaying (Program prog)
{
	VM_SAFEPARMCOUNT(0, VM_CL_videoplaying);
	PRVM_G_FLOAT(OFS_RETURN) = cl_videoplaying;
}

/*
=========
VM_M_callfunction

	callfunction(...,string function_name)
Extension: pass
=========
*/
void VM_callfunction(Program prog)
{
	mfunction_t *func;
	const char *s;

	VM_SAFEPARMCOUNTRANGE(1, 8, VM_callfunction);

	s = PRVM_G_STRING(OFS_PARM0+(prog->argc - 1)*3);

	VM_CheckEmptyString(prog, s);

	func = PRVM_ED_FindFunction(prog, s);

	if(!func)
		prog->error_cmd("VM_callfunciton: function %s not found !", s);
	else if (func->first_statement < 0)
	{
		// negative statements are built in functions
		int builtinnumber = -func->first_statement;
		prog->xfunction->builtinsprofile++;
		if (builtinnumber < prog->numbuiltins && prog->builtins[builtinnumber])
			prog->builtins[builtinnumber](prog);
		else
			prog->error_cmd("No such builtin #%i in %s; most likely cause: outdated engine build. Try updating!", builtinnumber, prog->name);
	}
	else if(func - prog->functions > 0)
	{
		prog->argc--;
		prog->ExecuteProgram(prog, func - prog->functions,"");
		prog->argc++;
	}
}

/*
=========
VM_isfunction

float	isfunction(string function_name)
=========
*/
void VM_isfunction(Program prog)
{
	mfunction_t *func;
	const char *s;

	VM_SAFEPARMCOUNT(1, VM_isfunction);

	s = PRVM_G_STRING(OFS_PARM0);

	VM_CheckEmptyString(prog, s);

	func = PRVM_ED_FindFunction(prog, s);

	if(!func)
		PRVM_G_FLOAT(OFS_RETURN) = false;
	else
		PRVM_G_FLOAT(OFS_RETURN) = true;
}



// surface querying

static dp_model_t *getmodel(Program prog, prvm_edict_t *ed)
{
	if (prog == SVVM_prog)
		return SV_GetModelFromEdict(ed);
	else if (prog == CLVM_prog)
		return CL_GetModelFromEdict(ed);
	else
		return nullptr;
}

typedef struct
{
	unsigned int progid;
	dp_model_t *model;
	frameblend_t frameblend[MAX_FRAMEBLENDS];
	skeleton_t *skeleton_p;
	skeleton_t skeleton;
	float *data_vertex3f;
	float *data_svector3f;
	float *data_tvector3f;
	float *data_normal3f;
	int max_vertices;
	float *buf_vertex3f;
	float *buf_svector3f;
	float *buf_tvector3f;
	float *buf_normal3f;
}
animatemodel_cache_t;
static animatemodel_cache_t animatemodel_cache;

void animatemodel(Program prog, dp_model_t *model, prvm_edict_t *ed)
{
	skeleton_t *skeleton;
	int skeletonindex = -1;
	bool need = false;
	if(!(model->surfmesh.isanimated && model->AnimateVertices))
	{
		animatemodel_cache.data_vertex3f = model->surfmesh.data_vertex3f;
		animatemodel_cache.data_svector3f = model->surfmesh.data_svector3f;
		animatemodel_cache.data_tvector3f = model->surfmesh.data_tvector3f;
		animatemodel_cache.data_normal3f = model->surfmesh.data_normal3f;
		return;
	}
	if(animatemodel_cache.progid != prog->id)
		memset(&animatemodel_cache, 0, sizeof(animatemodel_cache));
	need |= (animatemodel_cache.model != model);
	VM_GenerateFrameGroupBlend(prog, ed->priv.server->framegroupblend, ed);
	VM_FrameBlendFromFrameGroupBlend(ed->priv.server->frameblend, ed->priv.server->framegroupblend, model, PRVM_serverglobalfloat(time));
	need |= (memcmp(&animatemodel_cache.frameblend, &ed->priv.server->frameblend, sizeof(ed->priv.server->frameblend))) != 0;
	skeletonindex = (int)PRVM_gameedictfloat(ed, skeletonindex) - 1;
	if (!(skeletonindex >= 0 && skeletonindex < MAX_EDICTS && (skeleton = prog->skeletons[skeletonindex]) && skeleton->model->num_bones == ed->priv.server->skeleton.model->num_bones))
		skeleton = nullptr;
	need |= (animatemodel_cache.skeleton_p != skeleton);
	if(skeleton)
		need |= (memcmp(&animatemodel_cache.skeleton, skeleton, sizeof(ed->priv.server->skeleton))) != 0;
	if(!need)
		return;
	if(model->surfmesh.num_vertices > animatemodel_cache.max_vertices)
	{
		animatemodel_cache.max_vertices = model->surfmesh.num_vertices * 2;
		if(animatemodel_cache.buf_vertex3f) Mem_Free(animatemodel_cache.buf_vertex3f);
		if(animatemodel_cache.buf_svector3f) Mem_Free(animatemodel_cache.buf_svector3f);
		if(animatemodel_cache.buf_tvector3f) Mem_Free(animatemodel_cache.buf_tvector3f);
		if(animatemodel_cache.buf_normal3f) Mem_Free(animatemodel_cache.buf_normal3f);
		animatemodel_cache.buf_vertex3f = (float *)Mem_Alloc(prog->progs_mempool, sizeof(float[3]) * animatemodel_cache.max_vertices);
		animatemodel_cache.buf_svector3f = (float *)Mem_Alloc(prog->progs_mempool, sizeof(float[3]) * animatemodel_cache.max_vertices);
		animatemodel_cache.buf_tvector3f = (float *)Mem_Alloc(prog->progs_mempool, sizeof(float[3]) * animatemodel_cache.max_vertices);
		animatemodel_cache.buf_normal3f = (float *)Mem_Alloc(prog->progs_mempool, sizeof(float[3]) * animatemodel_cache.max_vertices);
	}
	animatemodel_cache.data_vertex3f = animatemodel_cache.buf_vertex3f;
	animatemodel_cache.data_svector3f = animatemodel_cache.buf_svector3f;
	animatemodel_cache.data_tvector3f = animatemodel_cache.buf_tvector3f;
	animatemodel_cache.data_normal3f = animatemodel_cache.buf_normal3f;
	VM_UpdateEdictSkeleton(prog, ed, model, ed->priv.server->frameblend);
	model->AnimateVertices(model, ed->priv.server->frameblend, &ed->priv.server->skeleton, animatemodel_cache.data_vertex3f, animatemodel_cache.data_normal3f, animatemodel_cache.data_svector3f, animatemodel_cache.data_tvector3f);
	animatemodel_cache.progid = prog->id;
	animatemodel_cache.model = model;
	memcpy(&animatemodel_cache.frameblend, &ed->priv.server->frameblend, sizeof(ed->priv.server->frameblend));
	animatemodel_cache.skeleton_p = skeleton;
	if(skeleton)
		memcpy(&animatemodel_cache.skeleton, skeleton, sizeof(ed->priv.server->skeleton));
}

void getmatrix(Program prog, prvm_edict_t *ed, matrix4x4_t *out)
{
	if (prog == SVVM_prog)
		SV_GetEntityMatrix(prog, ed, out, false);
	else if (prog == CLVM_prog)
		CL_GetEntityMatrix(prog, ed, out, false);
	else
		*out = identitymatrix;
}

void applytransform_forward(Program prog, const vec3_t in, prvm_edict_t *ed, vec3_t out)
{
	matrix4x4_t m;
	getmatrix(prog, ed, &m);
	Matrix4x4_Transform(&m, in, out);
}

void applytransform_forward_direction(Program prog, const vec3_t in, prvm_edict_t *ed, vec3_t out)
{
	matrix4x4_t m;
	getmatrix(prog, ed, &m);
	Matrix4x4_Transform3x3(&m, in, out);
}

void applytransform_inverted(Program prog, const vec3_t in, prvm_edict_t *ed, vec3_t out)
{
	matrix4x4_t m, n;
	getmatrix(prog, ed, &m);
	Matrix4x4_Invert_Full(&n, &m);
	Matrix4x4_Transform3x3(&n, in, out);
}

void applytransform_forward_normal(Program prog, const vec3_t in, prvm_edict_t *ed, vec3_t out)
{
	matrix4x4_t m;
	float p[4];
	getmatrix(prog, ed, &m);
	Matrix4x4_TransformPositivePlane(&m, in[0], in[1], in[2], 0, p);
	VectorCopy(p, out);
}

void clippointtosurface(Program prog, prvm_edict_t *ed, dp_model_t *model, msurface_t *surface, vec3_t p, vec3_t out)
{
	int i, j, k;
	float *v[3], facenormal[3], edgenormal[3], sidenormal[3], temp[3], offsetdist, dist, bestdist;
	const int *e;
	animatemodel(prog, model, ed);
	bestdist = 1000000000;
	VectorCopy(p, out);
	for (i = 0, e = (model->surfmesh.data_element3i + 3 * surface->num_firsttriangle);i < surface->num_triangles;i++, e += 3)
	{
		// clip original point to each triangle of the surface and find the
		// triangle that is closest
		v[0] = animatemodel_cache.data_vertex3f + e[0] * 3;
		v[1] = animatemodel_cache.data_vertex3f + e[1] * 3;
		v[2] = animatemodel_cache.data_vertex3f + e[2] * 3;
		TriangleNormal(v[0], v[1], v[2], facenormal);
		VectorNormalize(facenormal);
		offsetdist = DotProduct(v[0], facenormal) - DotProduct(p, facenormal);
		VectorMA(p, offsetdist, facenormal, temp);
		for (j = 0, k = 2;j < 3;k = j, j++)
		{
			VectorSubtract(v[k], v[j], edgenormal);
			CrossProduct(edgenormal, facenormal, sidenormal);
			VectorNormalize(sidenormal);
			offsetdist = DotProduct(v[k], sidenormal) - DotProduct(temp, sidenormal);
			if (offsetdist < 0)
				VectorMA(temp, offsetdist, sidenormal, temp);
		}
		dist = VectorDistance2(temp, p);
		if (bestdist > dist)
		{
			bestdist = dist;
			VectorCopy(temp, out);
		}
	}
}

static msurface_t *getsurface(dp_model_t *model, int surfacenum)
{
	if (surfacenum < 0 || surfacenum >= model->nummodelsurfaces)
		return nullptr;
	return model->data_surfaces + surfacenum + model->firstmodelsurface;
}


//PF_getsurfacenumpoints, // #434 float(entity e, float s) getsurfacenumpoints = #434;
void VM_getsurfacenumpoints(Program prog)
{
	dp_model_t *model;
	msurface_t *surface;
	VM_SAFEPARMCOUNT(2, VM_getsurfacenumpoints);
	// return 0 if no such surface
	if (!(model = getmodel(prog, PRVM_G_EDICT(OFS_PARM0))) || !(surface = getsurface(model, (int)PRVM_G_FLOAT(OFS_PARM1))))
	{
		PRVM_G_FLOAT(OFS_RETURN) = 0;
		return;
	}

	// note: this (incorrectly) assumes it is a simple polygon
	PRVM_G_FLOAT(OFS_RETURN) = surface->num_vertices;
}
//PF_getsurfacepoint,     // #435 vector(entity e, float s, float n) getsurfacepoint = #435;
void VM_getsurfacepoint(Program prog)
{
	prvm_edict_t *ed;
	dp_model_t *model;
	msurface_t *surface;
	int pointnum;
	vec3_t result;
	VM_SAFEPARMCOUNT(3, VM_getsurfacepoint);
	VectorClear(PRVM_G_VECTOR(OFS_RETURN));
	ed = PRVM_G_EDICT(OFS_PARM0);
	if (!(model = getmodel(prog, ed)) || !(surface = getsurface(model, (int)PRVM_G_FLOAT(OFS_PARM1))))
		return;
	// note: this (incorrectly) assumes it is a simple polygon
	pointnum = (int)PRVM_G_FLOAT(OFS_PARM2);
	if (pointnum < 0 || pointnum >= surface->num_vertices)
		return;
	animatemodel(prog, model, ed);
	applytransform_forward(prog, &(animatemodel_cache.data_vertex3f + 3 * surface->num_firstvertex)[pointnum * 3], ed, result);
	VectorCopy(result, PRVM_G_VECTOR(OFS_RETURN));
}
//PF_getsurfacepointattribute,
// #486 vector(entity e, float s, float n, float a) getsurfacepointattribute = #486;

void VM_getsurfacepointattribute(Program prog)
{
	prvm_edict_t *ed;
	dp_model_t *model;
	msurface_t *surface;
	int pointnum;
	int attributetype;
	vec3_t result;

	VM_SAFEPARMCOUNT(4, VM_getsurfacepoint);
	VectorClear(PRVM_G_VECTOR(OFS_RETURN));
	ed = PRVM_G_EDICT(OFS_PARM0);
	if (!(model = getmodel(prog, ed)) || !(surface = getsurface(model, (int)PRVM_G_FLOAT(OFS_PARM1))))
		return;
	pointnum = (int)PRVM_G_FLOAT(OFS_PARM2);
	if (pointnum < 0 || pointnum >= surface->num_vertices)
		return;
	attributetype = (int) PRVM_G_FLOAT(OFS_PARM3);

	animatemodel(prog, model, ed);

	switch( attributetype ) {
		// float SPA_POSITION = 0;
		case 0:
			applytransform_forward(prog, &(animatemodel_cache.data_vertex3f + 3 * surface->num_firstvertex)[pointnum * 3], ed, result);
			VectorCopy(result, PRVM_G_VECTOR(OFS_RETURN));
			break;
		// float SPA_S_AXIS = 1;
		case 1:
			applytransform_forward_direction(prog, &(animatemodel_cache.data_svector3f + 3 * surface->num_firstvertex)[pointnum * 3], ed, result);
			VectorCopy(result, PRVM_G_VECTOR(OFS_RETURN));
			break;
		// float SPA_T_AXIS = 2;
		case 2:
			applytransform_forward_direction(prog, &(animatemodel_cache.data_tvector3f + 3 * surface->num_firstvertex)[pointnum * 3], ed, result);
			VectorCopy(result, PRVM_G_VECTOR(OFS_RETURN));
			break;
		// float SPA_R_AXIS = 3; // same as SPA_NORMAL
		case 3:
			applytransform_forward_direction(prog, &(animatemodel_cache.data_normal3f + 3 * surface->num_firstvertex)[pointnum * 3], ed, result);
			VectorCopy(result, PRVM_G_VECTOR(OFS_RETURN));
			break;
		// float SPA_TEXCOORDS0 = 4;
		case 4: {
			float *texcoord = &(model->surfmesh.data_texcoordtexture2f + 2 * surface->num_firstvertex)[pointnum * 2];
			result[0] = texcoord[0];
			result[1] = texcoord[1];
			result[2] = 0.0f;
			VectorCopy(result, PRVM_G_VECTOR(OFS_RETURN));
			break;
		}
		// float SPA_LIGHTMAP0_TEXCOORDS = 5;
		case 5: {
			float *texcoord = &(model->surfmesh.data_texcoordlightmap2f + 2 * surface->num_firstvertex)[pointnum * 2];
			result[0] = texcoord[0];
			result[1] = texcoord[1];
			result[2] = 0.0f;
			VectorCopy(result, PRVM_G_VECTOR(OFS_RETURN));
			break;
		}
		// float SPA_LIGHTMAP0_COLOR = 6;
		case 6:
			// ignore alpha for now..
			VectorCopy( &(model->surfmesh.data_lightmapcolor4f + 4 * surface->num_firstvertex)[pointnum * 4], PRVM_G_VECTOR(OFS_RETURN));
			break;
		default:
			VectorSet( PRVM_G_VECTOR(OFS_RETURN), 0.0f, 0.0f, 0.0f );
			break;
	}
}
//PF_getsurfacenormal,    // #436 vector(entity e, float s) getsurfacenormal = #436;
void VM_getsurfacenormal(Program prog)
{
	dp_model_t *model;
	msurface_t *surface;
	vec3_t normal;
	vec3_t result;
	VM_SAFEPARMCOUNT(2, VM_getsurfacenormal);
	VectorClear(PRVM_G_VECTOR(OFS_RETURN));
	if (!(model = getmodel(prog, PRVM_G_EDICT(OFS_PARM0))) || !(surface = getsurface(model, (int)PRVM_G_FLOAT(OFS_PARM1))))
		return;
	// note: this only returns the first triangle, so it doesn't work very
	// well for curved surfaces or arbitrary meshes
	animatemodel(prog, model, PRVM_G_EDICT(OFS_PARM0));
	TriangleNormal((animatemodel_cache.data_vertex3f + 3 * surface->num_firstvertex), (animatemodel_cache.data_vertex3f + 3 * surface->num_firstvertex) + 3, (animatemodel_cache.data_vertex3f + 3 * surface->num_firstvertex) + 6, normal);
	applytransform_forward_normal(prog, normal, PRVM_G_EDICT(OFS_PARM0), result);
	VectorNormalize(result);
	VectorCopy(result, PRVM_G_VECTOR(OFS_RETURN));
}
//PF_getsurfacetexture,   // #437 string(entity e, float s) getsurfacetexture = #437;
void VM_getsurfacetexture(Program prog)
{
	dp_model_t *model;
	msurface_t *surface;
	VM_SAFEPARMCOUNT(2, VM_getsurfacetexture);
	PRVM_G_INT(OFS_RETURN) = OFS_NULL;
	if (!(model = getmodel(prog, PRVM_G_EDICT(OFS_PARM0))) || !(surface = getsurface(model, (int)PRVM_G_FLOAT(OFS_PARM1))))
		return;
	PRVM_G_INT(OFS_RETURN) = PRVM_SetTempString(prog, surface->texture->name);
}
//PF_getsurfacenearpoint, // #438 float(entity e, vector p) getsurfacenearpoint = #438;
void VM_getsurfacenearpoint(Program prog)
{
	int surfacenum, best;
	vec3_t clipped, p;
	vec_t dist, bestdist;
	prvm_edict_t *ed;
	dp_model_t *model;
	msurface_t *surface;
	vec3_t point;
	VM_SAFEPARMCOUNT(2, VM_getsurfacenearpoint);
	PRVM_G_FLOAT(OFS_RETURN) = -1;
	ed = PRVM_G_EDICT(OFS_PARM0);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM1), point);

	if (!ed || ed->priv.server->free)
		return;
	model = getmodel(prog, ed);
	if (!model || !model->num_surfaces)
		return;

	animatemodel(prog, model, ed);

	applytransform_inverted(prog, point, ed, p);
	best = -1;
	bestdist = 1000000000;
	for (surfacenum = 0;surfacenum < model->nummodelsurfaces;surfacenum++)
	{
		surface = model->data_surfaces + surfacenum + model->firstmodelsurface;
		// first see if the nearest point on the surface's box is closer than the previous match
		clipped[0] = bound(surface->mins[0], p[0], surface->maxs[0]) - p[0];
		clipped[1] = bound(surface->mins[1], p[1], surface->maxs[1]) - p[1];
		clipped[2] = bound(surface->mins[2], p[2], surface->maxs[2]) - p[2];
		dist = VectorLength2(clipped);
		if (dist < bestdist)
		{
			// it is, check the nearest point on the actual geometry
			clippointtosurface(prog, ed, model, surface, p, clipped);
			VectorSubtract(clipped, p, clipped);
			dist += VectorLength2(clipped);
			if (dist < bestdist)
			{
				// that's closer too, store it as the best match
				best = surfacenum;
				bestdist = dist;
			}
		}
	}
	PRVM_G_FLOAT(OFS_RETURN) = best;
}
//PF_getsurfaceclippedpoint, // #439 vector(entity e, float s, vector p) getsurfaceclippedpoint = #439;
void VM_getsurfaceclippedpoint(Program prog)
{
	prvm_edict_t *ed;
	dp_model_t *model;
	msurface_t *surface;
	vec3_t p, out, inp;
	VM_SAFEPARMCOUNT(3, VM_te_getsurfaceclippedpoint);
	VectorClear(PRVM_G_VECTOR(OFS_RETURN));
	ed = PRVM_G_EDICT(OFS_PARM0);
	if (!(model = getmodel(prog, ed)) || !(surface = getsurface(model, (int)PRVM_G_FLOAT(OFS_PARM1))))
		return;
	animatemodel(prog, model, ed);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM2), inp);
	applytransform_inverted(prog, inp, ed, p);
	clippointtosurface(prog, ed, model, surface, p, out);
	VectorAdd(out, PRVM_serveredictvector(ed, origin), PRVM_G_VECTOR(OFS_RETURN));
}

//PF_getsurfacenumtriangles, // #??? float(entity e, float s) getsurfacenumtriangles = #???;
void VM_getsurfacenumtriangles(Program prog)
{
       dp_model_t *model;
       msurface_t *surface;
       VM_SAFEPARMCOUNT(2, VM_SV_getsurfacenumtriangles);
       // return 0 if no such surface
       if (!(model = getmodel(prog, PRVM_G_EDICT(OFS_PARM0))) || !(surface = getsurface(model, (int)PRVM_G_FLOAT(OFS_PARM1))))
       {
               PRVM_G_FLOAT(OFS_RETURN) = 0;
               return;
       }

       PRVM_G_FLOAT(OFS_RETURN) = surface->num_triangles;
}
//PF_getsurfacetriangle,     // #??? vector(entity e, float s, float n) getsurfacetriangle = #???;
void VM_getsurfacetriangle(Program prog)
{
       const vec3_t d = {-1, -1, -1};
       prvm_edict_t *ed;
       dp_model_t *model;
       msurface_t *surface;
       int trinum;
       VM_SAFEPARMCOUNT(3, VM_SV_getsurfacetriangle);
       VectorClear(PRVM_G_VECTOR(OFS_RETURN));
       ed = PRVM_G_EDICT(OFS_PARM0);
       if (!(model = getmodel(prog, ed)) || !(surface = getsurface(model, (int)PRVM_G_FLOAT(OFS_PARM1))))
               return;
       trinum = (int)PRVM_G_FLOAT(OFS_PARM2);
       if (trinum < 0 || trinum >= surface->num_triangles)
               return;
       // FIXME: implement rotation/scaling
       VectorMA(&(model->surfmesh.data_element3i + 3 * surface->num_firsttriangle)[trinum * 3], surface->num_firstvertex, d, PRVM_G_VECTOR(OFS_RETURN));
}

//
// physics builtins
//

#define VM_physics_ApplyCmd(ed,f) if (!ed->priv.server->ode_body) VM_physics_newstackfunction(prog, ed, f); else World_Physics_ApplyCmd(ed, f)

static edict_odefunc_t *VM_physics_newstackfunction(Program prog, prvm_edict_t *ed, edict_odefunc_t *f)
{
	edict_odefunc_t *newfunc, *func;

	newfunc = (edict_odefunc_t *)Mem_Alloc(prog->progs_mempool, sizeof(edict_odefunc_t));
	memcpy(newfunc, f, sizeof(edict_odefunc_t));
	newfunc->next = nullptr;
	if (!ed->priv.server->ode_func)
		ed->priv.server->ode_func = newfunc;
	else
	{
		for (func = ed->priv.server->ode_func; func->next; func = func->next);
		func->next = newfunc;
	}
	return newfunc;
}

// void(entity e, float physics_enabled) physics_enable = #;
void vm::physics::enable(Program prog)
{
	prvm_edict_t *ed;
	edict_odefunc_t f;

	VM_SAFEPARMCOUNT(2, VM_physics_enable);
	ed = PRVM_G_EDICT(OFS_PARM0);
	if (!ed)
	{
		if (developer.integer > 0)
			VM_Warning(prog, "VM_physics_enable: null entity!\n");
		return;
	}
	// entity should have MOVETYPE_PHYSICS already set, this can damage memory (making leaked allocation) so warn about this even if non-developer
	if (PRVM_serveredictfloat(ed, movetype) != MOVETYPE_PHYSICS)
	{
		VM_Warning(prog, "VM_physics_enable: entity is not MOVETYPE_PHYSICS!\n");
		return;
	}
	f.type = PRVM_G_FLOAT(OFS_PARM1) == 0 ? ODEFUNC_DISABLE : ODEFUNC_ENABLE;
	VM_physics_ApplyCmd(ed, &f);
}

// void(entity e, vector force, vector relative_ofs) physics_addforce = #;
void vm::physics::addforce(Program prog)
{
	prvm_edict_t *ed;
	edict_odefunc_t f;

	VM_SAFEPARMCOUNT(3, VM_physics_addforce);
	ed = PRVM_G_EDICT(OFS_PARM0);
	if (!ed)
	{
		if (developer.integer > 0)
			VM_Warning(prog, "VM_physics_addforce: null entity!\n");
		return;
	}
	// entity should have MOVETYPE_PHYSICS already set, this can damage memory (making leaked allocation) so warn about this even if non-developer
	if (PRVM_serveredictfloat(ed, movetype) != MOVETYPE_PHYSICS)
	{
		VM_Warning(prog, "VM_physics_addforce: entity is not MOVETYPE_PHYSICS!\n");
		return;
	}
	f.type = ODEFUNC_FORCE;
	VectorCopy(PRVM_G_VECTOR(OFS_PARM1), f.v1);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM2), f.v2);
	VM_physics_ApplyCmd(ed, &f);
}

// void(entity e, vector torque) physics_addtorque = #;
void vm::physics::addtorque(Program prog)
{
	prvm_edict_t *ed;
	edict_odefunc_t f;

	VM_SAFEPARMCOUNT(2, VM_physics_addtorque);
	ed = PRVM_G_EDICT(OFS_PARM0);
	if (!ed)
	{
		if (developer.integer > 0)
			VM_Warning(prog, "VM_physics_addtorque: null entity!\n");
		return;
	}
	// entity should have MOVETYPE_PHYSICS already set, this can damage memory (making leaked allocation) so warn about this even if non-developer
	if (PRVM_serveredictfloat(ed, movetype) != MOVETYPE_PHYSICS)
	{
		VM_Warning(prog, "VM_physics_addtorque: entity is not MOVETYPE_PHYSICS!\n");
		return;
	}
	f.type = ODEFUNC_TORQUE;
	VectorCopy(PRVM_G_VECTOR(OFS_PARM1), f.v1);
	VM_physics_ApplyCmd(ed, &f);
}
#include "quakedef.h"

#include "prvm_cmds.h"
#include "csprogs.h"
#include "cl_collision.h"
#include "r_shadow.h"
#include "jpeg.h"
#include "image.h"

//============================================================================
// Client
//[515]: unsolved PROBLEMS
//- finish player physics code (cs_runplayerphysics)
//- EntWasFreed ?
//- RF_DEPTHHACK is not like it should be
//- add builtin that sets cl.viewangles instead of reading "input_angles" global
//- finish lines support for R_Polygon***
//- insert selecttraceline into traceline somehow

//4 feature darkplaces csqc: add builtin to clientside qc for reading triangles of model meshes (useful to orient a ui along a triangle of a model mesh)
//4 feature darkplaces csqc: add builtins to clientside qc for gl calls

extern cvar_t v_flipped;
extern cvar_t r_equalize_entities_fullbright;

r_refdef_view_t csqc_original_r_refdef_view;
r_refdef_view_t csqc_main_r_refdef_view;

// #1 void(vector ang) makevectors
void VM_CL_makevectors (prvm_prog_t *prog)
{
	vec3_t angles, forward, right, up;
	VM_SAFEPARMCOUNT(1, VM_CL_makevectors);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), angles);
	AngleVectors(angles, forward, right, up);
	VectorCopy(forward, PRVM_clientglobalvector(v_forward));
	VectorCopy(right, PRVM_clientglobalvector(v_right));
	VectorCopy(up, PRVM_clientglobalvector(v_up));
}

// #2 void(entity e, vector o) setorigin
void VM_CL_setorigin (prvm_prog_t *prog)
{
	prvm_edict_t	*e;
	prvm_vec_t	*org;
	VM_SAFEPARMCOUNT(2, VM_CL_setorigin);

	e = PRVM_G_EDICT(OFS_PARM0);
	if (e == prog->edicts)
	{
		VM_Warning(prog, "setorigin: can not modify world entity\n");
		return;
	}
	if (e->priv.required->free)
	{
		VM_Warning(prog, "setorigin: can not modify free entity\n");
		return;
	}
	org = PRVM_G_VECTOR(OFS_PARM1);
	VectorCopy (org, PRVM_clientedictvector(e, origin));
	if(e->priv.required->mark == PRVM_EDICT_MARK_WAIT_FOR_SETORIGIN)
		e->priv.required->mark = PRVM_EDICT_MARK_SETORIGIN_CAUGHT;
	CL_LinkEdict(e);
}

void SetMinMaxSizePRVM (prvm_prog_t *prog, prvm_edict_t *e, prvm_vec_t *min, prvm_vec_t *max)
{
	int		i;

	for (i=0 ; i<3 ; i++)
		if (min[i] > max[i])
			prog->error_cmd("SetMinMaxSize: backwards mins/maxs");

	// set derived values
	VectorCopy (min, PRVM_clientedictvector(e, mins));
	VectorCopy (max, PRVM_clientedictvector(e, maxs));
	VectorSubtract (max, min, PRVM_clientedictvector(e, size));

	CL_LinkEdict (e);
}

void SetMinMaxSize (prvm_prog_t *prog, prvm_edict_t *e, const vec_t *min, const vec_t *max)
{
	prvm_vec3_t mins, maxs;
	VectorCopy(min, mins);
	VectorCopy(max, maxs);
	SetMinMaxSizePRVM(prog, e, mins, maxs);
}

// #3 void(entity e, string m) setmodel
void vm::client::setmodel (prvm_prog_t *prog)
{
	prvm_edict_t	*e;
	const char		*m;
	dp_model_t *mod;
	int				i;

	VM_SAFEPARMCOUNT(2, VM_CL_setmodel);

	e = PRVM_G_EDICT(OFS_PARM0);
	PRVM_clientedictfloat(e, modelindex) = 0;
	PRVM_clientedictstring(e, model) = 0;

	m = PRVM_G_STRING(OFS_PARM1);
	mod = nullptr;
	for (i = 0;i < MAX_MODELS && cl.csqc_model_precache[i];i++)
	{
		if (!strcmp(cl.csqc_model_precache[i]->name, m))
		{
			mod = cl.csqc_model_precache[i];
			PRVM_clientedictstring(e, model) = PRVM_SetEngineString(prog, mod->name);
			PRVM_clientedictfloat(e, modelindex) = -(i+1);
			break;
		}
	}

	if( !mod ) {
		for (i = 0;i < MAX_MODELS;i++)
		{
			mod = cl.model_precache[i];
			if (mod && !strcmp(mod->name, m))
			{
				PRVM_clientedictstring(e, model) = PRVM_SetEngineString(prog, mod->name);
				PRVM_clientedictfloat(e, modelindex) = i;
				break;
			}
		}
	}

	if( mod ) {
		// TODO: check if this breaks needed consistency and maybe add a cvar for it too?? [1/10/2008 Black]
		// LordHavoc: erm you broke it by commenting this out - setmodel must do setsize or else the qc can't find out the model size, and ssqc does this by necessity, consistency.
		SetMinMaxSize (prog, e, mod->normalmins, mod->normalmaxs);
	}
	else
	{
		SetMinMaxSize (prog, e, vec3_origin, vec3_origin);
		VM_Warning(prog, "setmodel: model '%s' not precached\n", m);
	}
}

// #4 void(entity e, vector min, vector max) setsize
void vm::client::setsize (prvm_prog_t *prog)
{
	prvm_edict_t	*e;
	vec3_t		mins, maxs;
	VM_SAFEPARMCOUNT(3, VM_CL_setsize);

	e = PRVM_G_EDICT(OFS_PARM0);
	if (e == prog->edicts)
	{
		VM_Warning(prog, "setsize: can not modify world entity\n");
		return;
	}
	if (e->priv.server->free)
	{
		VM_Warning(prog, "setsize: can not modify free entity\n");
		return;
	}
	VectorCopy(PRVM_G_VECTOR(OFS_PARM1), mins);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM2), maxs);

	SetMinMaxSize( prog, e, mins, maxs );

	CL_LinkEdict(e);
}

// #8 void(entity e, float chan, string samp, float volume, float atten[, float pitchchange[, float flags]]) sound
void vm::client::sound (prvm_prog_t *prog)
{
	const char			*sample;
	int					channel;
	prvm_edict_t		*entity;
	float 				volume;
	float				attenuation;
	float pitchchange;
	float				startposition;
	int flags;
	vec3_t				org;

	VM_SAFEPARMCOUNTRANGE(5, 7, VM_CL_sound);

	entity = PRVM_G_EDICT(OFS_PARM0);
	channel = (int)PRVM_G_FLOAT(OFS_PARM1);
	sample = PRVM_G_STRING(OFS_PARM2);
	volume = PRVM_G_FLOAT(OFS_PARM3);
	attenuation = PRVM_G_FLOAT(OFS_PARM4);

	if (volume < 0 || volume > 1)
	{
		VM_Warning(prog, "VM_CL_sound: volume must be in range 0-1\n");
		return;
	}

	if (attenuation < 0 || attenuation > 4)
	{
		VM_Warning(prog, "VM_CL_sound: attenuation must be in range 0-4\n");
		return;
	}

	if (prog->argc < 6)
		pitchchange = 0;
	else
		pitchchange = PRVM_G_FLOAT(OFS_PARM5);

	if (prog->argc < 7)
		flags = 0;
	else
	{
		// LordHavoc: we only let the qc set certain flags, others are off-limits
		flags = (int)PRVM_G_FLOAT(OFS_PARM6) & (CHANNELFLAG_RELIABLE | CHANNELFLAG_FORCELOOP | CHANNELFLAG_PAUSED);
	}

	// sound_starttime exists instead of sound_startposition because in a
	// networking sense you might not know when something is being received,
	// so making sounds match up in sync would be impossible if relative
	// position was sent
	if (PRVM_clientglobalfloat(sound_starttime))
		startposition = cl.time - PRVM_clientglobalfloat(sound_starttime);
	else
		startposition = 0;

	channel = CHAN_USER2ENGINE(channel);

	if (!IS_CHAN(channel))
	{
		VM_Warning(prog, "VM_CL_sound: channel must be in range 0-127\n");
		return;
	}

	CL_VM_GetEntitySoundOrigin(MAX_EDICTS + PRVM_NUM_FOR_EDICT(entity), org);
	S_StartSound_StartPosition_Flags(MAX_EDICTS + PRVM_NUM_FOR_EDICT(entity), channel, S_FindName(sample), org, volume, attenuation, startposition, flags, pitchchange > 0.0f ? pitchchange * 0.01f : 1.0f);
}

// #483 void(vector origin, string sample, float volume, float attenuation) pointsound
void vm::client::pointsound(prvm_prog_t *prog)
{
	const char			*sample;
	float 				volume;
	float				attenuation;
	vec3_t				org;

	VM_SAFEPARMCOUNT(4, VM_CL_pointsound);

	VectorCopy( PRVM_G_VECTOR(OFS_PARM0), org);
	sample = PRVM_G_STRING(OFS_PARM1);
	volume = PRVM_G_FLOAT(OFS_PARM2);
	attenuation = PRVM_G_FLOAT(OFS_PARM3);

	if (volume < 0 || volume > 1)
	{
		VM_Warning(prog, "VM_CL_pointsound: volume must be in range 0-1\n");
		return;
	}

	if (attenuation < 0 || attenuation > 4)
	{
		VM_Warning(prog, "VM_CL_pointsound: attenuation must be in range 0-4\n");
		return;
	}

	// Send World Entity as Entity to Play Sound (for CSQC, that is MAX_EDICTS)
	S_StartSound(MAX_EDICTS, 0, S_FindName(sample), org, volume, attenuation);
}

// #14 entity() spawn
void vm::client::spawn (prvm_prog_t *prog)
{
	prvm_edict_t *ed;
	ed = PRVM_ED_Alloc(prog);
	VM_RETURN_EDICT(ed);
}

void vm::client::setTraceGlobals(prvm_prog_t *prog, const trace_t *trace, int svent)
{
	VM_SetTraceGlobals(prog, trace);
	PRVM_clientglobalfloat(trace_networkentity) = svent;
}

#define CL_HitNetworkBrushModels(move) !((move) == MOVE_WORLDONLY)
#define CL_HitNetworkPlayers(move)     !((move) == MOVE_WORLDONLY || (move) == MOVE_NOMONSTERS)

// #16 void(vector v1, vector v2, float movetype, entity ignore) traceline
void vm::client::traceline (prvm_prog_t *prog)
{
	vec3_t	v1, v2;
	trace_t	trace;
	int		move, svent;
	prvm_edict_t	*ent;


	VM_SAFEPARMCOUNTRANGE(4, 4, VM_CL_traceline);


	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), v1);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM1), v2);
	move = (int)PRVM_G_FLOAT(OFS_PARM2);
	ent = PRVM_G_EDICT(OFS_PARM3);

	if (VEC_IS_NAN(v1[0]) || VEC_IS_NAN(v1[1]) || VEC_IS_NAN(v1[2]) || VEC_IS_NAN(v2[0]) || VEC_IS_NAN(v2[1]) || VEC_IS_NAN(v2[2]))
		prog->error_cmd("%s: NAN errors detected in traceline('%f %f %f', '%f %f %f', %i, entity %i)\n", prog->name, v1[0], v1[1], v1[2], v2[0], v2[1], v2[2], move, PRVM_EDICT_TO_PROG(ent));

	trace = CL_TraceLine(v1, v2, move, ent, CL_GenericHitSuperContentsMask(ent), collision_extendtracelinelength.value, CL_HitNetworkBrushModels(move), CL_HitNetworkPlayers(move), &svent, true, false);

	CL_VM_SetTraceGlobals(prog, &trace, svent);
}

/*
=================
VM_CL_tracebox

Used for use tracing and shot targeting
Traces are blocked by bbox and exact bsp entityes, and also slide box entities
if the tryents flag is set.

tracebox (vector1, vector mins, vector maxs, vector2, tryents)
=================
*/
// LordHavoc: added this for my own use, VERY useful, similar to traceline
void vm::client::tracebox (prvm_prog_t *prog)
{
	vec3_t	v1, v2, m1, m2;
	trace_t	trace;
	int		move, svent;
	prvm_edict_t	*ent;

	VM_SAFEPARMCOUNTRANGE(6, 8, VM_CL_tracebox); // allow more parameters for future expansion

	prog->xfunction->builtinsprofile += 30;

	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), v1);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM1), m1);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM2), m2);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM3), v2);
	move = (int)PRVM_G_FLOAT(OFS_PARM4);
	ent = PRVM_G_EDICT(OFS_PARM5);

	if (VEC_IS_NAN(v1[0]) || VEC_IS_NAN(v1[1]) || VEC_IS_NAN(v1[2]) || VEC_IS_NAN(v2[0]) || VEC_IS_NAN(v2[1]) || VEC_IS_NAN(v2[2]))
		prog->error_cmd("%s: NAN errors detected in tracebox('%f %f %f', '%f %f %f', '%f %f %f', '%f %f %f', %i, entity %i)\n", prog->name, v1[0], v1[1], v1[2], m1[0], m1[1], m1[2], m2[0], m2[1], m2[2], v2[0], v2[1], v2[2], move, PRVM_EDICT_TO_PROG(ent));

	trace = CL_TraceBox(v1, m1, m2, v2, move, ent, CL_GenericHitSuperContentsMask(ent), collision_extendtraceboxlength.value, CL_HitNetworkBrushModels(move), CL_HitNetworkPlayers(move), &svent, true);

	CL_VM_SetTraceGlobals(prog, &trace, svent);
}

static trace_t CL_Trace_Toss (prvm_prog_t *prog, prvm_edict_t *tossent, prvm_edict_t *ignore, int *svent)
{
	int i;
	float gravity;
	vec3_t start, end, mins, maxs, move;
	vec3_t original_origin;
	vec3_t original_velocity;
	vec3_t original_angles;
	vec3_t original_avelocity;
	trace_t trace;

	VectorCopy(PRVM_clientedictvector(tossent, origin)   , original_origin   );
	VectorCopy(PRVM_clientedictvector(tossent, velocity) , original_velocity );
	VectorCopy(PRVM_clientedictvector(tossent, angles)   , original_angles   );
	VectorCopy(PRVM_clientedictvector(tossent, avelocity), original_avelocity);

	gravity = PRVM_clientedictfloat(tossent, gravity);
	if (!gravity)
		gravity = 1.0f;
	gravity *= cl.movevars_gravity * 0.05;

	for (i = 0;i < 200;i++) // LordHavoc: sanity check; never trace more than 10 seconds
	{
		PRVM_clientedictvector(tossent, velocity)[2] -= gravity;
		VectorMA (PRVM_clientedictvector(tossent, angles), 0.05, PRVM_clientedictvector(tossent, avelocity), PRVM_clientedictvector(tossent, angles));
		VectorScale (PRVM_clientedictvector(tossent, velocity), 0.05, move);
		VectorAdd (PRVM_clientedictvector(tossent, origin), move, end);
		VectorCopy(PRVM_clientedictvector(tossent, origin), start);
		VectorCopy(PRVM_clientedictvector(tossent, mins), mins);
		VectorCopy(PRVM_clientedictvector(tossent, maxs), maxs);
		trace = CL_TraceBox(start, mins, maxs, end, MOVE_NORMAL, tossent, CL_GenericHitSuperContentsMask(tossent), collision_extendmovelength.value, true, true, nullptr, true);
		VectorCopy (trace.endpos, PRVM_clientedictvector(tossent, origin));

		if (trace.fraction < 1)
			break;
	}

	VectorCopy(original_origin   , PRVM_clientedictvector(tossent, origin)   );
	VectorCopy(original_velocity , PRVM_clientedictvector(tossent, velocity) );
	VectorCopy(original_angles   , PRVM_clientedictvector(tossent, angles)   );
	VectorCopy(original_avelocity, PRVM_clientedictvector(tossent, avelocity));

	return trace;
}

void vm::client::tracetoss (prvm_prog_t *prog)
{
	trace_t	trace;
	prvm_edict_t	*ent;
	prvm_edict_t	*ignore;
	int svent = 0;

	prog->xfunction->builtinsprofile += 600;

	VM_SAFEPARMCOUNT(2, VM_CL_tracetoss);

	ent = PRVM_G_EDICT(OFS_PARM0);
	if (ent == prog->edicts)
	{
		VM_Warning(prog, "tracetoss: can not use world entity\n");
		return;
	}
	ignore = PRVM_G_EDICT(OFS_PARM1);

	trace = CL_Trace_Toss (prog, ent, ignore, &svent);

	CL_VM_SetTraceGlobals(prog, &trace, svent);
}


// #20 void(string s) precache_model
bool vm::client::precacheModel (Program prog, const char* name)
{
	for (size32 i = 0; likely(i < MAX_MODELS && cl.csqc_model_precache[i]); i++)
	{
		if(unlikely(!strcmp(cl.csqc_model_precache[i]->name, name)))
			return false;
	}

	dp_model_t	*m = Mod_ForName(name, false, false, name[0] == '*' ? cl.model_name[1] : nullptr);
	if(likely(m != nullptr && m->loaded == true))
	{
		for (size32 i = 0; i < MAX_MODELS; i++)
		{
			if (!cl.csqc_model_precache[i])
			{
				cl.csqc_model_precache[i] = (dp_model_t*)m;
				return true;
			}
		}
		vm::warn(prog, "vm::client::precacheModel: no free models\n");
		return false;
	}
	vm::warn(prog, "vm::client::precacheModel: model \"%s\" not found\n", name);
	return false;
}

// #22 entity(vector org, float rad) findradius
void vm::client::findRadius (prvm_prog_t *prog)
{
	prvm_edict_t	*ent, *chain;
	vec_t			radius, radius2;
	vec3_t			org, eorg, mins, maxs;
	int				i, numtouchedicts;
	static prvm_edict_t	*touchedicts[MAX_EDICTS];
	int             chainfield;

	VM_SAFEPARMCOUNTRANGE(2, 3, VM_CL_findradius);

	if(prog->argc == 3)
		chainfield = PRVM_G_INT(OFS_PARM2);
	else
		chainfield = prog->fieldoffsets.chain;
	if(unlikely(chainfield < 0))
		prog->error_cmd("VM_findchain: %s doesnt have the specified chain field !", prog->name);

	chain = (prvm_edict_t *)prog->edicts;

	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), org);
	radius = PRVM_G_FLOAT(OFS_PARM1);
	radius2 = radius * radius;

	mins[0] = org[0] - (radius + 1);
	mins[1] = org[1] - (radius + 1);
	mins[2] = org[2] - (radius + 1);
	maxs[0] = org[0] + (radius + 1);
	maxs[1] = org[1] + (radius + 1);
	maxs[2] = org[2] + (radius + 1);
	numtouchedicts = World_EntitiesInBox(&cl.world, mins, maxs, MAX_EDICTS, touchedicts);
	if (numtouchedicts > MAX_EDICTS)
	{
		// this never happens	//[515]: for what then ?
		Con_Printf("CSQC_EntitiesInBox returned %i edicts, max was %i\n", numtouchedicts, MAX_EDICTS);
		numtouchedicts = MAX_EDICTS;
	}
	for (i = 0;i < numtouchedicts;i++)
	{
		ent = touchedicts[i];
		// Quake did not return non-solid entities but darkplaces does
		// (note: this is the reason you can't blow up fallen zombies)
		if (PRVM_clientedictfloat(ent, solid) == SOLID_NOT && !sv_gameplayfix_blowupfallenzombies.integer)
			continue;
		// LordHavoc: compare against bounding box rather than center so it
		// doesn't miss large objects, and use DotProduct instead of Length
		// for a major speedup
		VectorSubtract(org, PRVM_clientedictvector(ent, origin), eorg);
		if (sv_gameplayfix_findradiusdistancetobox.integer)
		{
			eorg[0] -= bound(PRVM_clientedictvector(ent, mins)[0], eorg[0], PRVM_clientedictvector(ent, maxs)[0]);
			eorg[1] -= bound(PRVM_clientedictvector(ent, mins)[1], eorg[1], PRVM_clientedictvector(ent, maxs)[1]);
			eorg[2] -= bound(PRVM_clientedictvector(ent, mins)[2], eorg[2], PRVM_clientedictvector(ent, maxs)[2]);
		}
		else
			VectorMAMAM(1, eorg, -0.5f, PRVM_clientedictvector(ent, mins), -0.5f, PRVM_clientedictvector(ent, maxs), eorg);
		if (DotProduct(eorg, eorg) < radius2)
		{
			PRVM_EDICTFIELDEDICT(ent, chainfield) = PRVM_EDICT_TO_PROG(chain);
			chain = ent;
		}
	}

	VM_RETURN_EDICT(chain);
}

// #34 float() droptofloor
static bool vm::client::dropToFloor (Program prog)
{
	prvm_edict_t		*ent;
	vec3_t				start, end, mins, maxs;
	trace_t				trace;

	VM_SAFEPARMCOUNTRANGE(0, 2, VM_CL_droptofloor); // allow 2 parameters because the id1 defs.qc had an incorrect prototype

	// assume failure if it returns early
	PRVM_G_FLOAT(OFS_RETURN) = 0;

	//ent = PRVM_PROG_TO_EDICT(PRVM_clientglobaledict(self));
	Edict* ent = prog->edicts + static_cast<size_t>(
			((prvm_eval_t *)(prog->globals.fp + prog->globaloffsets.self))->edict);

	if (unlikely(ent == prog->edicts))
	{
		VM_Warning(prog, "droptofloor: can not modify world entity\n");
		return false;
	}
	if (unlikely(ent->priv.server->free))
	{
		VM_Warning(prog, "droptofloor: can not modify free entity\n");
		return false;
	}

	VectorCopy(PRVM_clientedictvector(ent, origin), start);
	VectorCopy(PRVM_clientedictvector(ent, mins), mins);
	VectorCopy(PRVM_clientedictvector(ent, maxs), maxs);
	VectorCopy(PRVM_clientedictvector(ent, origin), end);
	end[2] -= 256.0f;

	trace = CL_TraceBox(
		start,
		mins,
		maxs,
		end,
		MOVE_NORMAL,
		ent,
		CL_GenericHitSuperContentsMask(ent),
		collision_extendmovelength.value,
		true,
		true,
		nullptr,
		true
	);

	if (trace.fraction != 1)
	{
		VectorCopy (trace.endpos, PRVM_clientedictvector(ent, origin));
		PRVM_clientedictfloat(ent, flags) = (int)PRVM_clientedictfloat(ent, flags) | FL_ONGROUND;
		PRVM_clientedictedict(ent, groundentity) = PRVM_EDICT_TO_PROG(trace.ent);
		return true;
	}
	return false;
}

// #35 void(float style, string value) lightstyle
void vm::client::lightStyle (const Program prog, const size32 style, const char* value)
{
	if (unlikely(i >= cl.max_lightstyle))
	{
		vm::warn(prog, "VM_CL_lightstyle >= MAX_LIGHTSTYLES\n");
		return;
	}
	lightstyle_t* RESTRICT const stylePtr = &cl.lightstyle[style];

	strlcpy (stylePtr->map, value, sizeof(stylePtr->map));
	stylePtr->map[MAX_STYLESTRING - 1] = 0;
	stylePtr->length = static_cast<int>(strlen(stylePtr->map));
}

// #40 float(entity e) checkbottom
void VM_CL_checkbottom (prvm_prog_t *prog)
{
	static int		cs_yes, cs_no;
	prvm_edict_t	*ent;
	vec3_t			mins, maxs, start, stop;
	trace_t			trace;
	int				x, y;
	float			mid, bottom;

	VM_SAFEPARMCOUNT(1, VM_CL_checkbottom);
	ent = PRVM_G_EDICT(OFS_PARM0);
	PRVM_G_FLOAT(OFS_RETURN) = 0;

	VectorAdd (PRVM_clientedictvector(ent, origin), PRVM_clientedictvector(ent, mins), mins);
	VectorAdd (PRVM_clientedictvector(ent, origin), PRVM_clientedictvector(ent, maxs), maxs);

// if all of the points under the corners are solid world, don't bother
// with the tougher checks
// the corners must be within 16 of the midpoint
	start[2] = mins[2] - 1;
	for	(x=0 ; x<=1 ; x++)
		for	(y=0 ; y<=1 ; y++)
		{
			start[0] = x ? maxs[0] : mins[0];
			start[1] = y ? maxs[1] : mins[1];
			if (!(CL_PointSuperContents(start) & (SUPERCONTENTS_SOLID | SUPERCONTENTS_BODY)))
				goto realcheck;
		}

	cs_yes++;
	PRVM_G_FLOAT(OFS_RETURN) = true;
	return;		// we got out easy

realcheck:
	cs_no++;
//
// check it for real...
//
	start[2] = mins[2];

// the midpoint must be within 16 of the bottom
	start[0] = stop[0] = (mins[0] + maxs[0])*0.5;
	start[1] = stop[1] = (mins[1] + maxs[1])*0.5;
	stop[2] = start[2] - 2*sv_stepheight.value;
	trace = CL_TraceLine(start, stop, MOVE_NORMAL, ent, CL_GenericHitSuperContentsMask(ent), collision_extendmovelength.value, true, true, nullptr, true, false);

	if (trace.fraction == 1.0)
		return;

	mid = bottom = trace.endpos[2];

// the corners must be within 16 of the midpoint
	for	(x=0 ; x<=1 ; x++)
		for	(y=0 ; y<=1 ; y++)
		{
			start[0] = stop[0] = x ? maxs[0] : mins[0];
			start[1] = stop[1] = y ? maxs[1] : mins[1];

			trace = CL_TraceLine(start, stop, MOVE_NORMAL, ent, CL_GenericHitSuperContentsMask(ent), collision_extendmovelength.value, true, true, nullptr, true, false);

			if (trace.fraction != 1.0 && trace.endpos[2] > bottom)
				bottom = trace.endpos[2];
			if (trace.fraction == 1.0 || mid - trace.endpos[2] > sv_stepheight.value)
				return;
		}

	cs_yes++;
	PRVM_G_FLOAT(OFS_RETURN) = true;
}

// #41 float(vector v) pointcontents
void VM_CL_pointcontents (prvm_prog_t *prog)
{
	vec3_t point;
	VM_SAFEPARMCOUNT(1, VM_CL_pointcontents);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), point);
	PRVM_G_FLOAT(OFS_RETURN) = Mod_Q1BSP_NativeContentsFromSuperContents(nullptr, CL_PointSuperContents(point));
}

// #48 void(vector o, vector d, float color, float count) particle
void VM_CL_particle (prvm_prog_t *prog)
{
	vec3_t org, dir;
	int		count;
	unsigned char	color;
	VM_SAFEPARMCOUNT(4, VM_CL_particle);

	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), org);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM1), dir);
	color = (int)PRVM_G_FLOAT(OFS_PARM2);
	count = (int)PRVM_G_FLOAT(OFS_PARM3);
	CL_ParticleEffect(EFFECT_SVC_PARTICLE, count, org, org, dir, dir, nullptr, color);
}

// #74 void(vector pos, string samp, float vol, float atten) ambientsound
void VM_CL_ambientsound (prvm_prog_t *prog)
{
	vec3_t f;
	sfx_t	*s;
	VM_SAFEPARMCOUNT(4, VM_CL_ambientsound);
	s = S_FindName(PRVM_G_STRING(OFS_PARM0));
	VectorCopy(PRVM_G_VECTOR(OFS_PARM1), f);
	S_StaticSound (s, f, PRVM_G_FLOAT(OFS_PARM2), PRVM_G_FLOAT(OFS_PARM3)*64);
}

// #92 vector(vector org[, float lpflag]) getlight (DP_QC_GETLIGHT)
void VM_CL_getlight (prvm_prog_t *prog)
{
	vec3_t ambientcolor, diffusecolor, diffusenormal;
	vec3_t p;

	VM_SAFEPARMCOUNTRANGE(1, 3, VM_CL_getlight);

	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), p);
	VectorClear(ambientcolor);
	VectorClear(diffusecolor);
	VectorClear(diffusenormal);
	if (prog->argc >= 2)
		R_CompleteLightPoint(ambientcolor, diffusecolor, diffusenormal, p, PRVM_G_FLOAT(OFS_PARM1));
	else if (cl.worldmodel && cl.worldmodel->brush.LightPoint)
		cl.worldmodel->brush.LightPoint(cl.worldmodel, p, ambientcolor, diffusecolor, diffusenormal);
	VectorMA(ambientcolor, 0.5, diffusecolor, PRVM_G_VECTOR(OFS_RETURN));
	if (PRVM_clientglobalvector(getlight_ambient))
		VectorCopy(ambientcolor, PRVM_clientglobalvector(getlight_ambient));
	if (PRVM_clientglobalvector(getlight_diffuse))
		VectorCopy(diffusecolor, PRVM_clientglobalvector(getlight_diffuse));
	if (PRVM_clientglobalvector(getlight_dir))
		VectorCopy(diffusenormal, PRVM_clientglobalvector(getlight_dir));
}

//============================================================================
//[515]: SCENE MANAGER builtins

void CSQC_R_RecalcView ()
{
	extern matrix4x4_t viewmodelmatrix_nobob;
	extern matrix4x4_t viewmodelmatrix_withbob;
	Matrix4x4_CreateFromQuakeEntity(&r_refdef.view.matrix, cl.csqc_vieworigin[0], cl.csqc_vieworigin[1], cl.csqc_vieworigin[2], cl.csqc_viewangles[0], cl.csqc_viewangles[1], cl.csqc_viewangles[2], 1);
	Matrix4x4_Copy(&viewmodelmatrix_nobob, &r_refdef.view.matrix);
	Matrix4x4_ConcatScale(&viewmodelmatrix_nobob, cl_viewmodel_scale.value);
	Matrix4x4_Concat(&viewmodelmatrix_withbob, &r_refdef.view.matrix, &cl.csqc_viewmodelmatrixfromengine);
}

//#300 void() clearscene (EXT_CSQC)
void VM_CL_R_ClearScene (prvm_prog_t *prog)
{
	VM_SAFEPARMCOUNT(0, VM_CL_R_ClearScene);
	// clear renderable entity and light lists
	r_refdef.scene.numentities = 0;
	r_refdef.scene.numlights = 0;
	// restore the view settings to the values that VM_CL_UpdateView received from the client code
	r_refdef.view = csqc_original_r_refdef_view;
	VectorCopy(cl.csqc_vieworiginfromengine, cl.csqc_vieworigin);
	VectorCopy(cl.csqc_viewanglesfromengine, cl.csqc_viewangles);
	cl.csqc_vidvars.drawworld = r_drawworld.integer != 0;
	cl.csqc_vidvars.drawenginesbar = false;
	cl.csqc_vidvars.drawcrosshair = false;
	CSQC_R_RecalcView();
}

//#301 void(float mask) addentities (EXT_CSQC)
void VM_CL_R_AddEntities (prvm_prog_t *prog)
{
	double t = Sys_DirtyTime();
	int			i, drawmask;
	prvm_edict_t *ed;
	VM_SAFEPARMCOUNT(1, VM_CL_R_AddEntities);
	drawmask = (int)PRVM_G_FLOAT(OFS_PARM0);
	CSQC_RelinkAllEntities(drawmask);
	CL_RelinkLightFlashes();

	PRVM_clientglobalfloat(time) = cl.time;
	for(i=1;i<prog->num_edicts;i++)
	{
		// so we can easily check if CSQC entity #edictnum is currently drawn
		cl.csqcrenderentities[i].entitynumber = 0;
		ed = &prog->edicts[i];
		if(ed->priv.required->free)
			continue;
		CSQC_Think(ed);
		if(ed->priv.required->free)
			continue;
		// note that for RF_USEAXIS entities, Predraw sets v_forward/v_right/v_up globals that are read by CSQC_AddRenderEdict
		CSQC_Predraw(ed);
		if(ed->priv.required->free)
			continue;
		if(!((int)PRVM_clientedictfloat(ed, drawmask) & drawmask))
			continue;
		CSQC_AddRenderEdict(ed, i);
	}

	// callprofile fixing hack: do not include this time in what is counted for CSQC_UpdateView
	t = Sys_DirtyTime() - t;if (t < 0 || t >= 1800) t = 0;
	if(!prog->isNative)
		prog->functions[PRVM_clientfunction(CSQC_UpdateView)].totaltime -= t;
}

//#302 void(entity ent) addentity (EXT_CSQC)
void VM_CL_R_AddEntity (prvm_prog_t *prog)
{
	double t = Sys_DirtyTime();
	VM_SAFEPARMCOUNT(1, VM_CL_R_AddEntity);
	CSQC_AddRenderEdict(PRVM_G_EDICT(OFS_PARM0), 0);
	t = Sys_DirtyTime() - t;if (t < 0 || t >= 1800) t = 0;
	if(!prog->isNative)
		prog->functions[PRVM_clientfunction(CSQC_UpdateView)].totaltime -= t;
}

//#303 float(float property, ...) setproperty (EXT_CSQC)
//#303 float(float property) getproperty
//#303 vector(float property) getpropertyvec
//#309 float(float property) getproperty
//#309 vector(float property) getpropertyvec
// VorteX: make this function be able to return previously set property if new value is not given
void VM_CL_R_SetView (prvm_prog_t *prog)
{
	int		c;
	prvm_vec_t	*f;
	float	k;

	VM_SAFEPARMCOUNTRANGE(1, 3, VM_CL_R_SetView);

	c = (int)PRVM_G_FLOAT(OFS_PARM0);

	// return value?
	if (prog->argc < 2)
	{
		switch(c)
		{
		case VF_MIN:
			VectorSet(PRVM_G_VECTOR(OFS_RETURN), r_refdef.view.x, r_refdef.view.y, 0);
			break;
		case VF_MIN_X:
			PRVM_G_FLOAT(OFS_RETURN) = r_refdef.view.x;
			break;
		case VF_MIN_Y:
			PRVM_G_FLOAT(OFS_RETURN) = r_refdef.view.y;
			break;
		case VF_SIZE:
			VectorSet(PRVM_G_VECTOR(OFS_RETURN), r_refdef.view.width, r_refdef.view.height, 0);
			break;
		case VF_SIZE_X:
			PRVM_G_FLOAT(OFS_RETURN) = r_refdef.view.width;
			break;
		case VF_SIZE_Y:
			PRVM_G_FLOAT(OFS_RETURN) = r_refdef.view.height;
			break;
		case VF_VIEWPORT:
			VM_Warning(prog, "VM_CL_R_GetView : VF_VIEWPORT can't be retrieved, use VF_MIN/VF_SIZE instead\n");
			break;
		case VF_FOV:
			VectorSet(PRVM_G_VECTOR(OFS_RETURN), r_refdef.view.ortho_x, r_refdef.view.ortho_y, 0);
			break;
		case VF_FOVX:
			PRVM_G_FLOAT(OFS_RETURN) = r_refdef.view.ortho_x;
			break;
		case VF_FOVY:
			PRVM_G_FLOAT(OFS_RETURN) = r_refdef.view.ortho_y;
			break;
		case VF_ORIGIN:
			VectorCopy(cl.csqc_vieworigin, PRVM_G_VECTOR(OFS_RETURN));
			break;
		case VF_ORIGIN_X:
			PRVM_G_FLOAT(OFS_RETURN) = cl.csqc_vieworigin[0];
			break;
		case VF_ORIGIN_Y:
			PRVM_G_FLOAT(OFS_RETURN) = cl.csqc_vieworigin[1];
			break;
		case VF_ORIGIN_Z:
			PRVM_G_FLOAT(OFS_RETURN) = cl.csqc_vieworigin[2];
			break;
		case VF_ANGLES:
			VectorCopy(cl.csqc_viewangles, PRVM_G_VECTOR(OFS_RETURN));
			break;
		case VF_ANGLES_X:
			PRVM_G_FLOAT(OFS_RETURN) = cl.csqc_viewangles[0];
			break;
		case VF_ANGLES_Y:
			PRVM_G_FLOAT(OFS_RETURN) = cl.csqc_viewangles[1];
			break;
		case VF_ANGLES_Z:
			PRVM_G_FLOAT(OFS_RETURN) = cl.csqc_viewangles[2];
			break;
		case VF_DRAWWORLD:
			PRVM_G_FLOAT(OFS_RETURN) = cl.csqc_vidvars.drawworld;
			break;
		case VF_DRAWENGINESBAR:
			PRVM_G_FLOAT(OFS_RETURN) = cl.csqc_vidvars.drawenginesbar;
			break;
		case VF_DRAWCROSSHAIR:
			PRVM_G_FLOAT(OFS_RETURN) = cl.csqc_vidvars.drawcrosshair;
			break;
		case VF_CL_VIEWANGLES:
			VectorCopy(cl.viewangles, PRVM_G_VECTOR(OFS_RETURN));;
			break;
		case VF_CL_VIEWANGLES_X:
			PRVM_G_FLOAT(OFS_RETURN) = cl.viewangles[0];
			break;
		case VF_CL_VIEWANGLES_Y:
			PRVM_G_FLOAT(OFS_RETURN) = cl.viewangles[1];
			break;
		case VF_CL_VIEWANGLES_Z:
			PRVM_G_FLOAT(OFS_RETURN) = cl.viewangles[2];
			break;
		case VF_PERSPECTIVE:
			PRVM_G_FLOAT(OFS_RETURN) = r_refdef.view.useperspective;
			break;
		case VF_CLEARSCREEN:
			PRVM_G_FLOAT(OFS_RETURN) = r_refdef.view.isoverlay;
			break;
		case VF_MAINVIEW:
			PRVM_G_FLOAT(OFS_RETURN) = r_refdef.view.ismain;
			break;
		case VF_FOG_DENSITY:
			PRVM_G_FLOAT(OFS_RETURN) = r_refdef.fog_density;
			break;
		case VF_FOG_COLOR:
			PRVM_G_VECTOR(OFS_RETURN)[0] = r_refdef.fog_red;
			PRVM_G_VECTOR(OFS_RETURN)[1] = r_refdef.fog_green;
			PRVM_G_VECTOR(OFS_RETURN)[2] = r_refdef.fog_blue;
			break;
		case VF_FOG_COLOR_R:
			PRVM_G_VECTOR(OFS_RETURN)[0] = r_refdef.fog_red;
			break;
		case VF_FOG_COLOR_G:
			PRVM_G_VECTOR(OFS_RETURN)[1] = r_refdef.fog_green;
			break;
		case VF_FOG_COLOR_B:
			PRVM_G_VECTOR(OFS_RETURN)[2] = r_refdef.fog_blue;
			break;
		case VF_FOG_ALPHA:
			PRVM_G_FLOAT(OFS_RETURN) = r_refdef.fog_alpha;
			break;
		case VF_FOG_START:
			PRVM_G_FLOAT(OFS_RETURN) = r_refdef.fog_start;
			break;
		case VF_FOG_END:
			PRVM_G_FLOAT(OFS_RETURN) = r_refdef.fog_end;
			break;
		case VF_FOG_HEIGHT:
			PRVM_G_FLOAT(OFS_RETURN) = r_refdef.fog_height;
			break;
		case VF_FOG_FADEDEPTH:
			PRVM_G_FLOAT(OFS_RETURN) = r_refdef.fog_fadedepth;
			break;
		case VF_MINFPS_QUALITY:
			PRVM_G_FLOAT(OFS_RETURN) = r_refdef.view.quality;
			break;
		default:
			PRVM_G_FLOAT(OFS_RETURN) = 0;
			VM_Warning(prog, "VM_CL_R_GetView : unknown parm %i\n", c);
			return;
		}
		return;
	}

	f = PRVM_G_VECTOR(OFS_PARM1);
	k = PRVM_G_FLOAT(OFS_PARM1);
	switch(c)
	{
	case VF_MIN:
		r_refdef.view.x = (int)(f[0]);
		r_refdef.view.y = (int)(f[1]);
		DrawQ_RecalcView();
		break;
	case VF_MIN_X:
		r_refdef.view.x = (int)(k);
		DrawQ_RecalcView();
		break;
	case VF_MIN_Y:
		r_refdef.view.y = (int)(k);
		DrawQ_RecalcView();
		break;
	case VF_SIZE:
		r_refdef.view.width = (int)(f[0]);
		r_refdef.view.height = (int)(f[1]);
		DrawQ_RecalcView();
		break;
	case VF_SIZE_X:
		r_refdef.view.width = (int)(k);
		DrawQ_RecalcView();
		break;
	case VF_SIZE_Y:
		r_refdef.view.height = (int)(k);
		DrawQ_RecalcView();
		break;
	case VF_VIEWPORT:
		r_refdef.view.x = (int)(f[0]);
		r_refdef.view.y = (int)(f[1]);
		f = PRVM_G_VECTOR(OFS_PARM2);
		r_refdef.view.width = (int)(f[0]);
		r_refdef.view.height = (int)(f[1]);
		DrawQ_RecalcView();
		break;
	case VF_FOV:
		r_refdef.view.frustum_x = tan(f[0] * M_PI / 360.0);r_refdef.view.ortho_x = f[0];
		r_refdef.view.frustum_y = tan(f[1] * M_PI / 360.0);r_refdef.view.ortho_y = f[1];
		break;
	case VF_FOVX:
		r_refdef.view.frustum_x = tan(k * M_PI / 360.0);r_refdef.view.ortho_x = k;
		break;
	case VF_FOVY:
		r_refdef.view.frustum_y = tan(k * M_PI / 360.0);r_refdef.view.ortho_y = k;
		break;
	case VF_ORIGIN:
		VectorCopy(f, cl.csqc_vieworigin);
		CSQC_R_RecalcView();
		break;
	case VF_ORIGIN_X:
		cl.csqc_vieworigin[0] = k;
		CSQC_R_RecalcView();
		break;
	case VF_ORIGIN_Y:
		cl.csqc_vieworigin[1] = k;
		CSQC_R_RecalcView();
		break;
	case VF_ORIGIN_Z:
		cl.csqc_vieworigin[2] = k;
		CSQC_R_RecalcView();
		break;
	case VF_ANGLES:
		VectorCopy(f, cl.csqc_viewangles);
		CSQC_R_RecalcView();
		break;
	case VF_ANGLES_X:
		cl.csqc_viewangles[0] = k;
		CSQC_R_RecalcView();
		break;
	case VF_ANGLES_Y:
		cl.csqc_viewangles[1] = k;
		CSQC_R_RecalcView();
		break;
	case VF_ANGLES_Z:
		cl.csqc_viewangles[2] = k;
		CSQC_R_RecalcView();
		break;
	case VF_DRAWWORLD:
		cl.csqc_vidvars.drawworld = ((k != 0) && r_drawworld.integer);
		break;
	case VF_DRAWENGINESBAR:
		cl.csqc_vidvars.drawenginesbar = k != 0;
		break;
	case VF_DRAWCROSSHAIR:
		cl.csqc_vidvars.drawcrosshair = k != 0;
		break;
	case VF_CL_VIEWANGLES:
		VectorCopy(f, cl.viewangles);
		break;
	case VF_CL_VIEWANGLES_X:
		cl.viewangles[0] = k;
		break;
	case VF_CL_VIEWANGLES_Y:
		cl.viewangles[1] = k;
		break;
	case VF_CL_VIEWANGLES_Z:
		cl.viewangles[2] = k;
		break;
	case VF_PERSPECTIVE:
		r_refdef.view.useperspective = k != 0;
		break;
	case VF_CLEARSCREEN:
		r_refdef.view.isoverlay = !k;
		break;
	case VF_MAINVIEW:
		PRVM_G_FLOAT(OFS_RETURN) = r_refdef.view.ismain;
		break;
	case VF_FOG_DENSITY:
		r_refdef.fog_density = k;
		break;
	case VF_FOG_COLOR:
		r_refdef.fog_red = f[0];
		r_refdef.fog_green = f[1];
		r_refdef.fog_blue = f[2];
		break;
	case VF_FOG_COLOR_R:
		r_refdef.fog_red = k;
		break;
	case VF_FOG_COLOR_G:
		r_refdef.fog_green = k;
		break;
	case VF_FOG_COLOR_B:
		r_refdef.fog_blue = k;
		break;
	case VF_FOG_ALPHA:
		r_refdef.fog_alpha = k;
		break;
	case VF_FOG_START:
		r_refdef.fog_start = k;
		break;
	case VF_FOG_END:
		r_refdef.fog_end = k;
		break;
	case VF_FOG_HEIGHT:
		r_refdef.fog_height = k;
		break;
	case VF_FOG_FADEDEPTH:
		r_refdef.fog_fadedepth = k;
		break;
	case VF_MINFPS_QUALITY:
		r_refdef.view.quality = k;
		break;
	default:
		PRVM_G_FLOAT(OFS_RETURN) = 0;
		VM_Warning(prog, "VM_CL_R_SetView : unknown parm %i\n", c);
		return;
	}
	PRVM_G_FLOAT(OFS_RETURN) = 1;
}

//#305 void(vector org, float radius, vector lightcolours[, float style, string cubemapname, float pflags]) adddynamiclight (EXT_CSQC)
void vm::client::addDynamicLight (prvm_prog_t *prog)
{
	double t = Sys_DirtyTime();
	vec3_t org;
	float radius = 300;
	vec3_t col;
	int style = -1;
	const char *cubemapname = nullptr;
	int pflags = PFLAGS_CORONA | PFLAGS_FULLDYNAMIC;
	float coronaintensity = 1;
	float coronasizescale = 0.25;
	bool castshadow = true;
	float ambientscale = 0;
	float diffusescale = 1;
	float specularscale = 1;
	matrix4x4_t matrix;
	vec3_t forward, left, up;
	VM_SAFEPARMCOUNTRANGE(3, 8, VM_CL_R_AddDynamicLight);

	// if we've run out of dlights, just return
	if (r_refdef.scene.numlights >= MAX_DLIGHTS)
		return;

	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), org);
	radius = PRVM_G_FLOAT(OFS_PARM1);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM2), col);
	if (prog->argc >= 4)
	{
		style = (int)PRVM_G_FLOAT(OFS_PARM3);
		if (style >= MAX_LIGHTSTYLES)
		{
			Con_DPrintf("VM_CL_R_AddDynamicLight: out of bounds lightstyle index %i\n", style);
			style = -1;
		}
	}
	if (prog->argc >= 5)
		cubemapname = PRVM_G_STRING(OFS_PARM4);
	if (prog->argc >= 6)
		pflags = (int)PRVM_G_FLOAT(OFS_PARM5);
	coronaintensity = (pflags & PFLAGS_CORONA) != 0;
	castshadow = (pflags & PFLAGS_NOSHADOW) == 0;

	VectorScale(PRVM_clientglobalvector(v_forward), radius, forward);
	VectorScale(PRVM_clientglobalvector(v_right), -radius, left);
	VectorScale(PRVM_clientglobalvector(v_up), radius, up);
	Matrix4x4_FromVectors(&matrix, forward, left, up, org);

	R_RTLight_Update(&r_refdef.scene.templights[r_refdef.scene.numlights], false, &matrix, col, style, cubemapname, castshadow, coronaintensity, coronasizescale, ambientscale, diffusescale, specularscale, LIGHTFLAG_NORMALMODE | LIGHTFLAG_REALTIMEMODE);
	r_refdef.scene.lights[r_refdef.scene.numlights] = &r_refdef.scene.templights[r_refdef.scene.numlights];r_refdef.scene.numlights++;
	t = Sys_DirtyTime() - t;if (t < 0 || t >= 1800) t = 0;
	if(!prog->isNative)
		prog->functions[PRVM_clientfunction(CSQC_UpdateView)].totaltime -= t;
}

//============================================================================

//#310 vector (vector v) cs_unproject (EXT_CSQC)
void VM_CL_unproject (prvm_prog_t *prog)
{
	vec3_t f;
	vec3_t temp;
	vec3_t result;

	VM_SAFEPARMCOUNT(1, VM_CL_unproject);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), f);
	VectorSet(temp,
		f[2],
		(-1.0 + 2.0 * (f[0] / vid_conwidth.integer)) * f[2] * -r_refdef.view.frustum_x,
		(-1.0 + 2.0 * (f[1] / vid_conheight.integer)) * f[2] * -r_refdef.view.frustum_y);
	if(v_flipped.integer)
		temp[1] = -temp[1];
	Matrix4x4_Transform(&r_refdef.view.matrix, temp, result);
	VectorCopy(result, PRVM_G_VECTOR(OFS_RETURN));
}

//#311 vector (vector v) cs_project (EXT_CSQC)
void VM_CL_project (prvm_prog_t *prog)
{
	vec3_t f;
	vec3_t v;
	matrix4x4_t m;

	VM_SAFEPARMCOUNT(1, VM_CL_project);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), f);
	Matrix4x4_Invert_Simple(&m, &r_refdef.view.matrix);
	Matrix4x4_Transform(&m, f, v);
	if(v_flipped.integer)
		v[1] = -v[1];
	VectorSet(PRVM_G_VECTOR(OFS_RETURN),
		vid_conwidth.integer * (0.5*(1.0+v[1]/v[0]/-r_refdef.view.frustum_x)),
		vid_conheight.integer * (0.5*(1.0+v[2]/v[0]/-r_refdef.view.frustum_y)),
		v[0]);
	// explanation:
	// after transforming, relative position to viewport (0..1) = 0.5 * (1 + v[2]/v[0]/-frustum_{x \or y})
	// as 2D drawing honors the viewport too, to get the same pixel, we simply multiply this by conwidth/height
}

//#330 float(float stnum) getstatf (EXT_CSQC)
void VM_CL_getstatf (prvm_prog_t *prog)
{
	int i;
	union
	{
		float f;
		int l;
	}dat;
	VM_SAFEPARMCOUNT(1, VM_CL_getstatf);
	i = (int)PRVM_G_FLOAT(OFS_PARM0);
	if(i < 0 || i >= MAX_CL_STATS)
	{
		VM_Warning(prog, "VM_CL_getstatf: index>=MAX_CL_STATS or index<0\n");
		return;
	}
	dat.l = cl.stats[i];
	PRVM_G_FLOAT(OFS_RETURN) =  dat.f;
}

//#331 float(float stnum) getstati (EXT_CSQC)
void VM_CL_getstati (prvm_prog_t *prog)
{
	int i, index;
	int firstbit, bitcount;

	VM_SAFEPARMCOUNTRANGE(1, 3, VM_CL_getstati);

	index = (int)PRVM_G_FLOAT(OFS_PARM0);
	if (prog->argc > 1)
	{
		firstbit = (int)PRVM_G_FLOAT(OFS_PARM1);
		if (prog->argc > 2)
			bitcount = (int)PRVM_G_FLOAT(OFS_PARM2);
		else
			bitcount = 1;
	}
	else
	{
		firstbit = 0;
		bitcount = 32;
	}

	if(index < 0 || index >= MAX_CL_STATS)
	{
		VM_Warning(prog, "VM_CL_getstati: index>=MAX_CL_STATS or index<0\n");
		return;
	}
	i = cl.stats[index];
	if (bitcount != 32)	//32 causes the mask to overflow, so there's nothing to subtract from.
		i = (((unsigned int)i)&(((1<<bitcount)-1)<<firstbit))>>firstbit;
	PRVM_G_FLOAT(OFS_RETURN) = i;
}

//#332 string(float firststnum) getstats (EXT_CSQC)
void vm::client::getStats (Program prog)
{
	int i;
	char t[17];
	VM_SAFEPARMCOUNT(1, VM_CL_getstats);
	i = (int)PRVM_G_FLOAT(OFS_PARM0);
	if(i < 0 || i > MAX_CL_STATS-4)
	{
		PRVM_G_INT(OFS_RETURN) = OFS_NULL;
		VM_Warning(prog, "VM_CL_getstats: index>MAX_CL_STATS-4 or index<0\n");
		return;
	}
	strlcpy(t, (char*)&cl.stats[i], sizeof(t));
	PRVM_G_INT(OFS_RETURN) = PRVM_SetTempString(prog, t);
}

//#333 void(entity e, float mdlindex) setmodelindex (EXT_CSQC)
void vm::client::setModelIndex (Program prog, Edict* t, const size32 modelIndex)
{

	PRVM_clientedictstring(t, model) = 0;
	PRVM_clientedictfloat(t, modelindex) = .0f;

	if (unlikely(modelIndex == 0))
		return;

	struct model_s	*model = CL_GetModelByIndex(modelIndex);
	if (unlikely(model == nullptr))
	{
		vm::warn(prog, "VM_CL_setmodelindex: null model\n");
		return;
	}
	PRVM_clientedictstring(t, model) = PRVM_SetEngineString(prog, model->name);
	PRVM_clientedictfloat(t, modelindex) = modelIndex;

	// TODO: check if this breaks needed consistency and maybe add a cvar for it too?? [1/10/2008 Black]
	if (model != nullptr)
		SetMinMaxSize (prog, t, model->normalmins, model->normalmaxs);
	else
		SetMinMaxSize (prog, t, vec3_origin, vec3_origin);
}

//#334 string(float mdlindex) modelnameforindex (EXT_CSQC)
void VM_CL_modelnameforindex (prvm_prog_t *prog)
{
	dp_model_t *model;

	VM_SAFEPARMCOUNT(1, VM_CL_modelnameforindex);

	PRVM_G_INT(OFS_RETURN) = OFS_NULL;
	model = CL_GetModelByIndex((int)PRVM_G_FLOAT(OFS_PARM0));
	PRVM_G_INT(OFS_RETURN) = model ? PRVM_SetEngineString(prog, model->name) : 0;
}

//#335 float(string effectname) particleeffectnum (EXT_CSQC)
void VM_CL_particleeffectnum (prvm_prog_t *prog)
{
	int			i;
	VM_SAFEPARMCOUNT(1, VM_CL_particleeffectnum);
	i = CL_ParticleEffectIndexForName(PRVM_G_STRING(OFS_PARM0));
	if (i == 0)
		i = -1;
	PRVM_G_FLOAT(OFS_RETURN) = i;
}

// #336 void(entity ent, float effectnum, vector start, vector end[, float color]) trailparticles (EXT_CSQC)
void VM_CL_trailparticles (prvm_prog_t *prog)
{
	int				i;
	vec3_t			start, end, velocity;
	prvm_edict_t	*t;
	VM_SAFEPARMCOUNTRANGE(4, 5, VM_CL_trailparticles);

	t = PRVM_G_EDICT(OFS_PARM0);
	i		= (int)PRVM_G_FLOAT(OFS_PARM1);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM2), start);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM3), end);
	VectorCopy(PRVM_clientedictvector(t, velocity), velocity);

	if (i < 0)
		return;
	CL_ParticleTrail(i, 1, start, end, velocity, velocity, nullptr, prog->argc >= 5 ? (int)PRVM_G_FLOAT(OFS_PARM4) : 0, true, true, nullptr, nullptr, 1);
}

//#337 void(float effectnum, vector origin, vector dir, float count[, float color]) pointparticles (EXT_CSQC)
void VM_CL_pointparticles (prvm_prog_t *prog)
{
	int			i;
	float n;
	vec3_t f, v;
	VM_SAFEPARMCOUNTRANGE(4, 5, VM_CL_pointparticles);
	i = (int)PRVM_G_FLOAT(OFS_PARM0);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM1), f);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM2), v);
	n = PRVM_G_FLOAT(OFS_PARM3);
	if (i < 0)
		return;
	CL_ParticleEffect(i, n, f, f, v, v, nullptr, prog->argc >= 5 ? (int)PRVM_G_FLOAT(OFS_PARM4) : 0);
}

//#502 void(float effectnum, entity own, vector origin_from, vector origin_to, vector dir_from, vector dir_to, float count, float extflags) boxparticles (DP_CSQC_BOXPARTICLES)
void VM_CL_boxparticles (prvm_prog_t *prog)
{
	int effectnum;
	// prvm_edict_t *own;
	vec3_t origin_from, origin_to, dir_from, dir_to;
	float count;
	int flags;
	bool istrail;
	float tintmins[4], tintmaxs[4], fade;
	VM_SAFEPARMCOUNTRANGE(7, 8, VM_CL_boxparticles);

	effectnum = (int)PRVM_G_FLOAT(OFS_PARM0);
	if (effectnum < 0)
		return;
	// own = PRVM_G_EDICT(OFS_PARM1); // TODO find use for this
	VectorCopy(PRVM_G_VECTOR(OFS_PARM2), origin_from);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM3), origin_to  );
	VectorCopy(PRVM_G_VECTOR(OFS_PARM4), dir_from   );
	VectorCopy(PRVM_G_VECTOR(OFS_PARM5), dir_to     );
	count = PRVM_G_FLOAT(OFS_PARM6);
	if(prog->argc >= 8)
		flags = PRVM_G_FLOAT(OFS_PARM7);
	else
		flags = 0;

	Vector4Set(tintmins, 1, 1, 1, 1);
	Vector4Set(tintmaxs, 1, 1, 1, 1);
	fade = 1;
	istrail = false;

	if(flags & 1) // read alpha
	{
		tintmins[3] = PRVM_clientglobalfloat(particles_alphamin);
		tintmaxs[3] = PRVM_clientglobalfloat(particles_alphamax);
	}
	if(flags & 2) // read color
	{
		VectorCopy(PRVM_clientglobalvector(particles_colormin), tintmins);
		VectorCopy(PRVM_clientglobalvector(particles_colormax), tintmaxs);
	}
	if(flags & 4) // read fade
	{
		fade = PRVM_clientglobalfloat(particles_fade);
	}
	if(flags & 128) // draw as trail
	{
		istrail = true;
	}

	if (istrail)
		CL_ParticleTrail(effectnum, count, origin_from, origin_to, dir_from, dir_to, nullptr, 0, true, true, tintmins, tintmaxs, fade);
	else
		CL_ParticleBox(effectnum, count, origin_from, origin_to, dir_from, dir_to, nullptr, 0, true, true, tintmins, tintmaxs, fade);
}

//#531 void(float pause) setpause
void vm::client::setpause(const Program prog, const bool pause)
{
	cl.csqc_paused = pause;
}

//#343 void(float usecursor) setcursormode (DP_CSQC)
void vm::client::setCursorMode (const Program prog, const bool useCursor)
{
	cl.csqc_wantsmousemove = useCursor != false;
	cl_ignoremousemoves = 2;
}

//#344 vector() getmousepos (DP_CSQC)
vector3D vm::client::getMousePosition(Program prog)
{
	vector3D result;
	if (key_consoleactive || key_dest != key_game)
		VectorSet(result, .0f, .0f, .0f);
	else if (cl.csqc_wantsmousemove)
		VectorSet(result, in_windowmouse_x * vid_conwidth.integer / vid.width, in_windowmouse_y * vid_conheight.integer / vid.height, .0f);
	else
		VectorSet(result, in_mouse_x * vid_conwidth.integer / vid.width, in_mouse_y * vid_conheight.integer / vid.height, .0f);
	return result;
}

//#345 float(float framenum) getinputstate (EXT_CSQC)
bool vm::client::getInputState (Program prog, const int frame)
{
	usercmd_t* RESTRICT const moveCommands = cl.movecmd;
	for (size32 i = 0; i < CL_MAX_USERCMDS; i++)
	{
		usercmd_t * RESTRICT const moveCommand = &moveCommands[i];
		if (moveCommand->sequence != frame)
			continue;

		VectorCopy(moveCommand->viewangles, PRVM_clientglobalvector(input_angles));
		PRVM_clientglobalfloat(input_buttons) = moveCommand->buttons; // FIXME: this should not be directly exposed to csqc (translation layer needed?)
		PRVM_clientglobalvector(input_movevalues)[0] = moveCommand->forwardmove;
		PRVM_clientglobalvector(input_movevalues)[1] = moveCommand->sidemove;
		PRVM_clientglobalvector(input_movevalues)[2] = moveCommand->upmove;
		PRVM_clientglobalfloat(input_timelength) = moveCommand->frametime;
		// this probably shouldn't be here
		if(moveCommand->crouch)
		{
			VectorCopy(cl.playercrouchmins, PRVM_clientglobalvector(pmove_mins));
			VectorCopy(cl.playercrouchmaxs, PRVM_clientglobalvector(pmove_maxs));
		}
		else
		{
			VectorCopy(cl.playerstandmins, PRVM_clientglobalvector(pmove_mins));
			VectorCopy(cl.playerstandmaxs, PRVM_clientglobalvector(pmove_maxs));
		}
		return true;

	}
	return false;
}

//#346 void(float sens) setsensitivityscaler (EXT_CSQC)
void vm::client::setSensitivityScale (prvm_prog_t *prog)
{
	VM_SAFEPARMCOUNT(1, VM_CL_setsensitivityscale);
	cl.sensitivityscale = PRVM_G_FLOAT(OFS_PARM0);
}

//#347 void() runstandardplayerphysics (EXT_CSQC)
#define PMF_JUMP_HELD 1 // matches FTEQW
#define PMF_LADDER 2 // not used by DP, FTEQW sets this in runplayerphysics but does not read it
#define PMF_DUCKED 4 // FIXME FTEQW doesn't have this for Q1 like movement because Q1 cannot crouch
#define PMF_ONGROUND 8 // FIXME FTEQW doesn't have this for Q1 like movement and expects CSQC code to do its own trace, this is stupid CPU waste
void vm::client::runplayerphysics (prvm_prog_t *prog)
{
	cl_clientmovement_state_t s;
	prvm_edict_t *ent;

	memset(&s, 0, sizeof(s));

	VM_SAFEPARMCOUNTRANGE(0, 1, VM_CL_runplayerphysics);

	ent = (prog->argc == 1 ? PRVM_G_EDICT(OFS_PARM0) : prog->edicts);
	if(ent == prog->edicts)
	{
		// deprecated use
		s.self = nullptr;
		VectorCopy(PRVM_clientglobalvector(pmove_org), s.origin);
		VectorCopy(PRVM_clientglobalvector(pmove_vel), s.velocity);
		VectorCopy(PRVM_clientglobalvector(pmove_mins), s.mins);
		VectorCopy(PRVM_clientglobalvector(pmove_maxs), s.maxs);
		s.crouched = 0;
		s.waterjumptime = PRVM_clientglobalfloat(pmove_waterjumptime);
		s.cmd.canjump = (int)PRVM_clientglobalfloat(pmove_jump_held) == 0;
	}
	else
	{
		// new use
		s.self = ent;
		VectorCopy(PRVM_clientedictvector(ent, origin), s.origin);
		VectorCopy(PRVM_clientedictvector(ent, velocity), s.velocity);
		VectorCopy(PRVM_clientedictvector(ent, mins), s.mins);
		VectorCopy(PRVM_clientedictvector(ent, maxs), s.maxs);
		s.crouched = ((int)PRVM_clientedictfloat(ent, pmove_flags) & PMF_DUCKED) != 0;
		s.waterjumptime = 0; // FIXME where do we get this from? FTEQW lacks support for this too
		s.cmd.canjump = ((int)PRVM_clientedictfloat(ent, pmove_flags) & PMF_JUMP_HELD) == 0;
	}

	VectorCopy(PRVM_clientglobalvector(input_angles), s.cmd.viewangles);
	s.cmd.forwardmove = PRVM_clientglobalvector(input_movevalues)[0];
	s.cmd.sidemove = PRVM_clientglobalvector(input_movevalues)[1];
	s.cmd.upmove = PRVM_clientglobalvector(input_movevalues)[2];
	s.cmd.buttons = PRVM_clientglobalfloat(input_buttons);
	s.cmd.frametime = PRVM_clientglobalfloat(input_timelength);
	s.cmd.jump = (s.cmd.buttons & 2) != 0;
	s.cmd.crouch = (s.cmd.buttons & 16) != 0;

	CL_ClientMovement_PlayerMove_Frame(&s);

	if(ent == prog->edicts)
	{
		// deprecated use
		VectorCopy(s.origin, PRVM_clientglobalvector(pmove_org));
		VectorCopy(s.velocity, PRVM_clientglobalvector(pmove_vel));
		PRVM_clientglobalfloat(pmove_jump_held) = !s.cmd.canjump;
		PRVM_clientglobalfloat(pmove_waterjumptime) = s.waterjumptime;
	}
	else
	{
		// new use
		VectorCopy(s.origin, PRVM_clientedictvector(ent, origin));
		VectorCopy(s.velocity, PRVM_clientedictvector(ent, velocity));
		PRVM_clientedictfloat(ent, pmove_flags) =
			(s.crouched ? PMF_DUCKED : 0) |
			(s.cmd.canjump ? 0 : PMF_JUMP_HELD) |
			(s.onground ? PMF_ONGROUND : 0);
	}
}

//#348 string(float playernum, string keyname) getplayerkeyvalue (EXT_CSQC)
void vm::client::getplayerkey (prvm_prog_t *prog)
{
	int			i;
	char		t[128];
	const char	*c;

	VM_SAFEPARMCOUNT(2, VM_CL_getplayerkey);

	i = (int)PRVM_G_FLOAT(OFS_PARM0);
	c = PRVM_G_STRING(OFS_PARM1);
	PRVM_G_INT(OFS_RETURN) = OFS_NULL;
	Sbar_SortFrags();

	if (i < 0)
		i = Sbar_GetSortedPlayerIndex(-1-i);
	if(i < 0 || i >= cl.maxclients)
		return;

	t[0] = 0;

	if(!strcasecmp(c, "name"))
		strlcpy(t, cl.scores[i].name, sizeof(t));
	else
		if(!strcasecmp(c, "frags"))
			dpsnprintf(t, sizeof(t), "%i", cl.scores[i].frags);
	else
		if(!strcasecmp(c, "ping"))
			dpsnprintf(t, sizeof(t), "%i", cl.scores[i].qw_ping);
	else
		if(!strcasecmp(c, "pl"))
			dpsnprintf(t, sizeof(t), "%i", cl.scores[i].qw_packetloss);
	else
		if(!strcasecmp(c, "movementloss"))
			dpsnprintf(t, sizeof(t), "%i", cl.scores[i].qw_movementloss);
	else
		if(!strcasecmp(c, "entertime"))
			dpsnprintf(t, sizeof(t), "%f", cl.scores[i].qw_entertime);
	else
		if(!strcasecmp(c, "colors"))
			dpsnprintf(t, sizeof(t), "%i", cl.scores[i].colors);
	else
		if(!strcasecmp(c, "topcolor"))
			dpsnprintf(t, sizeof(t), "%i", cl.scores[i].colors & 0xf0);
	else
		if(!strcasecmp(c, "bottomcolor"))
			dpsnprintf(t, sizeof(t), "%i", (cl.scores[i].colors &15)<<4);
	else
		if(!strcasecmp(c, "viewentity"))
			dpsnprintf(t, sizeof(t), "%i", i+1);
	if(!t[0])
		return;
	PRVM_G_INT(OFS_RETURN) = PRVM_SetTempString(prog, t);
}

//#351 void(vector origin, vector forward, vector right, vector up) SetListener (EXT_CSQC)
void VM_CL_setlistener (prvm_prog_t *prog)
{
	vec3_t origin, forward, left, up;
	VM_SAFEPARMCOUNT(4, VM_CL_setlistener);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), origin);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM1), forward);
	VectorNegate(PRVM_G_VECTOR(OFS_PARM2), left);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM3), up);
	Matrix4x4_FromVectors(&cl.csqc_listenermatrix, forward, left, up, origin);
	cl.csqc_usecsqclistener = true;	//use csqc listener at this frame
}

//#360 float() readbyte (EXT_CSQC)
void VM_CL_ReadByte (prvm_prog_t *prog)
{
	VM_SAFEPARMCOUNT(0, VM_CL_ReadByte);
	PRVM_G_FLOAT(OFS_RETURN) = MSG_ReadByte(&cl_message);
}

//#361 float() readchar (EXT_CSQC)
void VM_CL_ReadChar (prvm_prog_t *prog)
{
	VM_SAFEPARMCOUNT(0, VM_CL_ReadChar);
	PRVM_G_FLOAT(OFS_RETURN) = MSG_ReadChar(&cl_message);
}

//#362 float() readshort (EXT_CSQC)
void VM_CL_ReadShort (prvm_prog_t *prog)
{
	VM_SAFEPARMCOUNT(0, VM_CL_ReadShort);
	PRVM_G_FLOAT(OFS_RETURN) = MSG_ReadShort(&cl_message);
}

//#363 float() readlong (EXT_CSQC)
void VM_CL_ReadLong (prvm_prog_t *prog)
{
	VM_SAFEPARMCOUNT(0, VM_CL_ReadLong);
	PRVM_G_FLOAT(OFS_RETURN) = MSG_ReadLong(&cl_message);
}

//#364 float() readcoord (EXT_CSQC)
void VM_CL_ReadCoord (prvm_prog_t *prog)
{
	VM_SAFEPARMCOUNT(0, VM_CL_ReadCoord);
	PRVM_G_FLOAT(OFS_RETURN) = MSG_ReadCoord(&cl_message, cls.protocol);
}

//#365 float() readangle (EXT_CSQC)
void VM_CL_ReadAngle (prvm_prog_t *prog)
{
	VM_SAFEPARMCOUNT(0, VM_CL_ReadAngle);
	PRVM_G_FLOAT(OFS_RETURN) = MSG_ReadAngle(&cl_message, cls.protocol);
}

//#366 string() readstring (EXT_CSQC)
void VM_CL_ReadString (prvm_prog_t *prog)
{
	VM_SAFEPARMCOUNT(0, VM_CL_ReadString);
	PRVM_G_INT(OFS_RETURN) = PRVM_SetTempString(prog, MSG_ReadString(&cl_message, cl_readstring, sizeof(cl_readstring)));
}

//#367 float() readfloat (EXT_CSQC)
void vm::client::readFloat (prvm_prog_t *prog)
{
	VM_SAFEPARMCOUNT(0, VM_CL_ReadFloat);
	PRVM_G_FLOAT(OFS_RETURN) = MSG_ReadFloat(&cl_message);
}

//#501 string() readpicture (DP_CSQC_READWRITEPICTURE)
extern cvar_t cl_readpicture_force;
void vm::client::readPicture (Program prog)
{
	unsigned char *data;
	unsigned char *buf;
	int i;
	cachepic_t *pic;


	const char* name = MSG_ReadString(&cl_message, cl_readstring, sizeof(cl_readstring));
	uint16 size = static_cast<uint16>(MSG_ReadShort(&cl_message));

	// check if a texture of that name exists
	// if yes, it is used and the data is discarded
	// if not, the (low quality) data is used to build a new texture, whose name will get returned

	cachepic_t *pic = Draw_CachePic_Flags (name, CACHEPICFLAG_NOTPERSISTENT);

	if(size)
	{
		if(pic->tex == r_texture_notexture)
			pic->tex = nullptr; // don't overwrite the notexture by Draw_NewPic
		if(pic->tex && !cl_readpicture_force.integer)
		{
			// texture found and loaded
			// skip over the jpeg as we don't need it
			for(i = 0; i < size; ++i)
				(void) MSG_ReadByte(&cl_message);
		}
		else
		{
			// texture not found
			// use the attached jpeg as texture
			buf = (unsigned char *) Mem_Alloc(tempmempool, size);
			MSG_ReadBytes(&cl_message, size, buf);
			data = JPEG_LoadImage_BGRA(buf, size, nullptr);
			Mem_Free(buf);
			Draw_NewPic(name, image_width, image_height, false, data);
			Mem_Free(data);
		}
	}

	PRVM_G_INT(OFS_RETURN) = PRVM_SetTempString(prog, name);
}

//////////////////////////////////////////////////////////

void vm::client::makestatic (Program prog, Edict* ent)
{
	if (unlikely(ent == prog->edicts))
	{
		VM_Warning(prog, "makestatic: can not modify world entity\n");
		return;
	}
	if (unlikely(ent->priv.server->free))
	{
		VM_Warning(prog, "makestatic: can not modify free entity\n");
		return;
	}

	if (likely(cl.num_static_entities < cl.max_static_entities))
	{
		int renderflags;
		entity_t *staticent = &cl.static_entities[cl.num_static_entities++];

		// copy it to the current state
		memset(staticent, 0, sizeof(*staticent));
		staticent->render.model = CL_GetModelByIndex((int)PRVM_clientedictfloat(ent, modelindex));
		staticent->render.framegroupblend[0].frame = (int)PRVM_clientedictfloat(ent, frame);
		staticent->render.framegroupblend[0].lerp = 1.0f;
		// make torchs play out of sync
		staticent->render.framegroupblend[0].start = lhrandom(-10, -1);
		staticent->render.skinnum = (int)PRVM_clientedictfloat(ent, skin);
		staticent->render.effects = (int)PRVM_clientedictfloat(ent, effects);
		staticent->render.alpha = PRVM_clientedictfloat(ent, alpha);
		staticent->render.scale = PRVM_clientedictfloat(ent, scale);
		VectorCopy(PRVM_clientedictvector(ent, colormod), staticent->render.colormod);
		VectorCopy(PRVM_clientedictvector(ent, glowmod), staticent->render.glowmod);

		// sanitize values
		if (!staticent->render.alpha)
			staticent->render.alpha = 1.0f;
		if (!staticent->render.scale)
			staticent->render.scale = 1.0f;
		if (!VectorLength2(staticent->render.colormod))
			VectorSet(staticent->render.colormod, 1.0f, 1.0f, 1.0f);
		if (!VectorLength2(staticent->render.glowmod))
			VectorSet(staticent->render.glowmod, 1.0f, 1.0f, 1.0f);

		renderflags = (int)PRVM_clientedictfloat(ent, renderflags);
		if (renderflags & RF_USEAXIS)
		{
			vec3_t forward, left, up, origin;
			VectorCopy(PRVM_clientglobalvector(v_forward), forward);
			VectorNegate(PRVM_clientglobalvector(v_right), left);
			VectorCopy(PRVM_clientglobalvector(v_up), up);
			VectorCopy(PRVM_clientedictvector(ent, origin), origin);
			Matrix4x4_FromVectors(&staticent->render.matrix, forward, left, up, origin);
			Matrix4x4_Scale(&staticent->render.matrix, staticent->render.scale, 1);
		}
		else
			Matrix4x4_CreateFromQuakeEntity(&staticent->render.matrix, PRVM_clientedictvector(ent, origin)[0], PRVM_clientedictvector(ent, origin)[1], PRVM_clientedictvector(ent, origin)[2], PRVM_clientedictvector(ent, angles)[0], PRVM_clientedictvector(ent, angles)[1], PRVM_clientedictvector(ent, angles)[2], staticent->render.scale);

		// either fullbright or lit
		if(likely(!r_fullbright.integer))
		{
			if (!(staticent->render.effects & EF_FULLBRIGHT))
				staticent->render.flags |= RENDER_LIGHT;
			else if(r_equalize_entities_fullbright.integer)
				staticent->render.flags |= RENDER_LIGHT | RENDER_EQUALIZE;
		}
		// turn off shadows from transparent objects
		if (!(staticent->render.effects & (EF_NOSHADOW | EF_ADDITIVE | EF_NODEPTHTEST)) && (staticent->render.alpha >= 1))
			staticent->render.flags |= RENDER_SHADOW;
		if (staticent->render.effects & EF_NODEPTHTEST)
			staticent->render.flags |= RENDER_NODEPTHTEST;
		if (staticent->render.effects & EF_ADDITIVE)
			staticent->render.flags |= RENDER_ADDITIVE;
		if (staticent->render.effects & EF_DOUBLESIDED)
			staticent->render.flags |= RENDER_DOUBLESIDED;

		staticent->render.allowdecals = true;
		CL_UpdateRenderEntity(&staticent->render);
	}
	else
		Con_Printf("Too many static entities");

// throw the entity away now
	vm::freeEdict(prog, ent);
}

//=================================================================//

/*
=================
VM_CL_copyentity

copies data from one entity to another

copyentity(src, dst)
=================
*/
void vm::client::copyentity (prvm_prog_t *prog)
{
	prvm_edict_t *in, *out;
	VM_SAFEPARMCOUNT(2, VM_CL_copyentity);
	in = PRVM_G_EDICT(OFS_PARM0);
	if (in == prog->edicts)
	{
		VM_Warning(prog, "copyentity: can not read world entity\n");
		return;
	}
	if (in->priv.server->free)
	{
		VM_Warning(prog, "copyentity: can not read free entity\n");
		return;
	}
	out = PRVM_G_EDICT(OFS_PARM1);
	if (out == prog->edicts)
	{
		VM_Warning(prog, "copyentity: can not modify world entity\n");
		return;
	}
	if (out->priv.server->free)
	{
		VM_Warning(prog, "copyentity: can not modify free entity\n");
		return;
	}
	memcpy(out->fields.fp, in->fields.fp, prog->entityfields * sizeof(prvm_vec_t));
	CL_LinkEdict(out);
}

//=================================================================//

// #404 void(vector org, string modelname, float startframe, float endframe, float framerate) effect (DP_SV_EFFECT)
void VM_CL_effect (prvm_prog_t *prog)
{
#if 1
	Con_Printf("WARNING: VM_CL_effect not implemented\n"); // FIXME: this needs to take modelname not modelindex, the csqc defs has it as string and so it shall be
#else
	vec3_t org;
	VM_SAFEPARMCOUNT(5, VM_CL_effect);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), org);
	CL_Effect(org, (int)PRVM_G_FLOAT(OFS_PARM1), (int)PRVM_G_FLOAT(OFS_PARM2), (int)PRVM_G_FLOAT(OFS_PARM3), PRVM_G_FLOAT(OFS_PARM4));
#endif
}

// #405 void(vector org, vector velocity, float howmany) te_blood (DP_TE_BLOOD)
void VM_CL_te_blood (prvm_prog_t *prog)
{
	vec3_t pos, vel, pos2;
	VM_SAFEPARMCOUNT(3, VM_CL_te_blood);
	if (PRVM_G_FLOAT(OFS_PARM2) < 1)
		return;
	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), pos);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM1), vel);
	CL_FindNonSolidLocation(pos, pos2, 4);
	CL_ParticleEffect(EFFECT_TE_BLOOD, PRVM_G_FLOAT(OFS_PARM2), pos2, pos2, vel, vel, nullptr, 0);
}

// #406 void(vector mincorner, vector maxcorner, float explosionspeed, float howmany) te_bloodshower (DP_TE_BLOODSHOWER)
void VM_CL_te_bloodshower (prvm_prog_t *prog)
{
	vec_t speed;
	vec3_t mincorner, maxcorner, vel1, vel2;
	VM_SAFEPARMCOUNT(4, VM_CL_te_bloodshower);
	if (PRVM_G_FLOAT(OFS_PARM3) < 1)
		return;
	speed = PRVM_G_FLOAT(OFS_PARM2);
	vel1[0] = -speed;
	vel1[1] = -speed;
	vel1[2] = -speed;
	vel2[0] = speed;
	vel2[1] = speed;
	vel2[2] = speed;
	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), mincorner);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM1), maxcorner);
	CL_ParticleEffect(EFFECT_TE_BLOOD, PRVM_G_FLOAT(OFS_PARM3), mincorner, maxcorner, vel1, vel2, nullptr, 0);
}

// #407 void(vector org, vector color) te_explosionrgb (DP_TE_EXPLOSIONRGB)
void VM_CL_te_explosionrgb (prvm_prog_t *prog)
{
	vec3_t		pos;
	vec3_t		pos2;
	matrix4x4_t	tempmatrix;
	VM_SAFEPARMCOUNT(2, VM_CL_te_explosionrgb);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), pos);
	CL_FindNonSolidLocation(pos, pos2, 10);
	CL_ParticleExplosion(pos2);
	Matrix4x4_CreateTranslate(&tempmatrix, pos2[0], pos2[1], pos2[2]);
	CL_AllocLightFlash(nullptr, &tempmatrix, 350, PRVM_G_VECTOR(OFS_PARM1)[0], PRVM_G_VECTOR(OFS_PARM1)[1], PRVM_G_VECTOR(OFS_PARM1)[2], 700, 0.5, 0, -1, true, 1, 0.25, 0.25, 1, 1, LIGHTFLAG_NORMALMODE | LIGHTFLAG_REALTIMEMODE);
}

// #408 void(vector mincorner, vector maxcorner, vector vel, float howmany, float color, float gravityflag, float randomveljitter) te_particlecube (DP_TE_PARTICLECUBE)
void VM_CL_te_particlecube (prvm_prog_t *prog)
{
	vec3_t mincorner, maxcorner, vel;
	VM_SAFEPARMCOUNT(7, VM_CL_te_particlecube);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), mincorner);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM1), maxcorner);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM2), vel);
	CL_ParticleCube(mincorner, maxcorner, vel, (int)PRVM_G_FLOAT(OFS_PARM3), (int)PRVM_G_FLOAT(OFS_PARM4), PRVM_G_FLOAT(OFS_PARM5), PRVM_G_FLOAT(OFS_PARM6));
}

// #409 void(vector mincorner, vector maxcorner, vector vel, float howmany, float color) te_particlerain (DP_TE_PARTICLERAIN)
void VM_CL_te_particlerain (prvm_prog_t *prog)
{
	vec3_t mincorner, maxcorner, vel;
	VM_SAFEPARMCOUNT(5, VM_CL_te_particlerain);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), mincorner);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM1), maxcorner);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM2), vel);
	CL_ParticleRain(mincorner, maxcorner, vel, (int)PRVM_G_FLOAT(OFS_PARM3), (int)PRVM_G_FLOAT(OFS_PARM4), 0);
}

// #410 void(vector mincorner, vector maxcorner, vector vel, float howmany, float color) te_particlesnow (DP_TE_PARTICLESNOW)
void VM_CL_te_particlesnow (prvm_prog_t *prog)
{
	vec3_t mincorner, maxcorner, vel;
	VM_SAFEPARMCOUNT(5, VM_CL_te_particlesnow);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), mincorner);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM1), maxcorner);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM2), vel);
	CL_ParticleRain(mincorner, maxcorner, vel, (int)PRVM_G_FLOAT(OFS_PARM3), (int)PRVM_G_FLOAT(OFS_PARM4), 1);
}

// #411 void(vector org, vector vel, float howmany) te_spark
void VM_CL_te_spark (prvm_prog_t *prog)
{
	vec3_t pos, pos2, vel;
	VM_SAFEPARMCOUNT(3, VM_CL_te_spark);

	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), pos);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM1), vel);
	CL_FindNonSolidLocation(pos, pos2, 4);
	CL_ParticleEffect(EFFECT_TE_SPARK, PRVM_G_FLOAT(OFS_PARM2), pos2, pos2, vel, vel, nullptr, 0);
}

extern cvar_t cl_sound_ric_gunshot;
// #412 void(vector org) te_gunshotquad (DP_QUADEFFECTS1)
void VM_CL_te_gunshotquad (prvm_prog_t *prog)
{
	vec3_t		pos, pos2;
	int			rnd;
	VM_SAFEPARMCOUNT(1, VM_CL_te_gunshotquad);

	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), pos);
	CL_FindNonSolidLocation(pos, pos2, 4);
	CL_ParticleEffect(EFFECT_TE_GUNSHOTQUAD, 1, pos2, pos2, vec3_origin, vec3_origin, nullptr, 0);
	if(cl_sound_ric_gunshot.integer >= 2)
	{
		if (rand() % 5)			S_StartSound(-1, 0, cl.sfx_tink1, pos2, 1, 1);
		else
		{
			rnd = rand() & 3;
			if (rnd == 1)		S_StartSound(-1, 0, cl.sfx_ric1, pos2, 1, 1);
			else if (rnd == 2)	S_StartSound(-1, 0, cl.sfx_ric2, pos2, 1, 1);
			else				S_StartSound(-1, 0, cl.sfx_ric3, pos2, 1, 1);
		}
	}
}

// #413 void(vector org) te_spikequad (DP_QUADEFFECTS1)
void VM_CL_te_spikequad (prvm_prog_t *prog)
{
	vec3_t		pos, pos2;
	int			rnd;
	VM_SAFEPARMCOUNT(1, VM_CL_te_spikequad);

	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), pos);
	CL_FindNonSolidLocation(pos, pos2, 4);
	CL_ParticleEffect(EFFECT_TE_SPIKEQUAD, 1, pos2, pos2, vec3_origin, vec3_origin, nullptr, 0);
	if (rand() % 5)			S_StartSound(-1, 0, cl.sfx_tink1, pos2, 1, 1);
	else
	{
		rnd = rand() & 3;
		if (rnd == 1)		S_StartSound(-1, 0, cl.sfx_ric1, pos2, 1, 1);
		else if (rnd == 2)	S_StartSound(-1, 0, cl.sfx_ric2, pos2, 1, 1);
		else				S_StartSound(-1, 0, cl.sfx_ric3, pos2, 1, 1);
	}
}

// #414 void(vector org) te_superspikequad (DP_QUADEFFECTS1)
void VM_CL_te_superspikequad (prvm_prog_t *prog)
{
	vec3_t		pos, pos2;
	int			rnd;
	VM_SAFEPARMCOUNT(1, VM_CL_te_superspikequad);

	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), pos);
	CL_FindNonSolidLocation(pos, pos2, 4);
	CL_ParticleEffect(EFFECT_TE_SUPERSPIKEQUAD, 1, pos2, pos2, vec3_origin, vec3_origin, nullptr, 0);
	if (rand() % 5)			S_StartSound(-1, 0, cl.sfx_tink1, pos, 1, 1);
	else
	{
		rnd = rand() & 3;
		if (rnd == 1)		S_StartSound(-1, 0, cl.sfx_ric1, pos2, 1, 1);
		else if (rnd == 2)	S_StartSound(-1, 0, cl.sfx_ric2, pos2, 1, 1);
		else				S_StartSound(-1, 0, cl.sfx_ric3, pos2, 1, 1);
	}
}

// #415 void(vector org) te_explosionquad (DP_QUADEFFECTS1)
void VM_CL_te_explosionquad (prvm_prog_t *prog)
{
	vec3_t		pos, pos2;
	VM_SAFEPARMCOUNT(1, VM_CL_te_explosionquad);

	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), pos);
	CL_FindNonSolidLocation(pos, pos2, 10);
	CL_ParticleEffect(EFFECT_TE_EXPLOSIONQUAD, 1, pos2, pos2, vec3_origin, vec3_origin, nullptr, 0);
	S_StartSound(-1, 0, cl.sfx_r_exp3, pos2, 1, 1);
}

// #416 void(vector org) te_smallflash (DP_TE_SMALLFLASH)
void VM_CL_te_smallflash (prvm_prog_t *prog)
{
	vec3_t		pos, pos2;
	VM_SAFEPARMCOUNT(1, VM_CL_te_smallflash);

	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), pos);
	CL_FindNonSolidLocation(pos, pos2, 10);
	CL_ParticleEffect(EFFECT_TE_SMALLFLASH, 1, pos2, pos2, vec3_origin, vec3_origin, nullptr, 0);
}

// #417 void(vector org, float radius, float lifetime, vector color) te_customflash (DP_TE_CUSTOMFLASH)
void VM_CL_te_customflash (prvm_prog_t *prog)
{
	vec3_t		pos, pos2;
	matrix4x4_t	tempmatrix;
	VM_SAFEPARMCOUNT(4, VM_CL_te_customflash);

	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), pos);
	CL_FindNonSolidLocation(pos, pos2, 4);
	Matrix4x4_CreateTranslate(&tempmatrix, pos2[0], pos2[1], pos2[2]);
	CL_AllocLightFlash(nullptr, &tempmatrix, PRVM_G_FLOAT(OFS_PARM1), PRVM_G_VECTOR(OFS_PARM3)[0], PRVM_G_VECTOR(OFS_PARM3)[1], PRVM_G_VECTOR(OFS_PARM3)[2], PRVM_G_FLOAT(OFS_PARM1) / PRVM_G_FLOAT(OFS_PARM2), PRVM_G_FLOAT(OFS_PARM2), 0, -1, true, 1, 0.25, 1, 1, 1, LIGHTFLAG_NORMALMODE | LIGHTFLAG_REALTIMEMODE);
}

// #418 void(vector org) te_gunshot (DP_TE_STANDARDEFFECTBUILTINS)
void VM_CL_te_gunshot (prvm_prog_t *prog)
{
	vec3_t		pos, pos2;
	int			rnd;
	VM_SAFEPARMCOUNT(1, VM_CL_te_gunshot);

	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), pos);
	CL_FindNonSolidLocation(pos, pos2, 4);
	CL_ParticleEffect(EFFECT_TE_GUNSHOT, 1, pos2, pos2, vec3_origin, vec3_origin, nullptr, 0);
	if(cl_sound_ric_gunshot.integer == 1 || cl_sound_ric_gunshot.integer == 3)
	{
		if (rand() % 5)			S_StartSound(-1, 0, cl.sfx_tink1, pos2, 1, 1);
		else
		{
			rnd = rand() & 3;
			if (rnd == 1)		S_StartSound(-1, 0, cl.sfx_ric1, pos2, 1, 1);
			else if (rnd == 2)	S_StartSound(-1, 0, cl.sfx_ric2, pos2, 1, 1);
			else				S_StartSound(-1, 0, cl.sfx_ric3, pos2, 1, 1);
		}
	}
}

// #419 void(vector org) te_spike (DP_TE_STANDARDEFFECTBUILTINS)
void VM_CL_te_spike (prvm_prog_t *prog)
{
	vec3_t		pos, pos2;
	int			rnd;
	VM_SAFEPARMCOUNT(1, VM_CL_te_spike);

	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), pos);
	CL_FindNonSolidLocation(pos, pos2, 4);
	CL_ParticleEffect(EFFECT_TE_SPIKE, 1, pos2, pos2, vec3_origin, vec3_origin, nullptr, 0);
	if (rand() % 5)			S_StartSound(-1, 0, cl.sfx_tink1, pos2, 1, 1);
	else
	{
		rnd = rand() & 3;
		if (rnd == 1)		S_StartSound(-1, 0, cl.sfx_ric1, pos2, 1, 1);
		else if (rnd == 2)	S_StartSound(-1, 0, cl.sfx_ric2, pos2, 1, 1);
		else				S_StartSound(-1, 0, cl.sfx_ric3, pos2, 1, 1);
	}
}

// #420 void(vector org) te_superspike (DP_TE_STANDARDEFFECTBUILTINS)
void VM_CL_te_superspike (prvm_prog_t *prog)
{
	vec3_t		pos, pos2;
	int			rnd;
	VM_SAFEPARMCOUNT(1, VM_CL_te_superspike);

	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), pos);
	CL_FindNonSolidLocation(pos, pos2, 4);
	CL_ParticleEffect(EFFECT_TE_SUPERSPIKE, 1, pos2, pos2, vec3_origin, vec3_origin, nullptr, 0);
	if (rand() % 5)			S_StartSound(-1, 0, cl.sfx_tink1, pos2, 1, 1);
	else
	{
		rnd = rand() & 3;
		if (rnd == 1)		S_StartSound(-1, 0, cl.sfx_ric1, pos2, 1, 1);
		else if (rnd == 2)	S_StartSound(-1, 0, cl.sfx_ric2, pos2, 1, 1);
		else				S_StartSound(-1, 0, cl.sfx_ric3, pos2, 1, 1);
	}
}

// #421 void(vector org) te_explosion (DP_TE_STANDARDEFFECTBUILTINS)
void VM_CL_te_explosion (prvm_prog_t *prog)
{
	vec3_t		pos, pos2;
	VM_SAFEPARMCOUNT(1, VM_CL_te_explosion);

	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), pos);
	CL_FindNonSolidLocation(pos, pos2, 10);
	CL_ParticleEffect(EFFECT_TE_EXPLOSION, 1, pos2, pos2, vec3_origin, vec3_origin, nullptr, 0);
	S_StartSound(-1, 0, cl.sfx_r_exp3, pos2, 1, 1);
}

// #422 void(vector org) te_tarexplosion (DP_TE_STANDARDEFFECTBUILTINS)
void VM_CL_te_tarexplosion (prvm_prog_t *prog)
{
	vec3_t		pos, pos2;
	VM_SAFEPARMCOUNT(1, VM_CL_te_tarexplosion);

	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), pos);
	CL_FindNonSolidLocation(pos, pos2, 10);
	CL_ParticleEffect(EFFECT_TE_TAREXPLOSION, 1, pos2, pos2, vec3_origin, vec3_origin, nullptr, 0);
	S_StartSound(-1, 0, cl.sfx_r_exp3, pos2, 1, 1);
}

// #423 void(vector org) te_wizspike (DP_TE_STANDARDEFFECTBUILTINS)
void VM_CL_te_wizspike (prvm_prog_t *prog)
{
	vec3_t		pos, pos2;
	VM_SAFEPARMCOUNT(1, VM_CL_te_wizspike);

	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), pos);
	CL_FindNonSolidLocation(pos, pos2, 4);
	CL_ParticleEffect(EFFECT_TE_WIZSPIKE, 1, pos2, pos2, vec3_origin, vec3_origin, nullptr, 0);
	S_StartSound(-1, 0, cl.sfx_wizhit, pos2, 1, 1);
}

// #424 void(vector org) te_knightspike (DP_TE_STANDARDEFFECTBUILTINS)
void VM_CL_te_knightspike (prvm_prog_t *prog)
{
	vec3_t		pos, pos2;
	VM_SAFEPARMCOUNT(1, VM_CL_te_knightspike);

	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), pos);
	CL_FindNonSolidLocation(pos, pos2, 4);
	CL_ParticleEffect(EFFECT_TE_KNIGHTSPIKE, 1, pos2, pos2, vec3_origin, vec3_origin, nullptr, 0);
	S_StartSound(-1, 0, cl.sfx_knighthit, pos2, 1, 1);
}

// #425 void(vector org) te_lavasplash (DP_TE_STANDARDEFFECTBUILTINS)
void VM_CL_te_lavasplash (prvm_prog_t *prog)
{
	vec3_t		pos;
	VM_SAFEPARMCOUNT(1, VM_CL_te_lavasplash);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), pos);
	CL_ParticleEffect(EFFECT_TE_LAVASPLASH, 1, pos, pos, vec3_origin, vec3_origin, nullptr, 0);
}

// #426 void(vector org) te_teleport (DP_TE_STANDARDEFFECTBUILTINS)
void VM_CL_te_teleport (prvm_prog_t *prog)
{
	vec3_t		pos;
	VM_SAFEPARMCOUNT(1, VM_CL_te_teleport);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), pos);
	CL_ParticleEffect(EFFECT_TE_TELEPORT, 1, pos, pos, vec3_origin, vec3_origin, nullptr, 0);
}

// #427 void(vector org, float colorstart, float colorlength) te_explosion2 (DP_TE_STANDARDEFFECTBUILTINS)
void VM_CL_te_explosion2 (prvm_prog_t *prog)
{
	vec3_t		pos, pos2, color;
	matrix4x4_t	tempmatrix;
	int			colorStart, colorLength;
	unsigned char		*tempcolor;
	VM_SAFEPARMCOUNT(3, VM_CL_te_explosion2);

	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), pos);
	colorStart = (int)PRVM_G_FLOAT(OFS_PARM1);
	colorLength = (int)PRVM_G_FLOAT(OFS_PARM2);
	CL_FindNonSolidLocation(pos, pos2, 10);
	CL_ParticleExplosion2(pos2, colorStart, colorLength);
	tempcolor = palette_rgb[(rand()%colorLength) + colorStart];
	color[0] = tempcolor[0] * (2.0f / 255.0f);
	color[1] = tempcolor[1] * (2.0f / 255.0f);
	color[2] = tempcolor[2] * (2.0f / 255.0f);
	Matrix4x4_CreateTranslate(&tempmatrix, pos2[0], pos2[1], pos2[2]);
	CL_AllocLightFlash(nullptr, &tempmatrix, 350, color[0], color[1], color[2], 700, 0.5, 0, -1, true, 1, 0.25, 0.25, 1, 1, LIGHTFLAG_NORMALMODE | LIGHTFLAG_REALTIMEMODE);
	S_StartSound(-1, 0, cl.sfx_r_exp3, pos2, 1, 1);
}


// #428 void(entity own, vector start, vector end) te_lightning1 (DP_TE_STANDARDEFFECTBUILTINS)
void VM_CL_te_lightning1 (prvm_prog_t *prog)
{
	vec3_t		start, end;
	VM_SAFEPARMCOUNT(3, VM_CL_te_lightning1);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM1), start);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM2), end);
	CL_NewBeam(PRVM_G_EDICTNUM(OFS_PARM0), start, end, cl.model_bolt, true);
}

// #429 void(entity own, vector start, vector end) te_lightning2 (DP_TE_STANDARDEFFECTBUILTINS)
void VM_CL_te_lightning2 (prvm_prog_t *prog)
{
	vec3_t		start, end;
	VM_SAFEPARMCOUNT(3, VM_CL_te_lightning2);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM1), start);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM2), end);
	CL_NewBeam(PRVM_G_EDICTNUM(OFS_PARM0), start, end, cl.model_bolt2, true);
}

// #430 void(entity own, vector start, vector end) te_lightning3 (DP_TE_STANDARDEFFECTBUILTINS)
void VM_CL_te_lightning3 (prvm_prog_t *prog)
{
	vec3_t		start, end;
	VM_SAFEPARMCOUNT(3, VM_CL_te_lightning3);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM1), start);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM2), end);
	CL_NewBeam(PRVM_G_EDICTNUM(OFS_PARM0), start, end, cl.model_bolt3, false);
}

// #431 void(entity own, vector start, vector end) te_beam (DP_TE_STANDARDEFFECTBUILTINS)
void VM_CL_te_beam (prvm_prog_t *prog)
{
	vec3_t		start, end;
	VM_SAFEPARMCOUNT(3, VM_CL_te_beam);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM1), start);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM2), end);
	CL_NewBeam(PRVM_G_EDICTNUM(OFS_PARM0), start, end, cl.model_beam, false);
}

// #433 void(vector org) te_plasmaburn (DP_TE_PLASMABURN)
void VM_CL_te_plasmaburn (prvm_prog_t *prog)
{
	vec3_t		pos, pos2;
	VM_SAFEPARMCOUNT(1, VM_CL_te_plasmaburn);

	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), pos);
	CL_FindNonSolidLocation(pos, pos2, 4);
	CL_ParticleEffect(EFFECT_TE_PLASMABURN, 1, pos2, pos2, vec3_origin, vec3_origin, nullptr, 0);
}

// #457 void(vector org, vector velocity, float howmany) te_flamejet (DP_TE_FLAMEJET)
void VM_CL_te_flamejet (prvm_prog_t *prog)
{
	vec3_t		pos, pos2, vel;
	VM_SAFEPARMCOUNT(3, VM_CL_te_flamejet);
	if (PRVM_G_FLOAT(OFS_PARM2) < 1)
		return;
	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), pos);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM1), vel);
	CL_FindNonSolidLocation(pos, pos2, 4);
	CL_ParticleEffect(EFFECT_TE_FLAMEJET, PRVM_G_FLOAT(OFS_PARM2), pos2, pos2, vel, vel, nullptr, 0);
}


// #443 void(entity e, entity tagentity, string tagname) setattachment
void vm::client::setattachment (prvm_prog_t *prog)
{
	prvm_edict_t *e;
	prvm_edict_t *tagentity;
	const char *tagname;
	int modelindex;
	int tagindex;
	dp_model_t *model;
	VM_SAFEPARMCOUNT(3, VM_CL_setattachment);

	e = PRVM_G_EDICT(OFS_PARM0);
	tagentity = PRVM_G_EDICT(OFS_PARM1);
	tagname = PRVM_G_STRING(OFS_PARM2);

	if (e == prog->edicts)
	{
		VM_Warning(prog, "setattachment: can not modify world entity\n");
		return;
	}
	if (e->priv.server->free)
	{
		VM_Warning(prog, "setattachment: can not modify free entity\n");
		return;
	}

	if (tagentity == nullptr)
		tagentity = prog->edicts;

	tagindex = 0;
	if (tagentity != nullptr && tagentity != prog->edicts && tagname && tagname[0])
	{
		modelindex = (int)PRVM_clientedictfloat(tagentity, modelindex);
		model = CL_GetModelByIndex(modelindex);
		if (model)
		{
			tagindex = Mod_Alias_GetTagIndexForName(model, (int)PRVM_clientedictfloat(tagentity, skin), tagname);
			if (tagindex == 0)
				Con_DPrintf("setattachment(edict %i, edict %i, string \"%s\"): tried to find tag named \"%s\" on entity %i (model \"%s\") but could not find it\n", PRVM_NUM_FOR_EDICT(e), PRVM_NUM_FOR_EDICT(tagentity), tagname, tagname, PRVM_NUM_FOR_EDICT(tagentity), model->name);
		}
		else
			Con_DPrintf("setattachment(edict %i, edict %i, string \"%s\"): tried to find tag named \"%s\" on entity %i but it has no model\n", PRVM_NUM_FOR_EDICT(e), PRVM_NUM_FOR_EDICT(tagentity), tagname, tagname, PRVM_NUM_FOR_EDICT(tagentity));
	}

	PRVM_clientedictedict(e, tag_entity) = PRVM_EDICT_TO_PROG(tagentity);
	PRVM_clientedictfloat(e, tag_index) = tagindex;
}

/////////////////////////////////////////
// DP_MD3_TAGINFO extension coded by VorteX

static int CL_GetTagIndex (prvm_prog_t *prog, prvm_edict_t *e, const char *tagname)
{
	dp_model_t *model = CL_GetModelFromEdict(e);
	if (model)
		return Mod_Alias_GetTagIndexForName(model, (int)PRVM_clientedictfloat(e, skin), tagname);
	else
		return -1;
}

static int CL_GetExtendedTagInfo (prvm_prog_t *prog, prvm_edict_t *e, int tagindex, int *parentindex, const char **tagname, matrix4x4_t *tag_localmatrix)
{
	int r;
	dp_model_t *model;

	*tagname = nullptr;
	*parentindex = 0;
	Matrix4x4_CreateIdentity(tag_localmatrix);

	if (tagindex >= 0
	 && (model = CL_GetModelFromEdict(e))
	 && model->animscenes)
	{
		r = Mod_Alias_GetExtendedTagInfoForIndex(model, (int)PRVM_clientedictfloat(e, skin), e->priv.server->frameblend, &e->priv.server->skeleton, tagindex - 1, parentindex, tagname, tag_localmatrix);

		if(!r) // success?
			*parentindex += 1;

		return r;
	}

	return 1;
}

int CL_GetPitchSign(prvm_prog_t *prog, prvm_edict_t *ent)
{
	dp_model_t *model;
	if ((model = CL_GetModelFromEdict(ent)) && model->type == mod_alias)
		return -1;
	return 1;
}

void CL_GetEntityMatrix (prvm_prog_t *prog, prvm_edict_t *ent, matrix4x4_t *out, bool viewmatrix)
{
	float scale;
	float pitchsign = 1;

	scale = PRVM_clientedictfloat(ent, scale);
	if (!scale)
		scale = 1.0f;

	if(viewmatrix)
		*out = r_refdef.view.matrix;
	else if ((int)PRVM_clientedictfloat(ent, renderflags) & RF_USEAXIS)
	{
		vec3_t forward;
		vec3_t left;
		vec3_t up;
		vec3_t origin;
		VectorScale(PRVM_clientglobalvector(v_forward), scale, forward);
		VectorScale(PRVM_clientglobalvector(v_right), -scale, left);
		VectorScale(PRVM_clientglobalvector(v_up), scale, up);
		VectorCopy(PRVM_clientedictvector(ent, origin), origin);
		Matrix4x4_FromVectors(out, forward, left, up, origin);
	}
	else
	{
		pitchsign = CL_GetPitchSign(prog, ent);
		Matrix4x4_CreateFromQuakeEntity(out, PRVM_clientedictvector(ent, origin)[0], PRVM_clientedictvector(ent, origin)[1], PRVM_clientedictvector(ent, origin)[2], pitchsign * PRVM_clientedictvector(ent, angles)[0], PRVM_clientedictvector(ent, angles)[1], PRVM_clientedictvector(ent, angles)[2], scale);
	}
}

static int CL_GetEntityLocalTagMatrix(prvm_prog_t *prog, prvm_edict_t *ent, int tagindex, matrix4x4_t *out)
{
	dp_model_t *model;
	if (tagindex >= 0
	 && (model = CL_GetModelFromEdict(ent))
	 && model->animscenes)
	{
		VM_GenerateFrameGroupBlend(prog, ent->priv.server->framegroupblend, ent);
		VM_FrameBlendFromFrameGroupBlend(ent->priv.server->frameblend, ent->priv.server->framegroupblend, model, cl.time);
		VM_UpdateEdictSkeleton(prog, ent, model, ent->priv.server->frameblend);
		return Mod_Alias_GetTagMatrix(model, ent->priv.server->frameblend, &ent->priv.server->skeleton, tagindex, out);
	}
	*out = identitymatrix;
	return 0;
}

// Warnings/errors code:
// 0 - normal (everything all-right)
// 1 - world entity
// 2 - free entity
// 3 - null or non-precached model
// 4 - no tags with requested index
// 5 - runaway loop at attachment chain
extern cvar_t cl_bob;
extern cvar_t cl_bobcycle;
extern cvar_t cl_bobup;
int CL_GetTagMatrix (prvm_prog_t *prog, matrix4x4_t *out, prvm_edict_t *ent, int tagindex)
{
	int ret;
	int attachloop;
	matrix4x4_t entitymatrix, tagmatrix, attachmatrix;
	dp_model_t *model;

	*out = identitymatrix; // warnings and errors return identical matrix

	if (ent == prog->edicts)
		return 1;
	if (ent->priv.server->free)
		return 2;

	model = CL_GetModelFromEdict(ent);
	if(!model)
		return 3;

	tagmatrix = identitymatrix;
	attachloop = 0;
	for(;;)
	{
		if(attachloop >= 256)
			return 5;
		// apply transformation by child's tagindex on parent entity and then
		// by parent entity itself
		ret = CL_GetEntityLocalTagMatrix(prog, ent, tagindex - 1, &attachmatrix);
		if(ret && attachloop == 0)
			return ret;
		CL_GetEntityMatrix(prog, ent, &entitymatrix, false);
		Matrix4x4_Concat(&tagmatrix, &attachmatrix, out);
		Matrix4x4_Concat(out, &entitymatrix, &tagmatrix);
		// next iteration we process the parent entity
		if (PRVM_clientedictedict(ent, tag_entity))
		{
			tagindex = (int)PRVM_clientedictfloat(ent, tag_index);
			ent = PRVM_EDICT_NUM(PRVM_clientedictedict(ent, tag_entity));
		}
		else
			break;
		attachloop++;
	}

	// RENDER_VIEWMODEL magic
	if ((int)PRVM_clientedictfloat(ent, renderflags) & RF_VIEWMODEL)
	{
		Matrix4x4_Copy(&tagmatrix, out);

		CL_GetEntityMatrix(prog, prog->edicts, &entitymatrix, true);
		Matrix4x4_Concat(out, &entitymatrix, &tagmatrix);

		/*
		// Cl_bob, ported from rendering code
		if (PRVM_clientedictfloat(ent, health) > 0 && cl_bob.value && cl_bobcycle.value)
		{
			double bob, cycle;
			// LordHavoc: this code is *weird*, but not replacable (I think it
			// should be done in QC on the server, but oh well, quake is quake)
			// LordHavoc: figured out bobup: the time at which the sin is at 180
			// degrees (which allows lengthening or squishing the peak or valley)
			cycle = cl.time/cl_bobcycle.value;
			cycle -= (int)cycle;
			if (cycle < cl_bobup.value)
				cycle = sin(M_PI * cycle / cl_bobup.value);
			else
				cycle = sin(M_PI + M_PI * (cycle-cl_bobup.value)/(1.0 - cl_bobup.value));
			// bob is proportional to velocity in the xy plane
			// (don't count Z, or jumping messes it up)
			bob = sqrt(PRVM_clientedictvector(ent, velocity)[0]*PRVM_clientedictvector(ent, velocity)[0] + PRVM_clientedictvector(ent, velocity)[1]*PRVM_clientedictvector(ent, velocity)[1])*cl_bob.value;
			bob = bob*0.3 + bob*0.7*cycle;
			Matrix4x4_AdjustOrigin(out, 0, 0, bound(-7, bob, 4));
		}
		*/
	}
	return 0;
}

// #451 float(entity ent, string tagname) gettagindex (DP_QC_GETTAGINFO)
void VM_CL_gettagindex (prvm_prog_t *prog)
{
	prvm_edict_t *ent;
	const char *tag_name;
	int tag_index;

	VM_SAFEPARMCOUNT(2, VM_CL_gettagindex);

	ent = PRVM_G_EDICT(OFS_PARM0);
	tag_name = PRVM_G_STRING(OFS_PARM1);
	if (ent == prog->edicts)
	{
		VM_Warning(prog, "VM_CL_gettagindex(entity #%i): can't affect world entity\n", PRVM_NUM_FOR_EDICT(ent));
		return;
	}
	if (ent->priv.server->free)
	{
		VM_Warning(prog, "VM_CL_gettagindex(entity #%i): can't affect free entity\n", PRVM_NUM_FOR_EDICT(ent));
		return;
	}

	tag_index = 0;
	if (!CL_GetModelFromEdict(ent))
		Con_DPrintf("VM_CL_gettagindex(entity #%i): null or non-precached model\n", PRVM_NUM_FOR_EDICT(ent));
	else
	{
		tag_index = CL_GetTagIndex(prog, ent, tag_name);
		if (tag_index == 0)
			if(developer_extra.integer)
				Con_DPrintf("VM_CL_gettagindex(entity #%i): tag \"%s\" not found\n", PRVM_NUM_FOR_EDICT(ent), tag_name);
	}
	PRVM_G_FLOAT(OFS_RETURN) = tag_index;
}

// #452 vector(entity ent, float tagindex) gettaginfo (DP_QC_GETTAGINFO)
void VM_CL_gettaginfo (prvm_prog_t *prog)
{
	prvm_edict_t *e;
	int tagindex;
	matrix4x4_t tag_matrix;
	matrix4x4_t tag_localmatrix;
	int parentindex;
	const char *tagname;
	int returncode;
	vec3_t forward, left, up, origin;
	const dp_model_t *model;

	VM_SAFEPARMCOUNT(2, VM_CL_gettaginfo);

	e = PRVM_G_EDICT(OFS_PARM0);
	tagindex = (int)PRVM_G_FLOAT(OFS_PARM1);
	returncode = CL_GetTagMatrix(prog, &tag_matrix, e, tagindex);
	Matrix4x4_ToVectors(&tag_matrix, forward, left, up, origin);
	VectorCopy(forward, PRVM_clientglobalvector(v_forward));
	VectorScale(left, -1, PRVM_clientglobalvector(v_right));
	VectorCopy(up, PRVM_clientglobalvector(v_up));
	VectorCopy(origin, PRVM_G_VECTOR(OFS_RETURN));
	model = CL_GetModelFromEdict(e);
	VM_GenerateFrameGroupBlend(prog, e->priv.server->framegroupblend, e);
	VM_FrameBlendFromFrameGroupBlend(e->priv.server->frameblend, e->priv.server->framegroupblend, model, cl.time);
	VM_UpdateEdictSkeleton(prog, e, model, e->priv.server->frameblend);
	CL_GetExtendedTagInfo(prog, e, tagindex, &parentindex, &tagname, &tag_localmatrix);
	Matrix4x4_ToVectors(&tag_localmatrix, forward, left, up, origin);

	PRVM_clientglobalfloat(gettaginfo_parent) = parentindex;
	PRVM_clientglobalstring(gettaginfo_name) = tagname ? PRVM_SetTempString(prog, tagname) : 0;
	VectorCopy(forward, PRVM_clientglobalvector(gettaginfo_forward));
	VectorScale(left, -1, PRVM_clientglobalvector(gettaginfo_right));
	VectorCopy(up, PRVM_clientglobalvector(gettaginfo_up));
	VectorCopy(origin, PRVM_clientglobalvector(gettaginfo_offset));

	switch(returncode)
	{
		case 1:
			VM_Warning(prog, "gettagindex: can't affect world entity\n");
			break;
		case 2:
			VM_Warning(prog, "gettagindex: can't affect free entity\n");
			break;
		case 3:
			Con_DPrintf("CL_GetTagMatrix(entity #%i): null or non-precached model\n", PRVM_NUM_FOR_EDICT(e));
			break;
		case 4:
			Con_DPrintf("CL_GetTagMatrix(entity #%i): model has no tag with requested index %i\n", PRVM_NUM_FOR_EDICT(e), tagindex);
			break;
		case 5:
			Con_DPrintf("CL_GetTagMatrix(entity #%i): runaway loop at attachment chain\n", PRVM_NUM_FOR_EDICT(e));
			break;
	}
}

//============================================================================

//====================
// DP_CSQC_SPAWNPARTICLE
// a QC hook to engine's CL_NewParticle
//====================

// particle theme struct
typedef struct vmparticletheme_s
{
	unsigned short typeindex;
	bool initialized;
	pblend_t blendmode;
	porientation_t orientation;
	int color1;
	int color2;
	int tex;
	float size;
	float sizeincrease;
	float alpha;
	float alphafade;
	float gravity;
	float bounce;
	float airfriction;
	float liquidfriction;
	float originjitter;
	float velocityjitter;
	bool qualityreduction;
	float lifetime;
	float stretch;
	int staincolor1;
	int staincolor2;
	int staintex;
	float stainalpha;
	float stainsize;
	float delayspawn;
	float delaycollision;
	float angle;
	float spin;
}vmparticletheme_t;

// particle spawner
typedef struct vmparticlespawner_s
{
	mempool_t			*pool;
	bool			initialized;
	bool			verified;
	vmparticletheme_t	*themes;
	int					max_themes;
}vmparticlespawner_t;

vmparticlespawner_t vmpartspawner;

// TODO: automatic max_themes grow
void VM_InitParticleSpawner (prvm_prog_t *prog, int maxthemes)
{
	// bound max themes to not be an insane value
	if (maxthemes < 4)
		maxthemes = 4;
	if (maxthemes > 2048)
		maxthemes = 2048;
	// allocate and set up structure
	if (vmpartspawner.initialized) // reallocate
	{
		Mem_FreePool(&vmpartspawner.pool);
		memset(&vmpartspawner, 0, sizeof(vmparticlespawner_t));
	}
	vmpartspawner.pool = Mem_AllocPool("VMPARTICLESPAWNER", 0, nullptr);
	vmpartspawner.themes = (vmparticletheme_t *)Mem_Alloc(vmpartspawner.pool, sizeof(vmparticletheme_t)*maxthemes);
	vmpartspawner.max_themes = maxthemes;
	vmpartspawner.initialized = true;
	vmpartspawner.verified = true;
}

// reset particle theme to default values
void VM_ResetParticleTheme (vmparticletheme_t *theme)
{
	theme->initialized = true;
	theme->typeindex = pt_static;
	theme->blendmode = PBLEND_ADD;
	theme->orientation = PARTICLE_BILLBOARD;
	theme->color1 = 0x808080;
	theme->color2 = 0xFFFFFF;
	theme->tex = 63;
	theme->size = 2;
	theme->sizeincrease = 0;
	theme->alpha = 256;
	theme->alphafade = 512;
	theme->gravity = 0.0f;
	theme->bounce = 0.0f;
	theme->airfriction = 1.0f;
	theme->liquidfriction = 4.0f;
	theme->originjitter = 0.0f;
	theme->velocityjitter = 0.0f;
	theme->qualityreduction = false;
	theme->lifetime = 4;
	theme->stretch = 1;
	theme->staincolor1 = -1;
	theme->staincolor2 = -1;
	theme->staintex = -1;
	theme->delayspawn = 0.0f;
	theme->delaycollision = 0.0f;
	theme->angle = 0.0f;
	theme->spin = 0.0f;
}

// particle theme -> QC globals
void VM_CL_ParticleThemeToGlobals(vmparticletheme_t *theme, prvm_prog_t *prog)
{
	PRVM_clientglobalfloat(particle_type) = theme->typeindex;
	PRVM_clientglobalfloat(particle_blendmode) = theme->blendmode;
	PRVM_clientglobalfloat(particle_orientation) = theme->orientation;
	// VorteX: int only can store 0-255, not 0-256 which means 0 - 0,99609375...
	VectorSet(PRVM_clientglobalvector(particle_color1), (theme->color1 >> 16) & 0xFF, (theme->color1 >> 8) & 0xFF, (theme->color1 >> 0) & 0xFF);
	VectorSet(PRVM_clientglobalvector(particle_color2), (theme->color2 >> 16) & 0xFF, (theme->color2 >> 8) & 0xFF, (theme->color2 >> 0) & 0xFF);
	PRVM_clientglobalfloat(particle_tex) = (prvm_vec_t)theme->tex;
	PRVM_clientglobalfloat(particle_size) = theme->size;
	PRVM_clientglobalfloat(particle_sizeincrease) = theme->sizeincrease;
	PRVM_clientglobalfloat(particle_alpha) = theme->alpha/256;
	PRVM_clientglobalfloat(particle_alphafade) = theme->alphafade/256;
	PRVM_clientglobalfloat(particle_time) = theme->lifetime;
	PRVM_clientglobalfloat(particle_gravity) = theme->gravity;
	PRVM_clientglobalfloat(particle_bounce) = theme->bounce;
	PRVM_clientglobalfloat(particle_airfriction) = theme->airfriction;
	PRVM_clientglobalfloat(particle_liquidfriction) = theme->liquidfriction;
	PRVM_clientglobalfloat(particle_originjitter) = theme->originjitter;
	PRVM_clientglobalfloat(particle_velocityjitter) = theme->velocityjitter;
	PRVM_clientglobalfloat(particle_qualityreduction) = theme->qualityreduction;
	PRVM_clientglobalfloat(particle_stretch) = theme->stretch;
	VectorSet(PRVM_clientglobalvector(particle_staincolor1), ((int)theme->staincolor1 >> 16) & 0xFF, ((int)theme->staincolor1 >> 8) & 0xFF, ((int)theme->staincolor1 >> 0) & 0xFF);
	VectorSet(PRVM_clientglobalvector(particle_staincolor2), ((int)theme->staincolor2 >> 16) & 0xFF, ((int)theme->staincolor2 >> 8) & 0xFF, ((int)theme->staincolor2 >> 0) & 0xFF);
	PRVM_clientglobalfloat(particle_staintex) = (prvm_vec_t)theme->staintex;
	PRVM_clientglobalfloat(particle_stainalpha) = (prvm_vec_t)theme->stainalpha/256;
	PRVM_clientglobalfloat(particle_stainsize) = (prvm_vec_t)theme->stainsize;
	PRVM_clientglobalfloat(particle_delayspawn) = theme->delayspawn;
	PRVM_clientglobalfloat(particle_delaycollision) = theme->delaycollision;
	PRVM_clientglobalfloat(particle_angle) = theme->angle;
	PRVM_clientglobalfloat(particle_spin) = theme->spin;
}

// QC globals ->  particle theme
void VM_CL_ParticleThemeFromGlobals(vmparticletheme_t *theme, prvm_prog_t *prog)
{
	theme->typeindex = (unsigned short)PRVM_clientglobalfloat(particle_type);
	theme->blendmode = (pblend_t)(int)PRVM_clientglobalfloat(particle_blendmode);
	theme->orientation = (porientation_t)(int)PRVM_clientglobalfloat(particle_orientation);
	theme->color1 = ((int)PRVM_clientglobalvector(particle_color1)[0] << 16) + ((int)PRVM_clientglobalvector(particle_color1)[1] << 8) + ((int)PRVM_clientglobalvector(particle_color1)[2]);
	theme->color2 = ((int)PRVM_clientglobalvector(particle_color2)[0] << 16) + ((int)PRVM_clientglobalvector(particle_color2)[1] << 8) + ((int)PRVM_clientglobalvector(particle_color2)[2]);
	theme->tex = (int)PRVM_clientglobalfloat(particle_tex);
	theme->size = PRVM_clientglobalfloat(particle_size);
	theme->sizeincrease = PRVM_clientglobalfloat(particle_sizeincrease);
	theme->alpha = PRVM_clientglobalfloat(particle_alpha)*256;
	theme->alphafade = PRVM_clientglobalfloat(particle_alphafade)*256;
	theme->lifetime = PRVM_clientglobalfloat(particle_time);
	theme->gravity = PRVM_clientglobalfloat(particle_gravity);
	theme->bounce = PRVM_clientglobalfloat(particle_bounce);
	theme->airfriction = PRVM_clientglobalfloat(particle_airfriction);
	theme->liquidfriction = PRVM_clientglobalfloat(particle_liquidfriction);
	theme->originjitter = PRVM_clientglobalfloat(particle_originjitter);
	theme->velocityjitter = PRVM_clientglobalfloat(particle_velocityjitter);
	theme->qualityreduction = PRVM_clientglobalfloat(particle_qualityreduction) != 0 ? true : false;
	theme->stretch = PRVM_clientglobalfloat(particle_stretch);
	theme->staincolor1 = ((int)PRVM_clientglobalvector(particle_staincolor1)[0])*65536 + (int)(PRVM_clientglobalvector(particle_staincolor1)[1])*256 + (int)(PRVM_clientglobalvector(particle_staincolor1)[2]);
	theme->staincolor2 = (int)(PRVM_clientglobalvector(particle_staincolor2)[0])*65536 + (int)(PRVM_clientglobalvector(particle_staincolor2)[1])*256 + (int)(PRVM_clientglobalvector(particle_staincolor2)[2]);
	theme->staintex =(int)PRVM_clientglobalfloat(particle_staintex);
	theme->stainalpha = PRVM_clientglobalfloat(particle_stainalpha)*256;
	theme->stainsize = PRVM_clientglobalfloat(particle_stainsize);
	theme->delayspawn = PRVM_clientglobalfloat(particle_delayspawn);
	theme->delaycollision = PRVM_clientglobalfloat(particle_delaycollision);
	theme->angle = PRVM_clientglobalfloat(particle_angle);
	theme->spin = PRVM_clientglobalfloat(particle_spin);
}

// init particle spawner interface
// # float(float max_themes) initparticlespawner
void VM_CL_InitParticleSpawner (prvm_prog_t *prog)
{
	VM_SAFEPARMCOUNTRANGE(0, 1, VM_CL_InitParticleSpawner);
	VM_InitParticleSpawner(prog, (int)PRVM_G_FLOAT(OFS_PARM0));
	vmpartspawner.themes[0].initialized = true;
	VM_ResetParticleTheme(&vmpartspawner.themes[0]);
	PRVM_G_FLOAT(OFS_RETURN) = (vmpartspawner.verified == true) ? 1 : 0;
}

// void() resetparticle
void VM_CL_ResetParticle (prvm_prog_t *prog)
{
	VM_SAFEPARMCOUNT(0, VM_CL_ResetParticle);
	if (vmpartspawner.verified == false)
	{
		VM_Warning(prog, "VM_CL_ResetParticle: particle spawner not initialized\n");
		return;
	}
	VM_CL_ParticleThemeToGlobals(&vmpartspawner.themes[0], prog);
}

// void(float themenum) particletheme
void VM_CL_ParticleTheme (prvm_prog_t *prog)
{
	int themenum;

	VM_SAFEPARMCOUNT(1, VM_CL_ParticleTheme);
	if (vmpartspawner.verified == false)
	{
		VM_Warning(prog, "VM_CL_ParticleTheme: particle spawner not initialized\n");
		return;
	}
	themenum = (int)PRVM_G_FLOAT(OFS_PARM0);
	if (themenum < 0 || themenum >= vmpartspawner.max_themes)
	{
		VM_Warning(prog, "VM_CL_ParticleTheme: bad theme number %i\n", themenum);
		VM_CL_ParticleThemeToGlobals(&vmpartspawner.themes[0], prog);
		return;
	}
	if (vmpartspawner.themes[themenum].initialized == false)
	{
		VM_Warning(prog, "VM_CL_ParticleTheme: theme #%i not exists\n", themenum);
		VM_CL_ParticleThemeToGlobals(&vmpartspawner.themes[0], prog);
		return;
	}
	// load particle theme into globals
	VM_CL_ParticleThemeToGlobals(&vmpartspawner.themes[themenum], prog);
}

// float() saveparticletheme
// void(float themenum) updateparticletheme
void VM_CL_ParticleThemeSave (prvm_prog_t *prog)
{
	int themenum;

	VM_SAFEPARMCOUNTRANGE(0, 1, VM_CL_ParticleThemeSave);
	if (vmpartspawner.verified == false)
	{
		VM_Warning(prog, "VM_CL_ParticleThemeSave: particle spawner not initialized\n");
		return;
	}
	// allocate new theme, save it and return
	if (prog->argc < 1)
	{
		for (themenum = 0; themenum < vmpartspawner.max_themes; themenum++)
			if (vmpartspawner.themes[themenum].initialized == false)
				break;
		if (themenum >= vmpartspawner.max_themes)
		{
			if (vmpartspawner.max_themes == 2048)
				VM_Warning(prog, "VM_CL_ParticleThemeSave: no free theme slots\n");
			else
				VM_Warning(prog, "VM_CL_ParticleThemeSave: no free theme slots, try initparticlespawner() with highter max_themes\n");
			PRVM_G_FLOAT(OFS_RETURN) = -1;
			return;
		}
		vmpartspawner.themes[themenum].initialized = true;
		VM_CL_ParticleThemeFromGlobals(&vmpartspawner.themes[themenum], prog);
		PRVM_G_FLOAT(OFS_RETURN) = themenum;
		return;
	}
	// update existing theme
	themenum = (int)PRVM_G_FLOAT(OFS_PARM0);
	if (themenum < 0 || themenum >= vmpartspawner.max_themes)
	{
		VM_Warning(prog, "VM_CL_ParticleThemeSave: bad theme number %i\n", themenum);
		return;
	}
	vmpartspawner.themes[themenum].initialized = true;
	VM_CL_ParticleThemeFromGlobals(&vmpartspawner.themes[themenum], prog);
}

// void(float themenum) freeparticletheme
void VM_CL_ParticleThemeFree (prvm_prog_t *prog)
{
	int themenum;

	VM_SAFEPARMCOUNT(1, VM_CL_ParticleThemeFree);
	if (vmpartspawner.verified == false)
	{
		VM_Warning(prog, "VM_CL_ParticleThemeFree: particle spawner not initialized\n");
		return;
	}
	themenum = (int)PRVM_G_FLOAT(OFS_PARM0);
	// check parms
	if (themenum <= 0 || themenum >= vmpartspawner.max_themes)
	{
		VM_Warning(prog, "VM_CL_ParticleThemeFree: bad theme number %i\n", themenum);
		return;
	}
	if (vmpartspawner.themes[themenum].initialized == false)
	{
		VM_Warning(prog, "VM_CL_ParticleThemeFree: theme #%i already freed\n", themenum);
		VM_CL_ParticleThemeToGlobals(&vmpartspawner.themes[0], prog);
		return;
	}
	// free theme
	VM_ResetParticleTheme(&vmpartspawner.themes[themenum]);
	vmpartspawner.themes[themenum].initialized = false;
}

// float(vector org, vector dir, [float theme]) particle
// returns 0 if failed, 1 if succesful
void VM_CL_SpawnParticle (prvm_prog_t *prog)
{
	vec3_t org, dir;
	vmparticletheme_t *theme;
	particle_t *part;
	int themenum;

	VM_SAFEPARMCOUNTRANGE(2, 3, VM_CL_SpawnParticle2);
	if (vmpartspawner.verified == false)
	{
		VM_Warning(prog, "VM_CL_SpawnParticle: particle spawner not initialized\n");
		PRVM_G_FLOAT(OFS_RETURN) = 0;
		return;
	}
	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), org);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM1), dir);

	if (prog->argc < 3) // global-set particle
	{
		part = CL_NewParticle(org,
			(unsigned short)PRVM_clientglobalfloat(particle_type),
			((int)PRVM_clientglobalvector(particle_color1)[0] << 16) + ((int)PRVM_clientglobalvector(particle_color1)[1] << 8) + ((int)PRVM_clientglobalvector(particle_color1)[2]),
			((int)PRVM_clientglobalvector(particle_color2)[0] << 16) + ((int)PRVM_clientglobalvector(particle_color2)[1] << 8) + ((int)PRVM_clientglobalvector(particle_color2)[2]),
			(int)PRVM_clientglobalfloat(particle_tex),
			PRVM_clientglobalfloat(particle_size),
			PRVM_clientglobalfloat(particle_sizeincrease),
			PRVM_clientglobalfloat(particle_alpha)*256,
			PRVM_clientglobalfloat(particle_alphafade)*256,
			PRVM_clientglobalfloat(particle_gravity),
			PRVM_clientglobalfloat(particle_bounce),
			org[0],
			org[1],
			org[2],
			dir[0],
			dir[1],
			dir[2],
			PRVM_clientglobalfloat(particle_airfriction),
			PRVM_clientglobalfloat(particle_liquidfriction),
			PRVM_clientglobalfloat(particle_originjitter),
			PRVM_clientglobalfloat(particle_velocityjitter),
			(PRVM_clientglobalfloat(particle_qualityreduction)) ? true : false,
			PRVM_clientglobalfloat(particle_time),
			PRVM_clientglobalfloat(particle_stretch),
			(pblend_t)(int)PRVM_clientglobalfloat(particle_blendmode),
			(porientation_t)(int)PRVM_clientglobalfloat(particle_orientation),
			(int)(PRVM_clientglobalvector(particle_staincolor1)[0])*65536 + (int)(PRVM_clientglobalvector(particle_staincolor1)[1])*256 + (int)(PRVM_clientglobalvector(particle_staincolor1)[2]),
			(int)(PRVM_clientglobalvector(particle_staincolor2)[0])*65536 + (int)(PRVM_clientglobalvector(particle_staincolor2)[1])*256 + (int)(PRVM_clientglobalvector(particle_staincolor2)[2]),
			(int)PRVM_clientglobalfloat(particle_staintex),
			PRVM_clientglobalfloat(particle_stainalpha)*256,
			PRVM_clientglobalfloat(particle_stainsize),
			PRVM_clientglobalfloat(particle_angle),
			PRVM_clientglobalfloat(particle_spin),
			nullptr);
		if (!part)
		{
			PRVM_G_FLOAT(OFS_RETURN) = 0;
			return;
		}
		if (PRVM_clientglobalfloat(particle_delayspawn))
			part->delayedspawn = cl.time + PRVM_clientglobalfloat(particle_delayspawn);
		//if (PRVM_clientglobalfloat(particle_delaycollision))
		//	part->delayedcollisions = cl.time + PRVM_clientglobalfloat(particle_delaycollision);
	}
	else // quick themed particle
	{
		themenum = (int)PRVM_G_FLOAT(OFS_PARM2);
		if (themenum <= 0 || themenum >= vmpartspawner.max_themes)
		{
			VM_Warning(prog, "VM_CL_SpawnParticle: bad theme number %i\n", themenum);
			PRVM_G_FLOAT(OFS_RETURN) = 0;
			return;
		}
		theme = &vmpartspawner.themes[themenum];
		part = CL_NewParticle(org,
			theme->typeindex,
			theme->color1,
			theme->color2,
			theme->tex,
			theme->size,
			theme->sizeincrease,
			theme->alpha,
			theme->alphafade,
			theme->gravity,
			theme->bounce,
			org[0],
			org[1],
			org[2],
			dir[0],
			dir[1],
			dir[2],
			theme->airfriction,
			theme->liquidfriction,
			theme->originjitter,
			theme->velocityjitter,
			theme->qualityreduction,
			theme->lifetime,
			theme->stretch,
			theme->blendmode,
			theme->orientation,
			theme->staincolor1,
			theme->staincolor2,
			theme->staintex,
			theme->stainalpha,
			theme->stainsize,
			theme->angle,
			theme->spin,
			nullptr);
		if (!part)
		{
			PRVM_G_FLOAT(OFS_RETURN) = 0;
			return;
		}
		if (theme->delayspawn)
			part->delayedspawn = cl.time + theme->delayspawn;
		//if (theme->delaycollision)
		//	part->delayedcollisions = cl.time + theme->delaycollision;
	}
	PRVM_G_FLOAT(OFS_RETURN) = 1;
}

// float(vector org, vector dir, float spawndelay, float collisiondelay, [float theme]) delayedparticle
// returns 0 if failed, 1 if success
void VM_CL_SpawnParticleDelayed (prvm_prog_t *prog)
{
	vec3_t org, dir;
	vmparticletheme_t *theme;
	particle_t *part;
	int themenum;

	VM_SAFEPARMCOUNTRANGE(4, 5, VM_CL_SpawnParticle2);
	if (vmpartspawner.verified == false)
	{
		VM_Warning(prog, "VM_CL_SpawnParticle: particle spawner not initialized\n");
		PRVM_G_FLOAT(OFS_RETURN) = 0;
		return;
	}
	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), org);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM1), dir);
	if (prog->argc < 5) // global-set particle
		part = CL_NewParticle(org,
			(unsigned short)PRVM_clientglobalfloat(particle_type),
			((int)PRVM_clientglobalvector(particle_color1)[0] << 16) + ((int)PRVM_clientglobalvector(particle_color1)[1] << 8) + ((int)PRVM_clientglobalvector(particle_color1)[2]),
			((int)PRVM_clientglobalvector(particle_color2)[0] << 16) + ((int)PRVM_clientglobalvector(particle_color2)[1] << 8) + ((int)PRVM_clientglobalvector(particle_color2)[2]),
			(int)PRVM_clientglobalfloat(particle_tex),
			PRVM_clientglobalfloat(particle_size),
			PRVM_clientglobalfloat(particle_sizeincrease),
			PRVM_clientglobalfloat(particle_alpha)*256,
			PRVM_clientglobalfloat(particle_alphafade)*256,
			PRVM_clientglobalfloat(particle_gravity),
			PRVM_clientglobalfloat(particle_bounce),
			org[0],
			org[1],
			org[2],
			dir[0],
			dir[1],
			dir[2],
			PRVM_clientglobalfloat(particle_airfriction),
			PRVM_clientglobalfloat(particle_liquidfriction),
			PRVM_clientglobalfloat(particle_originjitter),
			PRVM_clientglobalfloat(particle_velocityjitter),
			(PRVM_clientglobalfloat(particle_qualityreduction)) ? true : false,
			PRVM_clientglobalfloat(particle_time),
			PRVM_clientglobalfloat(particle_stretch),
			(pblend_t)(int)PRVM_clientglobalfloat(particle_blendmode),
			(porientation_t)(int)PRVM_clientglobalfloat(particle_orientation),
			((int)PRVM_clientglobalvector(particle_staincolor1)[0] << 16) + ((int)PRVM_clientglobalvector(particle_staincolor1)[1] << 8) + ((int)PRVM_clientglobalvector(particle_staincolor1)[2]),
			((int)PRVM_clientglobalvector(particle_staincolor2)[0] << 16) + ((int)PRVM_clientglobalvector(particle_staincolor2)[1] << 8) + ((int)PRVM_clientglobalvector(particle_staincolor2)[2]),
			(int)PRVM_clientglobalfloat(particle_staintex),
			PRVM_clientglobalfloat(particle_stainalpha)*256,
			PRVM_clientglobalfloat(particle_stainsize),
			PRVM_clientglobalfloat(particle_angle),
			PRVM_clientglobalfloat(particle_spin),
			nullptr);
	else // themed particle
	{
		themenum = (int)PRVM_G_FLOAT(OFS_PARM4);
		if (themenum <= 0 || themenum >= vmpartspawner.max_themes)
		{
			VM_Warning(prog, "VM_CL_SpawnParticle: bad theme number %i\n", themenum);
			PRVM_G_FLOAT(OFS_RETURN) = 0;
			return;
		}
		theme = &vmpartspawner.themes[themenum];
		part = CL_NewParticle(org,
			theme->typeindex,
			theme->color1,
			theme->color2,
			theme->tex,
			theme->size,
			theme->sizeincrease,
			theme->alpha,
			theme->alphafade,
			theme->gravity,
			theme->bounce,
			org[0],
			org[1],
			org[2],
			dir[0],
			dir[1],
			dir[2],
			theme->airfriction,
			theme->liquidfriction,
			theme->originjitter,
			theme->velocityjitter,
			theme->qualityreduction,
			theme->lifetime,
			theme->stretch,
			theme->blendmode,
			theme->orientation,
			theme->staincolor1,
			theme->staincolor2,
			theme->staintex,
			theme->stainalpha,
			theme->stainsize,
			theme->angle,
			theme->spin,
			nullptr);
	}
	if (!part)
	{
		PRVM_G_FLOAT(OFS_RETURN) = 0;
		return;
	}
	part->delayedspawn = cl.time + PRVM_G_FLOAT(OFS_PARM2);
	//part->delayedcollisions = cl.time + PRVM_G_FLOAT(OFS_PARM3);
	PRVM_G_FLOAT(OFS_RETURN) = 0;
}

//====================
//CSQC engine entities query
//====================

// float(float entitynum, float whatfld) getentity;
// vector(float entitynum, float whatfld) getentityvec;
// querying engine-drawn entity
// VorteX: currently it's only tested with whatfld = 1..7
void VM_CL_GetEntity (prvm_prog_t *prog)
{
	int entnum, fieldnum;
	vec3_t forward, left, up, org;
	VM_SAFEPARMCOUNT(2, VM_CL_GetEntityVec);

	entnum = PRVM_G_FLOAT(OFS_PARM0);
	if (entnum < 0 || entnum >= cl.num_entities)
	{
		PRVM_G_FLOAT(OFS_RETURN) = 0;
		return;
	}
	fieldnum = PRVM_G_FLOAT(OFS_PARM1);
	switch(fieldnum)
	{
		case 0: // active state
			PRVM_G_FLOAT(OFS_RETURN) = cl.entities_active[entnum];
			break;
		case 1: // origin
			Matrix4x4_OriginFromMatrix(&cl.entities[entnum].render.matrix, org);
			VectorCopy(org, PRVM_G_VECTOR(OFS_RETURN));
			break;
		case 2: // forward
			Matrix4x4_ToVectors(&cl.entities[entnum].render.matrix, forward, left, up, org);
			VectorCopy(forward, PRVM_G_VECTOR(OFS_RETURN));
			break;
		case 3: // right
			Matrix4x4_ToVectors(&cl.entities[entnum].render.matrix, forward, left, up, org);
			VectorNegate(left, PRVM_G_VECTOR(OFS_RETURN));
			break;
		case 4: // up
			Matrix4x4_ToVectors(&cl.entities[entnum].render.matrix, forward, left, up, org);
			VectorCopy(up, PRVM_G_VECTOR(OFS_RETURN));
			break;
		case 5: // scale
			PRVM_G_FLOAT(OFS_RETURN) = Matrix4x4_ScaleFromMatrix(&cl.entities[entnum].render.matrix);
			break;
		case 6: // origin + v_forward, v_right, v_up
			Matrix4x4_ToVectors(&cl.entities[entnum].render.matrix, forward, left, up, org);
			VectorCopy(forward, PRVM_clientglobalvector(v_forward));
			VectorNegate(left, PRVM_clientglobalvector(v_right));
			VectorCopy(up, PRVM_clientglobalvector(v_up));
			VectorCopy(org, PRVM_G_VECTOR(OFS_RETURN));
			break;
		case 7: // alpha
			PRVM_G_FLOAT(OFS_RETURN) = cl.entities[entnum].render.alpha;
			break;
		case 8: // colormor
			VectorCopy(cl.entities[entnum].render.colormod, PRVM_G_VECTOR(OFS_RETURN));
			break;
		case 9: // pants colormod
			VectorCopy(cl.entities[entnum].render.colormap_pantscolor, PRVM_G_VECTOR(OFS_RETURN));
			break;
		case 10: // shirt colormod
			VectorCopy(cl.entities[entnum].render.colormap_shirtcolor, PRVM_G_VECTOR(OFS_RETURN));
			break;
		case 11: // skinnum
			PRVM_G_FLOAT(OFS_RETURN) = cl.entities[entnum].render.skinnum;
			break;
		case 12: // mins
			VectorCopy(cl.entities[entnum].render.mins, PRVM_G_VECTOR(OFS_RETURN));
			break;
		case 13: // maxs
			VectorCopy(cl.entities[entnum].render.maxs, PRVM_G_VECTOR(OFS_RETURN));
			break;
		case 14: // absmin
			Matrix4x4_OriginFromMatrix(&cl.entities[entnum].render.matrix, org);
			VectorAdd(cl.entities[entnum].render.mins, org, PRVM_G_VECTOR(OFS_RETURN));
			break;
		case 15: // absmax
			Matrix4x4_OriginFromMatrix(&cl.entities[entnum].render.matrix, org);
			VectorAdd(cl.entities[entnum].render.maxs, org, PRVM_G_VECTOR(OFS_RETURN));
			break;
		case 16: // light
			VectorMA(cl.entities[entnum].render.modellight_ambient, 0.5, cl.entities[entnum].render.modellight_diffuse, PRVM_G_VECTOR(OFS_RETURN));
			break;
		default:
			PRVM_G_FLOAT(OFS_RETURN) = 0;
			break;
	}
}

//====================
//QC POLYGON functions
//====================

//#304 void() renderscene (EXT_CSQC)
// moved that here to reset the polygons,
// resetting them earlier causes R_Mesh_Draw to be called with numvertices = 0
// --blub
void VM_CL_R_RenderScene (prvm_prog_t *prog)
{
	double t = Sys_DirtyTime();
	vmpolygons_t *polys = &prog->vmpolygons;
	VM_SAFEPARMCOUNT(0, VM_CL_R_RenderScene);

	// update the views
	if(r_refdef.view.ismain)
	{
		// set the main view
		csqc_main_r_refdef_view = r_refdef.view;

		// clear the flags so no other view becomes "main" unless CSQC sets VF_MAINVIEW
		r_refdef.view.ismain = false;
		csqc_original_r_refdef_view.ismain = false;
	}

	// we need to update any RENDER_VIEWMODEL entities at this point because
	// csqc supplies its own view matrix
	CL_UpdateViewEntities();

	// now draw stuff!
	R_RenderView();

	polys->num_vertices = polys->num_triangles = 0;

	// callprofile fixing hack: do not include this time in what is counted for CSQC_UpdateView
	t = Sys_DirtyTime() - t;if (t < 0 || t >= 1800) t = 0;
	if(!prog->isNative)
		prog->functions[PRVM_clientfunction(CSQC_UpdateView)].totaltime -= t;
}

void VM_ResizePolygons(vmpolygons_t *polys)
{
	float *oldvertex3f = polys->data_vertex3f;
	float *oldcolor4f = polys->data_color4f;
	float *oldtexcoord2f = polys->data_texcoord2f;
	vmpolygons_triangle_t *oldtriangles = polys->data_triangles;
	unsigned short *oldsortedelement3s = polys->data_sortedelement3s;
	polys->max_vertices = min(polys->max_triangles*3, 65536);
	polys->data_vertex3f = (float *)Mem_Alloc(polys->pool, polys->max_vertices*sizeof(float[3]));
	polys->data_color4f = (float *)Mem_Alloc(polys->pool, polys->max_vertices*sizeof(float[4]));
	polys->data_texcoord2f = (float *)Mem_Alloc(polys->pool, polys->max_vertices*sizeof(float[2]));
	polys->data_triangles = (vmpolygons_triangle_t *)Mem_Alloc(polys->pool, polys->max_triangles*sizeof(vmpolygons_triangle_t));
	polys->data_sortedelement3s = (unsigned short *)Mem_Alloc(polys->pool, polys->max_triangles*sizeof(unsigned short[3]));
	if (polys->num_vertices)
	{
		memcpy(polys->data_vertex3f, oldvertex3f, polys->num_vertices*sizeof(float[3]));
		memcpy(polys->data_color4f, oldcolor4f, polys->num_vertices*sizeof(float[4]));
		memcpy(polys->data_texcoord2f, oldtexcoord2f, polys->num_vertices*sizeof(float[2]));
	}
	if (polys->num_triangles)
	{
		memcpy(polys->data_triangles, oldtriangles, polys->num_triangles*sizeof(vmpolygons_triangle_t));
		memcpy(polys->data_sortedelement3s, oldsortedelement3s, polys->num_triangles*sizeof(unsigned short[3]));
	}
	if (oldvertex3f)
		Mem_Free(oldvertex3f);
	if (oldcolor4f)
		Mem_Free(oldcolor4f);
	if (oldtexcoord2f)
		Mem_Free(oldtexcoord2f);
	if (oldtriangles)
		Mem_Free(oldtriangles);
	if (oldsortedelement3s)
		Mem_Free(oldsortedelement3s);
}

void VM_InitPolygons (vmpolygons_t* polys)
{
	memset(polys, 0, sizeof(*polys));
	polys->pool = Mem_AllocPool("VMPOLY", 0, nullptr);
	polys->max_triangles = 1024;
	VM_ResizePolygons(polys);
	polys->initialized = true;
}

void VM_DrawPolygonCallback (const entity_render_t *ent, const rtlight_t *rtlight, int numsurfaces, int *surfacelist)
{
	int surfacelistindex;
	vmpolygons_t *polys = (vmpolygons_t *)ent;
//	R_Mesh_ResetTextureState();
	R_EntityMatrix(reinterpret_cast<const matrix4x4_t*>(&identitymatrix));
	GL_CullFace(GL_NONE);
	GL_DepthTest(true); // polys in 3D space shall always have depth test
	GL_DepthRange(0, 1);
	R_Mesh_PrepareVertices_Generic_Arrays(polys->num_vertices, polys->data_vertex3f, polys->data_color4f, polys->data_texcoord2f);

	for (surfacelistindex = 0;surfacelistindex < numsurfaces;)
	{
		int numtriangles = 0;
		rtexture_t *tex = polys->data_triangles[surfacelist[surfacelistindex]].texture;
		int drawflag = polys->data_triangles[surfacelist[surfacelistindex]].drawflag;
		DrawQ_ProcessDrawFlag(drawflag, polys->data_triangles[surfacelist[surfacelistindex]].hasalpha);
		R_SetupShader_Generic(tex, nullptr, GL_MODULATE, 1, false, false, false);
		numtriangles = 0;
		for (;surfacelistindex < numsurfaces;surfacelistindex++)
		{
			if (polys->data_triangles[surfacelist[surfacelistindex]].texture != tex || polys->data_triangles[surfacelist[surfacelistindex]].drawflag != drawflag)
				break;
			VectorCopy(polys->data_triangles[surfacelist[surfacelistindex]].elements, polys->data_sortedelement3s + 3*numtriangles);
			numtriangles++;
		}
		R_Mesh_Draw(0, polys->num_vertices, 0, numtriangles, nullptr, nullptr, 0, polys->data_sortedelement3s, nullptr, 0);
	}
}

void VMPolygons_Store(vmpolygons_t *polys)
{
	bool hasalpha;
	int i;

	// detect if we have alpha
	hasalpha = polys->begin_texture_hasalpha;
	for(i = 0; !hasalpha && (i < polys->begin_vertices); ++i)
		if(polys->begin_color[i][3] < 1)
			hasalpha = true;

	if (polys->begin_draw2d)
	{
		// draw the polygon as 2D immediately
		drawqueuemesh_t mesh;
		mesh.texture = polys->begin_texture;
		mesh.num_vertices = polys->begin_vertices;
		mesh.num_triangles = polys->begin_vertices-2;
		mesh.data_element3i = polygonelement3i;
		mesh.data_element3s = polygonelement3s;
		mesh.data_vertex3f = polys->begin_vertex[0];
		mesh.data_color4f = polys->begin_color[0];
		mesh.data_texcoord2f = polys->begin_texcoord[0];
		DrawQ_Mesh(&mesh, polys->begin_drawflag, hasalpha);
	}
	else
	{
		// queue the polygon as 3D for sorted transparent rendering later
		int i;
		if (polys->max_triangles < polys->num_triangles + polys->begin_vertices-2)
		{
			while (polys->max_triangles < polys->num_triangles + polys->begin_vertices-2)
				polys->max_triangles *= 2;
			VM_ResizePolygons(polys);
		}
		if (polys->num_vertices + polys->begin_vertices <= polys->max_vertices)
		{
			// needle in a haystack!
			// polys->num_vertices was used for copying where we actually want to copy begin_vertices
			// that also caused it to not render the first polygon that is added
			// --blub
			memcpy(polys->data_vertex3f + polys->num_vertices * 3, polys->begin_vertex[0], polys->begin_vertices * sizeof(float[3]));
			memcpy(polys->data_color4f + polys->num_vertices * 4, polys->begin_color[0], polys->begin_vertices * sizeof(float[4]));
			memcpy(polys->data_texcoord2f + polys->num_vertices * 2, polys->begin_texcoord[0], polys->begin_vertices * sizeof(float[2]));
			for (i = 0;i < polys->begin_vertices-2;i++)
			{
				polys->data_triangles[polys->num_triangles].texture = polys->begin_texture;
				polys->data_triangles[polys->num_triangles].drawflag = polys->begin_drawflag;
				polys->data_triangles[polys->num_triangles].elements[0] = polys->num_vertices;
				polys->data_triangles[polys->num_triangles].elements[1] = polys->num_vertices + i+1;
				polys->data_triangles[polys->num_triangles].elements[2] = polys->num_vertices + i+2;
				polys->data_triangles[polys->num_triangles].hasalpha = hasalpha;
				polys->num_triangles++;
			}
			polys->num_vertices += polys->begin_vertices;
		}
	}
	polys->begin_active = false;
}

// TODO: move this into the client code and clean-up everything else, too! [1/6/2008 Black]
// LordHavoc: agreed, this is a mess
void VM_CL_AddPolygonsToMeshQueue (prvm_prog_t *prog)
{
	int i;
	vmpolygons_t *polys = &prog->vmpolygons;
	vec3_t center;

	// only add polygons of the currently active prog to the queue - if there is none, we're done
	if( !prog )
		return;

	if (!polys->num_triangles)
		return;

	for (i = 0;i < polys->num_triangles;i++)
	{
		VectorMAMAM(1.0f / 3.0f, polys->data_vertex3f + 3*polys->data_triangles[i].elements[0], 1.0f / 3.0f, polys->data_vertex3f + 3*polys->data_triangles[i].elements[1], 1.0f / 3.0f, polys->data_vertex3f + 3*polys->data_triangles[i].elements[2], center);
		R_MeshQueue_AddTransparent(TRANSPARENTSORT_DISTANCE, center, VM_DrawPolygonCallback, (entity_render_t *)polys, i, nullptr);
	}

	/*polys->num_triangles = 0; // now done after rendering the scene,
	  polys->num_vertices = 0;  // otherwise it's not rendered at all and prints an error message --blub */
}

//void(string texturename, float flag[, float is2d]) R_BeginPolygon
void VM_CL_R_PolygonBegin (prvm_prog_t *prog)
{
	const char		*picname;
	skinframe_t     *sf;
	vmpolygons_t *polys = &prog->vmpolygons;
	int tf;

	// TODO instead of using skinframes here (which provides the benefit of
	// better management of flags, and is more suited for 3D rendering), what
	// about supporting Q3 shaders?

	VM_SAFEPARMCOUNTRANGE(2, 3, VM_CL_R_PolygonBegin);

	if (!polys->initialized)
		VM_InitPolygons(polys);
	if (polys->begin_active)
	{
		VM_Warning(prog, "VM_CL_R_PolygonBegin: called twice without VM_CL_R_PolygonBegin after first\n");
		return;
	}
	picname = PRVM_G_STRING(OFS_PARM0);

	sf = nullptr;
	if(*picname)
	{
		tf = TEXF_ALPHA;
		if((int)PRVM_G_FLOAT(OFS_PARM1) & DRAWFLAG_MIPMAP)
			tf |= TEXF_MIPMAP;

		do
		{
			sf = R_SkinFrame_FindNextByName(sf, picname);
		}
		while(sf && sf->textureflags != tf);

		if(!sf || !sf->base)
			sf = R_SkinFrame_LoadExternal(picname, tf, true);

		if(sf)
			R_SkinFrame_MarkUsed(sf);
	}

	polys->begin_texture = (sf && sf->base) ? sf->base : r_texture_white;
	polys->begin_texture_hasalpha = (sf && sf->base) ? sf->hasalpha : false;
	polys->begin_drawflag = (int)PRVM_G_FLOAT(OFS_PARM1) & DRAWFLAG_MASK;
	polys->begin_vertices = 0;
	polys->begin_active = true;
	polys->begin_draw2d = (prog->argc >= 3 ? (int)PRVM_G_FLOAT(OFS_PARM2) : r_refdef.draw2dstage);
}

//void(vector org, vector texcoords, vector rgb, float alpha) R_PolygonVertex
void VM_CL_R_PolygonVertex (prvm_prog_t *prog)
{
	vmpolygons_t *polys = &prog->vmpolygons;

	VM_SAFEPARMCOUNT(4, VM_CL_R_PolygonVertex);

	if (!polys->begin_active)
	{
		VM_Warning(prog, "VM_CL_R_PolygonVertex: VM_CL_R_PolygonBegin wasn't called\n");
		return;
	}

	if (polys->begin_vertices >= VMPOLYGONS_MAXPOINTS)
	{
		VM_Warning(prog, "VM_CL_R_PolygonVertex: may have %i vertices max\n", VMPOLYGONS_MAXPOINTS);
		return;
	}

	polys->begin_vertex[polys->begin_vertices][0] = PRVM_G_VECTOR(OFS_PARM0)[0];
	polys->begin_vertex[polys->begin_vertices][1] = PRVM_G_VECTOR(OFS_PARM0)[1];
	polys->begin_vertex[polys->begin_vertices][2] = PRVM_G_VECTOR(OFS_PARM0)[2];
	polys->begin_texcoord[polys->begin_vertices][0] = PRVM_G_VECTOR(OFS_PARM1)[0];
	polys->begin_texcoord[polys->begin_vertices][1] = PRVM_G_VECTOR(OFS_PARM1)[1];
	polys->begin_color[polys->begin_vertices][0] = PRVM_G_VECTOR(OFS_PARM2)[0];
	polys->begin_color[polys->begin_vertices][1] = PRVM_G_VECTOR(OFS_PARM2)[1];
	polys->begin_color[polys->begin_vertices][2] = PRVM_G_VECTOR(OFS_PARM2)[2];
	polys->begin_color[polys->begin_vertices][3] = PRVM_G_FLOAT(OFS_PARM3);
	polys->begin_vertices++;
}

//void() R_EndPolygon
void VM_CL_R_PolygonEnd (prvm_prog_t *prog)
{
	vmpolygons_t *polys = &prog->vmpolygons;

	VM_SAFEPARMCOUNT(0, VM_CL_R_PolygonEnd);
	if (!polys->begin_active)
	{
		VM_Warning(prog, "VM_CL_R_PolygonEnd: VM_CL_R_PolygonBegin wasn't called\n");
		return;
	}
	polys->begin_active = false;
	if (polys->begin_vertices >= 3)
		VMPolygons_Store(polys);
	else
		VM_Warning(prog, "VM_CL_R_PolygonEnd: %i vertices isn't a good choice\n", polys->begin_vertices);
}

static vmpolygons_t debugPolys;

void Debug_PolygonBegin(const char *picname, int drawflag)
{
	if(!debugPolys.initialized)
		VM_InitPolygons(&debugPolys);
	if(debugPolys.begin_active)
	{
		Con_Printf("Debug_PolygonBegin: called twice without Debug_PolygonEnd after first\n");
		return;
	}
	debugPolys.begin_texture = picname[0] ? Draw_CachePic_Flags (picname, CACHEPICFLAG_NOTPERSISTENT)->tex : r_texture_white;
	debugPolys.begin_drawflag = drawflag;
	debugPolys.begin_vertices = 0;
	debugPolys.begin_active = true;
}

void Debug_PolygonVertex(float x, float y, float z, float s, float t, float r, float g, float b, float a)
{
	if(!debugPolys.begin_active)
	{
		Con_Printf("Debug_PolygonVertex: Debug_PolygonBegin wasn't called\n");
		return;
	}

	if(debugPolys.begin_vertices >= VMPOLYGONS_MAXPOINTS)
	{
		Con_Printf("Debug_PolygonVertex: may have %i vertices max\n", VMPOLYGONS_MAXPOINTS);
		return;
	}

	debugPolys.begin_vertex[debugPolys.begin_vertices][0] = x;
	debugPolys.begin_vertex[debugPolys.begin_vertices][1] = y;
	debugPolys.begin_vertex[debugPolys.begin_vertices][2] = z;
	debugPolys.begin_texcoord[debugPolys.begin_vertices][0] = s;
	debugPolys.begin_texcoord[debugPolys.begin_vertices][1] = t;
	debugPolys.begin_color[debugPolys.begin_vertices][0] = r;
	debugPolys.begin_color[debugPolys.begin_vertices][1] = g;
	debugPolys.begin_color[debugPolys.begin_vertices][2] = b;
	debugPolys.begin_color[debugPolys.begin_vertices][3] = a;
	debugPolys.begin_vertices++;
}

void Debug_PolygonEnd()
{
	if (!debugPolys.begin_active)
	{
		Con_Printf("Debug_PolygonEnd: Debug_PolygonBegin wasn't called\n");
		return;
	}
	debugPolys.begin_active = false;
	if (debugPolys.begin_vertices >= 3)
		VMPolygons_Store(&debugPolys);
	else
		Con_Printf("Debug_PolygonEnd: %i vertices isn't a good choice\n", debugPolys.begin_vertices);
}

/*
=============
CL_CheckBottom

Returns false if any part of the bottom of the entity is off an edge that
is not a staircase.

=============
*/
static bool CL_CheckBottom (prvm_edict_t *ent)
{
	prvm_prog_t *prog = CLVM_prog;
	vec3_t	mins, maxs, start, stop;
	trace_t	trace;
	int		x, y;
	float	mid, bottom;

	VectorAdd (PRVM_clientedictvector(ent, origin), PRVM_clientedictvector(ent, mins), mins);
	VectorAdd (PRVM_clientedictvector(ent, origin), PRVM_clientedictvector(ent, maxs), maxs);

// if all of the points under the corners are solid world, don't bother
// with the tougher checks
// the corners must be within 16 of the midpoint
	start[2] = mins[2] - 1;
	for	(x=0 ; x<=1 ; x++)
		for	(y=0 ; y<=1 ; y++)
		{
			start[0] = x ? maxs[0] : mins[0];
			start[1] = y ? maxs[1] : mins[1];
			if (!(CL_PointSuperContents(start) & (SUPERCONTENTS_SOLID | SUPERCONTENTS_BODY)))
				goto realcheck;
		}

	return true;		// we got out easy

realcheck:
//
// check it for real...
//
	start[2] = mins[2];

// the midpoint must be within 16 of the bottom
	start[0] = stop[0] = (mins[0] + maxs[0])*0.5;
	start[1] = stop[1] = (mins[1] + maxs[1])*0.5;
	stop[2] = start[2] - 2*sv_stepheight.value;
	trace = CL_TraceLine(start, stop, MOVE_NOMONSTERS, ent, CL_GenericHitSuperContentsMask(ent), collision_extendmovelength.value, true, false, nullptr, true, false);

	if (trace.fraction == 1.0)
		return false;
	mid = bottom = trace.endpos[2];

// the corners must be within 16 of the midpoint
	for	(x=0 ; x<=1 ; x++)
		for	(y=0 ; y<=1 ; y++)
		{
			start[0] = stop[0] = x ? maxs[0] : mins[0];
			start[1] = stop[1] = y ? maxs[1] : mins[1];

			trace = CL_TraceLine(start, stop, MOVE_NOMONSTERS, ent, CL_GenericHitSuperContentsMask(ent), collision_extendmovelength.value, true, false, nullptr, true, false);

			if (trace.fraction != 1.0 && trace.endpos[2] > bottom)
				bottom = trace.endpos[2];
			if (trace.fraction == 1.0 || mid - trace.endpos[2] > sv_stepheight.value)
				return false;
		}

	return true;
}

/*
=============
CL_movestep

Called by monster program code.
The move will be adjusted for slopes and stairs, but if the move isn't
possible, no move is done and false is returned
=============
*/
static bool CL_movestep (prvm_edict_t *ent, vec3_t move, bool relink, bool noenemy, bool settrace)
{
	prvm_prog_t *prog = CLVM_prog;
	float		dz;
	vec3_t		oldorg, neworg, end, traceendpos;
	vec3_t		mins, maxs, start;
	trace_t		trace;
	int			i, svent;
	prvm_edict_t		*enemy;

// try the move
	VectorCopy(PRVM_clientedictvector(ent, mins), mins);
	VectorCopy(PRVM_clientedictvector(ent, maxs), maxs);
	VectorCopy (PRVM_clientedictvector(ent, origin), oldorg);
	VectorAdd (PRVM_clientedictvector(ent, origin), move, neworg);

// flying monsters don't step up
	if ( (int)PRVM_clientedictfloat(ent, flags) & (FL_SWIM | FL_FLY) )
	{
	// try one move with vertical motion, then one without
		for (i=0 ; i<2 ; i++)
		{
			VectorAdd (PRVM_clientedictvector(ent, origin), move, neworg);
			enemy = PRVM_PROG_TO_EDICT(PRVM_clientedictedict(ent, enemy));
			if (i == 0 && enemy != prog->edicts)
			{
				dz = PRVM_clientedictvector(ent, origin)[2] - PRVM_clientedictvector(PRVM_PROG_TO_EDICT(PRVM_clientedictedict(ent, enemy)), origin)[2];
				if (dz > 40)
					neworg[2] -= 8;
				if (dz < 30)
					neworg[2] += 8;
			}
			VectorCopy(PRVM_clientedictvector(ent, origin), start);
			trace = CL_TraceBox(start, mins, maxs, neworg, MOVE_NORMAL, ent, CL_GenericHitSuperContentsMask(ent), collision_extendmovelength.value, true, true, &svent, true);
			if (settrace)
				CL_VM_SetTraceGlobals(prog, &trace, svent);

			if (trace.fraction == 1)
			{
				VectorCopy(trace.endpos, traceendpos);
				if (((int)PRVM_clientedictfloat(ent, flags) & FL_SWIM) && !(CL_PointSuperContents(traceendpos) & SUPERCONTENTS_LIQUIDSMASK))
					return false;	// swim monster left water

				VectorCopy (traceendpos, PRVM_clientedictvector(ent, origin));
				if (relink)
					CL_LinkEdict(ent);
				return true;
			}

			if (enemy == prog->edicts)
				break;
		}

		return false;
	}

// push down from a step height above the wished position
	neworg[2] += sv_stepheight.value;
	VectorCopy (neworg, end);
	end[2] -= sv_stepheight.value*2;

	trace = CL_TraceBox(neworg, mins, maxs, end, MOVE_NORMAL, ent, CL_GenericHitSuperContentsMask(ent), collision_extendmovelength.value, true, true, &svent, true);
	if (settrace)
		CL_VM_SetTraceGlobals(prog, &trace, svent);

	if (trace.startsolid)
	{
		neworg[2] -= sv_stepheight.value;
		trace = CL_TraceBox(neworg, mins, maxs, end, MOVE_NORMAL, ent, CL_GenericHitSuperContentsMask(ent), collision_extendmovelength.value, true, true, &svent, true);
		if (settrace)
			CL_VM_SetTraceGlobals(prog, &trace, svent);
		if (trace.startsolid)
			return false;
	}
	if (trace.fraction == 1)
	{
	// if monster had the ground pulled out, go ahead and fall
		if ( (int)PRVM_clientedictfloat(ent, flags) & FL_PARTIALGROUND )
		{
			VectorAdd (PRVM_clientedictvector(ent, origin), move, PRVM_clientedictvector(ent, origin));
			if (relink)
				CL_LinkEdict(ent);
			PRVM_clientedictfloat(ent, flags) = (int)PRVM_clientedictfloat(ent, flags) & ~FL_ONGROUND;
			return true;
		}

		return false;		// walked off an edge
	}

// check point traces down for dangling corners
	VectorCopy (trace.endpos, PRVM_clientedictvector(ent, origin));

	if (!CL_CheckBottom (ent))
	{
		if ( (int)PRVM_clientedictfloat(ent, flags) & FL_PARTIALGROUND )
		{	// entity had floor mostly pulled out from underneath it
			// and is trying to correct
			if (relink)
				CL_LinkEdict(ent);
			return true;
		}
		VectorCopy (oldorg, PRVM_clientedictvector(ent, origin));
		return false;
	}

	if ( (int)PRVM_clientedictfloat(ent, flags) & FL_PARTIALGROUND )
		PRVM_clientedictfloat(ent, flags) = (int)PRVM_clientedictfloat(ent, flags) & ~FL_PARTIALGROUND;

	PRVM_clientedictedict(ent, groundentity) = PRVM_EDICT_TO_PROG(trace.ent);

// the move is ok
	if (relink)
		CL_LinkEdict(ent);
	return true;
}

/*
===============
VM_CL_walkmove

float(float yaw, float dist[, settrace]) walkmove
===============
*/
void VM_CL_walkmove (prvm_prog_t *prog)
{
	prvm_edict_t	*ent;
	float	yaw, dist;
	vec3_t	move;
	mfunction_t	*oldf;
	int 	oldself;
	bool	settrace;

	VM_SAFEPARMCOUNTRANGE(2, 3, VM_CL_walkmove);

	// assume failure if it returns early
	PRVM_G_FLOAT(OFS_RETURN) = 0;

	ent = PRVM_PROG_TO_EDICT(PRVM_clientglobaledict(self));
	if (ent == prog->edicts)
	{
		VM_Warning(prog, "walkmove: can not modify world entity\n");
		return;
	}
	if (ent->priv.server->free)
	{
		VM_Warning(prog, "walkmove: can not modify free entity\n");
		return;
	}
	yaw = PRVM_G_FLOAT(OFS_PARM0);
	dist = PRVM_G_FLOAT(OFS_PARM1);
	settrace = prog->argc >= 3 && PRVM_G_FLOAT(OFS_PARM2);

	if ( !( (int)PRVM_clientedictfloat(ent, flags) & (FL_ONGROUND|FL_FLY|FL_SWIM) ) )
		return;

	yaw = yaw*M_PI*2 / 360;

	move[0] = cos(yaw)*dist;
	move[1] = sin(yaw)*dist;
	move[2] = 0;

// save program state, because CL_movestep may call other progs
	oldf = prog->xfunction;
	oldself = PRVM_clientglobaledict(self);

	PRVM_G_FLOAT(OFS_RETURN) = CL_movestep(ent, move, true, false, settrace);


// restore program state
	prog->xfunction = oldf;
	PRVM_clientglobaledict(self) = oldself;
}

/*
===============
VM_CL_serverkey

string(string key) serverkey
===============
*/
void VM_CL_serverkey(prvm_prog_t *prog)
{
	char string[VM_STRINGTEMP_LENGTH];
	VM_SAFEPARMCOUNT(1, VM_CL_serverkey);
	InfoString_GetValue(cl.qw_serverinfo, PRVM_G_STRING(OFS_PARM0), string, sizeof(string));
	PRVM_G_INT(OFS_RETURN) = PRVM_SetTempString(prog, string);
}

/*
=================
VM_CL_checkpvs

Checks if an entity is in a point's PVS.
Should be fast but can be inexact.

float checkpvs(vector viewpos, entity viewee) = #240;
=================
*/
void VM_CL_checkpvs (prvm_prog_t *prog)
{
	vec3_t viewpos;
	prvm_edict_t *viewee;
	vec3_t mi, ma;
#if 1
	unsigned char *pvs;
#else
	int fatpvsbytes;
	unsigned char fatpvs[MAX_MAP_LEAFS/8];
#endif

	VM_SAFEPARMCOUNT(2, VM_SV_checkpvs);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), viewpos);
	viewee = PRVM_G_EDICT(OFS_PARM1);

	if(viewee->priv.required->free)
	{
		VM_Warning(prog, "checkpvs: can not check free entity\n");
		PRVM_G_FLOAT(OFS_RETURN) = 4;
		return;
	}

	VectorAdd(PRVM_serveredictvector(viewee, origin), PRVM_serveredictvector(viewee, mins), mi);
	VectorAdd(PRVM_serveredictvector(viewee, origin), PRVM_serveredictvector(viewee, maxs), ma);

#if 1
	if(!cl.worldmodel || !cl.worldmodel->brush.GetPVS || !cl.worldmodel->brush.BoxTouchingPVS)
	{
		// no PVS support on this worldmodel... darn
		PRVM_G_FLOAT(OFS_RETURN) = 3;
		return;
	}
	pvs = cl.worldmodel->brush.GetPVS(cl.worldmodel, viewpos);
	if(!pvs)
	{
		// viewpos isn't in any PVS... darn
		PRVM_G_FLOAT(OFS_RETURN) = 2;
		return;
	}
	PRVM_G_FLOAT(OFS_RETURN) = cl.worldmodel->brush.BoxTouchingPVS(cl.worldmodel, pvs, mi, ma);
#else
	// using fat PVS like FTEQW does (slow)
	if(!cl.worldmodel || !cl.worldmodel->brush.FatPVS || !cl.worldmodel->brush.BoxTouchingPVS)
	{
		// no PVS support on this worldmodel... darn
		PRVM_G_FLOAT(OFS_RETURN) = 3;
		return;
	}
	fatpvsbytes = cl.worldmodel->brush.FatPVS(cl.worldmodel, viewpos, 8, fatpvs, sizeof(fatpvs), false);
	if(!fatpvsbytes)
	{
		// viewpos isn't in any PVS... darn
		PRVM_G_FLOAT(OFS_RETURN) = 2;
		return;
	}
	PRVM_G_FLOAT(OFS_RETURN) = cl.worldmodel->brush.BoxTouchingPVS(cl.worldmodel, fatpvs, mi, ma);
#endif
}

// #263 float(float modlindex) skel_create = #263; // (FTE_CSQC_SKELETONOBJECTS) create a skeleton (be sure to assign this value into .skeletonindex for use), returns skeleton index (1 or higher) on success, returns 0 on failure  (for example if the modelindex is not skeletal), it is recommended that you create a new skeleton if you change modelindex.
void VM_CL_skel_create(prvm_prog_t *prog)
{
	int modelindex = (int)PRVM_G_FLOAT(OFS_PARM0);
	dp_model_t *model = CL_GetModelByIndex(modelindex);
	skeleton_t *skeleton;
	int i;
	PRVM_G_FLOAT(OFS_RETURN) = 0;
	if (!model || !model->num_bones)
		return;
	for (i = 0;i < MAX_EDICTS;i++)
		if (!prog->skeletons[i])
			break;
	if (i == MAX_EDICTS)
		return;
	prog->skeletons[i] = skeleton = (skeleton_t *)Mem_Alloc(cls.levelmempool, sizeof(skeleton_t) + model->num_bones * sizeof(matrix4x4_t));
	PRVM_G_FLOAT(OFS_RETURN) = i + 1;
	skeleton->model = model;
	skeleton->relativetransforms = (matrix4x4_t *)(skeleton+1);
	// initialize to identity matrices
	for (i = 0;i < skeleton->model->num_bones;i++)
		skeleton->relativetransforms[i] = identitymatrix;
}

// #264 float(float skel, entity ent, float modlindex, float retainfrac, float firstbone, float lastbone) skel_build = #264; // (FTE_CSQC_SKELETONOBJECTS) blend in a percentage of standard animation, 0 replaces entirely, 1 does nothing, 0.5 blends half, etc, and this only alters the bones in the specified range for which out of bounds values like 0,100000 are safe (uses .frame, .frame2, .frame3, .frame4, .lerpfrac, .lerpfrac3, .lerpfrac4, .frame1time, .frame2time, .frame3time, .frame4time), returns skel on success, 0 on failure
void VM_CL_skel_build(prvm_prog_t *prog)
{
	int skeletonindex = (int)PRVM_G_FLOAT(OFS_PARM0) - 1;
	skeleton_t *skeleton;
	prvm_edict_t *ed = PRVM_G_EDICT(OFS_PARM1);
	int modelindex = (int)PRVM_G_FLOAT(OFS_PARM2);
	float retainfrac = PRVM_G_FLOAT(OFS_PARM3);
	int firstbone = PRVM_G_FLOAT(OFS_PARM4) - 1;
	int lastbone = PRVM_G_FLOAT(OFS_PARM5) - 1;
	dp_model_t *model = CL_GetModelByIndex(modelindex);
	int numblends;
	int bonenum;
	int blendindex;
	framegroupblend_t framegroupblend[MAX_FRAMEGROUPBLENDS];
	frameblend_t frameblend[MAX_FRAMEBLENDS];
	matrix4x4_t bonematrix;
	matrix4x4_t matrix;
	PRVM_G_FLOAT(OFS_RETURN) = 0;
	if (skeletonindex < 0 || skeletonindex >= MAX_EDICTS || !(skeleton = prog->skeletons[skeletonindex]))
		return;
	firstbone = max(0, firstbone);
	lastbone = min(lastbone, model->num_bones - 1);
	lastbone = min(lastbone, skeleton->model->num_bones - 1);
	VM_GenerateFrameGroupBlend(prog, framegroupblend, ed);
	VM_FrameBlendFromFrameGroupBlend(frameblend, framegroupblend, model, cl.time);
	for (numblends = 0;numblends < MAX_FRAMEBLENDS && frameblend[numblends].lerp;numblends++)
		;
	for (bonenum = firstbone;bonenum <= lastbone;bonenum++)
	{
		memset(&bonematrix, 0, sizeof(bonematrix));
		for (blendindex = 0;blendindex < numblends;blendindex++)
		{
			Matrix4x4_FromBonePose7s(&matrix, model->num_posescale, model->data_poses7s + 7 * (frameblend[blendindex].subframe * model->num_bones + bonenum));
			Matrix4x4_Accumulate(&bonematrix, &matrix, frameblend[blendindex].lerp);
		}
		Matrix4x4_Normalize3(&bonematrix, &bonematrix);
		Matrix4x4_Interpolate(&skeleton->relativetransforms[bonenum], &bonematrix, &skeleton->relativetransforms[bonenum], retainfrac);
	}
	PRVM_G_FLOAT(OFS_RETURN) = skeletonindex + 1;
}

// #265 float(float skel) skel_get_numbones = #265; // (FTE_CSQC_SKELETONOBJECTS) returns how many bones exist in the created skeleton
void VM_CL_skel_get_numbones(prvm_prog_t *prog)
{
	int skeletonindex = (int)PRVM_G_FLOAT(OFS_PARM0) - 1;
	skeleton_t *skeleton;
	PRVM_G_FLOAT(OFS_RETURN) = 0;
	if (skeletonindex < 0 || skeletonindex >= MAX_EDICTS || !(skeleton = prog->skeletons[skeletonindex]))
		return;
	PRVM_G_FLOAT(OFS_RETURN) = skeleton->model->num_bones;
}

// #266 string(float skel, float bonenum) skel_get_bonename = #266; // (FTE_CSQC_SKELETONOBJECTS) returns name of bone (as a tempstring)
void VM_CL_skel_get_bonename(prvm_prog_t *prog)
{
	int skeletonindex = (int)PRVM_G_FLOAT(OFS_PARM0) - 1;
	int bonenum = (int)PRVM_G_FLOAT(OFS_PARM1) - 1;
	skeleton_t *skeleton;
	PRVM_G_INT(OFS_RETURN) = 0;
	if (skeletonindex < 0 || skeletonindex >= MAX_EDICTS || !(skeleton = prog->skeletons[skeletonindex]))
		return;
	if (bonenum < 0 || bonenum >= skeleton->model->num_bones)
		return;
	PRVM_G_INT(OFS_RETURN) = PRVM_SetTempString(prog, skeleton->model->data_bones[bonenum].name);
}

// #267 float(float skel, float bonenum) skel_get_boneparent = #267; // (FTE_CSQC_SKELETONOBJECTS) returns parent num for supplied bonenum, 0 if bonenum has no parent or bone does not exist (returned value is always less than bonenum, you can loop on this)
void VM_CL_skel_get_boneparent(prvm_prog_t *prog)
{
	int skeletonindex = (int)PRVM_G_FLOAT(OFS_PARM0) - 1;
	int bonenum = (int)PRVM_G_FLOAT(OFS_PARM1) - 1;
	skeleton_t *skeleton;
	PRVM_G_FLOAT(OFS_RETURN) = 0;
	if (skeletonindex < 0 || skeletonindex >= MAX_EDICTS || !(skeleton = prog->skeletons[skeletonindex]))
		return;
	if (bonenum < 0 || bonenum >= skeleton->model->num_bones)
		return;
	PRVM_G_FLOAT(OFS_RETURN) = skeleton->model->data_bones[bonenum].parent + 1;
}

// #268 float(float skel, string tagname) skel_find_bone = #268; // (FTE_CSQC_SKELETONOBJECTS) get number of bone with specified name, 0 on failure, tagindex (bonenum+1) on success, same as using gettagindex on the modelindex
void VM_CL_skel_find_bone(prvm_prog_t *prog)
{
	int skeletonindex = (int)PRVM_G_FLOAT(OFS_PARM0) - 1;
	const char *tagname = PRVM_G_STRING(OFS_PARM1);
	skeleton_t *skeleton;
	PRVM_G_FLOAT(OFS_RETURN) = 0;
	if (skeletonindex < 0 || skeletonindex >= MAX_EDICTS || !(skeleton = prog->skeletons[skeletonindex]))
		return;
	PRVM_G_FLOAT(OFS_RETURN) = Mod_Alias_GetTagIndexForName(skeleton->model, 0, tagname);
}

// #269 vector(float skel, float bonenum) skel_get_bonerel = #269; // (FTE_CSQC_SKELETONOBJECTS) get matrix of bone in skeleton relative to its parent - sets v_forward, v_right, v_up, returns origin (relative to parent bone)
void VM_CL_skel_get_bonerel(prvm_prog_t *prog)
{
	int skeletonindex = (int)PRVM_G_FLOAT(OFS_PARM0) - 1;
	int bonenum = (int)PRVM_G_FLOAT(OFS_PARM1) - 1;
	skeleton_t *skeleton;
	matrix4x4_t matrix;
	vec3_t forward, left, up, origin;
	VectorClear(PRVM_G_VECTOR(OFS_RETURN));
	VectorClear(PRVM_clientglobalvector(v_forward));
	VectorClear(PRVM_clientglobalvector(v_right));
	VectorClear(PRVM_clientglobalvector(v_up));
	if (skeletonindex < 0 || skeletonindex >= MAX_EDICTS || !(skeleton = prog->skeletons[skeletonindex]))
		return;
	if (bonenum < 0 || bonenum >= skeleton->model->num_bones)
		return;
	matrix = skeleton->relativetransforms[bonenum];
	Matrix4x4_ToVectors(&matrix, forward, left, up, origin);
	VectorCopy(forward, PRVM_clientglobalvector(v_forward));
	VectorNegate(left, PRVM_clientglobalvector(v_right));
	VectorCopy(up, PRVM_clientglobalvector(v_up));
	VectorCopy(origin, PRVM_G_VECTOR(OFS_RETURN));
}

// #270 vector(float skel, float bonenum) skel_get_boneabs = #270; // (FTE_CSQC_SKELETONOBJECTS) get matrix of bone in skeleton in model space - sets v_forward, v_right, v_up, returns origin (relative to entity)
void VM_CL_skel_get_boneabs(prvm_prog_t *prog)
{
	int skeletonindex = (int)PRVM_G_FLOAT(OFS_PARM0) - 1;
	int bonenum = (int)PRVM_G_FLOAT(OFS_PARM1) - 1;
	skeleton_t *skeleton;
	matrix4x4_t matrix;
	matrix4x4_t temp;
	vec3_t forward, left, up, origin;
	VectorClear(PRVM_G_VECTOR(OFS_RETURN));
	VectorClear(PRVM_clientglobalvector(v_forward));
	VectorClear(PRVM_clientglobalvector(v_right));
	VectorClear(PRVM_clientglobalvector(v_up));
	if (skeletonindex < 0 || skeletonindex >= MAX_EDICTS || !(skeleton = prog->skeletons[skeletonindex]))
		return;
	if (bonenum < 0 || bonenum >= skeleton->model->num_bones)
		return;
	matrix = skeleton->relativetransforms[bonenum];
	// convert to absolute
	while ((bonenum = skeleton->model->data_bones[bonenum].parent) >= 0)
	{
		temp = matrix;
		Matrix4x4_Concat(&matrix, &skeleton->relativetransforms[bonenum], &temp);
	}
	Matrix4x4_ToVectors(&matrix, forward, left, up, origin);
	VectorCopy(forward, PRVM_clientglobalvector(v_forward));
	VectorNegate(left, PRVM_clientglobalvector(v_right));
	VectorCopy(up, PRVM_clientglobalvector(v_up));
	VectorCopy(origin, PRVM_G_VECTOR(OFS_RETURN));
}

// #271 void(float skel, float bonenum, vector org) skel_set_bone = #271; // (FTE_CSQC_SKELETONOBJECTS) set matrix of bone relative to its parent, reads v_forward, v_right, v_up, takes origin as parameter (relative to parent bone)
void VM_CL_skel_set_bone(prvm_prog_t *prog)
{
	int skeletonindex = (int)PRVM_G_FLOAT(OFS_PARM0) - 1;
	int bonenum = (int)PRVM_G_FLOAT(OFS_PARM1) - 1;
	vec3_t forward, left, up, origin;
	skeleton_t *skeleton;
	matrix4x4_t matrix;
	if (skeletonindex < 0 || skeletonindex >= MAX_EDICTS || !(skeleton = prog->skeletons[skeletonindex]))
		return;
	if (bonenum < 0 || bonenum >= skeleton->model->num_bones)
		return;
	VectorCopy(PRVM_clientglobalvector(v_forward), forward);
	VectorNegate(PRVM_clientglobalvector(v_right), left);
	VectorCopy(PRVM_clientglobalvector(v_up), up);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM2), origin);
	Matrix4x4_FromVectors(&matrix, forward, left, up, origin);
	skeleton->relativetransforms[bonenum] = matrix;
}

// #272 void(float skel, float bonenum, vector org) skel_mul_bone = #272; // (FTE_CSQC_SKELETONOBJECTS) transform bone matrix (relative to its parent) by the supplied matrix in v_forward, v_right, v_up, takes origin as parameter (relative to parent bone)
void VM_CL_skel_mul_bone(prvm_prog_t *prog)
{
	int skeletonindex = (int)PRVM_G_FLOAT(OFS_PARM0) - 1;
	int bonenum = (int)PRVM_G_FLOAT(OFS_PARM1) - 1;
	vec3_t forward, left, up, origin;
	skeleton_t *skeleton;
	matrix4x4_t matrix;
	matrix4x4_t temp;
	if (skeletonindex < 0 || skeletonindex >= MAX_EDICTS || !(skeleton = prog->skeletons[skeletonindex]))
		return;
	if (bonenum < 0 || bonenum >= skeleton->model->num_bones)
		return;
	VectorCopy(PRVM_G_VECTOR(OFS_PARM2), origin);
	VectorCopy(PRVM_clientglobalvector(v_forward), forward);
	VectorNegate(PRVM_clientglobalvector(v_right), left);
	VectorCopy(PRVM_clientglobalvector(v_up), up);
	Matrix4x4_FromVectors(&matrix, forward, left, up, origin);
	temp = skeleton->relativetransforms[bonenum];
	Matrix4x4_Concat(&skeleton->relativetransforms[bonenum], &matrix, &temp);
}

// #273 void(float skel, float startbone, float endbone, vector org) skel_mul_bones = #273; // (FTE_CSQC_SKELETONOBJECTS) transform bone matrices (relative to their parents) by the supplied matrix in v_forward, v_right, v_up, takes origin as parameter (relative to parent bones)
void VM_CL_skel_mul_bones(prvm_prog_t *prog)
{
	int skeletonindex = (int)PRVM_G_FLOAT(OFS_PARM0) - 1;
	int firstbone = PRVM_G_FLOAT(OFS_PARM1) - 1;
	int lastbone = PRVM_G_FLOAT(OFS_PARM2) - 1;
	int bonenum;
	vec3_t forward, left, up, origin;
	skeleton_t *skeleton;
	matrix4x4_t matrix;
	matrix4x4_t temp;
	if (skeletonindex < 0 || skeletonindex >= MAX_EDICTS || !(skeleton = prog->skeletons[skeletonindex]))
		return;
	VectorCopy(PRVM_G_VECTOR(OFS_PARM3), origin);
	VectorCopy(PRVM_clientglobalvector(v_forward), forward);
	VectorNegate(PRVM_clientglobalvector(v_right), left);
	VectorCopy(PRVM_clientglobalvector(v_up), up);
	Matrix4x4_FromVectors(&matrix, forward, left, up, origin);
	firstbone = max(0, firstbone);
	lastbone = min(lastbone, skeleton->model->num_bones - 1);
	for (bonenum = firstbone;bonenum <= lastbone;bonenum++)
	{
		temp = skeleton->relativetransforms[bonenum];
		Matrix4x4_Concat(&skeleton->relativetransforms[bonenum], &matrix, &temp);
	}
}

// #274 void(float skeldst, float skelsrc, float startbone, float endbone) skel_copybones = #274; // (FTE_CSQC_SKELETONOBJECTS) copy bone matrices (relative to their parents) from one skeleton to another, useful for copying a skeleton to a corpse
void VM_CL_skel_copybones(prvm_prog_t *prog)
{
	int skeletonindexdst = (int)PRVM_G_FLOAT(OFS_PARM0) - 1;
	int skeletonindexsrc = (int)PRVM_G_FLOAT(OFS_PARM1) - 1;
	int firstbone = PRVM_G_FLOAT(OFS_PARM2) - 1;
	int lastbone = PRVM_G_FLOAT(OFS_PARM3) - 1;
	int bonenum;
	skeleton_t *skeletondst;
	skeleton_t *skeletonsrc;
	if (skeletonindexdst < 0 || skeletonindexdst >= MAX_EDICTS || !(skeletondst = prog->skeletons[skeletonindexdst]))
		return;
	if (skeletonindexsrc < 0 || skeletonindexsrc >= MAX_EDICTS || !(skeletonsrc = prog->skeletons[skeletonindexsrc]))
		return;
	firstbone = max(0, firstbone);
	lastbone = min(lastbone, skeletondst->model->num_bones - 1);
	lastbone = min(lastbone, skeletonsrc->model->num_bones - 1);
	for (bonenum = firstbone;bonenum <= lastbone;bonenum++)
		skeletondst->relativetransforms[bonenum] = skeletonsrc->relativetransforms[bonenum];
}

// #275 void(float skel) skel_delete = #275; // (FTE_CSQC_SKELETONOBJECTS) deletes skeleton at the beginning of the next frame (you can add the entity, delete the skeleton, renderscene, and it will still work)
void VM_CL_skel_delete(prvm_prog_t *prog)
{
	int skeletonindex = (int)PRVM_G_FLOAT(OFS_PARM0) - 1;
	skeleton_t *skeleton;
	if (skeletonindex < 0 || skeletonindex >= MAX_EDICTS || !(skeleton = prog->skeletons[skeletonindex]))
		return;
	Mem_Free(skeleton);
	prog->skeletons[skeletonindex] = nullptr;
}

// #276 float(float modlindex, string framename) frameforname = #276; // (FTE_CSQC_SKELETONOBJECTS) finds number of a specified frame in the animation, returns -1 if no match found
void VM_CL_frameforname(prvm_prog_t *prog)
{
	int modelindex = (int)PRVM_G_FLOAT(OFS_PARM0);
	dp_model_t *model = CL_GetModelByIndex(modelindex);
	const char *name = PRVM_G_STRING(OFS_PARM1);
	int i;
	PRVM_G_FLOAT(OFS_RETURN) = -1;
	if (!model || !model->animscenes)
		return;
	for (i = 0;i < model->numframes;i++)
	{
		if (!strcasecmp(model->animscenes[i].name, name))
		{
			PRVM_G_FLOAT(OFS_RETURN) = i;
			break;
		}
	}
}

// #277 float(float modlindex, float framenum) frameduration = #277; // (FTE_CSQC_SKELETONOBJECTS) returns the intended play time (in seconds) of the specified framegroup, if it does not exist the result is 0, if it is a single frame it may be a small value around 0.1 or 0.
void VM_CL_frameduration(prvm_prog_t *prog)
{
	int modelindex = (int)PRVM_G_FLOAT(OFS_PARM0);
	dp_model_t *model = CL_GetModelByIndex(modelindex);
	int framenum = (int)PRVM_G_FLOAT(OFS_PARM1);
	PRVM_G_FLOAT(OFS_RETURN) = 0;
	if (!model || !model->animscenes || framenum < 0 || framenum >= model->numframes)
		return;
	if (model->animscenes[framenum].framerate)
		PRVM_G_FLOAT(OFS_RETURN) = model->animscenes[framenum].framecount / model->animscenes[framenum].framerate;
}

void VM_CL_RotateMoves(prvm_prog_t *prog)
{
	/*
	 * Obscure builtin used by GAME_XONOTIC.
	 *
	 * Edits the input history of cl_movement by rotating all move commands
	 * currently in the queue using the given transform.
	 *
	 * The vector passed is an "angles transform" as used by warpzonelib, i.e.
	 * v_angle-like (non-inverted) euler angles that perform the rotation
	 * of the space that is to be done.
	 *
	 * This is meant to be used as a fixangle replacement after passing
	 * through a warpzone/portal: the client is told about the warp transform,
	 * and calls this function in the same frame as the one on which the
	 * client's origin got changed by the serverside teleport. Then this code
	 * transforms the pre-warp input (which matches the empty space behind
	 * the warp plane) into post-warp input (which matches the target area
	 * of the warp). Also, at the same time, the client has to use
	 * R_SetView to adjust VF_CL_VIEWANGLES according to the same transform.
	 *
	 * This together allows warpzone motion to be perfectly predicted by
	 * the client!
	 *
	 * Furthermore, for perfect warpzone behaviour, the server side also
	 * has to detect input the client sent before it received the origin
	 * update, but after the warp occurred on the server, and has to adjust
	 * input appropriately.
    */
	matrix4x4_t m;
	vec3_t v = {0, 0, 0};
	vec3_t a, x, y, z;
	VM_SAFEPARMCOUNT(1, VM_CL_RotateMoves);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), a);
	AngleVectorsFLU(a, x, y, z);
	Matrix4x4_FromVectors(&m, x, y, z, v);
	CL_RotateMoves(&m);
}

// #358 void(string cubemapname) loadcubemap
void VM_CL_loadcubemap(prvm_prog_t *prog)
{
	const char *name;

	VM_SAFEPARMCOUNT(1, VM_CL_loadcubemap);
	name = PRVM_G_STRING(OFS_PARM0);
	R_GetCubemap(name);
}

#define REFDEFFLAG_TELEPORTED 1
#define REFDEFFLAG_JUMPING 2
#define REFDEFFLAG_DEAD 4
#define REFDEFFLAG_INTERMISSION 8
void VM_CL_V_CalcRefdef(prvm_prog_t *prog)
{
	matrix4x4_t entrendermatrix;
	vec3_t clviewangles;
	vec3_t clvelocity;
	bool teleported;
	bool clonground;
	bool clcmdjump;
	bool cldead;
	bool clintermission;
	float clstatsviewheight;
	prvm_edict_t *ent;
	int flags;

	VM_SAFEPARMCOUNT(2, VM_CL_V_CalcRefdef);
	ent = PRVM_G_EDICT(OFS_PARM0);
	flags = PRVM_G_FLOAT(OFS_PARM1);

	// use the CL_GetTagMatrix function on self to ensure consistent behavior (duplicate code would be bad)
	CL_GetTagMatrix(prog, &entrendermatrix, ent, 0);

	VectorCopy(cl.csqc_viewangles, clviewangles);
	teleported = (flags & REFDEFFLAG_TELEPORTED) != 0;
	clonground = ((int)PRVM_clientedictfloat(ent, pmove_flags) & PMF_ONGROUND) != 0;
	clcmdjump = (flags & REFDEFFLAG_JUMPING) != 0;
	clstatsviewheight = PRVM_clientedictvector(ent, view_ofs)[2];
	cldead = (flags & REFDEFFLAG_DEAD) != 0;
	clintermission = (flags & REFDEFFLAG_INTERMISSION) != 0;
	VectorCopy(PRVM_clientedictvector(ent, velocity), clvelocity);

	V_CalcRefdefUsing(&entrendermatrix, clviewangles, teleported, clonground, clcmdjump, clstatsviewheight, cldead, clintermission, clvelocity);

	VectorCopy(cl.csqc_vieworiginfromengine, cl.csqc_vieworigin);
	VectorCopy(cl.csqc_viewanglesfromengine, cl.csqc_viewangles);
	CSQC_R_RecalcView();
}


void VM_Polygons_Reset(prvm_prog_t *prog)
{
	vmpolygons_t *polys = &prog->vmpolygons;

	// TODO: replace vm_polygons stuff with a more general debugging polygon system, and make vm_polygons functions use that system
	if(polys->initialized)
	{
		Mem_FreePool(&polys->pool);
		polys->initialized = false;
	}
}

void CLVM_init_cmd(prvm_prog_t *prog)
{
	VM_Cmd_Init(prog);
	VM_Polygons_Reset(prog);
}

void CLVM_reset_cmd(prvm_prog_t *prog)
{
	World_End(&cl.world);
	VM_Cmd_Reset(prog);
	VM_Polygons_Reset(prog);
}

#endif
