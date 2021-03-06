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
// cvar.h

/*

cvar_t variables are used to hold scalar or string variables that can be changed or displayed at the console or prog code as well as accessed directly
in C code.

it is sufficient to initialize a cvar_t with just the first two fields, or
you can add a ,true flag for variables that you want saved to the configuration
file when the game is quit:

cvar_t	r_draworder = {"r_draworder","1"};
cvar_t	scr_screensize = {"screensize","1",true};

Cvars must be registered before use, or they will have a 0 value instead of the float interpretation of the string.  Generally, all cvar_t declarations should be registered in the apropriate init function before any console commands are executed:
Cvar_RegisterVariable (&host_framerate);


C code usually just references a cvar in place:
if ( r_draworder.value )

It could optionally ask for the value to be looked up for a string name:
if (Cvar_VariableValue ("r_draworder"))

Interpreted prog code can access cvars with the cvar(name) or
cvar_set (name, value) internal functions:
teamplay = cvar("teamplay");
cvar_set ("registered", "1");

The user can access cvars from the console in two ways:
r_draworder			prints the current value
r_draworder 0		sets the current value to 0
Cvars are restricted from having the same names as commands to keep this
interface from being ambiguous.
*/
#pragma once

// cvar flags

#define CVAR_SAVE 1
#define CVAR_NOTIFY 2
#define CVAR_READONLY 4
#define CVAR_SERVERINFO 8
#define CVAR_USERINFO 16
// CVAR_PRIVATE means do not $ expand or sendcvar this cvar under any circumstances (rcon_password uses this)
#define CVAR_PRIVATE 32
// this means that this cvar should update a userinfo key but the name does not correspond directly to the userinfo key to update, and may require additional conversion ("_cl_color" for example should update "topcolor" and "bottomcolor")
#define CVAR_NQUSERINFOHACK 64
// used to determine if flags is valid
#define CVAR_NORESETTODEFAULTS 128
// for engine-owned cvars that must not be reset on gametype switch (e.g. scr_screenshot_name, which otherwise isn't set to the mod name properly)
#define CVAR_MAXFLAGSVAL 255
// for internal use only!
#define CVAR_DEFAULTSET (1<<30)
#define CVAR_ALLOCATED (1<<31)


namespace cloture	{
namespace engine	{
namespace cvars		{

//typedef struct cvar_s
struct CVar
{
	int flags;

	const char *name;

	const char *string;
	const char *description;
	int integer;
	float value;
	float vector[3];

	const char *defstring;

	// values at init (for Cvar_RestoreInitState)
	bool initstate; // indicates this existed at init
	int initflags;
	const char *initstring;
	const char *initdescription;
	int initinteger;
	float initvalue;
	float initvector[3];
	const char *initdefstring;

	unsigned int globaldefindex_progid[3];
	int globaldefindex[3];
	int globaldefindex_stringno[3];

	//menucvar_t menuinfo;
	struct cvar_s *next;
	struct cvar_s *nextonhashchain;

	void Register();
	static void set(const char* name, const char* value);
	static void setValue(const char *name, const float value);
	void setQuick(const char* value);
	void setValueQuick(const float value);
	static struct CVar* find(const char* name);
	static struct CVar* findAfter(const char* prevName, const int neededflags);

	static float variableValue(const char* name);
}; //cvar_t;


}//namespace cvars
}//namespace engine
}//namespace cloture
//using cvar_s = struct cloture::engine::cvars::CVar;
using cvar_t = cloture::engine::cvars::CVar;
//using cvar_s = cvar_t;

/// registers a cvar that already has the name, string, and optionally the
/// archive elements set.
void Cvar_RegisterVariable (cvar_t *variable);

/// equivelant to "<name> <variable>" typed at the console
void Cvar_Set (const char *var_name, const char *value);

/// expands value to a string and calls Cvar_Set
void Cvar_SetValue (const char *var_name, float value);

void Cvar_SetQuick (cvar_t *var, const char *value);
void Cvar_SetValueQuick (cvar_t *var, float value);

float Cvar_VariableValueOr (const char *var_name, float def);
// returns def if not defined

float Cvar_VariableValue (const char *var_name);
// returns 0 if not defined or non numeric

const char *Cvar_VariableStringOr (const char *var_name, const char *def);
// returns def if not defined

const char *Cvar_VariableString (const char *var_name);
// returns an empty string if not defined

const char *Cvar_VariableDefString (const char *var_name);
// returns an empty string if not defined

const char *Cvar_VariableDescription (const char *var_name);
// returns an empty string if not defined

const char *Cvar_CompleteVariable (const char *partial);
// attempts to match a partial variable name for command line completion
// returns NULL if nothing fits

void Cvar_CompleteCvarPrint (const char *partial);

bool Cvar_Command ();
// called by Cmd_ExecuteString when Cmd_Argv(0) doesn't match a known
// command.  Returns true if the command was a variable reference that
// was handled. (print or change)

void Cvar_SaveInitState();
void Cvar_RestoreInitState();

void Cvar_UnlockDefaults ();
void Cvar_LockDefaults_f ();
void Cvar_ResetToDefaults_All_f ();
void Cvar_ResetToDefaults_NoSaveOnly_f ();
void Cvar_ResetToDefaults_SaveOnly_f ();

void Cvar_WriteVariables (qfile_t *f);
// Writes lines containing "set variable value" for all variables
// with the archive flag set to true.

cvar_t *Cvar_FindVar (const char *var_name);
cvar_t *Cvar_FindVarAfter (const char *prev_var_name, int neededflags);

int Cvar_CompleteCountPossible (const char *partial);
const char **Cvar_CompleteBuildList (const char *partial);
// Added by EvilTypeGuy - functions for tab completion system
// Thanks to Fett erich@heintz.com
// Thanks to taniwha

/// Prints a list of Cvars including a count of them to the user console
/// Referenced in cmd.c in Cmd_Init hence it's inclusion here.
/// Added by EvilTypeGuy eviltypeguy@qeradiant.com
/// Thanks to Matthias "Maddes" Buecher, http://www.inside3d.com/qip/
void Cvar_List_f ();

void Cvar_Set_f ();
void Cvar_SetA_f ();
void Cvar_Del_f ();
// commands to create new cvars (or set existing ones)
// seta creates an archived cvar (saved to config)

/// allocates a cvar by name and returns its address,
/// or merely sets its value if it already exists.
cvar_t *Cvar_Get (const char *name, const char *value, int flags, const char *newdescription);

extern const char *cvar_dummy_description; // ALWAYS the same pointer
extern cvar_t *cvar_vars; // used to list all cvars

void Cvar_UpdateAllAutoCvars(); // updates ALL autocvars of the active prog to the cvar values (savegame loading)

#ifdef FILLALLCVARSWITHRUBBISH
void Cvar_FillAll_f();
#endif /* FILLALLCVARSWITHRUBBISH */


namespace cloture::engine::cvars
{

	inline void CVar::Register()
	{
		Cvar_RegisterVariable(this);
	}
	inline void CVar::set(const char* name, const char* value)
	{
		Cvar_Set(name, value);
	}
	inline void CVar::setValue(const char *name, const float value)
	{
		Cvar_SetValue(name, value);
	}

	inline void CVar::setQuick(const char* value)
	{
		Cvar_SetQuick(this, value);
	}
	inline void CVar::setValueQuick(const float value)
	{
		Cvar_SetValueQuick(this, value);
	}

	inline struct CVar* CVar::find(const char* name)
	{
		return Cvar_FindVar(name);
	}
	inline struct CVar* CVar::findAfter(const char* prevName, const int neededflags)
	{
		return Cvar_FindVarAfter(prevName, neededflags);
	}
	inline float CVar::variableValue(const char* name)
	{
		return Cvar_VariableValue(name);
	}
}
