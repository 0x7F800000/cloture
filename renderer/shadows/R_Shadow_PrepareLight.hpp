#pragma once

static void R_Shadow_PrepareLight(rtlight_t *rtlight)
{
	int i;
	float f;
	int numleafs, numsurfaces;
	int *leaflist, *surfacelist;
	unsigned char *leafpvs;
	unsigned char *shadowtrispvs;
	unsigned char *lighttrispvs;
	int numlightentities;
	int numlightentities_noselfshadow;
	int numshadowentities;
	int numshadowentities_noselfshadow;
	static entity_render_t *lightentities[MAX_EDICTS];
	static entity_render_t *lightentities_noselfshadow[MAX_EDICTS];
	static entity_render_t *shadowentities[MAX_EDICTS];
	static entity_render_t *shadowentities_noselfshadow[MAX_EDICTS];
	bool nolight;
	bool castshadows;

	rtlight->draw = false;
	rtlight->cached_numlightentities               = 0;
	rtlight->cached_numlightentities_noselfshadow  = 0;
	rtlight->cached_numshadowentities              = 0;
	rtlight->cached_numshadowentities_noselfshadow = 0;
	rtlight->cached_numsurfaces                    = 0;
	rtlight->cached_lightentities                  = nullptr;
	rtlight->cached_lightentities_noselfshadow     = nullptr;
	rtlight->cached_shadowentities                 = nullptr;
	rtlight->cached_shadowentities_noselfshadow    = nullptr;
	rtlight->cached_shadowtrispvs                  = nullptr;
	rtlight->cached_lighttrispvs                   = nullptr;
	rtlight->cached_surfacelist                    = nullptr;

	// skip lights that don't light because of ambientscale+diffusescale+specularscale being 0 (corona only lights)
	// skip lights that are basically invisible (color 0 0 0)
	nolight = VectorLength2(rtlight->color) * (rtlight->ambientscale + rtlight->diffusescale + rtlight->specularscale) < (1.0f / 1048576.0f);

	// loading is done before visibility checks because loading should happen
	// all at once at the start of a level, not when it stalls gameplay.
	// (especially important to benchmarks)
	// compile light
	if (rtlight->isstatic && !nolight && (!rtlight->compiled || (rtlight->shadow && rtlight->shadowmode != (int)r_shadow_shadowmode)) && r_shadow_realtime_world_compile.integer)
	{
		if (rtlight->compiled)
			R_RTLight_Uncompile(rtlight);
		R_RTLight_Compile(rtlight);
	}

	// load cubemap
	rtlight->currentcubemap = rtlight->cubemapname[0] ? R_GetCubemap(rtlight->cubemapname) : r_texture_whitecube;

	// look up the light style value at this time
	f = ((rtlight->style >= 0 && rtlight->style < MAX_LIGHTSTYLES) ? r_refdef.scene.rtlightstylevalue[rtlight->style] : 1) * r_shadow_lightintensityscale.value;
	VectorScale(rtlight->color, f, rtlight->currentcolor);

	// if lightstyle is currently off, don't draw the light
	if (VectorLength2(rtlight->currentcolor) < (1.0f / 1048576.0f))
		return;

	// skip processing on corona-only lights
	if (nolight)
		return;

	// if the light box is offscreen, skip it
	if (R_CullBox(rtlight->cullmins, rtlight->cullmaxs))
		return;

	VectorCopy(rtlight->cullmins, rtlight->cached_cullmins);
	VectorCopy(rtlight->cullmaxs, rtlight->cached_cullmaxs);

	R_Shadow_ComputeShadowCasterCullingPlanes(rtlight);

	// don't allow lights to be drawn if using r_shadow_bouncegrid 2, except if we're using static bouncegrid where dynamic lights still need to draw
	if (r_shadow_bouncegrid.integer == 2 && (rtlight->isstatic || !r_shadow_bouncegrid_static.integer))
		return;

	if (rtlight->compiled && r_shadow_realtime_world_compile.integer)
	{
		// compiled light, world available and can receive realtime lighting
		// retrieve leaf information
		numleafs = rtlight->static_numleafs;
		leaflist = rtlight->static_leaflist;
		leafpvs = rtlight->static_leafpvs;
		numsurfaces = rtlight->static_numsurfaces;
		surfacelist = rtlight->static_surfacelist;
		shadowtrispvs = rtlight->static_shadowtrispvs;
		lighttrispvs = rtlight->static_lighttrispvs;
	}
	else if (r_refdef.scene.worldmodel && r_refdef.scene.worldmodel->GetLightInfo)
	{
		// dynamic light, world available and can receive realtime lighting
		// calculate lit surfaces and leafs
		r_refdef.scene.worldmodel->GetLightInfo(r_refdef.scene.worldentity, rtlight->shadoworigin, rtlight->radius, rtlight->cached_cullmins, rtlight->cached_cullmaxs, r_shadow_buffer_leaflist, r_shadow_buffer_leafpvs, &numleafs, r_shadow_buffer_surfacelist, r_shadow_buffer_surfacepvs, &numsurfaces, r_shadow_buffer_shadowtrispvs, r_shadow_buffer_lighttrispvs, r_shadow_buffer_visitingleafpvs, rtlight->cached_numfrustumplanes, rtlight->cached_frustumplanes);
		R_Shadow_ComputeShadowCasterCullingPlanes(rtlight);
		leaflist = r_shadow_buffer_leaflist;
		leafpvs = r_shadow_buffer_leafpvs;
		surfacelist = r_shadow_buffer_surfacelist;
		shadowtrispvs = r_shadow_buffer_shadowtrispvs;
		lighttrispvs = r_shadow_buffer_lighttrispvs;
		// if the reduced leaf bounds are offscreen, skip it
		if (R_CullBox(rtlight->cached_cullmins, rtlight->cached_cullmaxs))
			return;
	}
	else
	{
		// no world
		numleafs = 0;
		leaflist = nullptr;
		leafpvs = nullptr;
		numsurfaces = 0;
		surfacelist = nullptr;
		shadowtrispvs = nullptr;
		lighttrispvs = nullptr;
	}
	// check if light is illuminating any visible leafs
	if (numleafs)
	{
		for (i = 0;i < numleafs;i++)
			if (r_refdef.viewcache.world_leafvisible[leaflist[i]])
				break;
		if (i == numleafs)
			return;
	}

	// make a list of lit entities and shadow casting entities
	numlightentities = 0;
	numlightentities_noselfshadow = 0;
	numshadowentities = 0;
	numshadowentities_noselfshadow = 0;

	// add dynamic entities that are lit by the light
	for (i = 0;i < r_refdef.scene.numentities;i++)
	{
		dp_model_t *model;
		entity_render_t *ent = r_refdef.scene.entities[i];
		vec3_t org;
		if (!BoxesOverlap(ent->mins, ent->maxs, rtlight->cached_cullmins, rtlight->cached_cullmaxs))
			continue;
		// skip the object entirely if it is not within the valid
		// shadow-casting region (which includes the lit region)
		if (R_CullBoxCustomPlanes(ent->mins, ent->maxs, rtlight->cached_numfrustumplanes, rtlight->cached_frustumplanes))
			continue;
		if (!(model = ent->model))
			continue;
		if (r_refdef.viewcache.entityvisible[i] && model->DrawLight && (ent->flags & RENDER_LIGHT))
		{
			// this entity wants to receive light, is visible, and is
			// inside the light box
			// TODO: check if the surfaces in the model can receive light
			// so now check if it's in a leaf seen by the light
			if (r_refdef.scene.worldmodel && r_refdef.scene.worldmodel->brush.BoxTouchingLeafPVS && !r_refdef.scene.worldmodel->brush.BoxTouchingLeafPVS(r_refdef.scene.worldmodel, leafpvs, ent->mins, ent->maxs))
				continue;
			if (ent->flags & RENDER_NOSELFSHADOW)
				lightentities_noselfshadow[numlightentities_noselfshadow++] = ent;
			else
				lightentities[numlightentities++] = ent;
			// since it is lit, it probably also casts a shadow...
			// about the VectorDistance2 - light emitting entities should not cast their own shadow
			Matrix4x4_OriginFromMatrix(&ent->matrix, org);
			if ((ent->flags & RENDER_SHADOW) && model->DrawShadowVolume && VectorDistance2(org, rtlight->shadoworigin) > 0.1)
			{
				// note: exterior models without the RENDER_NOSELFSHADOW
				// flag still create a RENDER_NOSELFSHADOW shadow but
				// are lit normally, this means that they are
				// self-shadowing but do not shadow other
				// RENDER_NOSELFSHADOW entities such as the gun
				// (very weird, but keeps the player shadow off the gun)
				if (ent->flags & (RENDER_NOSELFSHADOW | RENDER_EXTERIORMODEL))
					shadowentities_noselfshadow[numshadowentities_noselfshadow++] = ent;
				else
					shadowentities[numshadowentities++] = ent;
			}
		}
		else if (ent->flags & RENDER_SHADOW)
		{
			// this entity is not receiving light, but may still need to
			// cast a shadow...
			// TODO: check if the surfaces in the model can cast shadow
			// now check if it is in a leaf seen by the light
			if (r_refdef.scene.worldmodel && r_refdef.scene.worldmodel->brush.BoxTouchingLeafPVS && !r_refdef.scene.worldmodel->brush.BoxTouchingLeafPVS(r_refdef.scene.worldmodel, leafpvs, ent->mins, ent->maxs))
				continue;
			// about the VectorDistance2 - light emitting entities should not cast their own shadow
			Matrix4x4_OriginFromMatrix(&ent->matrix, org);
			if ((ent->flags & RENDER_SHADOW) && model->DrawShadowVolume && VectorDistance2(org, rtlight->shadoworigin) > 0.1)
			{
				if (ent->flags & (RENDER_NOSELFSHADOW | RENDER_EXTERIORMODEL))
					shadowentities_noselfshadow[numshadowentities_noselfshadow++] = ent;
				else
					shadowentities[numshadowentities++] = ent;
			}
		}
	}

	// return if there's nothing at all to light
	if (numsurfaces + numlightentities + numlightentities_noselfshadow == 0)
		return;

	// count this light in the r_speeds
	r_refdef.stats[r_stat_lights]++;

	// flag it as worth drawing later
	rtlight->draw = true;

	// if we have shadows disabled, don't count the shadow entities, this way we don't do the R_AnimCache_GetEntity on each one
	castshadows = numsurfaces + numshadowentities + numshadowentities_noselfshadow > 0 && rtlight->shadow && (rtlight->isstatic ? r_refdef.scene.rtworldshadows : r_refdef.scene.rtdlightshadows);
	if (!castshadows)
		numshadowentities = numshadowentities_noselfshadow = 0;

	// cache all the animated entities that cast a shadow but are not visible
	for (i = 0;i < numshadowentities;i++)
		R_AnimCache_GetEntity(shadowentities[i], false, false);
	for (i = 0;i < numshadowentities_noselfshadow;i++)
		R_AnimCache_GetEntity(shadowentities_noselfshadow[i], false, false);

	// allocate some temporary memory for rendering this light later in the frame
	// reusable buffers need to be copied, static data can be used as-is
	rtlight->cached_numlightentities               = numlightentities;
	rtlight->cached_numlightentities_noselfshadow  = numlightentities_noselfshadow;
	rtlight->cached_numshadowentities              = numshadowentities;
	rtlight->cached_numshadowentities_noselfshadow = numshadowentities_noselfshadow;
	rtlight->cached_numsurfaces                    = numsurfaces;
	rtlight->cached_lightentities                  = (entity_render_t**)R_FrameData_Store(numlightentities*sizeof(entity_render_t*), (void*)lightentities);
	rtlight->cached_lightentities_noselfshadow     = (entity_render_t**)R_FrameData_Store(numlightentities_noselfshadow*sizeof(entity_render_t*), (void*)lightentities_noselfshadow);
	rtlight->cached_shadowentities                 = (entity_render_t**)R_FrameData_Store(numshadowentities*sizeof(entity_render_t*), (void*)shadowentities);
	rtlight->cached_shadowentities_noselfshadow    = (entity_render_t**)R_FrameData_Store(numshadowentities_noselfshadow*sizeof(entity_render_t *), (void*)shadowentities_noselfshadow);
	if (shadowtrispvs == r_shadow_buffer_shadowtrispvs)
	{
		int numshadowtrispvsbytes = (((r_refdef.scene.worldmodel->brush.shadowmesh ? r_refdef.scene.worldmodel->brush.shadowmesh->numtriangles : r_refdef.scene.worldmodel->surfmesh.num_triangles) + 7) >> 3);
		int numlighttrispvsbytes = ((r_refdef.scene.worldmodel->surfmesh.num_triangles + 7) >> 3);
		rtlight->cached_shadowtrispvs                  =   (unsigned char *)R_FrameData_Store(numshadowtrispvsbytes, shadowtrispvs);
		rtlight->cached_lighttrispvs                   =   (unsigned char *)R_FrameData_Store(numlighttrispvsbytes, lighttrispvs);
		rtlight->cached_surfacelist                    =              (int*)R_FrameData_Store(numsurfaces*sizeof(int), (void*)surfacelist);
	}
	else
	{
		// compiled light data
		rtlight->cached_shadowtrispvs = shadowtrispvs;
		rtlight->cached_lighttrispvs = lighttrispvs;
		rtlight->cached_surfacelist = surfacelist;
	}
}
