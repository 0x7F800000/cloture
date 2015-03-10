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
// world.c -- world query functions

#include "quakedef.h"
#include "clvm_cmds.h"
#include "cl_collision.h"

#include "util/common.hpp"
using namespace cloture::util::common;
using namespace cloture::engine::world;

union
{
	int i;
	float f;
} inf = {.i = 0x7F800000};

/*

entities never clip against themselves, or their owner

line of sight checks trace->inopen and trace->inwater, but bullets don't

*/

static void World_Physics_Init();
void World_Init()
{
	Collision_Init();
	World_Physics_Init();
}

static void World_Physics_Shutdown();
void World_Shutdown()
{
	World_Physics_Shutdown();
}

static void World_Physics_Start(World *world);
void World_Start(World *world)
{
	World_Physics_Start(world);
}

static void World_Physics_End(World *world);
void World_End(World *world)
{
	World_Physics_End(world);
}

//============================================================================

/// World_ClearLink is used for new headnodes
void World_ClearLink (link_t *l)
{
	l->entitynumber = 0;
	l->prev = l->next = l;
}

void World_RemoveLink (link_t *l)
{
	l->next->prev = l->prev;
	l->prev->next = l->next;
}

void World_InsertLinkBefore (link_t *l, link_t *before, int entitynumber)
{
	l->entitynumber = entitynumber;
	l->next = before;
	l->prev = before->prev;
	l->prev->next = l;
	l->next->prev = l;
}

/*
===============================================================================

ENTITY AREA CHECKING

===============================================================================
*/

void World_PrintAreaStats(world_t *world, const char *worldname)
{
	Con_Printf("%s areagrid check stats: %d calls %d nodes (%f per call) %d entities (%f per call)\n", worldname, world->areagrid_stats_calls, world->areagrid_stats_nodechecks, (double) world->areagrid_stats_nodechecks / (double) world->areagrid_stats_calls, world->areagrid_stats_entitychecks, (double) world->areagrid_stats_entitychecks / (double) world->areagrid_stats_calls);
	world->areagrid_stats_calls = 0;
	world->areagrid_stats_nodechecks = 0;
	world->areagrid_stats_entitychecks = 0;
}

/*
===============
World_SetSize

===============
*/
void World_SetSize(World *world, const char *filename, const vec3_t mins, const vec3_t maxs, prvm_prog_t *prog)
{
	strlcpy(world->filename, filename, sizeof(world->filename));
	VectorCopy(mins, world->mins);
	VectorCopy(maxs, world->maxs);
	world->prog = prog;

	// the areagrid_marknumber is not allowed to be 0
	if (world->areagrid_marknumber < 1)
		world->areagrid_marknumber = 1;
	// choose either the world box size, or a larger box to ensure the grid isn't too fine
	world->areagrid_size[0] = max(world->maxs[0] - world->mins[0], AREA_GRID * sv_areagrid_mingridsize.value);
	world->areagrid_size[1] = max(world->maxs[1] - world->mins[1], AREA_GRID * sv_areagrid_mingridsize.value);
	world->areagrid_size[2] = max(world->maxs[2] - world->mins[2], AREA_GRID * sv_areagrid_mingridsize.value);
	// figure out the corners of such a box, centered at the center of the world box
	world->areagrid_mins[0] = (world->mins[0] + world->maxs[0] - world->areagrid_size[0]) * 0.5f;
	world->areagrid_mins[1] = (world->mins[1] + world->maxs[1] - world->areagrid_size[1]) * 0.5f;
	world->areagrid_mins[2] = (world->mins[2] + world->maxs[2] - world->areagrid_size[2]) * 0.5f;
	world->areagrid_maxs[0] = (world->mins[0] + world->maxs[0] + world->areagrid_size[0]) * 0.5f;
	world->areagrid_maxs[1] = (world->mins[1] + world->maxs[1] + world->areagrid_size[1]) * 0.5f;
	world->areagrid_maxs[2] = (world->mins[2] + world->maxs[2] + world->areagrid_size[2]) * 0.5f;
	// now calculate the actual useful info from that
	VectorNegate(world->areagrid_mins, world->areagrid_bias);
	world->areagrid_scale[0] = AREA_GRID / world->areagrid_size[0];
	world->areagrid_scale[1] = AREA_GRID / world->areagrid_size[1];
	world->areagrid_scale[2] = AREA_GRID / world->areagrid_size[2];
	World_ClearLink(&world->areagrid_outside);
	
	for (size_t i = 0; i < AREA_GRIDNODES; i++)
		World_ClearLink(&world->areagrid[i]);
		
	if (unlikely(developer_extra.integer))
		Con_DPrintf("areagrid settings: divisions %ix%ix1 : box %f %f %f : %f %f %f size %f %f %f grid %f %f %f (mingrid %f)\n", AREA_GRID, AREA_GRID, world->areagrid_mins[0], world->areagrid_mins[1], world->areagrid_mins[2], world->areagrid_maxs[0], world->areagrid_maxs[1], world->areagrid_maxs[2], world->areagrid_size[0], world->areagrid_size[1], world->areagrid_size[2], 1.0f / world->areagrid_scale[0], 1.0f / world->areagrid_scale[1], 1.0f / world->areagrid_scale[2], sv_areagrid_mingridsize.value);
}

/*
===============
World_UnlinkAll

===============
*/
void World_UnlinkAll(World *world)
{
	prvm_prog_t *prog = world->prog;
	int i;
	link_t *grid;
	// unlink all entities one by one
	grid = &world->areagrid_outside;
	while (grid->next != grid)
		World_UnlinkEdict(PRVM_EDICT_NUM(grid->next->entitynumber));
	for (i = 0, grid = world->areagrid;i < AREA_GRIDNODES;i++, grid++)
		while (grid->next != grid)
			World_UnlinkEdict(PRVM_EDICT_NUM(grid->next->entitynumber));
}

/*
===============

===============
*/
void World_UnlinkEdict(prvm_edict_t *ent)
{
	for (size_t i = 0; i < ENTITYGRIDAREAS; i++)
	{
		if (ent->priv.server->areagrid[i].prev)
		{
			World_RemoveLink (&ent->priv.server->areagrid[i]);
			ent->priv.server->areagrid[i].prev = ent->priv.server->areagrid[i].next = nullptr;
		}
	}
}

int World_EntitiesInBox(World *world, const vec3_t requestmins, const vec3_t requestmaxs, int maxlist, prvm_edict_t **list)
{
	prvm_prog_t *prog = world->prog;
	int numlist;
	link_t *grid;
	link_t *l;
	prvm_edict_t *ent;
	vec3_t paddedmins, paddedmaxs;
	int igrid[3], igridmins[3], igridmaxs[3];


	VectorCopy(requestmins, paddedmins);
	VectorCopy(requestmaxs, paddedmaxs);

	// FIXME: if areagrid_marknumber wraps, all entities need their
	// ent->priv.server->areagridmarknumber reset
	world->areagrid_stats_calls++;
	world->areagrid_marknumber++;
	igridmins[0] = (int) floor((paddedmins[0] + world->areagrid_bias[0]) * world->areagrid_scale[0]);
	igridmins[1] = (int) floor((paddedmins[1] + world->areagrid_bias[1]) * world->areagrid_scale[1]);
	igridmaxs[0] = (int) floor((paddedmaxs[0] + world->areagrid_bias[0]) * world->areagrid_scale[0]) + 1;
	igridmaxs[1] = (int) floor((paddedmaxs[1] + world->areagrid_bias[1]) * world->areagrid_scale[1]) + 1;
	igridmins[0] = max(0, igridmins[0]);
	igridmins[1] = max(0, igridmins[1]);
	igridmaxs[0] = min(AREA_GRID, igridmaxs[0]);
	igridmaxs[1] = min(AREA_GRID, igridmaxs[1]);


	numlist = 0;
	// add entities not linked into areagrid because they are too big or
	// outside the grid bounds
	if (world->areagrid_outside.next)
	{
		grid = &world->areagrid_outside;
		for (l = grid->next;l != grid;l = l->next)
		{
			ent = PRVM_EDICT_NUM(l->entitynumber);
			if (ent->priv.server->areagridmarknumber != world->areagrid_marknumber)
			{
				ent->priv.server->areagridmarknumber = world->areagrid_marknumber;
				if (!ent->priv.server->free && BoxesOverlap(paddedmins, paddedmaxs, ent->priv.server->areamins, ent->priv.server->areamaxs))
				{
					if (numlist < maxlist)
						list[numlist] = ent;
					numlist++;
				}
				world->areagrid_stats_entitychecks++;
			}
		}
	}
	// add grid linked entities
	for (igrid[1] = igridmins[1];igrid[1] < igridmaxs[1];igrid[1]++)
	{
		grid = world->areagrid + igrid[1] * AREA_GRID + igridmins[0];
		for (igrid[0] = igridmins[0];igrid[0] < igridmaxs[0];igrid[0]++, grid++)
		{
			if (grid->next)
			{
				for (l = grid->next;l != grid;l = l->next)
				{
					ent = PRVM_EDICT_NUM(l->entitynumber);
					if (ent->priv.server->areagridmarknumber != world->areagrid_marknumber)
					{
						ent->priv.server->areagridmarknumber = world->areagrid_marknumber;
						if (!ent->priv.server->free && BoxesOverlap(paddedmins, paddedmaxs, ent->priv.server->areamins, ent->priv.server->areamaxs))
						{
							if (numlist < maxlist)
								list[numlist] = ent;
							numlist++;
						}
					}
					world->areagrid_stats_entitychecks++;
				}
			}
		}
	}
	return numlist;
}

static void World_LinkEdict_AreaGrid(World *world, prvm_edict_t *ent)
{
	prvm_prog_t *prog = world->prog;
	link_t *grid;
	int igrid[3], igridmins[3], igridmaxs[3], gridnum, entitynumber = PRVM_NUM_FOR_EDICT(ent);

	if (unlikely(entitynumber <= 0 || entitynumber >= prog->max_edicts || PRVM_EDICT_NUM(entitynumber) != ent))
	{
		Con_Printf ("World_LinkEdict_AreaGrid: invalid edict %p (edicts is %p, edict compared to prog->edicts is %i)\n", (void *)ent, (void *)prog->edicts, entitynumber);
		return;
	}

	igridmins[0] = (int) floor((ent->priv.server->areamins[0] + world->areagrid_bias[0]) * world->areagrid_scale[0]);
	igridmins[1] = (int) floor((ent->priv.server->areamins[1] + world->areagrid_bias[1]) * world->areagrid_scale[1]);
	igridmaxs[0] = (int) floor((ent->priv.server->areamaxs[0] + world->areagrid_bias[0]) * world->areagrid_scale[0]) + 1;
	igridmaxs[1] = (int) floor((ent->priv.server->areamaxs[1] + world->areagrid_bias[1]) * world->areagrid_scale[1]) + 1;
	if (igridmins[0] < 0 || igridmaxs[0] > AREA_GRID || igridmins[1] < 0 || igridmaxs[1] > AREA_GRID || ((igridmaxs[0] - igridmins[0]) * (igridmaxs[1] - igridmins[1])) > ENTITYGRIDAREAS)
	{
		// wow, something outside the grid, store it as such
		World_InsertLinkBefore (&ent->priv.server->areagrid[0], &world->areagrid_outside, entitynumber);
		return;
	}

	gridnum = 0;
	for (igrid[1] = igridmins[1];igrid[1] < igridmaxs[1];igrid[1]++)
	{
		grid = world->areagrid + igrid[1] * AREA_GRID + igridmins[0];
		for (igrid[0] = igridmins[0];igrid[0] < igridmaxs[0];igrid[0]++, grid++, gridnum++)
			World_InsertLinkBefore (&ent->priv.server->areagrid[gridnum], grid, entitynumber);
	}
}

/*
===============
World_LinkEdict

===============
*/
void World_LinkEdict(World *world, prvm_edict_t *ent, const vec3_t mins, const vec3_t maxs)
{
	prvm_prog_t *prog = world->prog;
	// unlink from old position first
	if (ent->priv.server->areagrid[0].prev)
		World_UnlinkEdict(ent);

	// don't add the world
	if (ent == prog->edicts)
		return;

	// don't add free entities
	if (ent->priv.server->free)
		return;

	VectorCopy(mins, ent->priv.server->areamins);
	VectorCopy(maxs, ent->priv.server->areamaxs);
	World_LinkEdict_AreaGrid(world, ent);
}




//============================================================================
// physics engine support
//============================================================================


cvar_t physics_ode_quadtree_depth = {0, "physics_ode_quadtree_depth","5", "desired subdivision level of quadtree culling space"};
cvar_t physics_ode_allowconvex = {0, "physics_ode_allowconvex", "0", "allow usage of Convex Hull primitive type on trimeshes that have custom 'collisionconvex' mesh. If disabled, trimesh primitive type are used."};
cvar_t physics_ode_contactsurfacelayer = {0, "physics_ode_contactsurfacelayer","1", "allows objects to overlap this many units to reduce jitter"};
cvar_t physics_ode_worldstep_iterations = {0, "physics_ode_worldstep_iterations", "20", "parameter to dWorldQuickStep"};
cvar_t physics_ode_contact_mu = {0, "physics_ode_contact_mu", "1", "contact solver mu parameter - friction pyramid approximation 1 (see ODE User Guide)"};
cvar_t physics_ode_contact_erp = {0, "physics_ode_contact_erp", "0.96", "contact solver erp parameter - Error Restitution Percent (see ODE User Guide)"};
cvar_t physics_ode_contact_cfm = {0, "physics_ode_contact_cfm", "0", "contact solver cfm parameter - Constraint Force Mixing (see ODE User Guide)"};
cvar_t physics_ode_contact_maxpoints = {0, "physics_ode_contact_maxpoints", "16", "maximal number of contact points between 2 objects, higher = stable (and slower), can be up to 32"};
cvar_t physics_ode_world_erp = {0, "physics_ode_world_erp", "-1", "world solver erp parameter - Error Restitution Percent (see ODE User Guide); use defaults when set to -1"};
cvar_t physics_ode_world_cfm = {0, "physics_ode_world_cfm", "-1", "world solver cfm parameter - Constraint Force Mixing (see ODE User Guide); not touched when -1"};
cvar_t physics_ode_world_damping = {0, "physics_ode_world_damping", "1", "enabled damping scale (see ODE User Guide), this scales all damping values, be aware that behavior depends of step type"};
cvar_t physics_ode_world_damping_linear = {0, "physics_ode_world_damping_linear", "0.01", "world linear damping scale (see ODE User Guide); use defaults when set to -1"};
cvar_t physics_ode_world_damping_linear_threshold = {0, "physics_ode_world_damping_linear_threshold", "0.1", "world linear damping threshold (see ODE User Guide); use defaults when set to -1"};
cvar_t physics_ode_world_damping_angular = {0, "physics_ode_world_damping_angular", "0.05", "world angular damping scale (see ODE User Guide); use defaults when set to -1"};
cvar_t physics_ode_world_damping_angular_threshold = {0, "physics_ode_world_damping_angular_threshold", "0.1", "world angular damping threshold (see ODE User Guide); use defaults when set to -1"};
cvar_t physics_ode_world_gravitymod = {0, "physics_ode_world_gravitymod", "1", "multiplies gravity got from sv_gravity, this may be needed to tweak if strong damping is used"};
cvar_t physics_ode_iterationsperframe = {0, "physics_ode_iterationsperframe", "1", "divisor for time step, runs multiple physics steps per frame"};
cvar_t physics_ode_constantstep = {0, "physics_ode_constantstep", "0", "use constant step instead of variable step which tends to increase stability, if set to 1 uses sys_ticrate, instead uses it's own value"};
cvar_t physics_ode_autodisable = {0, "physics_ode_autodisable", "1", "automatic disabling of objects which dont move for long period of time, makes object stacking a lot faster"};
cvar_t physics_ode_autodisable_steps = {0, "physics_ode_autodisable_steps", "10", "how many steps object should be dormant to be autodisabled"};
cvar_t physics_ode_autodisable_time = {0, "physics_ode_autodisable_time", "0", "how many seconds object should be dormant to be autodisabled"};
cvar_t physics_ode_autodisable_threshold_linear = {0, "physics_ode_autodisable_threshold_linear", "0.6", "body will be disabled if it's linear move below this value"};
cvar_t physics_ode_autodisable_threshold_angular = {0, "physics_ode_autodisable_threshold_angular", "6", "body will be disabled if it's angular move below this value"};
cvar_t physics_ode_autodisable_threshold_samples = {0, "physics_ode_autodisable_threshold_samples", "5", "average threshold with this number of samples"};
cvar_t physics_ode_movelimit = {0, "physics_ode_movelimit", "0.5", "clamp velocity if a single move would exceed this percentage of object thickness, to prevent flying through walls, be aware that behavior depends of step type"};
cvar_t physics_ode_spinlimit = {0, "physics_ode_spinlimit", "10000", "reset spin velocity if it gets too large"};
cvar_t physics_ode_trick_fixnan = {0, "physics_ode_trick_fixnan", "1", "engine trick that checks and fixes NaN velocity/origin/angles on objects, a value of 2 makes console prints on each fix"};
cvar_t physics_ode_printstats = {0, "physics_ode_printstats", "0", "print ODE stats each frame"};

cvar_t physics_ode = {0, "physics_ode", "1", "run ODE physics (VERY experimental and potentially buggy)"};


#ifdef LINK_TO_LIBODE
	#include "ode/ode.h"
#else
	#error You need to link to libode...
#endif

static void World_Physics_Init()
{


	Cvar_RegisterVariable(&physics_ode_quadtree_depth);
	Cvar_RegisterVariable(&physics_ode_contactsurfacelayer);
	Cvar_RegisterVariable(&physics_ode_worldstep_iterations);
	Cvar_RegisterVariable(&physics_ode_contact_mu);
	Cvar_RegisterVariable(&physics_ode_contact_erp);
	Cvar_RegisterVariable(&physics_ode_contact_cfm);
	Cvar_RegisterVariable(&physics_ode_contact_maxpoints);
	Cvar_RegisterVariable(&physics_ode_world_erp);
	Cvar_RegisterVariable(&physics_ode_world_cfm);
	Cvar_RegisterVariable(&physics_ode_world_damping);
	Cvar_RegisterVariable(&physics_ode_world_damping_linear);
	Cvar_RegisterVariable(&physics_ode_world_damping_linear_threshold);
	Cvar_RegisterVariable(&physics_ode_world_damping_angular);
	Cvar_RegisterVariable(&physics_ode_world_damping_angular_threshold);
	Cvar_RegisterVariable(&physics_ode_world_gravitymod);
	Cvar_RegisterVariable(&physics_ode_iterationsperframe);
	Cvar_RegisterVariable(&physics_ode_constantstep);
	Cvar_RegisterVariable(&physics_ode_movelimit);
	Cvar_RegisterVariable(&physics_ode_spinlimit);
	Cvar_RegisterVariable(&physics_ode_trick_fixnan);
	Cvar_RegisterVariable(&physics_ode_autodisable);
	Cvar_RegisterVariable(&physics_ode_autodisable_steps);
	Cvar_RegisterVariable(&physics_ode_autodisable_time);
	Cvar_RegisterVariable(&physics_ode_autodisable_threshold_linear);
	Cvar_RegisterVariable(&physics_ode_autodisable_threshold_angular);
	Cvar_RegisterVariable(&physics_ode_autodisable_threshold_samples);
	Cvar_RegisterVariable(&physics_ode_printstats);
	Cvar_RegisterVariable(&physics_ode_allowconvex);
	Cvar_RegisterVariable(&physics_ode);

	dInitODE();
}

static void World_Physics_Shutdown()
{
	dCloseODE();
}


static void World_Physics_UpdateODE(World *world)
{
	dWorldID odeworld;

	odeworld = (dWorldID)world->physics.ode_world;

	// ERP and CFM
	if (physics_ode_world_erp.value >= 0)
		dWorldSetERP(odeworld, physics_ode_world_erp.value);
	if (physics_ode_world_cfm.value >= 0)
		dWorldSetCFM(odeworld, physics_ode_world_cfm.value);
	// Damping
	if (physics_ode_world_damping.integer)
	{
		dWorldSetLinearDamping(odeworld, (physics_ode_world_damping_linear.value >= 0) ? (physics_ode_world_damping_linear.value * physics_ode_world_damping.value) : 0);
		dWorldSetLinearDampingThreshold(odeworld, (physics_ode_world_damping_linear_threshold.value >= 0) ? (physics_ode_world_damping_linear_threshold.value * physics_ode_world_damping.value) : 0);
		dWorldSetAngularDamping(odeworld, (physics_ode_world_damping_angular.value >= 0) ? (physics_ode_world_damping_angular.value * physics_ode_world_damping.value) : 0);
		dWorldSetAngularDampingThreshold(odeworld, (physics_ode_world_damping_angular_threshold.value >= 0) ? (physics_ode_world_damping_angular_threshold.value * physics_ode_world_damping.value) : 0);
	}
	else
	{
		dWorldSetLinearDamping(odeworld, 0);
		dWorldSetLinearDampingThreshold(odeworld, 0);
		dWorldSetAngularDamping(odeworld, 0);
		dWorldSetAngularDampingThreshold(odeworld, 0);
	}
	// Autodisable
	dWorldSetAutoDisableFlag(odeworld, (physics_ode_autodisable.integer) ? 1 : 0);
	if (physics_ode_autodisable.integer)
	{
		dWorldSetAutoDisableSteps(odeworld, bound(1, physics_ode_autodisable_steps.integer, 100)); 
		dWorldSetAutoDisableTime(odeworld, physics_ode_autodisable_time.value);
		dWorldSetAutoDisableAverageSamplesCount(odeworld, bound(1, physics_ode_autodisable_threshold_samples.integer, 100));
		dWorldSetAutoDisableLinearThreshold(odeworld, physics_ode_autodisable_threshold_linear.value); 
		dWorldSetAutoDisableAngularThreshold(odeworld, physics_ode_autodisable_threshold_angular.value); 
	}
}

static void World_Physics_EnableODE(World *world)
{
	dVector3 center, extents;
	if (world->physics.ode)
		return;

	world->physics.ode = true;
	VectorMAM(0.5f, world->mins, 0.5f, world->maxs, center);
	VectorSubtract(world->maxs, center, extents);
	world->physics.ode_world = dWorldCreate();
	world->physics.ode_space = dQuadTreeSpaceCreate(nullptr, center, extents, bound(1, physics_ode_quadtree_depth.integer, 10));
	world->physics.ode_contactgroup = dJointGroupCreate(0);

	World_Physics_UpdateODE(world);
}


static void World_Physics_Start(World *world)
{
	if (world->physics.ode)
		return;
	World_Physics_EnableODE(world);
}

static void World_Physics_End(World *world)
{
	if (world->physics.ode)
	{
		dWorldDestroy((dWorldID)world->physics.ode_world);
		dSpaceDestroy((dSpaceID)world->physics.ode_space);
		dJointGroupDestroy((dJointGroupID)world->physics.ode_contactgroup);
		world->physics.ode = false;
	}

}

void World_Physics_RemoveJointFromEntity(World *world, prvm_edict_t *ed)
{
	ed->priv.server->ode_joint_type = 0;

	if(ed->priv.server->ode_joint)
		dJointDestroy((dJointID)ed->priv.server->ode_joint);
	ed->priv.server->ode_joint = nullptr;

}

void World_Physics_RemoveFromEntity(World *world, prvm_edict_t *ed)
{
	edict_odefunc_t *f, *nf;

	// entity is not physics controlled, free any physics data
	ed->priv.server->ode_physics = false;

	if (ed->priv.server->ode_geom)
		dGeomDestroy((dGeomID)ed->priv.server->ode_geom);
	ed->priv.server->ode_geom = nullptr;
	if (ed->priv.server->ode_body)
	{
		dJointID j;
		dBodyID b1, b2;
		prvm_edict_t *ed2;
		while(dBodyGetNumJoints((dBodyID)ed->priv.server->ode_body))
		{
			j = dBodyGetJoint((dBodyID)ed->priv.server->ode_body, 0);
			ed2 = (prvm_edict_t *) dJointGetData(j);
			b1 = dJointGetBody(j, 0);
			b2 = dJointGetBody(j, 1);
			if(b1 == (dBodyID)ed->priv.server->ode_body)
			{
				b1 = 0;
				ed2->priv.server->ode_joint_enemy = 0;
			}
			if(b2 == (dBodyID)ed->priv.server->ode_body)
			{
				b2 = 0;
				ed2->priv.server->ode_joint_aiment = 0;
			}
			dJointAttach(j, b1, b2);
		}
		dBodyDestroy((dBodyID)ed->priv.server->ode_body);
	}
	ed->priv.server->ode_body = nullptr;
	if (ed->priv.server->ode_vertex3f)
		Mem_Free(ed->priv.server->ode_vertex3f);
	ed->priv.server->ode_vertex3f = nullptr;
	ed->priv.server->ode_numvertices = 0;
	if (ed->priv.server->ode_element3i)
		Mem_Free(ed->priv.server->ode_element3i);
	ed->priv.server->ode_element3i = nullptr;
	ed->priv.server->ode_numtriangles = 0;
	if(ed->priv.server->ode_massbuf)
		Mem_Free(ed->priv.server->ode_massbuf);
	ed->priv.server->ode_massbuf = nullptr;
	// clear functions stack
	for(f = ed->priv.server->ode_func; f; f = nf)
	{
		nf = f->next;
		Mem_Free(f);
	}
	ed->priv.server->ode_func = nullptr;
}

void World_Physics_ApplyCmd(prvm_edict_t *ed, edict_odefunc_t *f)
{
	dBodyID body = (dBodyID)ed->priv.server->ode_body;

	switch(f->type)
	{
	case ODEFUNC_ENABLE:
		dBodyEnable(body);
		break;
	case ODEFUNC_DISABLE:
		dBodyDisable(body);
		break;
	case ODEFUNC_FORCE:
		dBodyEnable(body);
		dBodyAddForceAtPos(body, f->v1[0], f->v1[1], f->v1[2], f->v2[0], f->v2[1], f->v2[2]);
		break;
	case ODEFUNC_TORQUE:
		dBodyEnable(body);
		dBodyAddTorque(body, f->v1[0], f->v1[1], f->v1[2]);
		break;
	default:
		break;
	}
}

static void World_Physics_Frame_BodyToEntity(World *world, prvm_edict_t *ed)
{
	prvm_prog_t *prog = world->prog;
	const dReal *avel;
	const dReal *o;
	const dReal *r; // for some reason dBodyGetRotation returns a [3][4] matrix
	const dReal *vel;
	dBodyID body = (dBodyID)ed->priv.server->ode_body;

	matrix4x4_t bodymatrix;
	matrix4x4_t entitymatrix;
	vec3_t angles;
	vec3_t avelocity;
	vec3_t forward, left, up;
	vec3_t origin;
	vec3_t spinvelocity;
	vec3_t velocity;
	int jointtype;
	if (!body)
		return;
	int movetype = (int)PRVM_gameedictfloat(ed, movetype);
	if (movetype != MOVETYPE_PHYSICS)
	{
		jointtype = (int)PRVM_gameedictfloat(ed, jointtype);
		switch(jointtype)
		{
			// TODO feed back data from physics
			case JOINTTYPE_POINT:
				break;
			case JOINTTYPE_HINGE:
				break;
			case JOINTTYPE_SLIDER:
				break;
			case JOINTTYPE_UNIVERSAL:
				break;
			case JOINTTYPE_HINGE2:
				break;
			case JOINTTYPE_FIXED:
				break;
		}
		return;
	}
	// store the physics engine data into the entity
	o = dBodyGetPosition(body);
	r = dBodyGetRotation(body);
	vel = dBodyGetLinearVel(body);
	avel = dBodyGetAngularVel(body);
	VectorCopy(o, origin);
	forward[0] = r[0];
	forward[1] = r[4];
	forward[2] = r[8];
	left[0] = r[1];
	left[1] = r[5];
	left[2] = r[9];
	up[0] = r[2];
	up[1] = r[6];
	up[2] = r[10];
	VectorCopy(vel, velocity);
	VectorCopy(avel, spinvelocity);
	Matrix4x4_FromVectors(&bodymatrix, forward, left, up, origin);
	Matrix4x4_Concat(&entitymatrix, &bodymatrix, &ed->priv.server->ode_offsetimatrix);
	Matrix4x4_ToVectors(&entitymatrix, forward, left, up, origin);

	AnglesFromVectors(angles, forward, up, false);
	VectorSet(avelocity, RAD2DEG(spinvelocity[PITCH]), RAD2DEG(spinvelocity[ROLL]), RAD2DEG(spinvelocity[YAW]));

	{
		float pitchsign = 1.0f;
		if(prog == SVVM_prog) // FIXME some better way?
			pitchsign = SV_GetPitchSign(prog, ed);
		else if(prog == CLVM_prog)
			pitchsign = CL_GetPitchSign(prog, ed);
		angles[PITCH] *= pitchsign;
		avelocity[PITCH] *= pitchsign;
	}

	VectorCopy(origin, PRVM_gameedictvector(ed, origin));
	VectorCopy(velocity, PRVM_gameedictvector(ed, velocity));

	VectorCopy(angles, PRVM_gameedictvector(ed, angles));
	VectorCopy(avelocity, PRVM_gameedictvector(ed, avelocity));

	// values for BodyFromEntity to check if the qc modified anything later
	VectorCopy(origin, ed->priv.server->ode_origin);
	VectorCopy(velocity, ed->priv.server->ode_velocity);
	VectorCopy(angles, ed->priv.server->ode_angles);
	VectorCopy(avelocity, ed->priv.server->ode_avelocity);
	ed->priv.server->ode_gravity = dBodyGetGravityMode(body) != 0;

	if(prog == SVVM_prog) // FIXME some better way?
	{
		SV_LinkEdict(ed);
		SV_LinkEdict_TouchAreaGrid(ed);
	}
}

static void World_Physics_Frame_ForceFromEntity(World *world, prvm_edict_t *ed)
{
	prvm_prog_t *prog = world->prog;
	vec3_t movedir, origin;

	int movetype = (int)PRVM_gameedictfloat(ed, movetype);
	int forcetype = (int)PRVM_gameedictfloat(ed, forcetype);
	
	if (movetype == MOVETYPE_PHYSICS)
		forcetype = FORCETYPE_NONE; // can't have both
		
	if (!forcetype)
		return;
		
	int enemy = PRVM_gameedictedict(ed, enemy);
	
	if (enemy <= 0 || enemy >= prog->num_edicts || prog->edicts[enemy].priv.required->free || prog->edicts[enemy].priv.server->ode_body == 0)
		return;
	VectorCopy(PRVM_gameedictvector(ed, movedir), movedir);
	VectorCopy(PRVM_gameedictvector(ed, origin), origin);
	dBodyEnable((dBodyID)prog->edicts[enemy].priv.server->ode_body);
	switch(forcetype)
	{
		case FORCETYPE_FORCE:
			if (movedir[0] || movedir[1] || movedir[2])
				dBodyAddForce((dBodyID)prog->edicts[enemy].priv.server->ode_body, movedir[0], movedir[1], movedir[2]);
			break;
		case FORCETYPE_FORCEATPOS:
			if (movedir[0] || movedir[1] || movedir[2])
				dBodyAddForceAtPos((dBodyID)prog->edicts[enemy].priv.server->ode_body, movedir[0], movedir[1], movedir[2], origin[0], origin[1], origin[2]);
			break;
		case FORCETYPE_TORQUE:
			if (movedir[0] || movedir[1] || movedir[2])
				dBodyAddTorque((dBodyID)prog->edicts[enemy].priv.server->ode_body, movedir[0], movedir[1], movedir[2]);
			break;
		case FORCETYPE_NONE:
		default:
			// bad force
			break;
	}
}

static void World_Physics_Frame_JointFromEntity(World *world, prvm_edict_t *ed)
{
	prvm_prog_t *prog = world->prog;
	dJointID j = 0;
	dBodyID b1 = 0;
	dBodyID b2 = 0;
	int movetype = 0;
	int jointtype = 0;
	int enemy = 0, aiment = 0;
	vec3_t origin, velocity, angles, forward, left, up, movedir;
	vec_t CFM, ERP, FMax, Stop, Vel;

	movetype = (int)PRVM_gameedictfloat(ed, movetype);
	jointtype = (int)PRVM_gameedictfloat(ed, jointtype);
	VectorClear(origin);
	VectorClear(velocity);
	VectorClear(angles);
	VectorClear(movedir);
	enemy = PRVM_gameedictedict(ed, enemy);
	aiment = PRVM_gameedictedict(ed, aiment);
	VectorCopy(PRVM_gameedictvector(ed, origin), origin);
	VectorCopy(PRVM_gameedictvector(ed, velocity), velocity);
	VectorCopy(PRVM_gameedictvector(ed, angles), angles);
	VectorCopy(PRVM_gameedictvector(ed, movedir), movedir);
	if(movetype == MOVETYPE_PHYSICS)
		jointtype = JOINTTYPE_NONE; // can't have both
	if(enemy <= 0 || enemy >= prog->num_edicts || prog->edicts[enemy].priv.required->free || prog->edicts[enemy].priv.server->ode_body == 0)
		enemy = 0;
	if(aiment <= 0 || aiment >= prog->num_edicts || prog->edicts[aiment].priv.required->free || prog->edicts[aiment].priv.server->ode_body == 0)
		aiment = 0;
	// see http://www.ode.org/old_list_archives/2006-January/017614.html
	// we want to set ERP? make it fps independent and work like a spring constant
	// note: if movedir[2] is 0, it becomes ERP = 1, CFM = 1.0 / (H * K)
	if(movedir[0] > 0 && movedir[1] > 0)
	{
		const float K = movedir[0];
		const float D = movedir[1];
		float R = 2.0f * D * sqrtf(K); // we assume D is premultiplied by sqrt(sprungMass)
		CFM = 1.0f / (world->physics.ode_step * K + R); // always > 0
		ERP = world->physics.ode_step * K * CFM;
		Vel = .0f;
		FMax = .0f;
		Stop = movedir[2];
	}
	else if(movedir[1] < .0f)
	{
		CFM = .0f;
		ERP = .0f;
		Vel = movedir[0];
		FMax = -movedir[1]; // TODO do we need to multiply with world.physics.ode_step?
		Stop = movedir[2] > 0 ? movedir[2] : inf.f;
	}
	else 
	{
		CFM = .0f;
		ERP = .0f;
		Vel = .0f;
		FMax = .0f;
		Stop = inf.f;
	}
	if(jointtype == ed->priv.server->ode_joint_type && VectorCompare(origin, ed->priv.server->ode_joint_origin) && VectorCompare(velocity, ed->priv.server->ode_joint_velocity) && VectorCompare(angles, ed->priv.server->ode_joint_angles) && enemy == ed->priv.server->ode_joint_enemy && aiment == ed->priv.server->ode_joint_aiment && VectorCompare(movedir, ed->priv.server->ode_joint_movedir))
		return; // nothing to do
	AngleVectorsFLU(angles, forward, left, up);
	switch(jointtype)
	{
		case JOINTTYPE_POINT:
			j = dJointCreateBall((dWorldID)world->physics.ode_world, 0);
			break;
		case JOINTTYPE_HINGE:
			j = dJointCreateHinge((dWorldID)world->physics.ode_world, 0);
			break;
		case JOINTTYPE_SLIDER:
			j = dJointCreateSlider((dWorldID)world->physics.ode_world, 0);
			break;
		case JOINTTYPE_UNIVERSAL:
			j = dJointCreateUniversal((dWorldID)world->physics.ode_world, 0);
			break;
		case JOINTTYPE_HINGE2:
			j = dJointCreateHinge2((dWorldID)world->physics.ode_world, 0);
			break;
		case JOINTTYPE_FIXED:
			j = dJointCreateFixed((dWorldID)world->physics.ode_world, 0);
			break;
		case JOINTTYPE_NONE:
		default:
			// no joint
			j = 0;
			break;
	}
	if(ed->priv.server->ode_joint)
	{
		dJointAttach((dJointID)ed->priv.server->ode_joint, 0, 0);
		dJointDestroy((dJointID)ed->priv.server->ode_joint);
	}
	ed->priv.server->ode_joint = (void *) j;
	ed->priv.server->ode_joint_type = jointtype;
	ed->priv.server->ode_joint_enemy = enemy;
	ed->priv.server->ode_joint_aiment = aiment;
	VectorCopy(origin, ed->priv.server->ode_joint_origin);
	VectorCopy(velocity, ed->priv.server->ode_joint_velocity);
	VectorCopy(angles, ed->priv.server->ode_joint_angles);
	VectorCopy(movedir, ed->priv.server->ode_joint_movedir);
	if(!j)
		return;

	dJointSetData(j, (void *) ed);
	if(enemy)
		b1 = (dBodyID)prog->edicts[enemy].priv.server->ode_body;
	if(aiment)
		b2 = (dBodyID)prog->edicts[aiment].priv.server->ode_body;
	dJointAttach(j, b1, b2);

	switch(jointtype)
	{
		case JOINTTYPE_POINT:
			dJointSetBallAnchor(j, origin[0], origin[1], origin[2]);
			break;
		case JOINTTYPE_HINGE:
			dJointSetHingeAnchor(j, origin[0], origin[1], origin[2]);
			dJointSetHingeAxis(j, forward[0], forward[1], forward[2]);
			dJointSetHingeParam(j, dParamFMax, FMax);
			dJointSetHingeParam(j, dParamHiStop, Stop);
			dJointSetHingeParam(j, dParamLoStop, -Stop);
			dJointSetHingeParam(j, dParamStopCFM, CFM);
			dJointSetHingeParam(j, dParamStopERP, ERP);
			dJointSetHingeParam(j, dParamVel, Vel);
			break;
		case JOINTTYPE_SLIDER:
			dJointSetSliderAxis(j, forward[0], forward[1], forward[2]);
			dJointSetSliderParam(j, dParamFMax, FMax);
			dJointSetSliderParam(j, dParamHiStop, Stop);
			dJointSetSliderParam(j, dParamLoStop, -Stop);
			dJointSetSliderParam(j, dParamStopCFM, CFM);
			dJointSetSliderParam(j, dParamStopERP, ERP);
			dJointSetSliderParam(j, dParamVel, Vel);
			break;
		case JOINTTYPE_UNIVERSAL:
			dJointSetUniversalAnchor(j, origin[0], origin[1], origin[2]);
			dJointSetUniversalAxis1(j, forward[0], forward[1], forward[2]);
			dJointSetUniversalAxis2(j, up[0], up[1], up[2]);
			dJointSetUniversalParam(j, dParamFMax, FMax);
			dJointSetUniversalParam(j, dParamHiStop, Stop);
			dJointSetUniversalParam(j, dParamLoStop, -Stop);
			dJointSetUniversalParam(j, dParamStopCFM, CFM);
			dJointSetUniversalParam(j, dParamStopERP, ERP);
			dJointSetUniversalParam(j, dParamVel, Vel);
			dJointSetUniversalParam(j, dParamFMax2, FMax);
			dJointSetUniversalParam(j, dParamHiStop2, Stop);
			dJointSetUniversalParam(j, dParamLoStop2, -Stop);
			dJointSetUniversalParam(j, dParamStopCFM2, CFM);
			dJointSetUniversalParam(j, dParamStopERP2, ERP);
			dJointSetUniversalParam(j, dParamVel2, Vel);
			break;
		case JOINTTYPE_HINGE2:
			dJointSetHinge2Anchor(j, origin[0], origin[1], origin[2]);
			dJointSetHinge2Axis1(j, forward[0], forward[1], forward[2]);
			dJointSetHinge2Axis2(j, velocity[0], velocity[1], velocity[2]);
			dJointSetHinge2Param(j, dParamFMax, FMax);
			dJointSetHinge2Param(j, dParamHiStop, Stop);
			dJointSetHinge2Param(j, dParamLoStop, -Stop);
			dJointSetHinge2Param(j, dParamStopCFM, CFM);
			dJointSetHinge2Param(j, dParamStopERP, ERP);
			dJointSetHinge2Param(j, dParamVel, Vel);
			dJointSetHinge2Param(j, dParamFMax2, FMax);
			dJointSetHinge2Param(j, dParamHiStop2, Stop);
			dJointSetHinge2Param(j, dParamLoStop2, -Stop);
			dJointSetHinge2Param(j, dParamStopCFM2, CFM);
			dJointSetHinge2Param(j, dParamStopERP2, ERP);
			dJointSetHinge2Param(j, dParamVel2, Vel);
			break;
		case JOINTTYPE_FIXED:
			break;
		case 0:
		default:
			Sys_Error("what? but above the joint was valid...\n");
			break;
	}
#undef SETPARAMS

	
}

// test convex geometry data
// planes for a cube, these should coincide with the 
dReal test_convex_planes[] = 
{
    1.0f ,0.0f ,0.0f ,2.25f,
    0.0f ,1.0f ,0.0f ,2.25f,
    0.0f ,0.0f ,1.0f ,2.25f,
    -1.0f,0.0f ,0.0f ,2.25f,
    0.0f ,-1.0f,0.0f ,2.25f,
    0.0f ,0.0f ,-1.0f,2.25f
};
const unsigned int test_convex_planecount = 6;
// points for a cube
dReal test_convex_points[] = 
{
	2.25f,2.25f,2.25f,    // point 0
	-2.25f,2.25f,2.25f,   // point 1
    2.25f,-2.25f,2.25f,   // point 2
    -2.25f,-2.25f,2.25f,  // point 3
    2.25f,2.25f,-2.25f,   // point 4
    -2.25f,2.25f,-2.25f,  // point 5
    2.25f,-2.25f,-2.25f,  // point 6
    -2.25f,-2.25f,-2.25f, // point 7
};
const unsigned int test_convex_pointcount = 8;
// polygons for a cube (6 squares), index 
unsigned int test_convex_polygons[] = 
{
	4,0,2,6,4, // positive X
    4,1,0,4,5, // positive Y
    4,0,1,3,2, // positive Z
    4,3,1,5,7, // negative X
    4,2,3,7,6, // negative Y
    4,5,4,6,7, // negative Z
};

static void World_Physics_Frame_BodyFromEntity(World *world, prvm_edict_t *ed)
{
	prvm_prog_t *prog = world->prog;
	const float *iv;
	const int *ie;
	dBodyID body;
	dMass mass;
	const dReal *ovelocity, *ospinvelocity;
	void *dataID;
	dp_model_t *model;
	float *ov;
	int *oe;
	int axisindex;
	int modelindex = 0;
	int movetype = MOVETYPE_NONE;
	int numtriangles;
	int numvertices;
	int solid = SOLID_NOT, geomtype = 0;
	int triangleindex;
	int vertexindex;
	mempool_t *mempool;
	bool modified = false;
	vec3_t angles;
	vec3_t avelocity;
	vec3_t entmaxs;
	vec3_t entmins;
	vec3_t forward;
	vec3_t geomcenter;
	vec3_t geomsize;
	vec3_t left;
	vec3_t origin;
	vec3_t spinvelocity;
	vec3_t up;
	vec3_t velocity;
	vec_t f;
	vec_t length;
	vec_t massval = 1.0f;
	vec_t movelimit;
	vec_t radius;
	vec3_t scale;
	vec_t spinlimit;
	vec_t test;
	bool gravity;
	bool geom_modified = false;
	edict_odefunc_t *func, *nextf;

	dReal *planes, *planesData, *pointsData;
	unsigned int *polygons, *polygonsData, polyvert;
	bool *mapped, *used, convex_compatible;
	int numplanes = 0, numpoints = 0, i;

	VectorClear(entmins);
	VectorClear(entmaxs);

	solid = (int)PRVM_gameedictfloat(ed, solid);
	geomtype = (int)PRVM_gameedictfloat(ed, geomtype);
	movetype = (int)PRVM_gameedictfloat(ed, movetype);
	// support scale and q3map/radiant's modelscale_vec
	if (PRVM_gameedictvector(ed, modelscale_vec)[0] != 0.0f || PRVM_gameedictvector(ed, modelscale_vec)[1] != 0.0f || PRVM_gameedictvector(ed, modelscale_vec)[2] != 0.0f)
		VectorCopy(PRVM_gameedictvector(ed, modelscale_vec), scale);
	else if (PRVM_gameedictfloat(ed, scale))
		VectorSet(scale, PRVM_gameedictfloat(ed, scale), PRVM_gameedictfloat(ed, scale), PRVM_gameedictfloat(ed, scale));
	else
		VectorSet(scale, 1.0f, 1.0f, 1.0f);
	modelindex = 0;
	if (PRVM_gameedictfloat(ed, mass))
		massval = PRVM_gameedictfloat(ed, mass);
	if (movetype != MOVETYPE_PHYSICS)
		massval = 1.0f;
	mempool = prog->progs_mempool;
	model = nullptr;
	if (!geomtype)
	{
		// VorteX: keep support for deprecated solid fields to not break mods
		if (solid == SOLID_PHYSICS_TRIMESH || solid == SOLID_BSP)
			geomtype = GEOMTYPE_TRIMESH;
		else if (solid == SOLID_NOT || solid == SOLID_TRIGGER)
			geomtype = GEOMTYPE_NONE;
		else if (solid == SOLID_PHYSICS_SPHERE)
			geomtype = GEOMTYPE_SPHERE;
		else if (solid == SOLID_PHYSICS_CAPSULE)
			geomtype = GEOMTYPE_CAPSULE;
		else if (solid == SOLID_PHYSICS_CYLINDER)
			geomtype = GEOMTYPE_CYLINDER;
		else if (solid == SOLID_PHYSICS_BOX)
			geomtype = GEOMTYPE_BOX;
		else
			geomtype = GEOMTYPE_BOX;
	}
	if (geomtype == GEOMTYPE_TRIMESH)
	{
		modelindex = (int)PRVM_gameedictfloat(ed, modelindex);
		if (world == &sv.world)
			model = SV_GetModelByIndex(modelindex);
		else if (world == &cl.world)
			model = CL_GetModelByIndex(modelindex);
		else
			model = nullptr;
		if (likely(model != nullptr))
		{
			entmins[0] = model->normalmins[0] * scale[0];
			entmins[1] = model->normalmins[1] * scale[1];
			entmins[2] = model->normalmins[2] * scale[2];
			entmaxs[0] = model->normalmaxs[0] * scale[0];
			entmaxs[1] = model->normalmaxs[1] * scale[1];
			entmaxs[2] = model->normalmaxs[2] * scale[2];
			geom_modified = !VectorCompare(ed->priv.server->ode_scale, scale) || ed->priv.server->ode_modelindex != modelindex;
		}
		else
		{
			Con_Printf("entity %i (classname %s) has no model\n", PRVM_NUM_FOR_EDICT(ed), PRVM_GetString(prog, PRVM_gameedictstring(ed, classname)));
			geomtype = GEOMTYPE_BOX;
			VectorCopy(PRVM_gameedictvector(ed, mins), entmins);
			VectorCopy(PRVM_gameedictvector(ed, maxs), entmaxs);
			modelindex = 0;
			geom_modified = !VectorCompare(ed->priv.server->ode_mins, entmins) || !VectorCompare(ed->priv.server->ode_maxs, entmaxs);
		}
	}
	else if (geomtype && geomtype != GEOMTYPE_NONE)
	{
		VectorCopy(PRVM_gameedictvector(ed, mins), entmins);
		VectorCopy(PRVM_gameedictvector(ed, maxs), entmaxs);
		geom_modified = !VectorCompare(ed->priv.server->ode_mins, entmins) || !VectorCompare(ed->priv.server->ode_maxs, entmaxs);
	}
	else
	{
		// geometry type not set, falling back
		if (ed->priv.server->ode_physics)
			World_Physics_RemoveFromEntity(world, ed);
		return;
	}

	VectorSubtract(entmaxs, entmins, geomsize);
	if (VectorLength2(geomsize) == 0)
	{
		// we don't allow point-size physics objects...
		if (ed->priv.server->ode_physics)
			World_Physics_RemoveFromEntity(world, ed);
		return;
	}

	// get friction
	ed->priv.server->ode_friction = PRVM_gameedictfloat(ed, friction) ? PRVM_gameedictfloat(ed, friction) : 1.0f;

	// check if we need to create or replace the geom
	if (!ed->priv.server->ode_physics || ed->priv.server->ode_mass != massval || geom_modified)
	{
		modified = true;
		World_Physics_RemoveFromEntity(world, ed);
		ed->priv.server->ode_physics = true;
		VectorMAM(0.5f, entmins, 0.5f, entmaxs, geomcenter);
		if (PRVM_gameedictvector(ed, massofs))
			VectorCopy(geomcenter, PRVM_gameedictvector(ed, massofs));

		// check geomsize
		if (geomsize[0] * geomsize[1] * geomsize[2] == 0)
		{
			if (movetype == MOVETYPE_PHYSICS)
				Con_Printf("entity %i (classname %s) .mass * .size_x * .size_y * .size_z == 0\n", PRVM_NUM_FOR_EDICT(ed), PRVM_GetString(prog, PRVM_gameedictstring(ed, classname)));
			VectorSet(geomsize, 1.0f, 1.0f, 1.0f);
		}

		// greate geom
		switch(geomtype)
		{
		case GEOMTYPE_TRIMESH:
			// add an optimized mesh to the model containing only the SUPERCONTENTS_SOLID surfaces
			if (!model->brush.collisionmesh)
				Mod_CreateCollisionMesh(model);
			if (!model->brush.collisionmesh)
			{
				Con_Printf("entity %i (classname %s) has no geometry\n", PRVM_NUM_FOR_EDICT(ed), PRVM_GetString(prog, PRVM_gameedictstring(ed, classname)));
				goto treatasbox;
			}

			// check if trimesh can be defined with convex
			convex_compatible = false;
			for (i = 0;i < model->nummodelsurfaces;i++)
			{
				if (!strcmp(((msurface_t *)(model->data_surfaces + model->firstmodelsurface + i))->texture->name, "collisionconvex"))
				{
					convex_compatible = true;
					break;
				}
			}

			// ODE requires persistent mesh storage, so we need to copy out
			// the data from the model because renderer restarts could free it
			// during the game, additionally we need to flip the triangles...
			// note: ODE does preprocessing of the mesh for culling, removing
			// concave edges, etc., so this is not a lightweight operation
			ed->priv.server->ode_numvertices = numvertices = model->brush.collisionmesh->numverts;
			ed->priv.server->ode_vertex3f = (float *)Mem_Alloc(mempool, numvertices * sizeof(float[3]));

			// VorteX: rebuild geomsize based on entity's collision mesh, honor scale
			VectorSet(entmins, 0, 0, 0);
			VectorSet(entmaxs, 0, 0, 0);
			for (vertexindex = 0, ov = ed->priv.server->ode_vertex3f, iv = model->brush.collisionmesh->vertex3f;vertexindex < numvertices;vertexindex++, ov += 3, iv += 3)
			{
				ov[0] = iv[0] * scale[0];
				ov[1] = iv[1] * scale[1];
				ov[2] = iv[2] * scale[2];
				entmins[0] = min(entmins[0], ov[0]);
				entmins[1] = min(entmins[1], ov[1]);
				entmins[2] = min(entmins[2], ov[2]);
				entmaxs[0] = max(entmaxs[0], ov[0]);
				entmaxs[1] = max(entmaxs[1], ov[1]);
				entmaxs[2] = max(entmaxs[2], ov[2]);
			}
			if (!PRVM_gameedictvector(ed, massofs))
				VectorMAM(0.5f, entmins, 0.5f, entmaxs, geomcenter);
			for (vertexindex = 0, ov = ed->priv.server->ode_vertex3f, iv = model->brush.collisionmesh->vertex3f;vertexindex < numvertices;vertexindex++, ov += 3, iv += 3)
			{
				ov[0] = ov[0] - geomcenter[0];
				ov[1] = ov[1] - geomcenter[1];
				ov[2] = ov[2] - geomcenter[2];
			}
			VectorSubtract(entmaxs, entmins, geomsize);
			if (VectorLength2(geomsize) == 0)
			{
				if (unlikely(movetype == MOVETYPE_PHYSICS))
					Con_Printf("entity %i collision mesh has null geomsize\n", PRVM_NUM_FOR_EDICT(ed));
				VectorSet(geomsize, 1.0f, 1.0f, 1.0f);
			}
			ed->priv.server->ode_numtriangles = numtriangles = model->brush.collisionmesh->numtriangles;
			ed->priv.server->ode_element3i = (int *)Mem_Alloc(mempool, numtriangles * sizeof(int[3]));
			for (triangleindex = 0, oe = ed->priv.server->ode_element3i, ie = model->brush.collisionmesh->element3i;triangleindex < numtriangles;triangleindex++, oe += 3, ie += 3)
			{
				oe[0] = ie[2];
				oe[1] = ie[1];
				oe[2] = ie[0];
			}
			// create geom
			Matrix4x4_CreateTranslate(&ed->priv.server->ode_offsetmatrix, geomcenter[0], geomcenter[1], geomcenter[2]);
			if (!convex_compatible || !physics_ode_allowconvex.integer)
			{
				// trimesh
				dataID = dGeomTriMeshDataCreate();
				dGeomTriMeshDataBuildSingle((dTriMeshDataID)dataID, (void*)ed->priv.server->ode_vertex3f, sizeof(float[3]), ed->priv.server->ode_numvertices, ed->priv.server->ode_element3i, ed->priv.server->ode_numtriangles*3, sizeof(int[3]));
				ed->priv.server->ode_geom = (void *)dCreateTriMesh((dSpaceID)world->physics.ode_space, (dTriMeshDataID)dataID, nullptr, nullptr, nullptr);
				dMassSetBoxTotal(&mass, massval, geomsize[0], geomsize[1], geomsize[2]);
			}
			else
			{
				// VorteX: this code is unfinished in two ways
				// - no duplicate vertex merging are done
				// - triangles that shares same edge and havee sam plane are not merget into poly
				// so, currently it only works for geosphere meshes with no UV

				Con_Printf("Build convex hull for model %s...\n", model->name);
				// build convex geometry from trimesh data
				// this ensures that trimesh's triangles can form correct convex geometry
				// not many of error checking is performed
				// ODE's conve hull data consist of:
				//    planes  : an array of planes in the form: normal X, normal Y, normal Z, distance
				//    points  : an array of points X,Y,Z
				//    polygons: an array of indices to the points of each  polygon,it should be the number of vertices
				//              followed by that amount of indices to "points" in counter clockwise order
				polygonsData = polygons = (unsigned int *)Mem_Alloc(mempool, numtriangles*sizeof(int)*4);
				planesData = planes = (dReal *)Mem_Alloc(mempool, numtriangles*sizeof(dReal)*4);
				mapped = (bool *)Mem_Alloc(mempool, numvertices*sizeof(bool));
				used = (bool *)Mem_Alloc(mempool, numtriangles*sizeof(bool));
				memset(mapped, 0, numvertices*sizeof(bool));
				memset(used, 0, numtriangles*sizeof(bool));
				numplanes = numpoints = polyvert = 0;
				// build convex hull
				// todo: merge duplicated verts here
				Con_Printf("Building...\n");
				iv = ed->priv.server->ode_vertex3f;
				for (triangleindex = 0; triangleindex < numtriangles; triangleindex++)
				{
					// already formed a polygon?
					if (used[triangleindex])
						continue; 
					// init polygon
					// switch clockwise->counterclockwise
					ie = &model->brush.collisionmesh->element3i[triangleindex*3];
					used[triangleindex] = true;
					TriangleNormal(&iv[ie[0]*3], &iv[ie[1]*3], &iv[ie[2]*3], planes);
					VectorNormalize(planes);
					polygons[0] = 3;
					polygons[3] = (unsigned int)ie[0]; mapped[polygons[3]] = true;
					polygons[2] = (unsigned int)ie[1]; mapped[polygons[2]] = true;
					polygons[1] = (unsigned int)ie[2]; mapped[polygons[1]] = true;

					// now find and include concave triangles
					for (i = triangleindex; i < numtriangles; i++)
					{
						if (used[i])
							continue;
						// should share at least 2 vertexes
						for (polyvert = 1; polyvert <= polygons[0]; polyvert++)
						{
							// todo: merge in triangles that shares an edge and have same plane here
						}
					}

					// add polygon to overall stats
					planes[3] = DotProduct(&iv[polygons[1]*3], planes);
					polygons += (polygons[0]+1);
					planes += 4;
					numplanes++;
				}
				Mem_Free(used);
				// save points
				for (vertexindex = 0, numpoints = 0; vertexindex < numvertices; vertexindex++)
					if (mapped[vertexindex])
						numpoints++;
				pointsData = (dReal *)Mem_Alloc(mempool, numpoints*sizeof(dReal)*3 + numplanes*sizeof(dReal)*4); // planes is appended
				for (vertexindex = 0, numpoints = 0; vertexindex < numvertices; vertexindex++)
				{
					if (mapped[vertexindex])
					{
						VectorCopy(&iv[vertexindex*3], &pointsData[numpoints*3]);
						numpoints++;
					}
				}
				Mem_Free(mapped);
				Con_Printf("Points: \n");
				for (i = 0; i < (int)numpoints; i++)
					Con_Printf("%3i: %3.1f %3.1f %3.1f\n", i, pointsData[i*3], pointsData[i*3+1], pointsData[i*3+2]);
				// save planes
				planes = planesData;
				planesData = pointsData + numpoints*3;
				memcpy(planesData, planes, numplanes*sizeof(dReal)*4);
				Mem_Free(planes);
				Con_Printf("planes...\n");
				for (i = 0; i < numplanes; i++)
					Con_Printf("%3i: %1.1f %1.1f %1.1f %1.1f\n", i, planesData[i*4], planesData[i*4 + 1], planesData[i*4 + 2], planesData[i*4 + 3]);
				// save polygons
				polyvert = polygons - polygonsData;
				polygons = polygonsData;
				polygonsData = (unsigned int *)Mem_Alloc(mempool, polyvert*sizeof(int));
				memcpy(polygonsData, polygons, polyvert*sizeof(int));
				Mem_Free(polygons);
				Con_Printf("Polygons: \n");
				polygons = polygonsData;
				for (i = 0; i < numplanes; i++)
				{
					Con_Printf("%3i : %i ", i, polygons[0]);
					for (triangleindex = 1; triangleindex <= (int)polygons[0]; triangleindex++)
						Con_Printf("%3i ", polygons[triangleindex]);
					polygons += (polygons[0]+1);
					Con_Printf("\n");
				}
				Mem_Free(ed->priv.server->ode_element3i);
				ed->priv.server->ode_element3i = (int *)polygonsData;
				Mem_Free(ed->priv.server->ode_vertex3f);
				ed->priv.server->ode_vertex3f = (float *)pointsData;
				// check for properly build polygons by calculating the determinant of the 3x3 matrix composed of the first 3 points in the polygon
				// this code is picked from ODE Source
				Con_Printf("Check...\n");
				polygons = polygonsData;
				for (i = 0; i < numplanes; i++)
				{
					if(unlikely((pointsData[(polygons[1]*3)+0]*pointsData[(polygons[2]*3)+1]*pointsData[(polygons[3]*3)+2] +
						pointsData[(polygons[1]*3)+1]*pointsData[(polygons[2]*3)+2]*pointsData[(polygons[3]*3)+0] +
						pointsData[(polygons[1]*3)+2]*pointsData[(polygons[2]*3)+0]*pointsData[(polygons[3]*3)+1] -
						pointsData[(polygons[1]*3)+2]*pointsData[(polygons[2]*3)+1]*pointsData[(polygons[3]*3)+0] -
						pointsData[(polygons[1]*3)+1]*pointsData[(polygons[2]*3)+0]*pointsData[(polygons[3]*3)+2] -
						pointsData[(polygons[1]*3)+0]*pointsData[(polygons[2]*3)+2]*pointsData[(polygons[3]*3)+1]) < 0))
						Con_Printf("WARNING: Polygon %d is not defined counterclockwise\n", i);
					if (unlikely(planesData[(i*4)+3] < 0))
						Con_Printf("WARNING: Plane %d does not contain the origin\n", i);
					polygons += (*polygons + 1);
				}
				// create geom
				Con_Printf("Create geom...\n");
				ed->priv.server->ode_geom = (void *)dCreateConvex((dSpaceID)world->physics.ode_space, planesData, numplanes, pointsData, numpoints, polygonsData);
				dMassSetBoxTotal(&mass, massval, geomsize[0], geomsize[1], geomsize[2]);
				Con_Printf("Done!\n");
			}
			break;
		case GEOMTYPE_BOX:
treatasbox:
			Matrix4x4_CreateTranslate(&ed->priv.server->ode_offsetmatrix, geomcenter[0], geomcenter[1], geomcenter[2]);
			ed->priv.server->ode_geom = (void *)dCreateBox((dSpaceID)world->physics.ode_space, geomsize[0], geomsize[1], geomsize[2]);
			dMassSetBoxTotal(&mass, massval, geomsize[0], geomsize[1], geomsize[2]);
			break;
		case GEOMTYPE_SPHERE:
			Matrix4x4_CreateTranslate(&ed->priv.server->ode_offsetmatrix, geomcenter[0], geomcenter[1], geomcenter[2]);
			ed->priv.server->ode_geom = (void *)dCreateSphere((dSpaceID)world->physics.ode_space, geomsize[0] * 0.5f);
			dMassSetSphereTotal(&mass, massval, geomsize[0] * 0.5f);
			break;
		case GEOMTYPE_CAPSULE:
			axisindex = 0;
			if (geomsize[axisindex] < geomsize[1])
				axisindex = 1;
			if (geomsize[axisindex] < geomsize[2])
				axisindex = 2;
			// the qc gives us 3 axis radius, the longest axis is the capsule
			// axis, since ODE doesn't like this idea we have to create a
			// capsule which uses the standard orientation, and apply a
			// transform to it
			if (axisindex == 0)
			{
				Matrix4x4_CreateFromQuakeEntity(&ed->priv.server->ode_offsetmatrix, geomcenter[0], geomcenter[1], geomcenter[2], 0, 0, 90, 1);
				radius = min(geomsize[1], geomsize[2]) * 0.5f;
			}
			else if (axisindex == 1)
			{
				Matrix4x4_CreateFromQuakeEntity(&ed->priv.server->ode_offsetmatrix, geomcenter[0], geomcenter[1], geomcenter[2], 90, 0, 0, 1);
				radius = min(geomsize[0], geomsize[2]) * 0.5f;
			}
			else
			{
				Matrix4x4_CreateFromQuakeEntity(&ed->priv.server->ode_offsetmatrix, geomcenter[0], geomcenter[1], geomcenter[2], 0, 0, 0, 1);
				radius = min(geomsize[0], geomsize[1]) * 0.5f;
			}
			length = geomsize[axisindex] - radius*2;
			// because we want to support more than one axisindex, we have to
			// create a transform, and turn on its cleanup setting (which will
			// cause the child to be destroyed when it is destroyed)
			ed->priv.server->ode_geom = (void *)dCreateCapsule((dSpaceID)world->physics.ode_space, radius, length);
			dMassSetCapsuleTotal(&mass, massval, axisindex+1, radius, length);
			break;
		case GEOMTYPE_CAPSULE_X:
			Matrix4x4_CreateFromQuakeEntity(&ed->priv.server->ode_offsetmatrix, geomcenter[0], geomcenter[1], geomcenter[2], 0, 0, 90, 1);
			radius = min(geomsize[1], geomsize[2]) * 0.5f;
			length = geomsize[0] - radius*2;
			// check if length is not enough, reduce radius then
			if (length <= 0)
			{
				radius -= (1 - length)*0.5;
				length = 1;
			}
			ed->priv.server->ode_geom = (void *)dCreateCapsule((dSpaceID)world->physics.ode_space, radius, length);
			dMassSetCapsuleTotal(&mass, massval, 1, radius, length);
			break;
		case GEOMTYPE_CAPSULE_Y:
			Matrix4x4_CreateFromQuakeEntity(&ed->priv.server->ode_offsetmatrix, geomcenter[0], geomcenter[1], geomcenter[2], 90, 0, 0, 1);
			radius = min(geomsize[0], geomsize[2]) * 0.5f;
			length = geomsize[1] - radius*2;
			// check if length is not enough, reduce radius then
			if (length <= 0)
			{
				radius -= (1 - length)*0.5;
				length = 1;
			}
			ed->priv.server->ode_geom = (void *)dCreateCapsule((dSpaceID)world->physics.ode_space, radius, length);
			dMassSetCapsuleTotal(&mass, massval, 2, radius, length);
			break;
		case GEOMTYPE_CAPSULE_Z:
			Matrix4x4_CreateFromQuakeEntity(&ed->priv.server->ode_offsetmatrix, geomcenter[0], geomcenter[1], geomcenter[2], 0, 0, 0, 1);
			radius = min(geomsize[1], geomsize[0]) * 0.5f;
			length = geomsize[2] - radius*2;
			// check if length is not enough, reduce radius then
			if (length <= 0)
			{
				radius -= (1 - length)*0.5;
				length = 1;
			}
			ed->priv.server->ode_geom = (void *)dCreateCapsule((dSpaceID)world->physics.ode_space, radius, length);
			dMassSetCapsuleTotal(&mass, massval, 3, radius, length);
			break;
		case GEOMTYPE_CYLINDER:
			axisindex = 0;
			if (geomsize[axisindex] < geomsize[1])
				axisindex = 1;
			if (geomsize[axisindex] < geomsize[2])
				axisindex = 2;
			// the qc gives us 3 axis radius, the longest axis is the capsule
			// axis, since ODE doesn't like this idea we have to create a
			// capsule which uses the standard orientation, and apply a
			// transform to it
			if (axisindex == 0)
			{
				Matrix4x4_CreateFromQuakeEntity(&ed->priv.server->ode_offsetmatrix, geomcenter[0], geomcenter[1], geomcenter[2], 0, 0, 90, 1);
				radius = min(geomsize[1], geomsize[2]) * 0.5f;
			}
			else if (axisindex == 1)
			{
				Matrix4x4_CreateFromQuakeEntity(&ed->priv.server->ode_offsetmatrix, geomcenter[0], geomcenter[1], geomcenter[2], 90, 0, 0, 1);
				radius = min(geomsize[0], geomsize[2]) * 0.5f;
			}
			else
			{
				Matrix4x4_CreateFromQuakeEntity(&ed->priv.server->ode_offsetmatrix, geomcenter[0], geomcenter[1], geomcenter[2], 0, 0, 0, 1);
				radius = min(geomsize[0], geomsize[1]) * 0.5f;
			}
			length = geomsize[axisindex];
			// check if length is not enough, reduce radius then
			if (length <= 0)
			{
				radius -= (1 - length)*0.5;
				length = 1;
			}
			ed->priv.server->ode_geom = (void *)dCreateCylinder((dSpaceID)world->physics.ode_space, radius, length);
			dMassSetCylinderTotal(&mass, massval, axisindex+1, radius, length);
			break;
		case GEOMTYPE_CYLINDER_X:
			Matrix4x4_CreateFromQuakeEntity(&ed->priv.server->ode_offsetmatrix, geomcenter[0], geomcenter[1], geomcenter[2], 0, 0, 90, 1);
			radius = min(geomsize[1], geomsize[2]) * 0.5f;
			length = geomsize[0];
			ed->priv.server->ode_geom = (void *)dCreateCylinder((dSpaceID)world->physics.ode_space, radius, length);
			dMassSetCylinderTotal(&mass, massval, 1, radius, length);
			break;
		case GEOMTYPE_CYLINDER_Y:
			Matrix4x4_CreateFromQuakeEntity(&ed->priv.server->ode_offsetmatrix, geomcenter[0], geomcenter[1], geomcenter[2], 90, 0, 0, 1);
			radius = min(geomsize[0], geomsize[2]) * 0.5f;
			length = geomsize[1];
			ed->priv.server->ode_geom = (void *)dCreateCylinder((dSpaceID)world->physics.ode_space, radius, length);
			dMassSetCylinderTotal(&mass, massval, 2, radius, length);
			break;
		case GEOMTYPE_CYLINDER_Z:
			Matrix4x4_CreateFromQuakeEntity(&ed->priv.server->ode_offsetmatrix, geomcenter[0], geomcenter[1], geomcenter[2], 0, 0, 0, 1);
			radius = min(geomsize[0], geomsize[1]) * 0.5f;
			length = geomsize[2];
			ed->priv.server->ode_geom = (void *)dCreateCylinder((dSpaceID)world->physics.ode_space, radius, length);
			dMassSetCylinderTotal(&mass, massval, 3, radius, length);
			break;
		default:
			Sys_Error("World_Physics_BodyFromEntity: unrecognized geomtype value %i was accepted by filter\n", solid);
			// this goto only exists to prevent warnings from the compiler
			// about uninitialized variables (mass), while allowing it to
			// catch legitimate uninitialized variable warnings
			goto treatasbox;
		}
		ed->priv.server->ode_mass = massval;
		ed->priv.server->ode_modelindex = modelindex;
		VectorCopy(entmins, ed->priv.server->ode_mins);
		VectorCopy(entmaxs, ed->priv.server->ode_maxs);
		VectorCopy(scale, ed->priv.server->ode_scale);
		ed->priv.server->ode_movelimit = min(geomsize[0], min(geomsize[1], geomsize[2]));
		Matrix4x4_Invert_Simple(&ed->priv.server->ode_offsetimatrix, &ed->priv.server->ode_offsetmatrix);
		ed->priv.server->ode_massbuf = Mem_Alloc(mempool, sizeof(mass));
		memcpy(ed->priv.server->ode_massbuf, &mass, sizeof(dMass));
	}

	if (ed->priv.server->ode_geom)
		dGeomSetData((dGeomID)ed->priv.server->ode_geom, (void*)ed);
	if (movetype == MOVETYPE_PHYSICS && ed->priv.server->ode_geom)
	{
		// entity is dynamic
		if (ed->priv.server->ode_body == nullptr)
		{
			ed->priv.server->ode_body = (void *)(body = dBodyCreate((dWorldID)world->physics.ode_world));
			dGeomSetBody((dGeomID)ed->priv.server->ode_geom, body);
			dBodySetData(body, (void*)ed);
			dBodySetMass(body, (dMass *) ed->priv.server->ode_massbuf);
			modified = true;
		}
	}
	else
	{
		// entity is deactivated
		if (ed->priv.server->ode_body != nullptr)
		{
			if(ed->priv.server->ode_geom)
				dGeomSetBody((dGeomID)ed->priv.server->ode_geom, 0);
			dBodyDestroy((dBodyID) ed->priv.server->ode_body);
			ed->priv.server->ode_body = nullptr;
			modified = true;
		}
	}

	// get current data from entity
	VectorClear(origin);
	VectorClear(velocity);

	VectorClear(angles);
	VectorClear(avelocity);
	gravity = true;
	VectorCopy(PRVM_gameedictvector(ed, origin), origin);
	VectorCopy(PRVM_gameedictvector(ed, velocity), velocity);

	VectorCopy(PRVM_gameedictvector(ed, angles), angles);
	VectorCopy(PRVM_gameedictvector(ed, avelocity), avelocity);
	if (PRVM_gameedictfloat(ed, gravity) != 0.0f && PRVM_gameedictfloat(ed, gravity) < 0.5f) gravity = false;
	if (ed == prog->edicts)
		gravity = false;


	{
		float pitchsign = 1;
		vec3_t qangles, qavelocity;
		VectorCopy(angles, qangles);
		VectorCopy(avelocity, qavelocity);

		if(prog == SVVM_prog) // FIXME some better way?
		{
			pitchsign = SV_GetPitchSign(prog, ed);
		}
		else if(prog == CLVM_prog)
		{
			pitchsign = CL_GetPitchSign(prog, ed);
		}
		qangles[PITCH] *= pitchsign;
		qavelocity[PITCH] *= pitchsign;

		AngleVectorsFLU(qangles, forward, left, up);
		// convert single-axis rotations in avelocity to spinvelocity
		// FIXME: untested math - check signs
		VectorSet(spinvelocity, DEG2RAD(qavelocity[PITCH]), DEG2RAD(qavelocity[ROLL]), DEG2RAD(qavelocity[YAW]));
	}

	// compatibility for legacy entities
	switch (solid)
	{
	case SOLID_BBOX:
	case SOLID_SLIDEBOX:
	case SOLID_CORPSE:
		VectorSet(forward, 1, 0, 0);
		VectorSet(left, 0, 1, 0);
		VectorSet(up, 0, 0, 1);
		VectorSet(spinvelocity, 0, 0, 0);
		break;
	}


	// we must prevent NANs...
	if (physics_ode_trick_fixnan.integer)
	{
		test = VectorLength2(origin) + VectorLength2(forward) + VectorLength2(left) + VectorLength2(up) + VectorLength2(velocity) + VectorLength2(spinvelocity);
		if (VEC_IS_NAN(test))
		{
			modified = true;
			if (unlikely(physics_ode_trick_fixnan.integer >= 2))
				Con_Printf("Fixing NAN values on entity %i : .classname = \"%s\" .origin = '%f %f %f' .velocity = '%f %f %f' .angles = '%f %f %f' .avelocity = '%f %f %f'\n", PRVM_NUM_FOR_EDICT(ed), PRVM_GetString(prog, PRVM_gameedictstring(ed, classname)), origin[0], origin[1], origin[2], velocity[0], velocity[1], velocity[2], angles[0], angles[1], angles[2], avelocity[0], avelocity[1], avelocity[2]);
			test = VectorLength2(origin);
			if (VEC_IS_NAN(test))
				VectorClear(origin);
			test = VectorLength2(forward) * VectorLength2(left) * VectorLength2(up);
			if (VEC_IS_NAN(test))
			{
				VectorSet(angles, 0, 0, 0);
				VectorSet(forward, 1, 0, 0);
				VectorSet(left, 0, 1, 0);
				VectorSet(up, 0, 0, 1);
			}
			test = VectorLength2(velocity);
			if (VEC_IS_NAN(test))
				VectorClear(velocity);
			test = VectorLength2(spinvelocity);
			if (VEC_IS_NAN(test))
			{
				VectorClear(avelocity);
				VectorClear(spinvelocity);
			}
		}
	}

	// check if the qc edited any position data
	if (!VectorCompare(origin, ed->priv.server->ode_origin)
	 || !VectorCompare(velocity, ed->priv.server->ode_velocity)
	 || !VectorCompare(angles, ed->priv.server->ode_angles)
	 || !VectorCompare(avelocity, ed->priv.server->ode_avelocity)
	 || gravity != ed->priv.server->ode_gravity)
		modified = true;

	// store the qc values into the physics engine
	body = (dBodyID)ed->priv.server->ode_body;
	if (modified && ed->priv.server->ode_geom)
	{
		dVector3 r[3];
		matrix4x4_t entitymatrix;
		matrix4x4_t bodymatrix;


		// values for BodyFromEntity to check if the qc modified anything later
		VectorCopy(origin, ed->priv.server->ode_origin);
		VectorCopy(velocity, ed->priv.server->ode_velocity);
		VectorCopy(angles, ed->priv.server->ode_angles);
		VectorCopy(avelocity, ed->priv.server->ode_avelocity);
		ed->priv.server->ode_gravity = gravity;

		Matrix4x4_FromVectors(&entitymatrix, forward, left, up, origin);
		Matrix4x4_Concat(&bodymatrix, &entitymatrix, &ed->priv.server->ode_offsetmatrix);
		Matrix4x4_ToVectors(&bodymatrix, forward, left, up, origin);
		r[0][0] = forward[0];
		r[1][0] = forward[1];
		r[2][0] = forward[2];
		r[0][1] = left[0];
		r[1][1] = left[1];
		r[2][1] = left[2];
		r[0][2] = up[0];
		r[1][2] = up[1];
		r[2][2] = up[2];
		if (body)
		{
			if (movetype == MOVETYPE_PHYSICS)
			{
				dGeomSetBody((dGeomID)ed->priv.server->ode_geom, body);
				dBodySetPosition(body, origin[0], origin[1], origin[2]);
				dBodySetRotation(body, r[0]);
				dBodySetLinearVel(body, velocity[0], velocity[1], velocity[2]);
				dBodySetAngularVel(body, spinvelocity[0], spinvelocity[1], spinvelocity[2]);
				dBodySetGravityMode(body, gravity);
			}
			else
			{
				dGeomSetBody((dGeomID)ed->priv.server->ode_geom, body);
				dBodySetPosition(body, origin[0], origin[1], origin[2]);
				dBodySetRotation(body, r[0]);
				dBodySetLinearVel(body, velocity[0], velocity[1], velocity[2]);
				dBodySetAngularVel(body, spinvelocity[0], spinvelocity[1], spinvelocity[2]);
				dBodySetGravityMode(body, gravity);
				dGeomSetBody((dGeomID)ed->priv.server->ode_geom, 0);
			}
		}
		else
		{
			// no body... then let's adjust the parameters of the geom directly
			dGeomSetBody((dGeomID)ed->priv.server->ode_geom, 0); // just in case we previously HAD a body (which should never happen)
			dGeomSetPosition((dGeomID)ed->priv.server->ode_geom, origin[0], origin[1], origin[2]);
			dGeomSetRotation((dGeomID)ed->priv.server->ode_geom, r[0]);
		}
	}

	if (body)
	{

		// limit movement speed to prevent missed collisions at high speed
		ovelocity = dBodyGetLinearVel(body);
		ospinvelocity = dBodyGetAngularVel(body);
		movelimit = ed->priv.server->ode_movelimit * world->physics.ode_movelimit;
		test = VectorLength2(ovelocity);
		if (test > movelimit*movelimit)
		{
			// scale down linear velocity to the movelimit
			// scale down angular velocity the same amount for consistency
			f = movelimit / sqrt(test);
			VectorScale(ovelocity, f, velocity);
			VectorScale(ospinvelocity, f, spinvelocity);
			dBodySetLinearVel(body, velocity[0], velocity[1], velocity[2]);
			dBodySetAngularVel(body, spinvelocity[0], spinvelocity[1], spinvelocity[2]);
		}

		// make sure the angular velocity is not exploding
		spinlimit = physics_ode_spinlimit.value;
		test = VectorLength2(ospinvelocity);
		if (test > spinlimit)
		{
			dBodySetAngularVel(body, 0, 0, 0);
		}

		// apply functions and clear stack
		for(func = ed->priv.server->ode_func; func; func = nextf)
		{
			nextf = func->next;
			World_Physics_ApplyCmd(ed, func);
			Mem_Free(func);
		}
		ed->priv.server->ode_func = nullptr;
	}
}

#define MAX_CONTACTS 32
static void nearCallback (void *data, dGeomID o1, dGeomID o2)
{
	world_t *world = (world_t *)data;
	prvm_prog_t *prog = world->prog;
	dContact contact[MAX_CONTACTS]; // max contacts per collision pair
	int b1enabled = 0, b2enabled = 0;
	dBodyID b1, b2;
	dJointID c;
	//int i;
	int numcontacts;
	float bouncefactor1 = 0.0f;
	float bouncestop1 = 60.0f / 800.0f;
	float bouncefactor2 = 0.0f;
	float bouncestop2 = 60.0f / 800.0f;
	float erp;
	dVector3 grav;
	prvm_edict_t *ed1, *ed2;

	if (dGeomIsSpace(o1) || dGeomIsSpace(o2))
	{
		// colliding a space with something
		dSpaceCollide2(o1, o2, data, &nearCallback);
		return;
	}

	b1 = dGeomGetBody(o1);
	if (b1)
		b1enabled = dBodyIsEnabled(b1);
	b2 = dGeomGetBody(o2);
	if (b2)
		b2enabled = dBodyIsEnabled(b2);

	// at least one object has to be using MOVETYPE_PHYSICS and should be enabled or we just don't care
	if (!b1enabled && !b2enabled)
		return;
	
	// exit without doing anything if the two bodies are connected by a joint
	if (b1 && b2 && dAreConnectedExcluding(b1, b2, dJointTypeContact))
		return;

	ed1 = (prvm_edict_t *) dGeomGetData(o1);
	if(ed1 && ed1->priv.server->free)
		ed1 = nullptr;
	if(ed1)
	{
		bouncefactor1 = PRVM_gameedictfloat(ed1, bouncefactor);
		bouncestop1 = PRVM_gameedictfloat(ed1, bouncestop);
		if (!bouncestop1)
			bouncestop1 = 60.0f / 800.0f;
	}

	ed2 = (prvm_edict_t *) dGeomGetData(o2);
	if(ed2 && ed2->priv.server->free)
		ed2 = nullptr;
	if(ed2)
	{
		bouncefactor2 = PRVM_gameedictfloat(ed2, bouncefactor);
		bouncestop2 = PRVM_gameedictfloat(ed2, bouncestop);
		if (!bouncestop2)
			bouncestop2 = 60.0f / 800.0f;
	}

	if(prog == SVVM_prog)
	{
		if(ed1 && PRVM_serveredictfunction(ed1, touch))
			SV_LinkEdict_TouchAreaGrid_Call(ed1, ed2 ? ed2 : prog->edicts);
			
		if(ed2 && PRVM_serveredictfunction(ed2, touch))
			SV_LinkEdict_TouchAreaGrid_Call(ed2, ed1 ? ed1 : prog->edicts);
	}

	// merge bounce factors and bounce stop
	if(bouncefactor2 > 0)
	{
		if(bouncefactor1 > 0)
		{
			// TODO possibly better logic to merge bounce factor data?
			if(bouncestop2 < bouncestop1)
				bouncestop1 = bouncestop2;
			if(bouncefactor2 > bouncefactor1)
				bouncefactor1 = bouncefactor2;
		}
		else
		{
			bouncestop1 = bouncestop2;
			bouncefactor1 = bouncefactor2;
		}
	}
	dWorldGetGravity((dWorldID)world->physics.ode_world, grav);
	bouncestop1 *= fabs(grav[2]);

	// get erp
	// select object that moves faster ang get it's erp
	erp = (VectorLength2(PRVM_gameedictvector(ed1, velocity)) > VectorLength2(PRVM_gameedictvector(ed2, velocity))) ? PRVM_gameedictfloat(ed1, erp) : PRVM_gameedictfloat(ed2, erp);

	// get max contact points for this collision
	numcontacts = (int)PRVM_gameedictfloat(ed1, maxcontacts);
	if (!numcontacts)
		numcontacts = physics_ode_contact_maxpoints.integer;
	if (PRVM_gameedictfloat(ed2, maxcontacts))
		numcontacts = max(numcontacts, (int)PRVM_gameedictfloat(ed2, maxcontacts));
	else
		numcontacts = max(numcontacts, physics_ode_contact_maxpoints.integer);

	// generate contact points between the two non-space geoms
	numcontacts = dCollide(o1, o2, min(MAX_CONTACTS, numcontacts), &(contact[0].geom), sizeof(contact[0]));
	// add these contact points to the simulation
	for (size_t i = 0; i < numcontacts; i++)
	{
		contact[i].surface.mode = (physics_ode_contact_mu.value != -1 ? dContactApprox1 : 0) | (physics_ode_contact_erp.value != -1 ? dContactSoftERP : 0) | (physics_ode_contact_cfm.value != -1 ? dContactSoftCFM : 0) | (bouncefactor1 > 0 ? dContactBounce : 0);
		contact[i].surface.mu = physics_ode_contact_mu.value * ed1->priv.server->ode_friction * ed2->priv.server->ode_friction;
		contact[i].surface.soft_erp = physics_ode_contact_erp.value + erp;
		contact[i].surface.soft_cfm = physics_ode_contact_cfm.value;
		contact[i].surface.bounce = bouncefactor1;
		contact[i].surface.bounce_vel = bouncestop1;
		c = dJointCreateContact((dWorldID)world->physics.ode_world, (dJointGroupID)world->physics.ode_contactgroup, contact + i);
		dJointAttach(c, b1, b2);
	}
}

void World_Physics_Frame(World *world, double frametime, double gravity)
{
	prvm_prog_t *prog = world->prog;
	double tdelta, tdelta2, tdelta3, simulationtime, collisiontime;

	tdelta = Sys_DirtyTime();
	if (!world->physics.ode || !physics_ode.integer)
		return;

	int i;
	prvm_edict_t *ed;

	if (!physics_ode_constantstep.value)
	{
		world->physics.ode_iterations = bound(1, physics_ode_iterationsperframe.integer, 1000);
		world->physics.ode_step = frametime / world->physics.ode_iterations;
	}
	else
	{
		world->physics.ode_time += frametime;
		// step size
		if (physics_ode_constantstep.value > 0 && physics_ode_constantstep.value < 1)
			world->physics.ode_step = physics_ode_constantstep.value;
		else
			world->physics.ode_step = sys_ticrate.value;
		if (world->physics.ode_time > 0.2f)
			world->physics.ode_time = world->physics.ode_step;
		// set number of iterations to process
		world->physics.ode_iterations = 0;
		while(world->physics.ode_time >= world->physics.ode_step)
		{
			world->physics.ode_iterations++;
			world->physics.ode_time -= world->physics.ode_step;
		}
	}	
	world->physics.ode_movelimit = physics_ode_movelimit.value / world->physics.ode_step;
	World_Physics_UpdateODE(world);

	// copy physics properties from entities to physics engine
	if (prog)
	{
		for (i = 0, ed = prog->edicts + i;i < prog->num_edicts;i++, ed++)
			if (!prog->edicts[i].priv.required->free)
				World_Physics_Frame_BodyFromEntity(world, ed);
		// oh, and it must be called after all bodies were created
		for (i = 0, ed = prog->edicts + i;i < prog->num_edicts;i++, ed++)
			if (!prog->edicts[i].priv.required->free)
				World_Physics_Frame_JointFromEntity(world, ed);
	}

	tdelta2 = Sys_DirtyTime();
	collisiontime = 0;
	for (i = 0;i < world->physics.ode_iterations;i++)
	{
		// set the gravity
		dWorldSetGravity((dWorldID)world->physics.ode_world, 0, 0, -gravity * physics_ode_world_gravitymod.value);
		// set the tolerance for closeness of objects
		dWorldSetContactSurfaceLayer((dWorldID)world->physics.ode_world, max(0, physics_ode_contactsurfacelayer.value));
		// run collisions for the current world state, creating JointGroup
		tdelta3 = Sys_DirtyTime();
		dSpaceCollide((dSpaceID)world->physics.ode_space, (void *)world, nearCallback);
		collisiontime += (Sys_DirtyTime() - tdelta3)*10000;
		// apply forces
		if (prog)
		{
			size_t j;
			const size_t numEdicts = prog->num_edicts;
			for (j = 0, ed = prog->edicts + j;j < numEdicts;j++, ed++)
				if (!prog->edicts[j].priv.required->free)
					World_Physics_Frame_ForceFromEntity(world, ed);
		}
		// run physics (move objects, calculate new velocities)
		// be sure not to pass 0 as step time because that causes an ODE error
		dWorldSetQuickStepNumIterations((dWorldID)world->physics.ode_world, bound(1, physics_ode_worldstep_iterations.integer, 200));
		if (world->physics.ode_step > 0)
			dWorldQuickStep((dWorldID)world->physics.ode_world, world->physics.ode_step);
		// clear the JointGroup now that we're done with it
		dJointGroupEmpty((dJointGroupID)world->physics.ode_contactgroup);
	}
	simulationtime = (Sys_DirtyTime() - tdelta2)*10000;

	// copy physics properties from physics engine to entities and do some stats
	if (prog)
	{
		for (i = 1, ed = prog->edicts + i;i < prog->num_edicts;i++, ed++)
			if (!prog->edicts[i].priv.required->free)
				World_Physics_Frame_BodyToEntity(world, ed);

		// print stats
		if (unlikely(physics_ode_printstats.integer))
		{
			dBodyID body;

			world->physics.ode_numobjects = 0;
			world->physics.ode_activeovjects = 0;
			for (i = 1, ed = prog->edicts + i;i < prog->num_edicts;i++, ed++)
			{
				if (prog->edicts[i].priv.required->free)
					continue;
				body = (dBodyID)prog->edicts[i].priv.server->ode_body;
				if (!body)
					continue;
				world->physics.ode_numobjects++;
				if (dBodyIsEnabled(body))
					world->physics.ode_activeovjects++;
			}
			Con_Printf("ODE Stats(%s): %i iterations, %3.01f (%3.01f collision) %3.01f total : %i objects %i active %i disabled\n", prog->name, world->physics.ode_iterations, simulationtime, collisiontime, (Sys_DirtyTime() - tdelta)*10000, world->physics.ode_numobjects, world->physics.ode_activeovjects, (world->physics.ode_numobjects - world->physics.ode_activeovjects));
		}
	}
}
