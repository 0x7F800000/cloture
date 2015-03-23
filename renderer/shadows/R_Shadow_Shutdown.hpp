#pragma once

static void r_shadow_shutdown()
{
	CHECKGLERROR
	R_Shadow_UncompileWorldLights();

	R_Shadow_FreeShadowMaps();

	r_shadow_usingdeferredprepass = false;
	
	if (r_shadow_prepass_width)
		R_Shadow_FreeDeferred();
		
	r_shadow_prepass_width = r_shadow_prepass_height = 0;

	CHECKGLERROR
	
	r_shadow_bouncegridtexture 			= nullptr;
	r_shadow_bouncegridpixels 			= nullptr;
	r_shadow_bouncegridhighpixels 		= nullptr;
	r_shadow_bouncegridnumpixels 		= 0;
	r_shadow_bouncegriddirectional 		= false;
	r_shadow_attenuationgradienttexture = nullptr;
	r_shadow_attenuation2dtexture 		= nullptr;
	r_shadow_attenuation3dtexture 		= nullptr;
	
	R_FreeTexturePool(&r_shadow_texturepool);
	R_FreeTexturePool(&r_shadow_filters_texturepool);
	
	maxshadowtriangles = 0;
	
	if (shadowelements)
		Mem_Free(shadowelements);
		
	shadowelements = nullptr;
	
	if (shadowvertex3f)
		Mem_Free(shadowvertex3f);
		
	shadowvertex3f = nullptr;
	maxvertexupdate = 0;
	
	if (vertexupdate)
		Mem_Free(vertexupdate);
		
	vertexupdate = nullptr;
	
	if (vertexremap)
		Mem_Free(vertexremap);
		
	vertexremap = nullptr;
	vertexupdatenum = 0;
	maxshadowmark = 0;
	numshadowmark = 0;
	if (shadowmark)
		Mem_Free(shadowmark);
	shadowmark = nullptr;
	if (shadowmarklist)
		Mem_Free(shadowmarklist);
	shadowmarklist = nullptr;
	shadowmarkcount = 0;
	maxshadowsides = 0;
	numshadowsides = 0;
	
	if (shadowsides)
		Mem_Free(shadowsides);
		
	shadowsides = nullptr;
	
	if (shadowsideslist)
		Mem_Free(shadowsideslist);
		
	shadowsideslist = nullptr;
	r_shadow_buffer_numleafpvsbytes = 0;
	
	if (r_shadow_buffer_visitingleafpvs)
		Mem_Free(r_shadow_buffer_visitingleafpvs);
	r_shadow_buffer_visitingleafpvs = nullptr;
	
	if (r_shadow_buffer_leafpvs)
		Mem_Free(r_shadow_buffer_leafpvs);
	r_shadow_buffer_leafpvs = nullptr;
	
	if (r_shadow_buffer_leaflist)
		Mem_Free(r_shadow_buffer_leaflist);
		
	r_shadow_buffer_leaflist = nullptr;
	
	r_shadow_buffer_numsurfacepvsbytes = 0;
	
	if (r_shadow_buffer_surfacepvs)
		Mem_Free(r_shadow_buffer_surfacepvs);
		
	r_shadow_buffer_surfacepvs = nullptr;
	
	if (r_shadow_buffer_surfacelist)
		Mem_Free(r_shadow_buffer_surfacelist);
		
	r_shadow_buffer_surfacelist = nullptr;
	
	if (r_shadow_buffer_surfacesides)
		Mem_Free(r_shadow_buffer_surfacesides);
		
	r_shadow_buffer_surfacesides = nullptr;
	
	r_shadow_buffer_numshadowtrispvsbytes = 0;
	
	if (r_shadow_buffer_shadowtrispvs)
		Mem_Free(r_shadow_buffer_shadowtrispvs);
		
	r_shadow_buffer_numlighttrispvsbytes = 0;
	
	if (r_shadow_buffer_lighttrispvs)
		Mem_Free(r_shadow_buffer_lighttrispvs);
}
