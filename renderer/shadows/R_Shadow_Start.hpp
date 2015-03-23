#pragma once


static void r_shadow_start()
{
	// allocate vertex processing arrays
	r_shadow_bouncegridpixels = nullptr;
	r_shadow_bouncegridhighpixels = nullptr;
	r_shadow_bouncegridnumpixels = 0;
	r_shadow_bouncegridtexture = nullptr;
	r_shadow_bouncegriddirectional = false;
	r_shadow_attenuationgradienttexture = nullptr;
	r_shadow_attenuation2dtexture = nullptr;
	r_shadow_attenuation3dtexture = nullptr;
	r_shadow_shadowmode = R_SHADOW_SHADOWMODE_STENCIL;
	r_shadow_shadowmap2ddepthtexture = nullptr;
	r_shadow_shadowmap2ddepthbuffer = nullptr;
	r_shadow_shadowmapvsdcttexture = nullptr;
	r_shadow_shadowmapmaxsize = 0;
	r_shadow_shadowmapsize = 0;
	r_shadow_shadowmaplod = 0;
	r_shadow_shadowmapfilterquality = -1;
	r_shadow_shadowmapdepthbits = 0;
	r_shadow_shadowmapvsdct = false;
	r_shadow_shadowmapsampler = false;
	r_shadow_shadowmappcf = 0;
	r_shadow_fbo2d = 0;

	R_Shadow_FreeShadowMaps();

	r_shadow_texturepool = nullptr;
	r_shadow_filters_texturepool = nullptr;
	R_Shadow_ValidateCvars();
	R_Shadow_MakeTextures();
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
	r_shadow_buffer_numshadowtrispvsbytes = 0;
	r_shadow_buffer_shadowtrispvs = nullptr;
	r_shadow_buffer_numlighttrispvsbytes = 0;
	r_shadow_buffer_lighttrispvs = nullptr;

	r_shadow_usingdeferredprepass = false;
	r_shadow_prepass_width = r_shadow_prepass_height = 0;
}
