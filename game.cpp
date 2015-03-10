#include "quakedef.h"

#include "util/common.hpp"

#ifdef BUILD_CLOTURE_GAME
#include "game.hpp"

using namespace cloture;
/*	import basic type definitions into the namespace	*/
using namespace common;
/*	import game type definitions into the namespace		*/
using namespace game::types;


void game::client::init()
{
	prvm_prog_t *prog = CLVM_prog;
	const char* csprogsfn = nullptr;
	unsigned char *csprogsdata = nullptr;
	fs_offset_t csprogsdatasize = 0;
	int csprogsdatacrc, requiredcrc;
	int requiredsize;
	char vabuf[1024];

	// reset csqc_progcrc after reading it, so that changing servers doesn't
	// expect csqc on the next server
	requiredcrc = csqc_progcrc.integer;
	requiredsize = csqc_progsize.integer;
	Cvar_SetValueQuick(&csqc_progcrc, -1);
	Cvar_SetValueQuick(&csqc_progsize, -1);

	// if the server is not requesting a csprogs, then we're done here
	if (requiredcrc < 0)
		return;

	// see if the requested csprogs.dat file matches the requested crc
	if (!cls.demoplayback || csqc_usedemoprogs.integer)
	{
		csprogsfn = va(vabuf, sizeof(vabuf), "dlcache/%s.%i.%i", csqc_progname.string, requiredsize, requiredcrc);
		if(cls.caughtcsprogsdata && cls.caughtcsprogsdatasize == requiredsize && CRC_Block(cls.caughtcsprogsdata, (size_t)cls.caughtcsprogsdatasize) == requiredcrc)
		{
			Con_DPrintf("Using buffered \"%s\"\n", csprogsfn);
			csprogsdata = cls.caughtcsprogsdata;
			csprogsdatasize = cls.caughtcsprogsdatasize;
			cls.caughtcsprogsdata = nullptr;
			cls.caughtcsprogsdatasize = 0;
		}
		else
		{
			Con_DPrintf("Not using buffered \"%s\" (buffered: %p, %d)\n", csprogsfn, cls.caughtcsprogsdata, (int) cls.caughtcsprogsdatasize);
			csprogsdata = FS_LoadFile(csprogsfn, tempmempool, true, &csprogsdatasize);
		}
	}
	if (!csprogsdata)
	{
		csprogsfn = csqc_progname.string;
		csprogsdata = FS_LoadFile(csprogsfn, tempmempool, true, &csprogsdatasize);
	}
	if (csprogsdata)
	{
		csprogsdatacrc = CRC_Block(csprogsdata, (size_t)csprogsdatasize);
		if (csprogsdatacrc != requiredcrc || csprogsdatasize != requiredsize)
		{
			if (cls.demoplayback)
			{
				Con_Printf("^1Warning: Your %s is not the same version as the demo was recorded with (CRC/size are %i/%i but should be %i/%i)\n", csqc_progname.string, csprogsdatacrc, (int)csprogsdatasize, requiredcrc, requiredsize);
				// We WANT to continue here, and play the demo with different csprogs!
				// After all, this is just a warning. Sure things may go wrong from here.
			}
			else
			{
				Mem_Free(csprogsdata);
				Con_Printf("^1Your %s is not the same version as the server (CRC is %i/%i but should be %i/%i)\n", csqc_progname.string, csprogsdatacrc, (int)csprogsdatasize, requiredcrc, requiredsize);
				CL_Disconnect();
				return;
			}
		}
	}
	else
	{
		if (requiredcrc >= 0)
		{
			if (cls.demoplayback)
				Con_Printf("CL_VM_Init: demo requires CSQC, but \"%s\" wasn't found\n", csqc_progname.string);
			else
				Con_Printf("CL_VM_Init: server requires CSQC, but \"%s\" wasn't found\n", csqc_progname.string);
			CL_Disconnect();
		}
		return;
	}

	PRVM_Prog_Init(prog);

	// allocate the mempools
	prog->progs_mempool = Mem_AllocPool(csqc_progname.string, 0, nullptr);
	prog->edictprivate_size = 0; // no private struct used
	prog->name = "client";
	prog->num_edicts = 1;
	prog->max_edicts = 512;
	prog->limit_edicts = CL_MAX_EDICTS;
	prog->reserved_edicts = 0;
	prog->edictprivate_size = sizeof(edict_engineprivate_t);
	// TODO: add a shared extension string #define and add real support for csqc extension strings [12/5/2007 Black]
	prog->extensionstring = vm_sv_extensions;
	prog->builtins = vm_cl_builtins;
	prog->numbuiltins = vm_cl_numbuiltins;

	// all callbacks must be defined (pointers are not checked before calling)
	prog->begin_increase_edicts = CLVM_begin_increase_edicts;
	prog->end_increase_edicts   = CLVM_end_increase_edicts;
	prog->init_edict            = CLVM_init_edict;
	prog->free_edict            = CLVM_free_edict;
	prog->count_edicts          = CLVM_count_edicts;
	prog->load_edict            = CLVM_load_edict;
	prog->init_cmd              = CLVM_init_cmd;
	prog->reset_cmd             = CLVM_reset_cmd;
	prog->error_cmd             = Host_Error;
	prog->ExecuteProgram        = CLVM_ExecuteProgram;

	PRVM_Prog_Load(prog, csprogsfn, csprogsdata, csprogsdatasize, cl_numrequiredfunc, cl_required_func, CL_REQFIELDS, cl_reqfields, CL_REQGLOBALS, cl_reqglobals);

	if (!prog->loaded)
	{
		Host_Error("CSQC %s ^2failed to load\n", csprogsfn);
		if(!sv.active)
			CL_Disconnect();
		Mem_Free(csprogsdata);
		return;
	}

	Con_DPrintf("CSQC %s ^5loaded (crc=%i, size=%i)\n", csprogsfn, csprogsdatacrc, (int)csprogsdatasize);

	if(cls.demorecording)
	{
		if(cls.demo_lastcsprogssize != csprogsdatasize || cls.demo_lastcsprogscrc != csprogsdatacrc)
		{
			int i;
			static char buf[NET_MAXMESSAGE];
			sizebuf_t sb;
			unsigned char *demobuf; fs_offset_t demofilesize;

			sb.data = (unsigned char *) buf;
			sb.maxsize = sizeof(buf);
			i = 0;

			CL_CutDemo(&demobuf, &demofilesize);
			while(MakeDownloadPacket(csqc_progname.string, csprogsdata, (size_t)csprogsdatasize, csprogsdatacrc, i++, &sb, cls.protocol))
				CL_WriteDemoMessage(&sb);
			CL_PasteDemo(&demobuf, &demofilesize);

			cls.demo_lastcsprogssize = csprogsdatasize;
			cls.demo_lastcsprogscrc = csprogsdatacrc;
		}
	}
	Mem_Free(csprogsdata);

	// check if OP_STATE animation is possible in this dat file
	if (prog->fieldoffsets.nextthink >= 0 && prog->fieldoffsets.frame >= 0 && prog->fieldoffsets.think >= 0 && prog->globaloffsets.self >= 0)
		prog->flag |= PRVM_OP_STATE;

	// set time
	PRVM_clientglobalfloat(time) = cl.time;
	PRVM_clientglobaledict(self) = 0;

	PRVM_clientglobalstring(mapname) = PRVM_SetEngineString(prog, cl.worldname);
	PRVM_clientglobalfloat(player_localnum) = cl.realplayerentity - 1;
	PRVM_clientglobalfloat(player_localentnum) = cl.viewentity;

	// set map description (use world entity 0)
	PRVM_clientedictstring(prog->edicts, message) = PRVM_SetEngineString(prog, cl.worldmessage);
	VectorCopy(cl.world.mins, PRVM_clientedictvector(prog->edicts, mins));
	VectorCopy(cl.world.maxs, PRVM_clientedictvector(prog->edicts, maxs));
	VectorCopy(cl.world.mins, PRVM_clientedictvector(prog->edicts, absmin));
	VectorCopy(cl.world.maxs, PRVM_clientedictvector(prog->edicts, absmax));

	// call the prog init
	prog->ExecuteProgram(prog, PRVM_clientfunction(CSQC_Init), "QC function CSQC_Init is missing");

	cl.csqc_loaded = true;

	cl.csqc_vidvars.drawcrosshair = false;
	cl.csqc_vidvars.drawenginesbar = false;

	// Update Coop and Deathmatch Globals (at this point the client knows them from ServerInfo)
	CL_VM_UpdateCoopDeathmatchGlobals(cl.gametype);
}

void game::server::init()
{
	Program* program = SVVM_prog;
	
	if(program->loaded)
	{
		program->resetCmd();
		Mem_FreePool(&program->progs_mempool);
	}
	memset(program, 0, sizeof(Program));
	program->break_statement = -1;
	program->watch_global_type = ev_void;
	program->watch_field_type = ev_void;
	
	program->progs_mempool = Mem_AllocPool("Server Progs", 0, nullptr);
	program->max_edicts = 512;
	program->limit_edicts = MAX_EDICTS;
	program->reserved_edicts = svc.maxclients;
	program->edictprivate_size = sizeof(edict_engineprivate_t);
	program->name = "server";
	program->extensionstring = vm_sv_extensions;
	program->loadintoworld = true;
	
	#if 0
	prvm_prog_t *prog = SVVM_prog;
	PRVM_Prog_Init(prog);

	// allocate the mempools
	// TODO: move the magic numbers/constants into #defines [9/13/2006 Black]
	prog->progs_mempool = Mem_AllocPool("Server Progs", 0, nullptr);
	prog->builtins = vm_sv_builtins;
	prog->numbuiltins = vm_sv_numbuiltins;
	prog->max_edicts = 512;
	if (sv.protocol == PROTOCOL_QUAKE)
		prog->limit_edicts = 640; // before quake mission pack 1 this was 512
	else if (sv.protocol == PROTOCOL_QUAKEDP)
		prog->limit_edicts = 2048; // guessing
	else if (sv.protocol == PROTOCOL_NEHAHRAMOVIE)
		prog->limit_edicts = 2048; // guessing!
	else if (sv.protocol == PROTOCOL_NEHAHRABJP || sv.protocol == PROTOCOL_NEHAHRABJP2 || sv.protocol == PROTOCOL_NEHAHRABJP3)
		prog->limit_edicts = 4096; // guessing!
	else
		prog->limit_edicts = MAX_EDICTS;
	prog->reserved_edicts = svs.maxclients;
	prog->edictprivate_size = sizeof(edict_engineprivate_t);
	prog->name = "server";
	prog->extensionstring = vm_sv_extensions;
	prog->loadintoworld = true;

	// all callbacks must be defined (pointers are not checked before calling)
	prog->begin_increase_edicts = SVVM_begin_increase_edicts;
	prog->end_increase_edicts   = SVVM_end_increase_edicts;
	prog->init_edict            = SVVM_init_edict;
	prog->free_edict            = SVVM_free_edict;
	prog->count_edicts          = SVVM_count_edicts;
	prog->load_edict            = SVVM_load_edict;
	prog->init_cmd              = SVVM_init_cmd;
	prog->reset_cmd             = SVVM_reset_cmd;
	prog->error_cmd             = Host_Error;
	prog->ExecuteProgram        = SVVM_ExecuteProgram;

	PRVM_Prog_Load(prog, sv_progs.string, nullptr, 0, SV_REQFUNCS, sv_reqfuncs, SV_REQFIELDS, sv_reqfields, SV_REQGLOBALS, sv_reqglobals);

	// some mods compiled with scrambling compilers lack certain critical
	// global names and field names such as "self" and "time" and "nextthink"
	// so we have to set these offsets manually, matching the entvars_t
	// but we only do this if the prog header crc matches, otherwise it's totally freeform
	if (prog->progs_crc == PROGHEADER_CRC || prog->progs_crc == PROGHEADER_CRC_TENEBRAE)
	{
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, modelindex);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, absmin);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, absmax);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, ltime);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, movetype);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, solid);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, origin);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, oldorigin);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, velocity);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, angles);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, avelocity);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, punchangle);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, classname);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, model);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, frame);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, skin);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, effects);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, mins);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, maxs);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, size);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, touch);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, use);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, think);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, blocked);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, nextthink);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, groundentity);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, health);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, frags);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, weapon);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, weaponmodel);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, weaponframe);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, currentammo);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, ammo_shells);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, ammo_nails);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, ammo_rockets);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, ammo_cells);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, items);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, takedamage);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, chain);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, deadflag);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, view_ofs);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, button0);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, button1);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, button2);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, impulse);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, fixangle);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, v_angle);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, idealpitch);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, netname);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, enemy);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, flags);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, colormap);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, team);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, max_health);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, teleport_time);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, armortype);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, armorvalue);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, waterlevel);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, watertype);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, ideal_yaw);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, yaw_speed);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, aiment);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, goalentity);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, spawnflags);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, target);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, targetname);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, dmg_take);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, dmg_save);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, dmg_inflictor);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, owner);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, movedir);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, message);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, sounds);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, noise);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, noise1);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, noise2);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, noise3);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, self);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, other);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, world);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, time);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, frametime);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, force_retouch);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, mapname);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, deathmatch);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, coop);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, teamplay);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, serverflags);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, total_secrets);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, total_monsters);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, found_secrets);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, killed_monsters);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, parm1);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, parm2);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, parm3);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, parm4);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, parm5);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, parm6);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, parm7);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, parm8);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, parm9);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, parm10);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, parm11);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, parm12);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, parm13);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, parm14);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, parm15);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, parm16);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, v_forward);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, v_up);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, v_right);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, trace_allsolid);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, trace_startsolid);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, trace_fraction);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, trace_endpos);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, trace_plane_normal);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, trace_plane_dist);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, trace_ent);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, trace_inopen);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, trace_inwater);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, msg_entity);

	}
	else
		Con_DPrintf("%s: %s system vars have been modified (CRC %i != engine %i), will not load in other engines", prog->name, sv_progs.string, prog->progs_crc, PROGHEADER_CRC);

	// OP_STATE is always supported on server because we add fields/globals for it
	prog->flag |= PRVM_OP_STATE;

	VM_CustomStats_Clear();//[515]: csqc

	SV_Prepare_CSQC();
	#endif
}

#endif //BUILD_CLOTURE_GAME
