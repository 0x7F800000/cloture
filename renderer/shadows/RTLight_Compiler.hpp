#pragma once


// compiles rtlight geometry
// (undone by R_FreeCompiledRTLight, which R_UpdateLight calls)
void R_RTLight_Compile(rtlight_t *rtlight)
{
	int i;
	int numsurfaces, numleafs, numleafpvsbytes, numshadowtrispvsbytes, numlighttrispvsbytes;
	int lighttris, shadowtris, shadowzpasstris, shadowzfailtris;
	entity_render_t *ent = r_refdef.scene.worldentity;
	dp_model_t *model = r_refdef.scene.worldmodel;
	unsigned char *data;
	shadowmesh_t *mesh;

	// compile the light
	rtlight->compiled = true;
	rtlight->shadowmode = rtlight->shadow ? (int)r_shadow_shadowmode : -1;
	rtlight->static_numleafs = 0;
	rtlight->static_numleafpvsbytes = 0;
	rtlight->static_leaflist = nullptr;
	rtlight->static_leafpvs = nullptr;
	rtlight->static_numsurfaces = 0;
	rtlight->static_surfacelist = nullptr;
	rtlight->static_shadowmap_receivers = 0x3F;
	rtlight->static_shadowmap_casters = 0x3F;
	rtlight->cullmins[0] = rtlight->shadoworigin[0] - rtlight->radius;
	rtlight->cullmins[1] = rtlight->shadoworigin[1] - rtlight->radius;
	rtlight->cullmins[2] = rtlight->shadoworigin[2] - rtlight->radius;
	rtlight->cullmaxs[0] = rtlight->shadoworigin[0] + rtlight->radius;
	rtlight->cullmaxs[1] = rtlight->shadoworigin[1] + rtlight->radius;
	rtlight->cullmaxs[2] = rtlight->shadoworigin[2] + rtlight->radius;

	if (model && model->GetLightInfo)
	{
		// this variable must be set for the CompileShadowVolume/CompileShadowMap code
		r_shadow_compilingrtlight = rtlight;
		R_FrameData_SetMark();
		model->GetLightInfo(ent, rtlight->shadoworigin, rtlight->radius, rtlight->cullmins, rtlight->cullmaxs, r_shadow_buffer_leaflist, r_shadow_buffer_leafpvs, &numleafs, r_shadow_buffer_surfacelist, r_shadow_buffer_surfacepvs, &numsurfaces, r_shadow_buffer_shadowtrispvs, r_shadow_buffer_lighttrispvs, r_shadow_buffer_visitingleafpvs, 0, nullptr);
		R_FrameData_ReturnToMark();
		numleafpvsbytes = (model->brush.num_leafs + 7) >> 3;
		numshadowtrispvsbytes = ((model->brush.shadowmesh ? model->brush.shadowmesh->numtriangles : model->surfmesh.num_triangles) + 7) >> 3;
		numlighttrispvsbytes = (model->surfmesh.num_triangles + 7) >> 3;
		data = (unsigned char *)Mem_Alloc(r_main_mempool, sizeof(int) * numsurfaces + sizeof(int) * numleafs + numleafpvsbytes + numshadowtrispvsbytes + numlighttrispvsbytes);
		rtlight->static_numsurfaces = numsurfaces;
		rtlight->static_surfacelist = (int *)data;data += sizeof(int) * numsurfaces;
		rtlight->static_numleafs = numleafs;
		rtlight->static_leaflist = (int *)data;data += sizeof(int) * numleafs;
		rtlight->static_numleafpvsbytes = numleafpvsbytes;
		rtlight->static_leafpvs = (unsigned char *)data;data += numleafpvsbytes;
		rtlight->static_numshadowtrispvsbytes = numshadowtrispvsbytes;
		rtlight->static_shadowtrispvs = (unsigned char *)data;data += numshadowtrispvsbytes;
		rtlight->static_numlighttrispvsbytes = numlighttrispvsbytes;
		rtlight->static_lighttrispvs = (unsigned char *)data;data += numlighttrispvsbytes;
		if (rtlight->static_numsurfaces)
			memcpy(rtlight->static_surfacelist, r_shadow_buffer_surfacelist, rtlight->static_numsurfaces * sizeof(*rtlight->static_surfacelist));
		if (rtlight->static_numleafs)
			memcpy(rtlight->static_leaflist, r_shadow_buffer_leaflist, rtlight->static_numleafs * sizeof(*rtlight->static_leaflist));
		if (rtlight->static_numleafpvsbytes)
			memcpy(rtlight->static_leafpvs, r_shadow_buffer_leafpvs, rtlight->static_numleafpvsbytes);
		if (rtlight->static_numshadowtrispvsbytes)
			memcpy(rtlight->static_shadowtrispvs, r_shadow_buffer_shadowtrispvs, rtlight->static_numshadowtrispvsbytes);
		if (rtlight->static_numlighttrispvsbytes)
			memcpy(rtlight->static_lighttrispvs, r_shadow_buffer_lighttrispvs, rtlight->static_numlighttrispvsbytes);
		R_FrameData_SetMark();
		switch (rtlight->shadowmode)
		{
		case R_SHADOW_SHADOWMODE_SHADOWMAP2D:
			if (model->CompileShadowMap && rtlight->shadow)
				model->CompileShadowMap(ent, rtlight->shadoworigin, nullptr, rtlight->radius, numsurfaces, r_shadow_buffer_surfacelist);
			break;
		default:
			if (model->CompileShadowVolume && rtlight->shadow)
				model->CompileShadowVolume(ent, rtlight->shadoworigin, nullptr, rtlight->radius, numsurfaces, r_shadow_buffer_surfacelist);
			break;
		}
		R_FrameData_ReturnToMark();
		// now we're done compiling the rtlight
		r_shadow_compilingrtlight = nullptr;
	}


	shadowzpasstris = 0;
	if (rtlight->static_meshchain_shadow_zpass)
		for (mesh = rtlight->static_meshchain_shadow_zpass;mesh;mesh = mesh->next)
			shadowzpasstris += mesh->numtriangles;

	shadowzfailtris = 0;
	if (rtlight->static_meshchain_shadow_zfail)
		for (mesh = rtlight->static_meshchain_shadow_zfail;mesh;mesh = mesh->next)
			shadowzfailtris += mesh->numtriangles;

	lighttris = 0;
	if (rtlight->static_numlighttrispvsbytes)
		for (i = 0;i < rtlight->static_numlighttrispvsbytes*8;i++)
			if (CHECKPVSBIT(rtlight->static_lighttrispvs, i))
				lighttris++;

	shadowtris = 0;
	if (rtlight->static_numshadowtrispvsbytes)
		for (i = 0;i < rtlight->static_numshadowtrispvsbytes*8;i++)
			if (CHECKPVSBIT(rtlight->static_shadowtrispvs, i))
				shadowtris++;

	if (unlikely(developer_extra.integer != 0))
		Con_DPrintf("static light built: %f %f %f : %f %f %f box, %i light triangles, %i shadow triangles, %i zpass/%i zfail compiled shadow volume triangles\n", rtlight->cullmins[0], rtlight->cullmins[1], rtlight->cullmins[2], rtlight->cullmaxs[0], rtlight->cullmaxs[1], rtlight->cullmaxs[2], lighttris, shadowtris, shadowzpasstris, shadowzfailtris);
}

void R_RTLight_Uncompile(rtlight_t *rtlight)
{
	if (!rtlight->compiled)
		return;

	if (rtlight->static_meshchain_shadow_zpass)
		Mod_ShadowMesh_Free(rtlight->static_meshchain_shadow_zpass);

	rtlight->static_meshchain_shadow_zpass 		= nullptr;

	if (rtlight->static_meshchain_shadow_zfail)
		Mod_ShadowMesh_Free(rtlight->static_meshchain_shadow_zfail);

	rtlight->static_meshchain_shadow_zfail 		= nullptr;

	if (rtlight->static_meshchain_shadow_shadowmap)
		Mod_ShadowMesh_Free(rtlight->static_meshchain_shadow_shadowmap);

	rtlight->static_meshchain_shadow_shadowmap 	= nullptr;

	// these allocations are grouped
	if (rtlight->static_surfacelist)
		Mem_Free(rtlight->static_surfacelist);

	rtlight->static_numleafs 				= 0;
	rtlight->static_numleafpvsbytes 		= 0;
	rtlight->static_leaflist 				= nullptr;
	rtlight->static_leafpvs 				= nullptr;
	rtlight->static_numsurfaces 			= 0;
	rtlight->static_surfacelist 			= nullptr;
	rtlight->static_numshadowtrispvsbytes 	= 0;
	rtlight->static_shadowtrispvs 			= nullptr;
	rtlight->static_numlighttrispvsbytes 	= 0;
	rtlight->static_lighttrispvs 			= nullptr;
	rtlight->compiled 						= false;

}

void R_Shadow_UncompileWorldLights()
{
	const size32 range = Mem_ExpandableArray_IndexRange(&r_shadow_worldlightsarray); // checked
	for (size32 lightindex = 0; lightindex < range; lightindex++)
	{
		dlight_t *light = (dlight_t *) Mem_ExpandableArray_RecordAtIndex(&r_shadow_worldlightsarray, lightindex);
		if (!light)
			continue;
		R_RTLight_Uncompile(&light->rtlight);
	}
}
