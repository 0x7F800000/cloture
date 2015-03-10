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

#include "quakedef.h"
#include "cl_collision.h"

using namespace cloture::util;
using namespace common;
using namespace math::vector;
using namespace generic;

#ifdef MAX_EXPLOSIONS
#define EXPLOSIONGRID 8
#define EXPLOSIONVERTS ((EXPLOSIONGRID+1)*(EXPLOSIONGRID+1))
#define EXPLOSIONTRIS (EXPLOSIONGRID*EXPLOSIONGRID*2)

static size_t numexplosions = 0;

static float explosiontexcoord2f[EXPLOSIONVERTS][2];
static unsigned short explosiontris[EXPLOSIONTRIS][3];
static int explosionnoiseindex[EXPLOSIONVERTS];
static /*vec3_t*/ vector3D explosionpoint[EXPLOSIONVERTS];

//typedef struct explosion_s
__align(16) struct explosion_s
{

	/*vec3_t origin;
	vec3_t vert[EXPLOSIONVERTS];
	vec3_t vertvel[EXPLOSIONVERTS];*/
	vector3D origin;
	vector3D vert[EXPLOSIONVERTS];
	vector3D vertvel[EXPLOSIONVERTS];
	float starttime;
	float endtime;
	float time;
	float alpha;
	float fade;
	bool clipping;
};
using explosion_t = explosion_s;
//explosion_t;

static explosion_t explosion[MAX_EXPLOSIONS];

static rtexture_t	*explosiontexture;

static rtexturepool_t	*explosiontexturepool;
#endif

cvar_t r_explosionclip = {CVAR_SAVE, "r_explosionclip", "1", "enables collision detection for explosion shell (so that it flattens against walls and floors)"};
#ifdef MAX_EXPLOSIONS
static cvar_t r_drawexplosions = {0, "r_drawexplosions", "1", "enables rendering of explosion shells (see also cl_particles_explosions_shell)"};

static void r_explosion_start()
{
	static unsigned char noise1[128][128];
	static unsigned char noise2[128][128];
	static unsigned char noise3[128][128];
	static unsigned char data[128][128][4];

	explosiontexturepool = R_AllocTexturePool();
	explosiontexture = nullptr;
	fractalnoise(&noise1[0][0], 128, 32);
	fractalnoise(&noise2[0][0], 128, 4);
	fractalnoise(&noise3[0][0], 128, 4);

	static constexpr size_t iterationCount = 128;
	for (size_t y = 0; y < iterationCount; y++)
	{
		for (size_t x = 0; x < iterationCount; x++)
		{
			int r, g, b;

			{
				const int j = (noise1[y][x] * noise2[y][x]) * 3 / 256 - 128;
				r = (j * 512) / 256;
				g = (j * 256) / 256;
				b = (j * 128) / 256;
			}
			const int a = noise3[y][x] * 3 - 128;
			data[y][x][2] = bound(0, r, 255);
			data[y][x][1] = bound(0, g, 255);
			data[y][x][0] = bound(0, b, 255);
			data[y][x][3] = bound(0, a, 255);
		}
	}
	explosiontexture = R_LoadTexture2D(
		explosiontexturepool,
		"explosiontexture",
		128,
		128,
		&data[0][0][0],
		TEXTYPE_BGRA,
		TEXF_MIPMAP | TEXF_ALPHA | TEXF_FORCELINEAR,
		-1,
		nullptr
	);

}

static void r_explosion_shutdown()
{
	R_FreeTexturePool(&explosiontexturepool);
}

static void r_explosion_newmap()
{
	numexplosions = 0;
	memset(explosion, 0, sizeof(explosion));
}

static int R_ExplosionVert(int column, const int row)
{
	// top and bottom rows are all one position...
	if (row == 0 || row == EXPLOSIONGRID)
		column = 0;

	const ptrdiff_t i 	= row * (EXPLOSIONGRID + 1) + column;
	const float yaw 	= ((double) column / EXPLOSIONGRID) * M_PI * 2.0;
	const float pitch 	= (((double) row / EXPLOSIONGRID) - .5) * M_PI;
	//changed to use float versions of sin and cos
	#if 0
		explosionpoint[i][0] 	= 	cos(yaw) 	*	cos(pitch);
		explosionpoint[i][1] 	= 	sin(yaw) 	*	cos(pitch);
		explosionpoint[i][2] 	= 	1 			*	-sin(pitch);
	#else
		{
			const float pitchCos	=	cosf(pitch);
			explosionpoint[i][0] 	= 	cosf(yaw) 	*	pitchCos;
			explosionpoint[i][1] 	= 	sinf(yaw) 	*	pitchCos;
		}
		explosionpoint[i][2] 	= 	1.0f 		*	-sinf(pitch);

	#endif

	static constexpr float expGridf	=	static_cast<float>(EXPLOSIONGRID);
	explosiontexcoord2f[i][0] 		=	static_cast<float>(column) 	/	expGridf;
	explosiontexcoord2f[i][1] 		=	static_cast<float>(row) 	/	expGridf;
	explosionnoiseindex[i] 			=	(row % EXPLOSIONGRID) * EXPLOSIONGRID + (column % EXPLOSIONGRID);
	return i;
}
#endif

void R_Explosion_Init()
{
#ifdef MAX_EXPLOSIONS
	size_t i = 0;
	for(int y = 0; y < EXPLOSIONGRID; y++)
	{
		for(int x = 0; x < EXPLOSIONGRID; x++)
		{
			explosiontris[i][0] = R_ExplosionVert(x    , y    );
			explosiontris[i][1] = R_ExplosionVert(x + 1, y    );
			explosiontris[i][2] = R_ExplosionVert(x    , y + 1);
			i++;
			explosiontris[i][0] = R_ExplosionVert(x + 1, y    );
			explosiontris[i][1] = R_ExplosionVert(x + 1, y + 1);
			explosiontris[i][2] = R_ExplosionVert(x    , y + 1);
			i++;
		}
	}

#endif
	Cvar_RegisterVariable(&r_explosionclip);
#ifdef MAX_EXPLOSIONS
	Cvar_RegisterVariable(&r_drawexplosions);

	R_RegisterModule("R_Explosions", r_explosion_start, r_explosion_shutdown, r_explosion_newmap, nullptr, nullptr);
#endif
}

void R_NewExplosion(const vec3_t org)
{
#ifdef MAX_EXPLOSIONS
	int i, j;
	float dist, n;
	//explosion_t *e;
	trace_t trace;
	unsigned char noise[EXPLOSIONGRID*EXPLOSIONGRID];
	fractalnoisequick(noise, EXPLOSIONGRID, 4); // adjust noise grid size according to explosion
	//for (i = 0, e = explosion; i < MAX_EXPLOSIONS; i++, e++)
	for(size_t i = 0; i < MAX_EXPLOSIONS; ++i)
	{
		explosion_t* const e = &explosion[i];
		if (!IS_FLOAT_ZERO(&e->alpha))
			continue;

		numexplosions 	= max(numexplosions, i + 1);
		e->starttime 	= cl.time;
		e->endtime 		= cl.time + cl_explosions_lifetime.value;
		e->time 		= e->starttime;
		e->alpha 		= cl_explosions_alpha_start.value;
		e->fade 		= (cl_explosions_alpha_start.value - cl_explosions_alpha_end.value) / cl_explosions_lifetime.value;
		e->clipping		= r_explosionclip.integer != 0;
		VectorCopy(org, e->origin);
		for (j = 0;j < EXPLOSIONVERTS;j++)
		{
			// calculate start origin and velocity
			n 		= noise[explosionnoiseindex[j]] * (1.0f / 255.0f) + 0.5;
			dist 	= n * cl_explosions_size_start.value;
			VectorMA(e->origin, dist, explosionpoint[j], e->vert[j]);
			dist 	= n * (cl_explosions_size_end.value - cl_explosions_size_start.value) / cl_explosions_lifetime.value;
			VectorScale(explosionpoint[j], dist, e->vertvel[j]);
			// clip start origin
			if (e->clipping)
			{
				trace = CL_TraceLine(
					e->origin,
					e->vert[j],
					MOVE_NOMONSTERS,
					nullptr,
					SUPERCONTENTS_SOLID,
					collision_extendmovelength.value,
					true,
					false,
					nullptr,
					false,
					false
				);
				VectorCopy(trace.endpos, e->vert[i]);
			}
		}
		break;

	}
#endif
}

#ifdef MAX_EXPLOSIONS
static void R_DrawExplosion_TransparentCallback(const entity_render_t *ent, const rtlight_t *rtlight, int numsurfaces, int *surfacelist)
{
	int surfacelistindex = 0;
	const int numtriangles = EXPLOSIONTRIS, numverts = EXPLOSIONVERTS;
	GL_BlendFunc(GL_SRC_ALPHA, GL_ONE);
	GL_DepthMask(false);
	GL_DepthRange(.0f, 1.0f);
	GL_PolygonOffset(r_refdef.polygonfactor, r_refdef.polygonoffset);
	GL_DepthTest(true);
	GL_CullFace(r_refdef.view.cullface_back);
	R_EntityMatrix(reinterpret_cast<const matrix4x4_t*>(&identitymatrix));

	R_SetupShader_Generic(explosiontexture, nullptr, GL_MODULATE, 1, false, false, false);
	for (surfacelistindex = 0;surfacelistindex < numsurfaces;surfacelistindex++)
	{
		const explosion_t *RESTRICT const e = explosion + surfacelist[surfacelistindex];
		// FIXME: this can't properly handle r_refdef.view.colorscale > 1
		GL_Color(
			e->alpha * r_refdef.view.colorscale,
			e->alpha * r_refdef.view.colorscale,
			e->alpha * r_refdef.view.colorscale,
			1.0f
		);
		R_Mesh_PrepareVertices_Generic_Arrays(
			numverts,
			e->vert[0],
			nullptr,
			explosiontexcoord2f[0]
		);
		R_Mesh_Draw(
			0,
			numverts,
			0,
			numtriangles,
			nullptr,
			nullptr,
			0,
			explosiontris[0],
			nullptr,
			0
		);
	}
}

static void R_MoveExplosion(explosion_t *e)
{
	float end[3];
	float frametime = cl.time - e->time;

	e->time = cl.time;
	e->alpha = e->alpha - (e->fade * frametime);
	if (e->alpha < 0 || cl.time > e->endtime)
	{
		e->alpha = 0;
		return;
	}
	for (size_t i = 0; i < EXPLOSIONVERTS; i++)
	{

		if(!e->vertvel[i][0] && !e->vertvel[i][0] && !e->vertvel[i][2])
			continue;
		//if (e->vertvel[i][0] || e->vertvel[i][1] || e->vertvel[i][2])
		//{
		VectorMA(e->vert[i], frametime, e->vertvel[i], end);
		if (e->clipping)
		{
			trace_t trace = CL_TraceLine(
				e->vert[i],
				end,
				MOVE_NOMONSTERS,
				nullptr,
				SUPERCONTENTS_SOLID,
				collision_extendmovelength.value,
				true,
				false,
				nullptr,
				false,
				false
			);
			if (trace.fraction < 1)
			{
				// clip velocity against the wall
				float dot = -DotProduct(e->vertvel[i], trace.plane.normal);
				VectorMA(e->vertvel[i], dot, trace.plane.normal, e->vertvel[i]);
			}
			VectorCopy(trace.endpos, e->vert[i]);
		}
		else
			VectorCopy(end, e->vert[i]);
		//}
	}
}
#endif

void R_DrawExplosions()
{
#ifdef MAX_EXPLOSIONS
	if (unlikely(!r_drawexplosions.integer))
		return;
	size_t i;
	for (i = 0; i < numexplosions; i++)
	{
		explosion_t* RESTRICT const e = &explosion[i];
		if (IS_FLOAT_ZERO(&e->alpha))
				continue;
		R_MoveExplosion(e);
		if (!IS_FLOAT_ZERO(&e->alpha))
		{
			R_MeshQueue_AddTransparent(
				TRANSPARENTSORT_DISTANCE,
				e->origin,
				R_DrawExplosion_TransparentCallback,
				nullptr,
				i,
				nullptr
			);
		}

	}
	while (numexplosions > 0 && explosion[i-1].alpha <= 0)
		numexplosions--;
#endif
}

