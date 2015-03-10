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

#include "pr_comp.h"			// defs shared with qcc

#define ENTITYGRIDAREAS 16
#define MAX_ENTITYCLUSTERS 16

#define	GEOMTYPE_NONE      -1
#define	GEOMTYPE_SOLID      0
#define	GEOMTYPE_BOX		1
#define	GEOMTYPE_SPHERE		2
#define	GEOMTYPE_CAPSULE	3
#define	GEOMTYPE_TRIMESH	4
#define	GEOMTYPE_CYLINDER	5
#define	GEOMTYPE_CAPSULE_X	6
#define	GEOMTYPE_CAPSULE_Y	7
#define	GEOMTYPE_CAPSULE_Z	8
#define	GEOMTYPE_CYLINDER_X	9
#define	GEOMTYPE_CYLINDER_Y	10
#define	GEOMTYPE_CYLINDER_Z	11

#define JOINTTYPE_NONE      0
#define JOINTTYPE_POINT     1
#define JOINTTYPE_HINGE     2
#define JOINTTYPE_SLIDER    3
#define JOINTTYPE_UNIVERSAL 4
#define JOINTTYPE_HINGE2    5
#define JOINTTYPE_FIXED    -1

#define FORCETYPE_NONE       0
#define FORCETYPE_FORCE      1
#define FORCETYPE_FORCEATPOS 2
#define FORCETYPE_TORQUE     3

#define ODEFUNC_ENABLE		1
#define ODEFUNC_DISABLE		2
#define ODEFUNC_FORCE       3
#define ODEFUNC_TORQUE      4

typedef struct edict_odefunc_s
{
	int type;
	float v1[3];
	float v2[3];
	struct edict_odefunc_s *next;
}edict_odefunc_t;

typedef struct edict_engineprivate_s
{
	// true if this edict is unused
	bool free;
	// sv.time when the object was freed (to prevent early reuse which could
	// mess up client interpolation or obscure severe QuakeC bugs)
	float freetime;
	// mark for the leak detector
	int mark;
	// place in the code where it was allocated (for the leak detector)
	const char *allocation_origin;
	// initially false to prevent projectiles from moving on their first frame
	// (even if they were spawned by an synchronous client think)
	bool move;

	// cached cluster links for quick stationary object visibility checking
	float cullmins[3];
	float cullmaxs[3];
	int pvs_numclusters;
	int pvs_clusterlist[MAX_ENTITYCLUSTERS];

	// physics grid areas this edict is linked into
	link_t areagrid[ENTITYGRIDAREAS];
	// since the areagrid can have multiple references to one entity,
	// we should avoid extensive checking on entities already encountered
	int areagridmarknumber;
	// mins/maxs passed to World_LinkEdict
	float areamins[3];
	float areamaxs[3];

	// PROTOCOL_QUAKE, PROTOCOL_QUAKEDP, PROTOCOL_NEHAHRAMOVIE, PROTOCOL_QUAKEWORLD
	// baseline values
	entity_state_t baseline;

	// LordHavoc: gross hack to make floating items still work
	int suspendedinairflag;

	// cached position to avoid redundant SV_CheckWaterTransition calls on monsters
	bool waterposition_forceupdate; // force an update on this entity (set by SV_PushMove code for moving water entities)
	float waterposition_origin[3]; // updates whenever this changes

	// used by PushMove to keep track of where objects were before they were
	// moved, in case they need to be moved back
	float moved_from[3];
	float moved_fromangles[3];

	framegroupblend_t framegroupblend[MAX_FRAMEGROUPBLENDS];
	frameblend_t frameblend[MAX_FRAMEBLENDS];
	skeleton_t skeleton;

	// physics parameters
	bool ode_physics;
	void *ode_body;
	void *ode_geom;
	void *ode_joint;
	float *ode_vertex3f;
	int *ode_element3i;
	int ode_numvertices;
	int ode_numtriangles;
	edict_odefunc_t *ode_func;
	float ode_mins[3];
	float ode_maxs[3];
	float ode_scale[3];
	vec_t ode_mass;
	float ode_friction;
	float ode_origin[3];
	float ode_velocity[3];
	float ode_angles[3];
	float ode_avelocity[3];
	bool ode_gravity;
	int ode_modelindex;
	vec_t ode_movelimit; // smallest component of (maxs[]-mins[])
	matrix4x4_t ode_offsetmatrix;
	matrix4x4_t ode_offsetimatrix;
	int ode_joint_type;
	int ode_joint_enemy;
	int ode_joint_aiment;
	float ode_joint_origin[3]; // joint anchor
	float ode_joint_angles[3]; // joint axis
	float ode_joint_velocity[3]; // second joint axis
	float ode_joint_movedir[3]; // parameters
	void *ode_massbuf;
}
edict_engineprivate_t;

