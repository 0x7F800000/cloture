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
/*
This is a try to make the vm more generic, it is mainly based on the progs.h file.
For the license refer to progs.h.

Generic means, less as possible hard-coded links with the other parts of the engine.
This means no edict_engineprivate struct usage, etc.
The code uses void pointers instead.
*/
#pragma once

#include "pr_comp.h"			// defs shared with qcc
#include "progdefs.h"			// generated by program cdefs
#include "clprogdefs.h"			// generated by program cdefs

#ifndef DP_SMALLMEMORY
#define PROFILING
#endif

#define	EDICTPRIVATE_CONSTEXPR	0
#define	CONSTEXPR_OFFSETS		0

typedef struct prvm_stack_s
{
	int				s;
	mfunction_t		*f;
	double			tprofile_acc;
	double			profile_acc;
	double			builtinsprofile_acc;
} prvm_stack_t;


typedef union prvm_eval_s
//union prvm_eval_t
{
	prvm_int_t		string;
	prvm_vec_t	_float;
	prvm_vec_t	vector[3];
	prvm_int_t		function;
	prvm_int_t		ivector[3];
	prvm_int_t		_int;
	prvm_int_t		edict;
}prvm_eval_t;

typedef struct prvm_required_field_s
{
	int type;
	const char *name;
} prvm_required_field_t;


// AK: I dont call it engine private cause it doesnt really belongs to the engine
//     it belongs to prvm.
typedef struct prvm_edict_private_s
{
	bool free;
	float freetime;
	int mark; // used during leaktest (0 = unref, >0 = referenced); special values during server physics:
#define PRVM_EDICT_MARK_WAIT_FOR_SETORIGIN -1
#define PRVM_EDICT_MARK_SETORIGIN_CAUGHT -2
	const char *allocation_origin;
} prvm_edict_private_t;

typedef struct prvm_edict_s
{
	// engine-private fields (stored in dynamically resized array)
	union
	{
		prvm_edict_private_t *required;
		prvm_vec_t *fp;
		prvm_int_t *ip;
		// FIXME: this server pointer really means world, not server
		// (it is used by both server qc and client qc, but not menu qc)
		edict_engineprivate_t *server;
		// add other private structs as you desire
		// new structs have to start with the elements of prvm_edit_private_t
		// e.g. a new struct has to either look like this:
		//	typedef struct server_edict_private_s {
		//		prvm_edict_private_t base;
		//		vec3_t moved_from;
		//      vec3_t moved_fromangles;
		//		... } server_edict_private_t;
		// or:
		//	typedef struct server_edict_private_s {
		//		bool free;
		//		float freetime;
		//		vec3_t moved_from;
		//      vec3_t moved_fromangles;
		//		... } server_edict_private_t;
		// However, the first one should be preferred.
	} priv;
	// QuakeC fields (stored in dynamically resized array)
	union
	{
		prvm_vec_t *fp;
		prvm_int_t *ip;

	} fields;
} prvm_edict_t;

#define VMPOLYGONS_MAXPOINTS 64

typedef struct vmpolygons_triangle_s
{
	rtexture_t		*texture;
	int				drawflag;
	bool hasalpha;
	unsigned short	elements[3];
} vmpolygons_triangle_t;

typedef struct vmpolygons_s
{
	mempool_t		*pool;
	bool		initialized;

	int				max_vertices;
	int				num_vertices;
	float			*data_vertex3f;
	float			*data_color4f;
	float			*data_texcoord2f;

	int				max_triangles;
	int				num_triangles;
	vmpolygons_triangle_t *data_triangles;
	unsigned short	*data_sortedelement3s;

	bool		begin_active;
	int	begin_draw2d;
	rtexture_t		*begin_texture;
	int				begin_drawflag;
	int				begin_vertices;
	float			begin_vertex[VMPOLYGONS_MAXPOINTS][3];
	float			begin_color[VMPOLYGONS_MAXPOINTS][4];
	float			begin_texcoord[VMPOLYGONS_MAXPOINTS][2];
	bool		begin_texture_hasalpha;
} vmpolygons_t;

extern prvm_eval_t prvm_badvalue;

#define PRVM_alledictfloat(ed, fieldname)    (PRVM_EDICTFIELDFLOAT(ed, prog->fieldoffsets.fieldname))
#define PRVM_alledictvector(ed, fieldname)   (PRVM_EDICTFIELDVECTOR(ed, prog->fieldoffsets.fieldname))
#define PRVM_alledictstring(ed, fieldname)   (PRVM_EDICTFIELDSTRING(ed, prog->fieldoffsets.fieldname))
#define PRVM_alledictedict(ed, fieldname)    (PRVM_EDICTFIELDEDICT(ed, prog->fieldoffsets.fieldname))
#define PRVM_alledictfunction(ed, fieldname) (PRVM_EDICTFIELDFUNCTION(ed, prog->fieldoffsets.fieldname))
#define PRVM_allglobalfloat(fieldname)       (PRVM_GLOBALFIELDFLOAT(prog->globaloffsets.fieldname))
#define PRVM_allglobalvector(fieldname)      (PRVM_GLOBALFIELDVECTOR(prog->globaloffsets.fieldname))
#define PRVM_allglobalstring(fieldname)      (PRVM_GLOBALFIELDSTRING(prog->globaloffsets.fieldname))
#define PRVM_allglobaledict(fieldname)       (PRVM_GLOBALFIELDEDICT(prog->globaloffsets.fieldname))
#define PRVM_allglobalfunction(fieldname)    (PRVM_GLOBALFIELDFUNCTION(prog->globaloffsets.fieldname))
#define PRVM_allfunction(funcname)           (prog->funcoffsets.funcname)

#define PRVM_drawedictfloat(ed, fieldname)    (PRVM_EDICTFIELDFLOAT(ed, prog->fieldoffsets.fieldname))
#define PRVM_drawedictvector(ed, fieldname)   (PRVM_EDICTFIELDVECTOR(ed, prog->fieldoffsets.fieldname))
#define PRVM_drawedictstring(ed, fieldname)   (PRVM_EDICTFIELDSTRING(ed, prog->fieldoffsets.fieldname))
#define PRVM_drawedictedict(ed, fieldname)    (PRVM_EDICTFIELDEDICT(ed, prog->fieldoffsets.fieldname))
#define PRVM_drawedictfunction(ed, fieldname) (PRVM_EDICTFIELDFUNCTION(ed, prog->fieldoffsets.fieldname))
#define PRVM_drawglobalfloat(fieldname)       (PRVM_GLOBALFIELDFLOAT(prog->globaloffsets.fieldname))
#define PRVM_drawglobalvector(fieldname)      (PRVM_GLOBALFIELDVECTOR(prog->globaloffsets.fieldname))
#define PRVM_drawglobalstring(fieldname)      (PRVM_GLOBALFIELDSTRING(prog->globaloffsets.fieldname))
#define PRVM_drawglobaledict(fieldname)       (PRVM_GLOBALFIELDEDICT(prog->globaloffsets.fieldname))
#define PRVM_drawglobalfunction(fieldname)    (PRVM_GLOBALFIELDFUNCTION(prog->globaloffsets.fieldname))
#define PRVM_drawfunction(funcname)           (prog->funcoffsets.funcname)

#define PRVM_gameedictfloat(ed, fieldname)    (PRVM_EDICTFIELDFLOAT(ed, prog->fieldoffsets.fieldname))
#define PRVM_gameedictvector(ed, fieldname)   (PRVM_EDICTFIELDVECTOR(ed, prog->fieldoffsets.fieldname))
#define PRVM_gameedictstring(ed, fieldname)   (PRVM_EDICTFIELDSTRING(ed, prog->fieldoffsets.fieldname))
#define PRVM_gameedictedict(ed, fieldname)    (PRVM_EDICTFIELDEDICT(ed, prog->fieldoffsets.fieldname))
#define PRVM_gameedictfunction(ed, fieldname) (PRVM_EDICTFIELDFUNCTION(ed, prog->fieldoffsets.fieldname))
#define PRVM_gameglobalfloat(fieldname)       (PRVM_GLOBALFIELDFLOAT(prog->globaloffsets.fieldname))
#define PRVM_gameglobalvector(fieldname)      (PRVM_GLOBALFIELDVECTOR(prog->globaloffsets.fieldname))
#define PRVM_gameglobalstring(fieldname)      (PRVM_GLOBALFIELDSTRING(prog->globaloffsets.fieldname))
#define PRVM_gameglobaledict(fieldname)       (PRVM_GLOBALFIELDEDICT(prog->globaloffsets.fieldname))
#define PRVM_gameglobalfunction(fieldname)    (PRVM_GLOBALFIELDFUNCTION(prog->globaloffsets.fieldname))
#define PRVM_gamefunction(funcname)           (prog->funcoffsets.funcname)

#define PRVM_serveredictfloat(ed, fieldname)    (PRVM_EDICTFIELDFLOAT(ed, prog->fieldoffsets.fieldname))
#define PRVM_serveredictvector(ed, fieldname)   (PRVM_EDICTFIELDVECTOR(ed, prog->fieldoffsets.fieldname))
#define PRVM_serveredictstring(ed, fieldname)   (PRVM_EDICTFIELDSTRING(ed, prog->fieldoffsets.fieldname))
#define PRVM_serveredictedict(ed, fieldname)    (PRVM_EDICTFIELDEDICT(ed, prog->fieldoffsets.fieldname))
#define PRVM_serveredictfunction(ed, fieldname) (PRVM_EDICTFIELDFUNCTION(ed, prog->fieldoffsets.fieldname))
#define PRVM_serverglobalfloat(fieldname)       (PRVM_GLOBALFIELDFLOAT(prog->globaloffsets.fieldname))
#define PRVM_serverglobalvector(fieldname)      (PRVM_GLOBALFIELDVECTOR(prog->globaloffsets.fieldname))
#define PRVM_serverglobalstring(fieldname)      (PRVM_GLOBALFIELDSTRING(prog->globaloffsets.fieldname))
#define PRVM_serverglobaledict(fieldname)       (PRVM_GLOBALFIELDEDICT(prog->globaloffsets.fieldname))
#define PRVM_serverglobalfunction(fieldname)    (PRVM_GLOBALFIELDFUNCTION(prog->globaloffsets.fieldname))
#define PRVM_serverfunction(funcname)           (prog->funcoffsets.funcname)

#define PRVM_clientedictfloat(ed, fieldname)    (PRVM_EDICTFIELDFLOAT(ed, prog->fieldoffsets.fieldname))
#define PRVM_clientedictvector(ed, fieldname)   (PRVM_EDICTFIELDVECTOR(ed, prog->fieldoffsets.fieldname))
#define PRVM_clientedictstring(ed, fieldname)   (PRVM_EDICTFIELDSTRING(ed, prog->fieldoffsets.fieldname))
#define PRVM_clientedictedict(ed, fieldname)    (PRVM_EDICTFIELDEDICT(ed, prog->fieldoffsets.fieldname))
#define PRVM_clientedictfunction(ed, fieldname) (PRVM_EDICTFIELDFUNCTION(ed, prog->fieldoffsets.fieldname))
#define PRVM_clientglobalfloat(fieldname)       (PRVM_GLOBALFIELDFLOAT(prog->globaloffsets.fieldname))
#define PRVM_clientglobalvector(fieldname)      (PRVM_GLOBALFIELDVECTOR(prog->globaloffsets.fieldname))
#define PRVM_clientglobalstring(fieldname)      (PRVM_GLOBALFIELDSTRING(prog->globaloffsets.fieldname))
#define PRVM_clientglobaledict(fieldname)       (PRVM_GLOBALFIELDEDICT(prog->globaloffsets.fieldname))
#define PRVM_clientglobalfunction(fieldname)    (PRVM_GLOBALFIELDFUNCTION(prog->globaloffsets.fieldname))
#define PRVM_clientfunction(funcname)           (prog->funcoffsets.funcname)

#define PRVM_menuedictfloat(ed, fieldname)    (PRVM_EDICTFIELDFLOAT(ed, prog->fieldoffsets.fieldname))
#define PRVM_menuedictvector(ed, fieldname)   (PRVM_EDICTFIELDVECTOR(ed, prog->fieldoffsets.fieldname))
#define PRVM_menuedictstring(ed, fieldname)   (PRVM_EDICTFIELDSTRING(ed, prog->fieldoffsets.fieldname))
#define PRVM_menuedictedict(ed, fieldname)    (PRVM_EDICTFIELDEDICT(ed, prog->fieldoffsets.fieldname))
#define PRVM_menuedictfunction(ed, fieldname) (PRVM_EDICTFIELDFUNCTION(ed, prog->fieldoffsets.fieldname))
#define PRVM_menuglobalfloat(fieldname)       (PRVM_GLOBALFIELDFLOAT(prog->globaloffsets.fieldname))
#define PRVM_menuglobalvector(fieldname)      (PRVM_GLOBALFIELDVECTOR(prog->globaloffsets.fieldname))
#define PRVM_menuglobalstring(fieldname)      (PRVM_GLOBALFIELDSTRING(prog->globaloffsets.fieldname))
#define PRVM_menuglobaledict(fieldname)       (PRVM_GLOBALFIELDEDICT(prog->globaloffsets.fieldname))
#define PRVM_menuglobalfunction(fieldname)    (PRVM_GLOBALFIELDFUNCTION(prog->globaloffsets.fieldname))
#define PRVM_menufunction(funcname)           (prog->funcoffsets.funcname)

#define PRVM_EDICTFIELDVALUE(ed, fieldoffset) ((prvm_eval_t *)(ed->fields.fp + fieldoffset))
#define PRVM_EDICTFIELDFLOAT(ed, fieldoffset) (((prvm_eval_t *)(ed->fields.fp + fieldoffset))->_float)
#define PRVM_EDICTFIELDVECTOR(ed, fieldoffset) (((prvm_eval_t *)(ed->fields.fp + fieldoffset))->vector)
#define PRVM_EDICTFIELDSTRING(ed, fieldoffset) (((prvm_eval_t *)(ed->fields.fp + fieldoffset))->string)
#define PRVM_EDICTFIELDEDICT(ed, fieldoffset) (((prvm_eval_t *)(ed->fields.fp + fieldoffset))->edict)
#define PRVM_EDICTFIELDFUNCTION(ed, fieldoffset) (((prvm_eval_t *)(ed->fields.fp + fieldoffset))->function)
#define PRVM_GLOBALFIELDVALUE(fieldoffset) ((prvm_eval_t *)(prog->globals.fp + fieldoffset))
#define PRVM_GLOBALFIELDFLOAT(fieldoffset) (((prvm_eval_t *)(prog->globals.fp + fieldoffset))->_float)
#define PRVM_GLOBALFIELDVECTOR(fieldoffset) (((prvm_eval_t *)(prog->globals.fp + fieldoffset))->vector)
#define PRVM_GLOBALFIELDSTRING(fieldoffset) (((prvm_eval_t *)(prog->globals.fp + fieldoffset))->string)
#define PRVM_GLOBALFIELDEDICT(fieldoffset) (((prvm_eval_t *)(prog->globals.fp + fieldoffset))->edict)
#define PRVM_GLOBALFIELDFUNCTION(fieldoffset) (((prvm_eval_t *)(prog->globals.fp + fieldoffset))->function)

//============================================================================
#define PRVM_OP_STATE		1

#ifdef DP_SMALLMEMORY
#define	PRVM_MAX_STACK_DEPTH		128
#define	PRVM_LOCALSTACK_SIZE		2048

#define PRVM_MAX_OPENFILES 16
#define PRVM_MAX_OPENSEARCHES 8
#else
#define	PRVM_MAX_STACK_DEPTH		1024
#define	PRVM_LOCALSTACK_SIZE		16384

#define PRVM_MAX_OPENFILES 256
#define PRVM_MAX_OPENSEARCHES 128
#endif

struct prvm_prog_s;
typedef void (*prvm_builtin_t) (struct prvm_prog_s *prog);

// NOTE: field offsets use -1 for NULL
//typedef struct prvm_prog_fieldoffsets_s
struct prvm_prog_fieldoffsets_t
{
		int SendEntity;
		int SendFlags;
		int Version;
		int absmax;
		int absmin;
		int aiment;
		int alpha;
		int ammo_cells;
		int ammo_cells1;
		int ammo_lava_nails;
		int ammo_multi_rockets;
		int ammo_nails;
		int ammo_nails1;
		int ammo_plasma;
		int ammo_rockets;
		int ammo_rockets1;
		int ammo_shells;
		int ammo_shells1;
		int angles;
		int armortype;
		int armorvalue;
		int avelocity;
		int blocked;
		int bouncefactor;
		int bouncestop;
		int button0;
		int button1;
		int button2;
		int button3;
		int button4;
		int button5;
		int button6;
		int button7;
		int button8;
		int button9;
		int button10;
		int button11;
		int button12;
		int button13;
		int button14;
		int button15;
		int button16;
		int buttonchat;
		int buttonuse;
		int camera_transform;
		int chain;
		int classname;
		int clientcamera;
		int clientcolors;
		int clientstatus;
		int color;
		int colormap;
		int colormod;
		int contentstransition;
		int crypto_encryptmethod;
		int crypto_idfp;
		int crypto_keyfp;
		int crypto_mykeyfp;
		int crypto_signmethod;
		int currentammo;
		int cursor_active;
		int cursor_screen;
		int cursor_trace_endpos;
		int cursor_trace_ent;
		int cursor_trace_start;
		int customizeentityforclient;
		int deadflag;
		int disableclientprediction;
		int discardabledemo;
		int dmg_inflictor;
		int dmg_save;
		int dmg_take;
		int dphitcontentsmask;
		int drawmask;
		int drawonlytoclient;
		int effects;
		int enemy;
		int entnum;
		int exteriormodeltoclient;
		int fixangle;
		int flags;
		int frags;
		int frame;
		int frame1time;
		int frame2;
		int frame2time;
		int frame3;
		int frame3time;
		int frame4;
		int frame4time;
		int fullbright;
		int glow_color;
		int glow_size;
		int glow_trail;
		int glowmod;
		int goalentity;
		int gravity;
		int groundentity;
		int health;
		int ideal_yaw;
		int idealpitch;
		int impulse;
		int items;
		int items2;
		int geomtype;
		int jointtype;
		int forcetype;
		int lerpfrac;
		int lerpfrac3;
		int lerpfrac4;
		int light_lev;
		int ltime;
		int mass;
		int massofs;
		int friction;
		int maxcontacts;
		int erp;
		int max_health;
		int maxs;
		int message;
		int mins;
		int model;
		int modelflags;
		int modelindex;
		int movedir;
		int movement;
		int movetype;
		int movetypesteplandevent;
		int netaddress;
		int netname;
		int nextthink;
		int nodrawtoclient;
		int noise;
		int noise1;
		int noise2;
		int noise3;
		int oldorigin;
		int origin;
		int owner;
		int pflags;
		int ping;
		int ping_movementloss;
		int ping_packetloss;
		int pitch_speed;
		int playermodel;
		int playerskin;
		int pmodel;
		int pmove_flags;
		int predraw;
		int punchangle;
		int punchvector;
		int renderamt;
		int renderflags;
		int scale;
		int modelscale_vec;
		int sendcomplexanimation;
		int shadertime;
		int size;
		int skeletonindex;
		int skin;
		int solid;
		int sounds;
		int spawnflags;
		int style;
		int tag_entity;
		int tag_index;
		int takedamage;
		int target;
		int targetname;
		int team;
		int teleport_time;
		int think;
		int touch;
		int traileffectnum;
		int use;
		int userwavefunc_param0;
		int userwavefunc_param1;
		int userwavefunc_param2;
		int userwavefunc_param3;
		int v_angle;
		int velocity;
		int modellight_ambient;
		int modellight_diffuse;
		int modellight_dir;
		int view_ofs;
		int viewmodelforclient;
		int viewzoom;
		int waterlevel;
		int watertype;
		int weapon;
		int weaponframe;
		int weaponmodel;
		int yaw_speed;

};

// NOTE: global offsets use -1 for NULL
//typedef struct prvm_prog_globaloffsets_s
struct prvm_prog_globaloffsets_t
{
	int SV_InitCmd;
	int clientcommandframe;
	int cltime;
	int coop;
	int deathmatch;
	int dmg_origin;
	int dmg_save;
	int dmg_take;
	int drawfont;
	int drawfontscale;
	int force_retouch;
	int found_secrets;
	int frametime;
	int gettaginfo_forward;
	int gettaginfo_name;
	int gettaginfo_offset;
	int gettaginfo_parent;
	int gettaginfo_right;
	int gettaginfo_up;
	int getlight_ambient;
	int getlight_diffuse;
	int getlight_dir;
	int input_angles;
	int input_buttons;
	int input_movevalues;
	int input_timelength;
	int intermission;
	int killed_monsters;
	int mapname;
	int maxclients;
	int movevar_accelerate;
	int movevar_airaccelerate;
	int movevar_entgravity;
	int movevar_friction;
	int movevar_gravity;
	int movevar_maxspeed;
	int movevar_spectatormaxspeed;
	int movevar_stopspeed;
	int movevar_wateraccelerate;
	int movevar_waterfriction;
	int msg_entity;
	int other;
	int parm1;
	int parm2;
	int parm3;
	int parm4;
	int parm5;
	int parm6;
	int parm7;
	int parm8;
	int parm9;
	int parm10;
	int parm11;
	int parm12;
	int parm13;
	int parm14;
	int parm15;
	int parm16;
	int particle_airfriction;
	int particle_alpha;
	int particle_alphafade;
	int particle_angle;
	int particle_blendmode;
	int particle_bounce;
	int particle_color1;
	int particle_color2;
	int particle_delaycollision;
	int particle_delayspawn;
	int particle_gravity;
	int particle_liquidfriction;
	int particle_orientation;
	int particle_originjitter;
	int particle_qualityreduction;
	int particle_size;
	int particle_sizeincrease;
	int particle_spin;
	int particle_stainalpha;
	int particle_staincolor1;
	int particle_staincolor2;
	int particle_stainsize;
	int particle_staintex;
	int particle_stretch;
	int particle_tex;
	int particle_time;
	int particle_type;
	int particle_velocityjitter;
	int particles_alphamax;
	int particles_alphamin;
	int particles_fade;
	int particles_colormax;
	int particles_colormin;
	int player_localentnum;
	int player_localnum;
	int pmove_inwater;
	int pmove_maxs;
	int pmove_mins;
	int pmove_onground;
	int pmove_waterjumptime;
	int pmove_jump_held;
	int pmove_org;
	int pmove_vel;
	int require_spawnfunc_prefix;
	int sb_showscores;
	int self;
	int servercommandframe;
	int serverdeltatime;
	int serverflags;
	int serverprevtime;
	int servertime;
	int teamplay;
	int time;
	int total_monsters;
	int total_secrets;
	int trace_allsolid;
	int trace_dphitcontents;
	int trace_dphitq3surfaceflags;
	int trace_dphittexturename;
	int trace_dpstartcontents;
	int trace_endpos;
	int trace_ent;
	int trace_fraction;
	int trace_inopen;
	int trace_inwater;
	int trace_networkentity;
	int trace_plane_dist;
	int trace_plane_normal;
	int trace_startsolid;
	int transparent_offset;
	int v_forward;
	int v_right;
	int v_up;
	int view_angles;
	int view_punchangle;
	int view_punchvector;
	int world;
	int worldstatus;
	int sound_starttime;
};

// NOTE: function offsets use 0 for NULL
struct prvm_prog_funcoffsets_t
{
	int CSQC_ConsoleCommand;
	int CSQC_Ent_Remove;
	int CSQC_Ent_Spawn;
	int CSQC_Ent_Update;
	int CSQC_Event;
	int CSQC_Event_Sound;
	int CSQC_Init;
	int CSQC_InputEvent;
	int CSQC_Parse_CenterPrint;
	int CSQC_Parse_Print;
	int CSQC_Parse_StuffCmd;
	int CSQC_Parse_TempEntity;
	int CSQC_Shutdown;
	int CSQC_UpdateView;
	int ClientConnect;
	int ClientDisconnect;
	int ClientKill;
	int EndFrame;
	int GameCommand;
	int PlayerPostThink;
	int PlayerPreThink;
	int PutClientInServer;
	int RestoreGame;
	int SV_ChangeTeam;
	int SV_OnEntityNoSpawnFunction;
	int SV_OnEntityPostSpawnFunction;
	int SV_OnEntityPreSpawnFunction;
	int SV_ParseClientCommand;
	int SV_PausedTic;
	int SV_PlayerPhysics;
	int SV_Shutdown;
	int SetChangeParms;
	int SetNewParms;
	int StartFrame;
	int URI_Get_Callback;
	int m_draw;
	int m_init;
	int m_keydown;
	int m_keyup;
	int m_newmap;
	int m_gethostcachecategory;
	int m_shutdown;
	int m_toggle;
	int main;
};

// stringbuffer flags
#define STRINGBUFFER_SAVED     1   // saved in savegames
#define STRINGBUFFER_QCFLAGS   1   // allowed to be set by QC
#define STRINGBUFFER_TEMP      128 // internal use ONLY 
typedef struct prvm_stringbuffer_s
{
	int max_strings;
	int num_strings;
	char **strings;
	const char *origin;
	unsigned char flags;
}
prvm_stringbuffer_t;

// [INIT] variables flagged with this token can be initialized by 'you'
// NOTE: external code has to create and free the mempools but everything else is done by prvm !
typedef struct prvm_prog_s
{
	double				starttime; // system time when PRVM_Prog_Load was called
	double				profiletime; // system time when last PRVM_CallProfile was called (or PRVM_Prog_Load initially)
	unsigned int		id; // increasing unique id of progs instance
	mfunction_t			*functions;
	int				functions_covered;
	char				*strings;
	int					stringssize;
	ddef_t				*fielddefs;
	ddef_t				*globaldefs;
	mstatement_t		*statements;
	int					entityfields;			// number of vec_t fields in progs (some variables are 3)
	int					entityfieldsarea;		// LordHavoc: equal to max_edicts * entityfields (for bounds checking)

	// loaded values from the disk format
	int					progs_version;
	int					progs_crc;
	int					progs_numstatements;
	int					progs_numglobaldefs;
	int					progs_numfielddefs;
	int					progs_numfunctions;
	int					progs_numstrings;
	int					progs_numglobals;
	int					progs_entityfields;

	// real values in memory (some modified by loader)
	int					numstatements;
	int					numglobaldefs;
	int					numfielddefs;
	int					numfunctions;
	int					numstrings;
	int					numglobals;

	int					*statement_linenums; // NULL if not available
	int					*statement_columnnums; // NULL if not available

	double				*statement_profile; // only incremented if prvm_statementprofiling is on
	int				statements_covered;
	double				*explicit_profile; // only incremented if prvm_statementprofiling is on
	int				explicit_covered;
	int				numexplicitcoveragestatements;

	union {
		prvm_vec_t *fp;
		prvm_int_t *ip;
	} globals;

	int					maxknownstrings;
	int					numknownstrings;
	// this is updated whenever a string is removed or added
	// (simple optimization of the free string search)
	int					firstfreeknownstring;
	const char			**knownstrings;
	unsigned char		*knownstrings_freeable;
	const char          **knownstrings_origin;
	const char			***stringshash;

	memexpandablearray_t	stringbuffersarray;

	// all memory allocations related to this vm_prog (code, edicts, strings)
	mempool_t			*progs_mempool; // [INIT]

	prvm_builtin_t		*builtins; // [INIT]
	int					numbuiltins; // [INIT]

	int					argc;

	int					trace;
	int					break_statement;
	int					break_stack_index;
	int					watch_global;
	etype_t					watch_global_type;
	prvm_eval_t				watch_global_value;
	int					watch_edict;
	int					watch_field;
	etype_t					watch_field_type;
	prvm_eval_t				watch_edictfield_value;

	mfunction_t			*xfunction;
	int					xstatement;

	// stacktrace writes into stack[MAX_STACK_DEPTH]
	// thus increase the array, so depth wont be overwritten
	prvm_stack_t		stack[PRVM_MAX_STACK_DEPTH+1];
	int					depth;

	prvm_int_t			localstack[PRVM_LOCALSTACK_SIZE];
	int					localstack_used;

	unsigned short		filecrc;

	//============================================================================
	// until this point everything also exists (with the pr_ prefix) in the old vm

	qfile_t				*openfiles[PRVM_MAX_OPENFILES];
	const char *         openfiles_origin[PRVM_MAX_OPENFILES];
	fssearch_t			*opensearches[PRVM_MAX_OPENSEARCHES];
	const char *         opensearches_origin[PRVM_MAX_OPENSEARCHES];
	skeleton_t			*skeletons[MAX_EDICTS];

	// buffer for storing all tempstrings created during one invocation of ExecuteProgram
	sizebuf_t			tempstringsbuf;

	// LordHavoc: moved this here to clean up things that relied on prvm_prog_list too much
	// FIXME: make VM_CL_R_Polygon functions use Debug_Polygon functions?
	vmpolygons_t		vmpolygons;

	// copies of some vars that were former read from sv
	int					num_edicts;
	// number of edicts for which space has been (should be) allocated
	int					max_edicts; // [INIT]
	// used instead of the constant MAX_EDICTS
	int					limit_edicts; // [INIT]

	// number of reserved edicts (allocated from 1)
	int					reserved_edicts; // [INIT]

	prvm_edict_t		*edicts;
	prvm_vec_t		*edictsfields;
	void				*edictprivate;

	// size of the engine private struct
	#if !EDICTPRIVATE_CONSTEXPR
		int					edictprivate_size; // [INIT]
	#else
		static constexpr int edictprivate_size = sizeof(edict_engineprivate_t);
	#endif
	prvm_prog_fieldoffsets_t	fieldoffsets;
	prvm_prog_globaloffsets_t	globaloffsets;
	prvm_prog_funcoffsets_t	funcoffsets;

	// allow writing to world entity fields, this is set by server init and
	// cleared before first server frame
	bool			allowworldwrites;

	// name of the prog, e.g. "Server", "Client" or "Menu" (used for text output)
	const char			*name; // [INIT]

	// flag - used to store general flags like PRVM_GE_SELF, etc.
	int				flag;

	const char			*extensionstring; // [INIT]

	bool			loadintoworld; // [INIT]

	// used to indicate whether a prog is loaded
	bool			loaded;
	bool			leaktest_active;

	// translation buffer (only needs to be freed on unloading progs, type is private to prvm_edict.c)
	void *po;

	// printed together with backtraces
	const char *statestring;


	//============================================================================

	ddef_t				*self; // if self != 0 then there is a global self

	//============================================================================
	// function pointers

	void				(*begin_increase_edicts)(struct prvm_prog_s *prog); // [INIT] used by PRVM_MEM_Increase_Edicts
	void				(*end_increase_edicts)(struct prvm_prog_s *prog); // [INIT]

	void				(*init_edict)(struct prvm_prog_s *prog, prvm_edict_t *edict); // [INIT] used by PRVM_ED_ClearEdict
	void				(*free_edict)(struct prvm_prog_s *prog, prvm_edict_t *ed); // [INIT] used by PRVM_ED_Free

	void				(*count_edicts)(struct prvm_prog_s *prog); // [INIT] used by PRVM_ED_Count_f

	bool			(*load_edict)(struct prvm_prog_s *prog, prvm_edict_t *ent); // [INIT] used by PRVM_ED_LoadFromFile

	void				(*init_cmd)(struct prvm_prog_s *prog); // [INIT] used by PRVM_InitProg
	void				(*reset_cmd)(struct prvm_prog_s *prog); // [INIT] used by PRVM_ResetProg

	void				(*error_cmd)(const char *format, ...) DP_FUNC_PRINTF(1); // [INIT]

	void				(*ExecuteProgram)(struct prvm_prog_s *prog, func_t fnum, const char *errormessage); // pointer to one of the *VM_ExecuteProgram functions
	
	/**
		function pointer wrappers
	*/
	inline void beginIncreaseEdicts()
	{
		begin_increase_edicts(this);
	}
	
	inline void endIncreaseEdicts()
	{
		end_increase_edicts(this);
	}
	
	inline void initEdict(prvm_edict_t* edict)
	{
		init_edict(this, edict);
	}
	
	inline void freeEdict(prvm_edict_t* edict)
	{
		free_edict(this, edict);
	}
	
	inline void countEdicts()
	{
		count_edicts(this);
	}
	
	inline bool loadEdict(prvm_edict_t* edict)
	{
		return load_edict(this, edict);
	}
	
	inline void initCmd()
	{
		init_cmd(this);
	}
	
	inline void resetCmd()
	{
		reset_cmd(this);
	}

	inline void executeProgram(func_t fnum, const char* errMsg)
	{
		ExecuteProgram(this, fnum, errMsg);
	}
	/**
		helper functions
	*/
	inline __pseudopure bool isWorld(const prvm_edict_t* const edict) const
	{
		return edict == edicts;
	}
} prvm_prog_t;



typedef enum prvm_progindex_e
{
	PRVM_PROG_SERVER,
	PRVM_PROG_CLIENT,
	PRVM_PROG_MENU,
	PRVM_PROG_MAX
}
prvm_progindex_t;

extern prvm_prog_t prvm_prog_list[PRVM_PROG_MAX];
prvm_prog_t *PRVM_ProgFromString(const char *str);
prvm_prog_t *PRVM_FriendlyProgFromString(const char *str); // for console commands (prints error if name unknown and returns NULL, prints error if prog not loaded and returns NULL)
#define PRVM_GetProg(n) (&prvm_prog_list[(n)])
#define PRVM_ProgLoaded(n) (PRVM_GetProg(n)->loaded)
#define SVVM_prog (&prvm_prog_list[PRVM_PROG_SERVER])
#define CLVM_prog (&prvm_prog_list[PRVM_PROG_CLIENT])
#ifdef CONFIG_MENU
#define MVM_prog (&prvm_prog_list[PRVM_PROG_MENU])
#endif

//============================================================================
// prvm_cmds part

extern prvm_builtin_t vm_sv_builtins[];
extern prvm_builtin_t vm_cl_builtins[];
extern prvm_builtin_t vm_m_builtins[];

extern const int vm_sv_numbuiltins;
extern const int vm_cl_numbuiltins;
extern const int vm_m_numbuiltins;

extern const char * vm_sv_extensions; // client also uses this
extern const char * vm_m_extensions;

void SVVM_init_cmd(prvm_prog_t *prog);
void SVVM_reset_cmd(prvm_prog_t *prog);

void CLVM_init_cmd(prvm_prog_t *prog);
void CLVM_reset_cmd(prvm_prog_t *prog);

#ifdef CONFIG_MENU
void MVM_init_cmd(prvm_prog_t *prog);
void MVM_reset_cmd(prvm_prog_t *prog);
#endif

void VM_Cmd_Init(prvm_prog_t *prog);
void VM_Cmd_Reset(prvm_prog_t *prog);
//============================================================================

void PRVM_Init ();

#ifdef PROFILING
void SVVM_ExecuteProgram (prvm_prog_t *prog, func_t fnum, const char *errormessage);
void CLVM_ExecuteProgram (prvm_prog_t *prog, func_t fnum, const char *errormessage);
#ifdef CONFIG_MENU
void MVM_ExecuteProgram (prvm_prog_t *prog, func_t fnum, const char *errormessage);
#endif
#else
#define SVVM_ExecuteProgram PRVM_ExecuteProgram
#define CLVM_ExecuteProgram PRVM_ExecuteProgram
#ifdef CONFIG_MENU
#define MVM_ExecuteProgram PRVM_ExecuteProgram
#endif
void PRVM_ExecuteProgram (prvm_prog_t *prog, func_t fnum, const char *errormessage);
#endif

#define PRVM_Alloc(buffersize) Mem_Alloc(prog->progs_mempool, buffersize)
#define PRVM_Free(buffer) Mem_Free(buffer)

void PRVM_Profile (prvm_prog_t *prog, int maxfunctions, double mintime, int sortby);
void PRVM_Profile_f ();
void PRVM_ChildProfile_f ();
void PRVM_CallProfile_f ();
void PRVM_PrintFunction_f ();

void PRVM_PrintState(prvm_prog_t *prog, int stack_index);
void PRVM_Crash(prvm_prog_t *prog);
void PRVM_ShortStackTrace(prvm_prog_t *prog, char *buf, size_t bufsize);
const char *PRVM_AllocationOrigin(prvm_prog_t *prog);

ddef_t *PRVM_ED_FindField(prvm_prog_t *prog, const char *name);
ddef_t *PRVM_ED_FindGlobal(prvm_prog_t *prog, const char *name);
mfunction_t *PRVM_ED_FindFunction(prvm_prog_t *prog, const char *name);

int PRVM_ED_FindFieldOffset(prvm_prog_t *prog, const char *name);
int PRVM_ED_FindGlobalOffset(prvm_prog_t *prog, const char *name);
func_t PRVM_ED_FindFunctionOffset(prvm_prog_t *prog, const char *name);

#if 1
//!defined(__offsetOf)
	#define PRVM_ED_FindFieldOffset_FromStruct(st, field) prog->fieldoffsets . field = ((int *)(&((st *)NULL)-> field ) - ((int *)NULL))
	#define PRVM_ED_FindGlobalOffset_FromStruct(st, field) prog->globaloffsets . field = ((int *)(&((st *)NULL)-> field ) - ((int *)NULL))
#else
	#define PRVM_ED_FindFieldOffset_FromStruct(st, field)	prog->fieldoffsets.field	= __offsetOf(st, field)
	#define PRVM_ED_FindGlobalOffset_FromStruct(st, field)	prog->globaloffsets.field	= __offsetOf(st, field)
#endif

void PRVM_MEM_IncreaseEdicts(prvm_prog_t *prog);

bool PRVM_ED_CanAlloc(prvm_prog_t *prog, prvm_edict_t *e);
prvm_edict_t *PRVM_ED_Alloc(prvm_prog_t *prog);
void PRVM_ED_Free(prvm_prog_t *prog, prvm_edict_t *ed);
void PRVM_ED_ClearEdict(prvm_prog_t *prog, prvm_edict_t *e);

void PRVM_PrintFunctionStatements(prvm_prog_t *prog, const char *name);
void PRVM_ED_Print(prvm_prog_t *prog, prvm_edict_t *ed, const char *wildcard_fieldname);
void PRVM_ED_Write(prvm_prog_t *prog, qfile_t *f, prvm_edict_t *ed);
const char *PRVM_ED_ParseEdict(prvm_prog_t *prog, const char *data, prvm_edict_t *ent);

void PRVM_ED_WriteGlobals(prvm_prog_t *prog, qfile_t *f);
void PRVM_ED_ParseGlobals(prvm_prog_t *prog, const char *data);

void PRVM_ED_LoadFromFile(prvm_prog_t *prog, const char *data);

unsigned int PRVM_EDICT_NUM_ERROR(prvm_prog_t *prog, unsigned int n, const char *filename, int fileline);

#define	PRVM_EDICT(n) (((unsigned)(n) < (unsigned int)prog->max_edicts) ? (unsigned int)(n) : PRVM_EDICT_NUM_ERROR(prog, (unsigned int)(n), __FILE__, __LINE__))
#define	PRVM_EDICT_NUM(n) (prog->edicts + static_cast<size_t>(n))//PRVM_EDICT(n))

//int NUM_FOR_EDICT_ERROR(prvm_edict_t *e);
#define PRVM_NUM_FOR_EDICT(e) ((int)((prvm_edict_t *)(e) - prog->edicts))
//int PRVM_NUM_FOR_EDICT(prvm_edict_t *e);

#define	PRVM_NEXT_EDICT(e) ((e) + 1)

#define PRVM_EDICT_TO_PROG(e) (PRVM_NUM_FOR_EDICT(e))
//int PRVM_EDICT_TO_PROG(prvm_edict_t *e);
#define PRVM_PROG_TO_EDICT(n) (PRVM_EDICT_NUM(n))
//prvm_edict_t *PRVM_PROG_TO_EDICT(int n);

//============================================================================

#define	PRVM_G_FLOAT(o) (prog->globals.fp[o])
#define	PRVM_G_INT(o) (prog->globals.ip[o])
#define	PRVM_G_EDICT(o) (PRVM_PROG_TO_EDICT(prog->globals.ip[o]))
#define PRVM_G_EDICTNUM(o) PRVM_NUM_FOR_EDICT(PRVM_G_EDICT(o))
#define	PRVM_G_VECTOR(o) (&prog->globals.fp[o])
#define	PRVM_G_STRING(o) (PRVM_GetString(prog, prog->globals.ip[o]))
//#define	PRVM_G_FUNCTION(prog, o) (prog->globals.ip[o])

// FIXME: make these go away?
#define	PRVM_E_FLOAT(e,o) (e->fields.fp[o])
#define	PRVM_E_INT(e,o) (e->fields.ip[o])
//#define	PRVM_E_VECTOR(e,o) (&(e->fields.fp[o]))
#define	PRVM_E_STRING(e,o) (PRVM_GetString(prog, e->fields.ip[o]))

extern	int		prvm_type_size[8]; // for consistency : I think a goal of this sub-project is to
// make the new vm mostly independent from the old one, thus if it's necessary, I copy everything

void PRVM_Init_Exec(prvm_prog_t *prog);

void PRVM_ED_PrintEdicts_f ();
void PRVM_ED_PrintNum (prvm_prog_t *prog, int ent, const char *wildcard_fieldname);

const char *PRVM_GetString(prvm_prog_t *prog, int num);
int PRVM_SetEngineString(prvm_prog_t *prog, const char *s);
const char *PRVM_ChangeEngineString(prvm_prog_t *prog, int i, const char *s);
int PRVM_SetTempString(prvm_prog_t *prog, const char *s);
int PRVM_AllocString(prvm_prog_t *prog, size_t bufferlength, char **pointer);
void PRVM_FreeString(prvm_prog_t *prog, int num);

ddef_t *PRVM_ED_FieldAtOfs(prvm_prog_t *prog, int ofs);
bool PRVM_ED_ParseEpair(prvm_prog_t *prog, prvm_edict_t *ent, ddef_t *key, const char *s, bool parsebackslash);
char *PRVM_UglyValueString(prvm_prog_t *prog, etype_t type, prvm_eval_t *val, char *line, size_t linelength);
char *PRVM_GlobalString(prvm_prog_t *prog, int ofs, char *line, size_t linelength);
char *PRVM_GlobalStringNoContents(prvm_prog_t *prog, int ofs, char *line, size_t linelength);

//============================================================================

/*
Initializing a vm:
Call InitProg with the num
Set up the fields marked with [INIT] in the prog struct
Load a program with LoadProgs
*/
// Load expects to be called right after Reset
void PRVM_Prog_Init(prvm_prog_t *prog);
void PRVM_Prog_Load(prvm_prog_t *prog, const char *filename, unsigned char *data, fs_offset_t size, int numrequiredfunc, const char **required_func, int numrequiredfields, prvm_required_field_t *required_field, int numrequiredglobals, prvm_required_field_t *required_global);
void PRVM_Prog_Reset(prvm_prog_t *prog);

void PRVM_StackTrace(prvm_prog_t *prog);
void PRVM_Breakpoint(prvm_prog_t *prog, int stack_index, const char *text);
void PRVM_Watchpoint(prvm_prog_t *prog, int stack_index, const char *text, etype_t type, prvm_eval_t *o, prvm_eval_t *n);

void VM_Warning(prvm_prog_t *prog, const char *fmt, ...) DP_FUNC_PRINTF(2);

void VM_GenerateFrameGroupBlend(prvm_prog_t *prog, framegroupblend_t *framegroupblend, const prvm_edict_t *ed);
void VM_FrameBlendFromFrameGroupBlend(frameblend_t *frameblend, const framegroupblend_t *framegroupblend, const dp_model_t *model, double curtime);
void VM_UpdateEdictSkeleton(prvm_prog_t *prog, prvm_edict_t *ed, const dp_model_t *edmodel, const frameblend_t *frameblend);
void VM_RemoveEdictSkeleton(prvm_prog_t *prog, prvm_edict_t *ed);

void PRVM_ExplicitCoverageEvent(prvm_prog_t *prog, mfunction_t *func, int statement);


namespace cloture	{
namespace vm		{

#if 0
#define		PROGRAM_INLINE	inline
class Program
{
	prvm_prog_t* ptr;
public:

	constexpr PROGRAM_INLINE Program(prvm_prog_t* p) : ptr(p)
	{}

	constexpr PROGRAM_INLINE prvm_prog_t* operator ->()
	{
		return ptr;
	}

	__pseudopure constexpr PROGRAM_INLINE bool operator ==(const prvm_prog_t* const other) const
	{
		return ptr == other;
	}

	__pseudopure constexpr PROGRAM_INLINE bool operator !=(const prvm_prog_t* const other) const
	{
		return ptr != other;
	}


	__pseudopure constexpr PROGRAM_INLINE explicit operator bool() const
	{
		return ptr != nullptr;
	}

	__pseudopure constexpr PROGRAM_INLINE bool operator !() const
	{
		return ptr == nullptr;
	}

	__pseudopure constexpr PROGRAM_INLINE explicit operator prvm_prog_t*()
	{
		return ptr;
	}

	__pseudopure constexpr PROGRAM_INLINE prvm_prog_t* getPtr()
	{
		return ptr;
	}

};//class Program
#else

using util::pointers::wrapped_ptr;

class Program : public wrapped_ptr<prvm_prog_t>
{
public:
	using wrapped_ptr::wrapped_ptr;
};

#endif

}//namespace vm
}//namespace cloture
