#pragma once


static void R_Shadow_DrawWorldShadow_ShadowMap(int numsurfaces, int *surfacelist, const unsigned char *trispvs, const unsigned char *surfacesides)
{
	shadowmesh_t *mesh;

	RSurf_ActiveWorldEntity();

	if (rsurface.rtlight->compiled && r_shadow_realtime_world_compile.integer && r_shadow_realtime_world_compileshadow.integer)
	{
		CHECKGLERROR
		GL_CullFace(GL_NONE);
		mesh = rsurface.rtlight->static_meshchain_shadow_shadowmap;
		for (;mesh;mesh = mesh->next)
		{
			if (!mesh->sidetotals[r_shadow_shadowmapside])
				continue;
			r_refdef.stats[r_stat_lights_shadowtriangles] += mesh->sidetotals[r_shadow_shadowmapside];
			R_Mesh_PrepareVertices_Vertex3f(mesh->numverts, mesh->vertex3f, mesh->vbo_vertexbuffer, mesh->vbooffset_vertex3f);
			R_Mesh_Draw(0, mesh->numverts, mesh->sideoffsets[r_shadow_shadowmapside], mesh->sidetotals[r_shadow_shadowmapside], mesh->element3i, mesh->element3i_indexbuffer, mesh->element3i_bufferoffset, mesh->element3s, mesh->element3s_indexbuffer, mesh->element3s_bufferoffset);
		}
		CHECKGLERROR
	}
	else if (r_refdef.scene.worldentity->model)
		r_refdef.scene.worldmodel->DrawShadowMap(r_shadow_shadowmapside, r_refdef.scene.worldentity, rsurface.rtlight->shadoworigin, nullptr, rsurface.rtlight->radius, numsurfaces, surfacelist, surfacesides, rsurface.rtlight->cached_cullmins, rsurface.rtlight->cached_cullmaxs);

	rsurface.entity = nullptr; // used only by R_GetCurrentTexture and RSurf_ActiveWorldEntity/RSurf_ActiveModelEntity
}

static void R_Shadow_DrawWorldShadow_ShadowVolume(int numsurfaces, int *surfacelist, const unsigned char *trispvs)
{
	bool zpass = false;
	shadowmesh_t *mesh;
	int t, tend;
	int surfacelistindex;
	msurface_t *surface;

	// if triangle neighbors are disabled, shadowvolumes are disabled
	if (r_refdef.scene.worldmodel->brush.shadowmesh ? !r_refdef.scene.worldmodel->brush.shadowmesh->neighbor3i : !r_refdef.scene.worldmodel->surfmesh.data_neighbor3i)
		return;

	RSurf_ActiveWorldEntity();

	if (rsurface.rtlight->compiled && r_shadow_realtime_world_compile.integer && r_shadow_realtime_world_compileshadow.integer)
	{
		CHECKGLERROR
		if (r_shadow_rendermode != R_SHADOW_RENDERMODE_VISIBLEVOLUMES)
		{
			zpass = R_Shadow_UseZPass(r_refdef.scene.worldmodel->normalmins, r_refdef.scene.worldmodel->normalmaxs);
			R_Shadow_RenderMode_StencilShadowVolumes(zpass);
		}
		mesh = zpass ? rsurface.rtlight->static_meshchain_shadow_zpass : rsurface.rtlight->static_meshchain_shadow_zfail;
		for (;mesh;mesh = mesh->next)
		{
			r_refdef.stats[r_stat_lights_shadowtriangles] += mesh->numtriangles;
			R_Mesh_PrepareVertices_Vertex3f(mesh->numverts, mesh->vertex3f, mesh->vbo_vertexbuffer, mesh->vbooffset_vertex3f);
			if (r_shadow_rendermode == R_SHADOW_RENDERMODE_ZPASS_STENCIL)
			{
				// increment stencil if frontface is infront of depthbuffer
				GL_CullFace(r_refdef.view.cullface_back);
				R_SetStencil(true, 255, GL_KEEP, GL_KEEP, GL_INCR, GL_ALWAYS, 128, 255);
				R_Mesh_Draw(0, mesh->numverts, 0, mesh->numtriangles, mesh->element3i, mesh->element3i_indexbuffer, mesh->element3i_bufferoffset, mesh->element3s, mesh->element3s_indexbuffer, mesh->element3s_bufferoffset);
				// decrement stencil if backface is infront of depthbuffer
				GL_CullFace(r_refdef.view.cullface_front);
				R_SetStencil(true, 255, GL_KEEP, GL_KEEP, GL_DECR, GL_ALWAYS, 128, 255);
			}
			else if (r_shadow_rendermode == R_SHADOW_RENDERMODE_ZFAIL_STENCIL)
			{
				// decrement stencil if backface is behind depthbuffer
				GL_CullFace(r_refdef.view.cullface_front);
				R_SetStencil(true, 255, GL_KEEP, GL_DECR, GL_KEEP, GL_ALWAYS, 128, 255);
				R_Mesh_Draw(0, mesh->numverts, 0, mesh->numtriangles, mesh->element3i, mesh->element3i_indexbuffer, mesh->element3i_bufferoffset, mesh->element3s, mesh->element3s_indexbuffer, mesh->element3s_bufferoffset);
				// increment stencil if frontface is behind depthbuffer
				GL_CullFace(r_refdef.view.cullface_back);
				R_SetStencil(true, 255, GL_KEEP, GL_INCR, GL_KEEP, GL_ALWAYS, 128, 255);
			}
			R_Mesh_Draw(0, mesh->numverts, 0, mesh->numtriangles, mesh->element3i, mesh->element3i_indexbuffer, mesh->element3i_bufferoffset, mesh->element3s, mesh->element3s_indexbuffer, mesh->element3s_bufferoffset);
		}
		CHECKGLERROR
	}
	else if (numsurfaces && r_refdef.scene.worldmodel->brush.shadowmesh)
	{
		// use the shadow trispvs calculated earlier by GetLightInfo to cull world triangles on this dynamic light
		R_Shadow_PrepareShadowMark(r_refdef.scene.worldmodel->brush.shadowmesh->numtriangles);
		for (surfacelistindex = 0;surfacelistindex < numsurfaces;surfacelistindex++)
		{
			surface = r_refdef.scene.worldmodel->data_surfaces + surfacelist[surfacelistindex];
			for (t = surface->num_firstshadowmeshtriangle, tend = t + surface->num_triangles;t < tend;t++)
				if (CHECKPVSBIT(trispvs, t))
					shadowmarklist[numshadowmark++] = t;
		}
		R_Shadow_VolumeFromList(r_refdef.scene.worldmodel->brush.shadowmesh->numverts, r_refdef.scene.worldmodel->brush.shadowmesh->numtriangles, r_refdef.scene.worldmodel->brush.shadowmesh->vertex3f, r_refdef.scene.worldmodel->brush.shadowmesh->element3i, r_refdef.scene.worldmodel->brush.shadowmesh->neighbor3i, rsurface.rtlight->shadoworigin, nullptr, rsurface.rtlight->radius + r_refdef.scene.worldmodel->radius*2 + r_shadow_projectdistance.value, numshadowmark, shadowmarklist, r_refdef.scene.worldmodel->normalmins, r_refdef.scene.worldmodel->normalmaxs);
	}
	else if (numsurfaces)
	{
		r_refdef.scene.worldmodel->DrawShadowVolume(r_refdef.scene.worldentity, rsurface.rtlight->shadoworigin, nullptr, rsurface.rtlight->radius, numsurfaces, surfacelist, rsurface.rtlight->cached_cullmins, rsurface.rtlight->cached_cullmaxs);
	}

	rsurface.entity = nullptr; // used only by R_GetCurrentTexture and RSurf_ActiveWorldEntity/RSurf_ActiveModelEntity
}
