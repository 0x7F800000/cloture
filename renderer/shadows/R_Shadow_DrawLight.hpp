#pragma once


#if 0
static void R_Shadow_DrawLight(rtlight_t *rtlight)
{
	int i;
	int numsurfaces;
	unsigned char *shadowtrispvs, *lighttrispvs, *surfacesides;
	int numlightentities;
	int numlightentities_noselfshadow;
	int numshadowentities;
	int numshadowentities_noselfshadow;
	entity_render_t **lightentities;
	entity_render_t **lightentities_noselfshadow;
	entity_render_t **shadowentities;
	entity_render_t **shadowentities_noselfshadow;
	int *surfacelist;
	static unsigned char entitysides[MAX_EDICTS];
	static unsigned char entitysides_noselfshadow[MAX_EDICTS];
	vec3_t nearestpoint;
	vec_t distance;
	bool castshadows;
	int lodlinear;

	// check if we cached this light this frame (meaning it is worth drawing)
	if (!rtlight->draw)
		return;

	numlightentities = rtlight->cached_numlightentities;
	numlightentities_noselfshadow = rtlight->cached_numlightentities_noselfshadow;
	numshadowentities = rtlight->cached_numshadowentities;
	numshadowentities_noselfshadow = rtlight->cached_numshadowentities_noselfshadow;
	numsurfaces = rtlight->cached_numsurfaces;
	lightentities = rtlight->cached_lightentities;
	lightentities_noselfshadow = rtlight->cached_lightentities_noselfshadow;
	shadowentities = rtlight->cached_shadowentities;
	shadowentities_noselfshadow = rtlight->cached_shadowentities_noselfshadow;
	shadowtrispvs = rtlight->cached_shadowtrispvs;
	lighttrispvs = rtlight->cached_lighttrispvs;
	surfacelist = rtlight->cached_surfacelist;

	// set up a scissor rectangle for this light
	if (R_Shadow_ScissorForBBox(rtlight->cached_cullmins, rtlight->cached_cullmaxs))
		return;

	// don't let sound skip if going slow
	if (r_refdef.scene.extraupdate)
		S_ExtraUpdate ();

	// make this the active rtlight for rendering purposes
	R_Shadow_RenderMode_ActiveLight(rtlight);

	if (r_showshadowvolumes.integer && r_refdef.view.showdebug && numsurfaces + numshadowentities + numshadowentities_noselfshadow && rtlight->shadow && (rtlight->isstatic ? r_refdef.scene.rtworldshadows : r_refdef.scene.rtdlightshadows))
	{
		// optionally draw visible shape of the shadow volumes
		// for performance analysis by level designers
		R_Shadow_RenderMode_VisibleShadowVolumes();
		if (numsurfaces)
			R_Shadow_DrawWorldShadow_ShadowVolume(numsurfaces, surfacelist, shadowtrispvs);
		for (i = 0;i < numshadowentities;i++)
			R_Shadow_DrawEntityShadow(shadowentities[i]);
		for (i = 0;i < numshadowentities_noselfshadow;i++)
			R_Shadow_DrawEntityShadow(shadowentities_noselfshadow[i]);
		R_Shadow_RenderMode_VisibleLighting(false, false);
	}

	if (unlikely(r_refdef.view.showdebug) && r_showlighting.integer && numsurfaces + numlightentities + numlightentities_noselfshadow)
	{
		// optionally draw the illuminated areas
		// for performance analysis by level designers
		R_Shadow_RenderMode_VisibleLighting(false, false);
		if (numsurfaces)
			R_Shadow_DrawWorldLight(numsurfaces, surfacelist, lighttrispvs);
		for (i = 0;i < numlightentities;i++)
			R_Shadow_DrawEntityLight(lightentities[i]);
		for (i = 0;i < numlightentities_noselfshadow;i++)
			R_Shadow_DrawEntityLight(lightentities_noselfshadow[i]);
	}

	castshadows = numsurfaces + numshadowentities + numshadowentities_noselfshadow > 0 && rtlight->shadow && (rtlight->isstatic ? r_refdef.scene.rtworldshadows : r_refdef.scene.rtdlightshadows);

	nearestpoint[0] = bound(rtlight->cullmins[0], r_refdef.view.origin[0], rtlight->cullmaxs[0]);
	nearestpoint[1] = bound(rtlight->cullmins[1], r_refdef.view.origin[1], rtlight->cullmaxs[1]);
	nearestpoint[2] = bound(rtlight->cullmins[2], r_refdef.view.origin[2], rtlight->cullmaxs[2]);
	distance = VectorDistance(nearestpoint, r_refdef.view.origin);

	lodlinear = (rtlight->radius * r_shadow_shadowmapping_precision.value) / sqrt(max(1.0f, distance/rtlight->radius));
	lodlinear = bound(r_shadow_shadowmapping_minsize.integer, lodlinear, r_shadow_shadowmapmaxsize);

	if (castshadows && r_shadow_shadowmode == R_SHADOW_SHADOWMODE_SHADOWMAP2D)
	{
		float borderbias;
		int side;
		int size;
		int castermask = 0;
		int receivermask = 0;
		matrix4x4_t radiustolight = rtlight->matrix_worldtolight;
		//Matrix4x4_Abs(&radiustolight);
		radiustolight.fabs();
		r_shadow_shadowmaplod = 0;
		for (i = 1;i < R_SHADOW_SHADOWMAP_NUMCUBEMAPS;i++)
			if ((r_shadow_shadowmapmaxsize >> i) > lodlinear)
				r_shadow_shadowmaplod = i;

		size = bound(r_shadow_shadowmapborder, lodlinear, r_shadow_shadowmapmaxsize);
			
		borderbias = r_shadow_shadowmapborder / (float)(size - r_shadow_shadowmapborder);

		surfacesides = nullptr;
		if (numsurfaces)
		{
			if (rtlight->compiled && r_shadow_realtime_world_compile.integer && r_shadow_realtime_world_compileshadow.integer)
			{
				castermask = rtlight->static_shadowmap_casters;
				receivermask = rtlight->static_shadowmap_receivers;
			}
			else
			{
				surfacesides = r_shadow_buffer_surfacesides;
				for(i = 0;i < numsurfaces;i++)
				{
					msurface_t *surface = r_refdef.scene.worldmodel->data_surfaces + surfacelist[i];
					surfacesides[i] = R_Shadow_CalcBBoxSideMask(surface->mins, surface->maxs, &rtlight->matrix_worldtolight, &radiustolight, borderbias);		
					castermask |= surfacesides[i];
					receivermask |= surfacesides[i];
				}
			}
		}
		if (receivermask < 0x3F) 
		{
			for (i = 0;i < numlightentities;i++)
				receivermask |= R_Shadow_CalcEntitySideMask(lightentities[i], &rtlight->matrix_worldtolight, &radiustolight, borderbias);
			if (receivermask < 0x3F)
				for(i = 0; i < numlightentities_noselfshadow;i++)
					receivermask |= R_Shadow_CalcEntitySideMask(lightentities_noselfshadow[i], &rtlight->matrix_worldtolight, &radiustolight, borderbias);
		}

		receivermask &= R_Shadow_CullFrustumSides(rtlight, size, r_shadow_shadowmapborder);

		if (receivermask)
		{
			for (i = 0;i < numshadowentities;i++)
				castermask |= (entitysides[i] = R_Shadow_CalcEntitySideMask(shadowentities[i], &rtlight->matrix_worldtolight, &radiustolight, borderbias));
			for (i = 0;i < numshadowentities_noselfshadow;i++)
				castermask |= (entitysides_noselfshadow[i] = R_Shadow_CalcEntitySideMask(shadowentities_noselfshadow[i], &rtlight->matrix_worldtolight, &radiustolight, borderbias)); 
		}


		// render shadow casters into 6 sided depth texture
		for (side = 0;side < 6;side++) if (receivermask & (1 << side))
		{
			R_Shadow_RenderMode_ShadowMap(side, receivermask, size);
			if (! (castermask & (1 << side))) continue;
			if (numsurfaces)
				R_Shadow_DrawWorldShadow_ShadowMap(numsurfaces, surfacelist, shadowtrispvs, surfacesides);
			for (i = 0;i < numshadowentities;i++) if (entitysides[i] & (1 << side))
				R_Shadow_DrawEntityShadow(shadowentities[i]);
		}

		if (numlightentities_noselfshadow)
		{
			// render lighting using the depth texture as shadowmap
			// draw lighting in the unmasked areas
			R_Shadow_RenderMode_Lighting(false, false, true);
			for (i = 0;i < numlightentities_noselfshadow;i++)
				R_Shadow_DrawEntityLight(lightentities_noselfshadow[i]);
		}

		// render shadow casters into 6 sided depth texture
		if (numshadowentities_noselfshadow)
		{
			for (side = 0;side < 6;side++) if ((receivermask & castermask) & (1 << side))
			{
				R_Shadow_RenderMode_ShadowMap(side, 0, size);
				for (i = 0;i < numshadowentities_noselfshadow;i++) if (entitysides_noselfshadow[i] & (1 << side))
					R_Shadow_DrawEntityShadow(shadowentities_noselfshadow[i]);
			}
		}

		// render lighting using the depth texture as shadowmap
		// draw lighting in the unmasked areas
		R_Shadow_RenderMode_Lighting(false, false, true);
		// draw lighting in the unmasked areas
		if (numsurfaces)
			R_Shadow_DrawWorldLight(numsurfaces, surfacelist, lighttrispvs);
		for (i = 0;i < numlightentities;i++)
			R_Shadow_DrawEntityLight(lightentities[i]);
	}
	else if (castshadows && vid.stencil)
	{
		// draw stencil shadow volumes to mask off pixels that are in shadow
		// so that they won't receive lighting
		GL_Scissor(r_shadow_lightscissor[0], r_shadow_lightscissor[1], r_shadow_lightscissor[2], r_shadow_lightscissor[3]);
		R_Shadow_ClearStencil();

		if (numsurfaces)
			R_Shadow_DrawWorldShadow_ShadowVolume(numsurfaces, surfacelist, shadowtrispvs);
		for (i = 0;i < numshadowentities;i++)
			R_Shadow_DrawEntityShadow(shadowentities[i]);

		// draw lighting in the unmasked areas
		R_Shadow_RenderMode_Lighting(true, false, false);
		for (i = 0;i < numlightentities_noselfshadow;i++)
			R_Shadow_DrawEntityLight(lightentities_noselfshadow[i]);

		for (i = 0;i < numshadowentities_noselfshadow;i++)
			R_Shadow_DrawEntityShadow(shadowentities_noselfshadow[i]);

		// draw lighting in the unmasked areas
		R_Shadow_RenderMode_Lighting(true, false, false);
		if (numsurfaces)
			R_Shadow_DrawWorldLight(numsurfaces, surfacelist, lighttrispvs);
		for (i = 0;i < numlightentities;i++)
			R_Shadow_DrawEntityLight(lightentities[i]);
	}
	else
	{
		// draw lighting in the unmasked areas
		R_Shadow_RenderMode_Lighting(false, false, false);
		if (numsurfaces)
			R_Shadow_DrawWorldLight(numsurfaces, surfacelist, lighttrispvs);
		for (i = 0;i < numlightentities;i++)
			R_Shadow_DrawEntityLight(lightentities[i]);
		for (i = 0;i < numlightentities_noselfshadow;i++)
			R_Shadow_DrawEntityLight(lightentities_noselfshadow[i]);
	}

	if (r_shadow_usingdeferredprepass)
	{
		// when rendering deferred lighting, we simply rasterize the box
		if (castshadows && r_shadow_shadowmode == R_SHADOW_SHADOWMODE_SHADOWMAP2D)
			R_Shadow_RenderMode_DrawDeferredLight(false, true);
		else if (castshadows && vid.stencil)
			R_Shadow_RenderMode_DrawDeferredLight(true, false);
		else
			R_Shadow_RenderMode_DrawDeferredLight(false, false);
	}
}
#else
static void R_Shadow_DrawLight(rtlight_t *rtlight)
{
	unsigned char *surfacesides;

	static unsigned char entitysides[MAX_EDICTS];
	static unsigned char entitysides_noselfshadow[MAX_EDICTS];

	float distance;
	int lodlinear;

	RenderState* renderState = &r_refdef;
	// check if we cached this light this frame (meaning it is worth drawing)
	if (!rtlight->draw)
		return;

	const size32 numlightentities 					= rtlight->cached_numlightentities;
	const size32 numlightentities_noselfshadow 		= rtlight->cached_numlightentities_noselfshadow;
	const size32 numshadowentities 					= rtlight->cached_numshadowentities;
	const size32 numshadowentities_noselfshadow 	= rtlight->cached_numshadowentities_noselfshadow;
	const size32 numsurfaces 						= rtlight->cached_numsurfaces;
	entity_render_t **lightentities 				= rtlight->cached_lightentities;
	entity_render_t **lightentities_noselfshadow 	= rtlight->cached_lightentities_noselfshadow;
	entity_render_t **shadowentities 				= rtlight->cached_shadowentities;
	entity_render_t **shadowentities_noselfshadow 	= rtlight->cached_shadowentities_noselfshadow;

	uint8 *shadowtrispvs 	= rtlight->cached_shadowtrispvs;
	uint8 *lighttrispvs 	= rtlight->cached_lighttrispvs;
	int *surfacelist 		= rtlight->cached_surfacelist;

	// set up a scissor rectangle for this light
	if (R_Shadow_ScissorForBBox(rtlight->cached_cullmins, rtlight->cached_cullmaxs))
		return;

	// don't let sound skip if going slow
	if (renderState->scene.extraupdate)
		S_ExtraUpdate ();

	// make this the active rtlight for rendering purposes
	R_Shadow_RenderMode_ActiveLight(rtlight);

	if (
			unlikely(renderState->view.showdebug)
			&& r_showshadowvolumes.integer
			&& numsurfaces + numshadowentities + numshadowentities_noselfshadow
			&& rtlight->shadow
			&& (rtlight->isstatic ? renderState->scene.rtworldshadows : renderState->scene.rtdlightshadows))
	{
		// optionally draw visible shape of the shadow volumes
		// for performance analysis by level designers
		R_Shadow_RenderMode_VisibleShadowVolumes();

		if (numsurfaces)
			R_Shadow_DrawWorldShadow_ShadowVolume(numsurfaces, surfacelist, shadowtrispvs);

		for (size32 i = 0; i < numshadowentities; i++)
			R_Shadow_DrawEntityShadow(shadowentities[i]);

		for (size32 i = 0; i < numshadowentities_noselfshadow; i++)
			R_Shadow_DrawEntityShadow(shadowentities_noselfshadow[i]);

		R_Shadow_RenderMode_VisibleLighting(false, false);
	}

	if (
	unlikely(renderState->view.showdebug)
	&& r_showlighting.integer
	&& numsurfaces + numlightentities + numlightentities_noselfshadow
	)
	{
		// optionally draw the illuminated areas
		// for performance analysis by level designers
		R_Shadow_RenderMode_VisibleLighting(false, false);
		if (numsurfaces)
			R_Shadow_DrawWorldLight(numsurfaces, surfacelist, lighttrispvs);
		for (size32 i = 0; i < numlightentities; i++)
			R_Shadow_DrawEntityLight(lightentities[i]);
		for (size32 i = 0; i < numlightentities_noselfshadow; i++)
			R_Shadow_DrawEntityLight(lightentities_noselfshadow[i]);
	}
	vector3f nearestpoint;
	const bool castshadows =
	numsurfaces + numshadowentities + numshadowentities_noselfshadow > 0
	&& rtlight->shadow
	&& (rtlight->isstatic ? renderState->scene.rtworldshadows : renderState->scene.rtdlightshadows);
	{
		const vector3f localCullmins 	= rtlight->cullmins;
		const vector3f localCullmaxs 	= rtlight->cullmaxs;
		const vector3f viewOrigin 		= renderState->view.origin;
		nearestpoint[0] = bound(localCullmins[0], viewOrigin[0], localCullmaxs[0]);
		nearestpoint[1] = bound(localCullmins[1], viewOrigin[1], localCullmaxs[1]);
		nearestpoint[2] = bound(localCullmins[2], viewOrigin[2], localCullmaxs[2]);
	}
	distance = VectorDistance(nearestpoint, renderState->view.origin);

	lodlinear = (rtlight->radius * r_shadow_shadowmapping_precision.value) / math::sqrtf( max( 1.0f, distance / rtlight->radius ));
	lodlinear = bound(r_shadow_shadowmapping_minsize.integer, lodlinear, r_shadow_shadowmapmaxsize);

	if (castshadows && r_shadow_shadowmode == R_SHADOW_SHADOWMODE_SHADOWMAP2D)
	{
		float borderbias;
		int side;
		int size;
		int castermask = 0;
		int receivermask = 0;
		matrix4x4_t radiustolight = rtlight->matrix_worldtolight;
		radiustolight.fabs();
		r_shadow_shadowmaplod = 0;
		for (size32 i = 1; i < R_SHADOW_SHADOWMAP_NUMCUBEMAPS; i++)
			if ((r_shadow_shadowmapmaxsize >> i) > lodlinear)
				r_shadow_shadowmaplod = i;

		size = bound(r_shadow_shadowmapborder, lodlinear, r_shadow_shadowmapmaxsize);

		borderbias = r_shadow_shadowmapborder / static_cast<float>(size - r_shadow_shadowmapborder);

		surfacesides = nullptr;
		if (numsurfaces != 0)
		{
			if (rtlight->compiled && r_shadow_realtime_world_compile.integer && r_shadow_realtime_world_compileshadow.integer)
			{
				castermask = rtlight->static_shadowmap_casters;
				receivermask = rtlight->static_shadowmap_receivers;
			}
			else
			{
				surfacesides = r_shadow_buffer_surfacesides;
				for(size32 i = 0; i < numsurfaces; i++)
				{
					msurface_t *surface = renderState->scene.worldmodel->data_surfaces + surfacelist[i];
					surfacesides[i] = R_Shadow_CalcBBoxSideMask(
						surface->mins,
						surface->maxs,
						&rtlight->matrix_worldtolight,
						&radiustolight,
						borderbias
					);
					castermask 		|= surfacesides[i];
					receivermask 	|= surfacesides[i];
				}
			}
		}
		if (receivermask < 0x3F)
		{
			for (size32 i = 0; i < numlightentities; i++)
			{
				receivermask |= R_Shadow_CalcEntitySideMask(
						lightentities[i],
						&rtlight->matrix_worldtolight,
						&radiustolight,
						borderbias
				);
			}
			if (receivermask < 0x3F)
			{
				for(size32 i = 0; i < numlightentities_noselfshadow; i++)
				{
					receivermask |= R_Shadow_CalcEntitySideMask(
							lightentities_noselfshadow[i],
							&rtlight->matrix_worldtolight,
							&radiustolight,
							borderbias
					);
				}
			}
		}

		receivermask &= R_Shadow_CullFrustumSides(
			rtlight,
			size,
			r_shadow_shadowmapborder
		);

		if (receivermask != 0)
		{
			for (size32 i = 0; i < numshadowentities; i++)
			{
				castermask |= (entitysides[i] = R_Shadow_CalcEntitySideMask(
						shadowentities[i],
						&rtlight->matrix_worldtolight,
						&radiustolight,
						borderbias
				));
			}
			for (size32 i = 0; i < numshadowentities_noselfshadow; i++)
			{
				castermask |= (entitysides_noselfshadow[i] = R_Shadow_CalcEntitySideMask(
						shadowentities_noselfshadow[i],
						&rtlight->matrix_worldtolight,
						&radiustolight,
						borderbias
				));
			}
		}

		{
			int sidemask = 1;
			// render shadow casters into 6 sided depth texture
			for (side = 0;side < 6;side++, sidemask <<= 1)
			{
				if (!(receivermask & sidemask))
					continue;

				R_Shadow_RenderMode_ShadowMap(side, receivermask, size);
				if (! (castermask & sidemask))
					continue;
				if (numsurfaces != 0)
					R_Shadow_DrawWorldShadow_ShadowMap(numsurfaces, surfacelist, shadowtrispvs, surfacesides);

				for (size32 i = 0; i < numshadowentities; i++)
				{
					if (entitysides[i] & sidemask)
						R_Shadow_DrawEntityShadow( shadowentities[i] );
				}

			}
		}
		if (numlightentities_noselfshadow != 0)
		{
			// render lighting using the depth texture as shadowmap
			// draw lighting in the unmasked areas
			R_Shadow_RenderMode_Lighting(false, false, true);
			for (size32 i = 0; i < numlightentities_noselfshadow; i++)
				R_Shadow_DrawEntityLight(lightentities_noselfshadow[i]);
		}

		// render shadow casters into 6 sided depth texture
		if (numshadowentities_noselfshadow != 0)
		{
			int sidemask = 1;
			for (side = 0; side < 6; side++, sidemask <<= 1)
			{
				if (!((receivermask & castermask) & sidemask))
					continue;

				R_Shadow_RenderMode_ShadowMap(side, 0, size);
				for (size32 i = 0; i < numshadowentities_noselfshadow; i++)
				{
					if (entitysides_noselfshadow[i] & sidemask)
						R_Shadow_DrawEntityShadow(shadowentities_noselfshadow[i]);
				}

			}
		}

		// render lighting using the depth texture as shadowmap
		// draw lighting in the unmasked areas
		R_Shadow_RenderMode_Lighting(false, false, true);
		// draw lighting in the unmasked areas
		if (numsurfaces != 0)
			R_Shadow_DrawWorldLight(numsurfaces, surfacelist, lighttrispvs);
		for (size32 i = 0; i < numlightentities; i++)
			R_Shadow_DrawEntityLight(lightentities[i]);
	}
	else if (castshadows && vid.stencil)
	{
		// draw stencil shadow volumes to mask off pixels that are in shadow
		// so that they won't receive lighting
		GL_Scissor(r_shadow_lightscissor[0], r_shadow_lightscissor[1], r_shadow_lightscissor[2], r_shadow_lightscissor[3]);
		R_Shadow_ClearStencil();

		if (numsurfaces)
			R_Shadow_DrawWorldShadow_ShadowVolume(numsurfaces, surfacelist, shadowtrispvs);
		for (size32 i = 0; i < numshadowentities; i++)
			R_Shadow_DrawEntityShadow(shadowentities[i]);

		// draw lighting in the unmasked areas
		R_Shadow_RenderMode_Lighting(true, false, false);
		for (size32 i = 0; i < numlightentities_noselfshadow; i++)
			R_Shadow_DrawEntityLight(lightentities_noselfshadow[i]);

		for (size32 i = 0; i < numshadowentities_noselfshadow; i++)
			R_Shadow_DrawEntityShadow(shadowentities_noselfshadow[i]);

		// draw lighting in the unmasked areas
		R_Shadow_RenderMode_Lighting(true, false, false);
		if (numsurfaces)
			R_Shadow_DrawWorldLight(numsurfaces, surfacelist, lighttrispvs);
		for (size32 i = 0; i < numlightentities; i++)
			R_Shadow_DrawEntityLight(lightentities[i]);
	}
	else
	{
		// draw lighting in the unmasked areas
		R_Shadow_RenderMode_Lighting(false, false, false);
		if (numsurfaces)
			R_Shadow_DrawWorldLight(numsurfaces, surfacelist, lighttrispvs);

		for (size32 i = 0; i < numlightentities; i++)
			R_Shadow_DrawEntityLight(lightentities[i]);

		for (size32 i = 0; i < numlightentities_noselfshadow; i++)
			R_Shadow_DrawEntityLight(lightentities_noselfshadow[i]);
	}

	if (r_shadow_usingdeferredprepass)
	{
		// when rendering deferred lighting, we simply rasterize the box
		if (castshadows && r_shadow_shadowmode == R_SHADOW_SHADOWMODE_SHADOWMAP2D)
			R_Shadow_RenderMode_DrawDeferredLight(false, true);
		else if (castshadows && vid.stencil)
			R_Shadow_RenderMode_DrawDeferredLight(true, false);
		else
			R_Shadow_RenderMode_DrawDeferredLight(false, false);
	}
}
#endif
