#pragma once

void R_Shadow_Init()
{
	r_shadow_bumpscale_basetexture.Register();
	r_shadow_bumpscale_bumpmap.Register();
	r_shadow_usebihculling.Register();
	r_shadow_usenormalmap.Register();
	r_shadow_debuglight.Register();
	r_shadow_deferred.Register();
	r_shadow_gloss.Register();
	r_shadow_gloss2intensity.Register();
	r_shadow_glossintensity.Register();
	r_shadow_glossexponent.Register();
	r_shadow_gloss2exponent.Register();
	r_shadow_glossexact.Register();
	r_shadow_lightattenuationdividebias.Register();
	r_shadow_lightattenuationlinearscale.Register();
	r_shadow_lightintensityscale.Register();
	r_shadow_lightradiusscale.Register();
	r_shadow_projectdistance.Register();
	r_shadow_frontsidecasting.Register();
	r_shadow_realtime_dlight.Register();
	r_shadow_realtime_dlight_shadows.Register();
	r_shadow_realtime_dlight_svbspculling.Register();
	r_shadow_realtime_dlight_portalculling.Register();
	r_shadow_realtime_world.Register();
	r_shadow_realtime_world_lightmaps.Register();
	r_shadow_realtime_world_shadows.Register();
	r_shadow_realtime_world_compile.Register();
	r_shadow_realtime_world_compileshadow.Register();
	r_shadow_realtime_world_compilesvbsp.Register();
	r_shadow_realtime_world_compileportalculling.Register();
	r_shadow_scissor.Register();
	r_shadow_shadowmapping.Register();
	r_shadow_shadowmapping_vsdct.Register();
	r_shadow_shadowmapping_filterquality.Register();
	r_shadow_shadowmapping_useshadowsampler.Register();
	r_shadow_shadowmapping_depthbits.Register();
	r_shadow_shadowmapping_precision.Register();
	r_shadow_shadowmapping_maxsize.Register();
	r_shadow_shadowmapping_minsize.Register();

	r_shadow_shadowmapping_bordersize.Register();
	r_shadow_shadowmapping_nearclip.Register();
	r_shadow_shadowmapping_bias.Register();
	r_shadow_shadowmapping_polygonfactor.Register();
	r_shadow_shadowmapping_polygonoffset.Register();
	r_shadow_sortsurfaces.Register();
	r_shadow_polygonfactor.Register();
	r_shadow_polygonoffset.Register();
	r_shadow_texture3d.Register();
	r_shadow_bouncegrid.Register();
	r_shadow_bouncegrid_bounceanglediffuse.Register();
	r_shadow_bouncegrid_directionalshading.Register();
	r_shadow_bouncegrid_dlightparticlemultiplier.Register();
	r_shadow_bouncegrid_hitmodels.Register();
	r_shadow_bouncegrid_includedirectlighting.Register();
	r_shadow_bouncegrid_intensity.Register();
	r_shadow_bouncegrid_lightradiusscale.Register();
	r_shadow_bouncegrid_maxbounce.Register();
	r_shadow_bouncegrid_particlebounceintensity.Register();
	r_shadow_bouncegrid_particleintensity.Register();
	r_shadow_bouncegrid_photons.Register();
	r_shadow_bouncegrid_spacing.Register();
	r_shadow_bouncegrid_stablerandom.Register();
	r_shadow_bouncegrid_static.Register();
	r_shadow_bouncegrid_static_directionalshading.Register();
	r_shadow_bouncegrid_static_lightradiusscale.Register();
	r_shadow_bouncegrid_static_maxbounce.Register();
	r_shadow_bouncegrid_static_photons.Register();
	r_shadow_bouncegrid_updateinterval.Register();
	r_shadow_bouncegrid_x.Register();
	r_shadow_bouncegrid_y.Register();
	r_shadow_bouncegrid_z.Register();
	r_coronas.Register();
	r_coronas_occlusionsizescale.Register();
	r_coronas_occlusionquery.Register();
	gl_flashblend.Register();
	gl_ext_separatestencil.Register();
	gl_ext_stenciltwoside.Register();
	R_Shadow_EditLights_Init();
	Mem_ExpandableArray_NewArray(&r_shadow_worldlightsarray, r_main_mempool, sizeof(dlight_t), 128);
	maxshadowtriangles = 0;
	shadowelements = nullptr;
	maxshadowvertices = 0;
	shadowvertex3f = nullptr;
	maxvertexupdate = 0;
	vertexupdate = nullptr;
	vertexremap = nullptr;
	vertexupdatenum = 0;
	maxshadowmark = 0;
	numshadowmark = 0;
	shadowmark = nullptr;
	shadowmarklist = nullptr;
	shadowmarkcount = 0;
	maxshadowsides = 0;
	numshadowsides = 0;
	shadowsides = nullptr;
	shadowsideslist = nullptr;
	r_shadow_buffer_numleafpvsbytes = 0;
	r_shadow_buffer_visitingleafpvs = nullptr;
	r_shadow_buffer_leafpvs = nullptr;
	r_shadow_buffer_leaflist = nullptr;
	r_shadow_buffer_numsurfacepvsbytes = 0;
	r_shadow_buffer_surfacepvs = nullptr;
	r_shadow_buffer_surfacelist = nullptr;
	r_shadow_buffer_surfacesides = nullptr;
	r_shadow_buffer_shadowtrispvs = nullptr;
	r_shadow_buffer_lighttrispvs = nullptr;
	R_RegisterModule("R_Shadow", r_shadow_start, r_shadow_shutdown, r_shadow_newmap, nullptr, nullptr);
}
