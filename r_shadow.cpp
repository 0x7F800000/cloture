
/*
Terminology: Stencil Shadow Volume (sometimes called Stencil Shadows)
An extrusion of the lit faces, beginning at the original geometry and ending
further from the light source than the original geometry (presumably at least
as far as the light's radius, if the light has a radius at all), capped at
both front and back to avoid any problems (extrusion from dark faces also
works but has a different set of problems)

This is normally rendered using Carmack's Reverse technique, in which
backfaces behind zbuffer (zfail) increment the stencil, and frontfaces behind
zbuffer (zfail) decrement the stencil, the result is a stencil value of zero
where shadows did not intersect the visible geometry, suitable as a stencil
mask for rendering lighting everywhere but shadow.

In our case to hopefully avoid the Creative Labs patent, we draw the backfaces
as decrement and the frontfaces as increment, and we redefine the DepthFunc to
GL_LESS (the patent uses GL_GEQUAL) which causes zfail when behind surfaces
and zpass when infront (the patent draws where zpass with a GL_GEQUAL test),
additionally we clear stencil to 128 to avoid the need for the unclamped
incr/decr extension (not related to patent).

Patent warning:
This algorithm may be covered by Creative's patent (US Patent #6384822),
however that patent is quite specific about increment on backfaces and
decrement on frontfaces where zpass with GL_GEQUAL depth test, which is
opposite this implementation and partially opposite Carmack's Reverse paper
(which uses GL_LESS, but increments on backfaces and decrements on frontfaces).



Terminology: Stencil Light Volume (sometimes called Light Volumes)
Similar to a Stencil Shadow Volume, but inverted; rather than containing the
areas in shadow it contains the areas in light, this can only be built
quickly for certain limited cases (such as portal visibility from a point),
but is quite useful for some effects (sunlight coming from sky polygons is
one possible example, translucent occluders is another example).



Terminology: Optimized Stencil Shadow Volume
A Stencil Shadow Volume that has been processed sufficiently to ensure it has
no duplicate coverage of areas (no need to shadow an area twice), often this
greatly improves performance but is an operation too costly to use on moving
lights (however completely optimal Stencil Light Volumes can be constructed
in some ideal cases).



Terminology: Per Pixel Lighting (sometimes abbreviated PPL)
Per pixel evaluation of lighting equations, at a bare minimum this involves
DOT3 shading of diffuse lighting (per pixel dotproduct of negated incidence
vector and surface normal, using a texture of the surface bumps, called a
NormalMap) if supported by hardware; in our case there is support for cards
which are incapable of DOT3, the quality is quite poor however.  Additionally
it is desirable to have specular evaluation per pixel, per vertex
normalization of specular halfangle vectors causes noticable distortion but
is unavoidable on hardware without GL_ARB_fragment_program or
GL_ARB_fragment_shader.



Terminology: Normalization CubeMap
A cubemap containing normalized dot3-encoded (vectors of length 1 or less
encoded as RGB colors) for any possible direction, this technique allows per
pixel calculation of incidence vector for per pixel lighting purposes, which
would not otherwise be possible per pixel without GL_ARB_fragment_program or
GL_ARB_fragment_shader.



Terminology: 2D+1D Attenuation Texturing
A very crude approximation of light attenuation with distance which results
in cylindrical light shapes which fade vertically as a streak (some games
such as Doom3 allow this to be rotated to be less noticable in specific
cases), the technique is simply modulating lighting by two 2D textures (which
can be the same) on different axes of projection (XY and Z, typically), this
is the second best technique available without 3D Attenuation Texturing,
GL_ARB_fragment_program or GL_ARB_fragment_shader technology.



Terminology: 2D+1D Inverse Attenuation Texturing
A clever method described in papers on the Abducted engine, this has a squared
distance texture (bright on the outside, black in the middle), which is used
twice using GL_ADD blending, the result of this is used in an inverse modulate
(GL_ONE_MINUS_DST_ALPHA, GL_ZERO) to implement the equation
lighting*=(1-((X*X+Y*Y)+(Z*Z))) which is spherical (unlike 2D+1D attenuation
texturing).



Terminology: 3D Attenuation Texturing
A slightly crude approximation of light attenuation with distance, its flaws
are limited radius and resolution (performance tradeoffs).



Terminology: 3D Attenuation-Normalization Texturing
A 3D Attenuation Texture merged with a Normalization CubeMap, by making the
vectors shorter the lighting becomes darker, a very effective optimization of
diffuse lighting if 3D Attenuation Textures are already used.



Terminology: Light Cubemap Filtering
A technique for modeling non-uniform light distribution according to
direction, for example a lantern may use a cubemap to describe the light
emission pattern of the cage around the lantern (as well as soot buildup
discoloring the light in certain areas), often also used for softened grate
shadows and light shining through a stained glass window (done crudely by
texturing the lighting with a cubemap), another good example would be a disco
light.  This technique is used heavily in many games (Doom3 does not support
this however).



Terminology: Light Projection Filtering
A technique for modeling shadowing of light passing through translucent
surfaces, allowing stained glass windows and other effects to be done more
elegantly than possible with Light Cubemap Filtering by applying an occluder
texture to the lighting combined with a stencil light volume to limit the lit
area, this technique is used by Doom3 for spotlights and flashlights, among
other things, this can also be used more generally to render light passing
through multiple translucent occluders in a scene (using a light volume to
describe the area beyond the occluder, and thus mask off rendering of all
other areas).



Terminology: Doom3 Lighting
A combination of Stencil Shadow Volume, Per Pixel Lighting, Normalization
CubeMap, 2D+1D Attenuation Texturing, and Light Projection Filtering, as
demonstrated by the game Doom3.
*/

#include "quakedef.h"
#include "r_shadow.h"
#include "cl_collision.h"
#include "portals.h"
#include "image.h"
#include "dpsoftrast.h"

#ifdef SUPPORTD3D
#include <d3d9.h>
extern LPDIRECT3DDEVICE9 vid_d3d9dev;
#endif


using namespace cloture;
using namespace util;
using namespace common;
using namespace math::vector;
using templates::casts::vector_cast;

#include "renderer/r_common.hpp"

#if RSHADOW_MAY_ASSUME
	#define	r_assume(x)		__assume(x)
	#define r_unreachable()	__builtin_unreachable()
#else
	#define	r_assume(x)		static_cast<void>(0)
	#define r_unreachable()	static_cast<void>(0)
#endif


static void R_Shadow_EditLights_Init();

typedef enum r_shadow_rendermode_e
{
	R_SHADOW_RENDERMODE_NONE,
	R_SHADOW_RENDERMODE_ZPASS_STENCIL,
	R_SHADOW_RENDERMODE_ZPASS_SEPARATESTENCIL,
	R_SHADOW_RENDERMODE_ZPASS_STENCILTWOSIDE,
	R_SHADOW_RENDERMODE_ZFAIL_STENCIL,
	R_SHADOW_RENDERMODE_ZFAIL_SEPARATESTENCIL,
	R_SHADOW_RENDERMODE_ZFAIL_STENCILTWOSIDE,
	R_SHADOW_RENDERMODE_LIGHT_VERTEX,
	R_SHADOW_RENDERMODE_LIGHT_VERTEX2DATTEN,
	R_SHADOW_RENDERMODE_LIGHT_VERTEX2D1DATTEN,
	R_SHADOW_RENDERMODE_LIGHT_VERTEX3DATTEN,
	R_SHADOW_RENDERMODE_LIGHT_GLSL,
	R_SHADOW_RENDERMODE_VISIBLEVOLUMES,
	R_SHADOW_RENDERMODE_VISIBLELIGHTING,
	R_SHADOW_RENDERMODE_SHADOWMAP2D
}
r_shadow_rendermode_t;

// note the table actually includes one more value, just to avoid the need to clamp the distance index due to minor math error
static constexpr size32 ATTENTABLESIZE = 256;

// 1D gradient, 2D circle and 3D sphere attenuation textures
static constexpr size32 ATTEN1DSIZE = 32;
static constexpr size32 ATTEN2DSIZE = 64;
static constexpr size32 ATTEN3DSIZE = 32;

#define EDLIGHTSPRSIZE			8

__align(mSimdAlign)
struct r_shadow_bouncegrid_settings_t
{
	__import_vector3f();
	#if 1
		float spacing[3];
	#else
		vector3f spacing;
	#endif
	//16 bytes
	float dlightparticlemultiplier;
	float lightradiusscale;
	float particlebounceintensity;
	float particleintensity;
	//32 bytes

	int maxbounce;
	int photons;
	int stablerandom;
	struct
	{
		unsigned int
			staticmode 				: 1,
			bounceanglediffuse 		: 1,
			directionalshading 		: 1,
			includedirectlighting 	: 1,
			hitmodels 				: 1;
	};
	//48 bytes...
	unsigned int padding[4];
	//64 bytes
};

#include "renderer/shadows/R_Shadow_GlobalVars.hpp"

#include "renderer/shadows/Shadows_Cvars.hpp"


static void R_Shadow_SetShadowMode()
{
	r_shadow_shadowmapmaxsize = bound(1, r_shadow_shadowmapping_maxsize.integer, (int)vid.maxtexturesize_2d / 4);
	r_shadow_shadowmapvsdct = r_shadow_shadowmapping_vsdct.integer != 0 && vid.renderpath == RENDERPATH_GL20;
	r_shadow_shadowmapfilterquality = r_shadow_shadowmapping_filterquality.integer;
	r_shadow_shadowmapshadowsampler = r_shadow_shadowmapping_useshadowsampler.integer != 0;
	r_shadow_shadowmapdepthbits = r_shadow_shadowmapping_depthbits.integer;
	r_shadow_shadowmapborder = bound(0, r_shadow_shadowmapping_bordersize.integer, 16);
	r_shadow_shadowmaplod = -1;
	r_shadow_shadowmapsize = 0;
	r_shadow_shadowmapsampler = false;
	r_shadow_shadowmappcf = 0;
	r_shadow_shadowmapdepthtexture = r_fb.usedepthtextures;
	r_shadow_shadowmode = R_SHADOW_SHADOWMODE_STENCIL;
	if ((r_shadow_shadowmapping.integer || r_shadow_deferred.integer) && vid.support.ext_framebuffer_object)
	{
		switch(vid.renderpath)
		{
		case RENDERPATH_GL20:
			if(r_shadow_shadowmapfilterquality < 0)
			{
				if (!r_fb.usedepthtextures)
					r_shadow_shadowmappcf = 1;
				else if((strstr(gl_vendor, "NVIDIA") || strstr(gl_renderer, "Radeon HD")) && vid.support.arb_shadow && r_shadow_shadowmapshadowsampler) 
				{
					r_shadow_shadowmapsampler = true;
					r_shadow_shadowmappcf = 1;
				}
				else if(vid.support.amd_texture_texture4 || vid.support.arb_texture_gather)
					r_shadow_shadowmappcf = 1;
				else if((strstr(gl_vendor, "ATI") || strstr(gl_vendor, "Advanced Micro Devices")) && !strstr(gl_renderer, "Mesa") && !strstr(gl_version, "Mesa")) 
					r_shadow_shadowmappcf = 1;
				else 
					r_shadow_shadowmapsampler = vid.support.arb_shadow && r_shadow_shadowmapshadowsampler;
			}
			else 
			{
                r_shadow_shadowmapsampler = vid.support.arb_shadow && r_shadow_shadowmapshadowsampler;
				switch (r_shadow_shadowmapfilterquality)
				{
				case 1:
					break;
				case 2:
					r_shadow_shadowmappcf = 1;
					break;
				case 3:
					r_shadow_shadowmappcf = 1;
					break;
				case 4:
					r_shadow_shadowmappcf = 2;
					break;
				}
			}
			if (!r_fb.usedepthtextures)
				r_shadow_shadowmapsampler = false;
			r_shadow_shadowmode = R_SHADOW_SHADOWMODE_SHADOWMAP2D;
			break;
#ifndef EXCLUDE_D3D_CODEPATHS
		case RENDERPATH_D3D9:
		case RENDERPATH_D3D10:
		case RENDERPATH_D3D11:
#endif
#ifndef NO_SOFTWARE_RENDERER
		case RENDERPATH_SOFT:
#endif
#if !defined(EXCLUDE_D3D_CODEPATHS) || !defined(NO_SOFTWARE_RENDERER)
			r_shadow_shadowmapsampler = false;
			r_shadow_shadowmappcf = 1;
			r_shadow_shadowmode = R_SHADOW_SHADOWMODE_SHADOWMAP2D;
			break;
#endif
		case RENDERPATH_GL11:
		case RENDERPATH_GL13:
		case RENDERPATH_GLES1:
		case RENDERPATH_GLES2:
			break;
		}
	}

	if(R_CompileShader_CheckStaticParms())
		R_GLSL_Restart_f();
}

bool R_Shadow_ShadowMappingEnabled()
{
	switch (r_shadow_shadowmode)
	{
	case R_SHADOW_SHADOWMODE_SHADOWMAP2D:
		return true;
	default:
		return false;
	}
}

static void R_Shadow_FreeShadowMaps()
{
	R_Shadow_SetShadowMode();

	R_Mesh_DestroyFramebufferObject(r_shadow_fbo2d);

	r_shadow_fbo2d = 0;

	if (r_shadow_shadowmap2ddepthtexture)
		R_FreeTexture(r_shadow_shadowmap2ddepthtexture);
	r_shadow_shadowmap2ddepthtexture = nullptr;

	if (r_shadow_shadowmap2ddepthbuffer)
		R_FreeTexture(r_shadow_shadowmap2ddepthbuffer);
	r_shadow_shadowmap2ddepthbuffer = nullptr;

	if (r_shadow_shadowmapvsdcttexture)
		R_FreeTexture(r_shadow_shadowmapvsdcttexture);
	r_shadow_shadowmapvsdcttexture = nullptr;
}

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

static void R_Shadow_FreeDeferred();
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
	r_shadow_bouncegridtexture = nullptr;
	r_shadow_bouncegridpixels = nullptr;
	r_shadow_bouncegridhighpixels = nullptr;
	r_shadow_bouncegridnumpixels = 0;
	r_shadow_bouncegriddirectional = false;
	r_shadow_attenuationgradienttexture = nullptr;
	r_shadow_attenuation2dtexture = nullptr;
	r_shadow_attenuation3dtexture = nullptr;
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

static void r_shadow_newmap()
{
	if (r_shadow_bouncegridtexture) 
		R_FreeTexture(r_shadow_bouncegridtexture);
		
	r_shadow_bouncegridtexture = nullptr;
	
	if (r_shadow_lightcorona)                 
		R_SkinFrame_MarkUsed(r_shadow_lightcorona);
	if (r_editlights_sprcursor)               
		R_SkinFrame_MarkUsed(r_editlights_sprcursor);
	if (r_editlights_sprlight)                
		R_SkinFrame_MarkUsed(r_editlights_sprlight);
	if (r_editlights_sprnoshadowlight)        
		R_SkinFrame_MarkUsed(r_editlights_sprnoshadowlight);
	if (r_editlights_sprcubemaplight)         
		R_SkinFrame_MarkUsed(r_editlights_sprcubemaplight);
	if (r_editlights_sprcubemapnoshadowlight) 
		R_SkinFrame_MarkUsed(r_editlights_sprcubemapnoshadowlight);
	if (r_editlights_sprselection)            
		R_SkinFrame_MarkUsed(r_editlights_sprselection);
		
	if ( strncmp(cl.worldname, r_shadow_mapname, sizeof(r_shadow_mapname)) )
		R_Shadow_EditLights_Reload_f();
}

void R_Shadow_Init()
{
	Cvar_RegisterVariable(&r_shadow_bumpscale_basetexture);
	Cvar_RegisterVariable(&r_shadow_bumpscale_bumpmap);
	Cvar_RegisterVariable(&r_shadow_usebihculling);
	Cvar_RegisterVariable(&r_shadow_usenormalmap);
	Cvar_RegisterVariable(&r_shadow_debuglight);
	Cvar_RegisterVariable(&r_shadow_deferred);
	Cvar_RegisterVariable(&r_shadow_gloss);
	Cvar_RegisterVariable(&r_shadow_gloss2intensity);
	Cvar_RegisterVariable(&r_shadow_glossintensity);
	Cvar_RegisterVariable(&r_shadow_glossexponent);
	Cvar_RegisterVariable(&r_shadow_gloss2exponent);
	Cvar_RegisterVariable(&r_shadow_glossexact);
	Cvar_RegisterVariable(&r_shadow_lightattenuationdividebias);
	Cvar_RegisterVariable(&r_shadow_lightattenuationlinearscale);
	Cvar_RegisterVariable(&r_shadow_lightintensityscale);
	Cvar_RegisterVariable(&r_shadow_lightradiusscale);
	Cvar_RegisterVariable(&r_shadow_projectdistance);
	Cvar_RegisterVariable(&r_shadow_frontsidecasting);
	Cvar_RegisterVariable(&r_shadow_realtime_dlight);
	Cvar_RegisterVariable(&r_shadow_realtime_dlight_shadows);
	Cvar_RegisterVariable(&r_shadow_realtime_dlight_svbspculling);
	Cvar_RegisterVariable(&r_shadow_realtime_dlight_portalculling);
	Cvar_RegisterVariable(&r_shadow_realtime_world);
	Cvar_RegisterVariable(&r_shadow_realtime_world_lightmaps);
	Cvar_RegisterVariable(&r_shadow_realtime_world_shadows);
	Cvar_RegisterVariable(&r_shadow_realtime_world_compile);
	Cvar_RegisterVariable(&r_shadow_realtime_world_compileshadow);
	Cvar_RegisterVariable(&r_shadow_realtime_world_compilesvbsp);
	Cvar_RegisterVariable(&r_shadow_realtime_world_compileportalculling);
	Cvar_RegisterVariable(&r_shadow_scissor);
	Cvar_RegisterVariable(&r_shadow_shadowmapping);
	Cvar_RegisterVariable(&r_shadow_shadowmapping_vsdct);
	Cvar_RegisterVariable(&r_shadow_shadowmapping_filterquality);
	Cvar_RegisterVariable(&r_shadow_shadowmapping_useshadowsampler);
	Cvar_RegisterVariable(&r_shadow_shadowmapping_depthbits);
	Cvar_RegisterVariable(&r_shadow_shadowmapping_precision);
	Cvar_RegisterVariable(&r_shadow_shadowmapping_maxsize);
	Cvar_RegisterVariable(&r_shadow_shadowmapping_minsize);

	Cvar_RegisterVariable(&r_shadow_shadowmapping_bordersize);
	Cvar_RegisterVariable(&r_shadow_shadowmapping_nearclip);
	Cvar_RegisterVariable(&r_shadow_shadowmapping_bias);
	Cvar_RegisterVariable(&r_shadow_shadowmapping_polygonfactor);
	Cvar_RegisterVariable(&r_shadow_shadowmapping_polygonoffset);
	Cvar_RegisterVariable(&r_shadow_sortsurfaces);
	Cvar_RegisterVariable(&r_shadow_polygonfactor);
	Cvar_RegisterVariable(&r_shadow_polygonoffset);
	Cvar_RegisterVariable(&r_shadow_texture3d);
	Cvar_RegisterVariable(&r_shadow_bouncegrid);
	Cvar_RegisterVariable(&r_shadow_bouncegrid_bounceanglediffuse);
	Cvar_RegisterVariable(&r_shadow_bouncegrid_directionalshading);
	Cvar_RegisterVariable(&r_shadow_bouncegrid_dlightparticlemultiplier);
	Cvar_RegisterVariable(&r_shadow_bouncegrid_hitmodels);
	Cvar_RegisterVariable(&r_shadow_bouncegrid_includedirectlighting);
	Cvar_RegisterVariable(&r_shadow_bouncegrid_intensity);
	Cvar_RegisterVariable(&r_shadow_bouncegrid_lightradiusscale);
	Cvar_RegisterVariable(&r_shadow_bouncegrid_maxbounce);
	Cvar_RegisterVariable(&r_shadow_bouncegrid_particlebounceintensity);
	Cvar_RegisterVariable(&r_shadow_bouncegrid_particleintensity);
	Cvar_RegisterVariable(&r_shadow_bouncegrid_photons);
	Cvar_RegisterVariable(&r_shadow_bouncegrid_spacing);
	Cvar_RegisterVariable(&r_shadow_bouncegrid_stablerandom);
	Cvar_RegisterVariable(&r_shadow_bouncegrid_static);
	Cvar_RegisterVariable(&r_shadow_bouncegrid_static_directionalshading);
	Cvar_RegisterVariable(&r_shadow_bouncegrid_static_lightradiusscale);
	Cvar_RegisterVariable(&r_shadow_bouncegrid_static_maxbounce);
	Cvar_RegisterVariable(&r_shadow_bouncegrid_static_photons);
	Cvar_RegisterVariable(&r_shadow_bouncegrid_updateinterval);
	Cvar_RegisterVariable(&r_shadow_bouncegrid_x);
	Cvar_RegisterVariable(&r_shadow_bouncegrid_y);
	Cvar_RegisterVariable(&r_shadow_bouncegrid_z);
	Cvar_RegisterVariable(&r_coronas);
	Cvar_RegisterVariable(&r_coronas_occlusionsizescale);
	Cvar_RegisterVariable(&r_coronas_occlusionquery);
	Cvar_RegisterVariable(&gl_flashblend);
	Cvar_RegisterVariable(&gl_ext_separatestencil);
	Cvar_RegisterVariable(&gl_ext_stenciltwoside);
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

float atten1[] = {0.5f, 0.0f, 0.0f, 0.5f};
float atten2[] = {0.0f, 0.5f, 0.0f, 0.5f};
float atten3[] = {0.0f, 0.0f, 0.5f, 0.5f};
float atten4[] = {0.0f, 0.0f, 0.0f, 1.0f};

matrix4x4_t matrix_attenuationxyz =
{
		atten1, atten2, atten3, atten4
};

float atten5[] = {0.0f, 0.0f, 0.5f, 0.5f};
float atten6[] = {0.0f, 0.0f, 0.0f, 0.5f};
float atten7[] = {0.0f, 0.0f, 0.0f, 0.5f};
float atten8[] = {0.0f, 0.0f, 0.0f, 1.0f};

matrix4x4_t matrix_attenuationz = /*cloture::util::math::matrix::matrix4D(
		((float[4]){0.0f, 0.0f, 0.5f, 0.5f}),
		((float[4]){0.0f, 0.0f, 0.0f, 0.5f}),
		((float[4]){0.0f, 0.0f, 0.0f, 0.5f}),
		((float[4]){0.0f, 0.0f, 0.0f, 1.0f}));
*/
{
		atten5, atten6, atten7, atten8
};
static void R_Shadow_ResizeShadowArrays(int numvertices, int numtriangles, int vertscale, int triscale)
{
	numvertices = ((numvertices + 255) & ~255) * vertscale;
	numtriangles = ((numtriangles + 255) & ~255) * triscale;
	// make sure shadowelements is big enough for this volume
	if (maxshadowtriangles < numtriangles)
	{
		maxshadowtriangles = numtriangles;
		if (shadowelements)
			Mem_Free(shadowelements);
		shadowelements = (int *)Mem_Alloc(r_main_mempool, maxshadowtriangles * sizeof(int[3]));
	}
	// make sure shadowvertex3f is big enough for this volume
	if (maxshadowvertices < numvertices)
	{
		maxshadowvertices = numvertices;
		if (shadowvertex3f)
			Mem_Free(shadowvertex3f);
		shadowvertex3f = (float *)Mem_Alloc(r_main_mempool, maxshadowvertices * sizeof(float[3]));
	}
}

static void R_Shadow_EnlargeLeafSurfaceTrisBuffer(int numleafs, int numsurfaces, int numshadowtriangles, int numlighttriangles)
{
	int numleafpvsbytes = (((numleafs + 7) >> 3) + 255) & ~255;
	int numsurfacepvsbytes = (((numsurfaces + 7) >> 3) + 255) & ~255;
	int numshadowtrispvsbytes = (((numshadowtriangles + 7) >> 3) + 255) & ~255;
	int numlighttrispvsbytes = (((numlighttriangles + 7) >> 3) + 255) & ~255;
	
	if (r_shadow_buffer_numleafpvsbytes < numleafpvsbytes)
	{
		if (r_shadow_buffer_visitingleafpvs != nullptr)
			r_main_mempool->dealloc(r_shadow_buffer_visitingleafpvs);
			
		//Mem_Free(r_shadow_buffer_visitingleafpvs);
			
		if (r_shadow_buffer_leafpvs != nullptr)
			r_main_mempool->dealloc(r_shadow_buffer_leafpvs);
		
		//Mem_Free(r_shadow_buffer_leafpvs);
			
		if (r_shadow_buffer_leaflist != nullptr)
			r_main_mempool->dealloc(r_shadow_buffer_leaflist);
			
		//Mem_Free(r_shadow_buffer_leaflist);
			
		r_shadow_buffer_numleafpvsbytes = numleafpvsbytes;
		
		r_shadow_buffer_visitingleafpvs = r_main_mempool->alloc<uint8>(r_shadow_buffer_numleafpvsbytes);
		
		//(unsigned char *)Mem_Alloc(r_main_mempool, r_shadow_buffer_numleafpvsbytes);
		
		r_shadow_buffer_leafpvs = r_main_mempool->alloc<uint8>(r_shadow_buffer_numleafpvsbytes);
		//(unsigned char *)Mem_Alloc(r_main_mempool, r_shadow_buffer_numleafpvsbytes);
		
		r_shadow_buffer_leaflist = r_main_mempool->alloc<typeof(*r_shadow_buffer_leaflist)>(r_shadow_buffer_numleafpvsbytes * 8);
		//(int *)Mem_Alloc(r_main_mempool, r_shadow_buffer_numleafpvsbytes * 8 * sizeof(*r_shadow_buffer_leaflist));
	}
	if (r_shadow_buffer_numsurfacepvsbytes < numsurfacepvsbytes)
	{
		if (r_shadow_buffer_surfacepvs)
			Mem_Free(r_shadow_buffer_surfacepvs);
		if (r_shadow_buffer_surfacelist)
			Mem_Free(r_shadow_buffer_surfacelist);
		if (r_shadow_buffer_surfacesides)
			Mem_Free(r_shadow_buffer_surfacesides);
		r_shadow_buffer_numsurfacepvsbytes = numsurfacepvsbytes;
		r_shadow_buffer_surfacepvs = (unsigned char *)Mem_Alloc(r_main_mempool, r_shadow_buffer_numsurfacepvsbytes);
		r_shadow_buffer_surfacelist = (int *)Mem_Alloc(r_main_mempool, r_shadow_buffer_numsurfacepvsbytes * 8 * sizeof(*r_shadow_buffer_surfacelist));
		r_shadow_buffer_surfacesides = (unsigned char *)Mem_Alloc(r_main_mempool, r_shadow_buffer_numsurfacepvsbytes * 8 * sizeof(*r_shadow_buffer_surfacelist));
	}
	if (r_shadow_buffer_numshadowtrispvsbytes < numshadowtrispvsbytes)
	{
		if (r_shadow_buffer_shadowtrispvs)
			Mem_Free(r_shadow_buffer_shadowtrispvs);
		r_shadow_buffer_numshadowtrispvsbytes = numshadowtrispvsbytes;
		r_shadow_buffer_shadowtrispvs = (unsigned char *)Mem_Alloc(r_main_mempool, r_shadow_buffer_numshadowtrispvsbytes);
	}
	if (r_shadow_buffer_numlighttrispvsbytes < numlighttrispvsbytes)
	{
		if (r_shadow_buffer_lighttrispvs)
			Mem_Free(r_shadow_buffer_lighttrispvs);
		r_shadow_buffer_numlighttrispvsbytes = numlighttrispvsbytes;
		r_shadow_buffer_lighttrispvs = (unsigned char *)Mem_Alloc(r_main_mempool, r_shadow_buffer_numlighttrispvsbytes);
	}
}

void R_Shadow_PrepareShadowMark(int numtris)
{
	// make sure shadowmark is big enough for this volume
	if (maxshadowmark < numtris)
	{
		maxshadowmark = numtris;
		if (shadowmark)
			Mem_Free(shadowmark);
		if (shadowmarklist)
			Mem_Free(shadowmarklist);
		shadowmark = (int *)Mem_Alloc(r_main_mempool, maxshadowmark * sizeof(*shadowmark));
		shadowmarklist = (int *)Mem_Alloc(r_main_mempool, maxshadowmark * sizeof(*shadowmarklist));
		shadowmarkcount = 0;
	}
	shadowmarkcount++;
	// if shadowmarkcount wrapped we clear the array and adjust accordingly
	if (shadowmarkcount == 0)
	{
		shadowmarkcount = 1;
		memset(shadowmark, 0, maxshadowmark * sizeof(*shadowmark));
	}
	numshadowmark = 0;
}

void R_Shadow_PrepareShadowSides(int numtris)
{
	if (maxshadowsides < numtris)
	{
		maxshadowsides = numtris;
		if (shadowsides)
			Mem_Free(shadowsides);
		if (shadowsideslist)
			Mem_Free(shadowsideslist);
		shadowsides = (unsigned char *)Mem_Alloc(r_main_mempool, maxshadowsides * sizeof(*shadowsides));
		shadowsideslist = (int *)Mem_Alloc(r_main_mempool, maxshadowsides * sizeof(*shadowsideslist));
	}
	numshadowsides = 0;
}

static int R_Shadow_ConstructShadowVolume_ZFail(int innumvertices, int innumtris, const int *inelement3i, const int *inneighbor3i, const float *invertex3f, int *outnumvertices, int *outelement3i, float *outvertex3f, const float *projectorigin, const float *projectdirection, float projectdistance, int numshadowmarktris, const int *shadowmarktris)
{
	int i, j;
	int outtriangles = 0, outvertices = 0;
	const int *element;
	const float *vertex;
	float ratio, direction[3], projectvector[3];

	if (projectdirection)
		VectorScale(projectdirection, projectdistance, projectvector);
	else
		VectorClear(projectvector);

	// create the vertices
	if (projectdirection)
	{
		for (i = 0;i < numshadowmarktris;i++)
		{
			element = inelement3i + shadowmarktris[i] * 3;
			for (j = 0;j < 3;j++)
			{
				if (vertexupdate[element[j]] != vertexupdatenum)
				{
					vertexupdate[element[j]] = vertexupdatenum;
					vertexremap[element[j]] = outvertices;
					vertex = invertex3f + element[j] * 3;
					// project one copy of the vertex according to projectvector
					VectorCopy(vertex, outvertex3f);
					VectorAdd(vertex, projectvector, (outvertex3f + 3));
					outvertex3f += 6;
					outvertices += 2;
				}
			}
		}
	}
	else
	{
		for (i = 0;i < numshadowmarktris;i++)
		{
			element = inelement3i + shadowmarktris[i] * 3;
			for (j = 0;j < 3;j++)
			{
				if (vertexupdate[element[j]] != vertexupdatenum)
				{
					vertexupdate[element[j]] = vertexupdatenum;
					vertexremap[element[j]] = outvertices;
					vertex = invertex3f + element[j] * 3;
					// project one copy of the vertex to the sphere radius of the light
					// (FIXME: would projecting it to the light box be better?)
					VectorSubtract(vertex, projectorigin, direction);
					ratio = projectdistance / VectorLength(direction);
					VectorCopy(vertex, outvertex3f);
					VectorMA(projectorigin, ratio, direction, (outvertex3f + 3));
					outvertex3f += 6;
					outvertices += 2;
				}
			}
		}
	}

	if (r_shadow_frontsidecasting.integer)
	{
		for (i = 0;i < numshadowmarktris;i++)
		{
			int remappedelement[3];
			int markindex;
			const int *neighbortriangle;

			markindex = shadowmarktris[i] * 3;
			element = inelement3i + markindex;
			neighbortriangle = inneighbor3i + markindex;
			// output the front and back triangles
			outelement3i[0] = vertexremap[element[0]];
			outelement3i[1] = vertexremap[element[1]];
			outelement3i[2] = vertexremap[element[2]];
			outelement3i[3] = vertexremap[element[2]] + 1;
			outelement3i[4] = vertexremap[element[1]] + 1;
			outelement3i[5] = vertexremap[element[0]] + 1;

			outelement3i += 6;
			outtriangles += 2;
			// output the sides (facing outward from this triangle)
			if (shadowmark[neighbortriangle[0]] != shadowmarkcount)
			{
				remappedelement[0] = vertexremap[element[0]];
				remappedelement[1] = vertexremap[element[1]];
				outelement3i[0] = remappedelement[1];
				outelement3i[1] = remappedelement[0];
				outelement3i[2] = remappedelement[0] + 1;
				outelement3i[3] = remappedelement[1];
				outelement3i[4] = remappedelement[0] + 1;
				outelement3i[5] = remappedelement[1] + 1;

				outelement3i += 6;
				outtriangles += 2;
			}
			if (shadowmark[neighbortriangle[1]] != shadowmarkcount)
			{
				remappedelement[1] = vertexremap[element[1]];
				remappedelement[2] = vertexremap[element[2]];
				outelement3i[0] = remappedelement[2];
				outelement3i[1] = remappedelement[1];
				outelement3i[2] = remappedelement[1] + 1;
				outelement3i[3] = remappedelement[2];
				outelement3i[4] = remappedelement[1] + 1;
				outelement3i[5] = remappedelement[2] + 1;

				outelement3i += 6;
				outtriangles += 2;
			}
			if (shadowmark[neighbortriangle[2]] != shadowmarkcount)
			{
				remappedelement[0] = vertexremap[element[0]];
				remappedelement[2] = vertexremap[element[2]];
				outelement3i[0] = remappedelement[0];
				outelement3i[1] = remappedelement[2];
				outelement3i[2] = remappedelement[2] + 1;
				outelement3i[3] = remappedelement[0];
				outelement3i[4] = remappedelement[2] + 1;
				outelement3i[5] = remappedelement[0] + 1;

				outelement3i += 6;
				outtriangles += 2;
			}
		}
	}
	else
	{
		for (i = 0;i < numshadowmarktris;i++)
		{
			int remappedelement[3];
			int markindex;
			const int *neighbortriangle;

			markindex = shadowmarktris[i] * 3;
			element = inelement3i + markindex;
			neighbortriangle = inneighbor3i + markindex;
			// output the front and back triangles
			outelement3i[0] = vertexremap[element[2]];
			outelement3i[1] = vertexremap[element[1]];
			outelement3i[2] = vertexremap[element[0]];
			outelement3i[3] = vertexremap[element[0]] + 1;
			outelement3i[4] = vertexremap[element[1]] + 1;
			outelement3i[5] = vertexremap[element[2]] + 1;

			outelement3i += 6;
			outtriangles += 2;
			// output the sides (facing outward from this triangle)
			if (shadowmark[neighbortriangle[0]] != shadowmarkcount)
			{
				remappedelement[0] = vertexremap[element[0]];
				remappedelement[1] = vertexremap[element[1]];
				outelement3i[0] = remappedelement[0];
				outelement3i[1] = remappedelement[1];
				outelement3i[2] = remappedelement[1] + 1;
				outelement3i[3] = remappedelement[0];
				outelement3i[4] = remappedelement[1] + 1;
				outelement3i[5] = remappedelement[0] + 1;

				outelement3i += 6;
				outtriangles += 2;
			}
			if (shadowmark[neighbortriangle[1]] != shadowmarkcount)
			{
				remappedelement[1] = vertexremap[element[1]];
				remappedelement[2] = vertexremap[element[2]];
				outelement3i[0] = remappedelement[1];
				outelement3i[1] = remappedelement[2];
				outelement3i[2] = remappedelement[2] + 1;
				outelement3i[3] = remappedelement[1];
				outelement3i[4] = remappedelement[2] + 1;
				outelement3i[5] = remappedelement[1] + 1;

				outelement3i += 6;
				outtriangles += 2;
			}
			if (shadowmark[neighbortriangle[2]] != shadowmarkcount)
			{
				remappedelement[0] = vertexremap[element[0]];
				remappedelement[2] = vertexremap[element[2]];
				outelement3i[0] = remappedelement[2];
				outelement3i[1] = remappedelement[0];
				outelement3i[2] = remappedelement[0] + 1;
				outelement3i[3] = remappedelement[2];
				outelement3i[4] = remappedelement[0] + 1;
				outelement3i[5] = remappedelement[2] + 1;

				outelement3i += 6;
				outtriangles += 2;
			}
		}
	}
	if (outnumvertices)
		*outnumvertices = outvertices;
	return outtriangles;
}

static int R_Shadow_ConstructShadowVolume_ZPass(int innumvertices, int innumtris, const int *inelement3i, const int *inneighbor3i, const float *invertex3f, int *outnumvertices, int *outelement3i, float *outvertex3f, const float *projectorigin, const float *projectdirection, float projectdistance, int numshadowmarktris, const int *shadowmarktris)
{
	int i;
	int j;
	int k;
	
	int outtriangles = 0;
	int outvertices = 0;
	
	const int *element;
	const float *vertex;
	
	float ratio;
	float direction[3];
	float projectvector[3];
	//vector3f projectvector
	
	bool side[4];

	if (projectdirection)
		VectorScale(projectdirection, projectdistance, projectvector);
	else
		VectorClear(projectvector);

	for (i = 0; i < numshadowmarktris; i++)
	{
		int remappedelement[3];
		const int *neighbortriangle;

		int markindex = shadowmarktris[i] * 3;
		neighbortriangle = inneighbor3i + markindex;
		side[0] = shadowmark[neighbortriangle[0]] == shadowmarkcount;
		side[1] = shadowmark[neighbortriangle[1]] == shadowmarkcount;
		side[2] = shadowmark[neighbortriangle[2]] == shadowmarkcount;
		if (side[0] + side[1] + side[2] == 0)
			continue;

		side[3] = side[0];
		element = inelement3i + markindex;

		// create the vertices
		for (j = 0;j < 3;j++)
		{
			if (side[j] + side[j+1] == 0)
				continue;
			k = element[j];
			if (vertexupdate[k] != vertexupdatenum)
			{
				vertexupdate[k] = vertexupdatenum;
				vertexremap[k] = outvertices;
				vertex = invertex3f + k * 3;
				VectorCopy(vertex, outvertex3f);
				if (projectdirection)
				{
					// project one copy of the vertex according to projectvector
					VectorAdd(vertex, projectvector, (outvertex3f + 3));
				}
				else
				{
					// project one copy of the vertex to the sphere radius of the light
					// (FIXME: would projecting it to the light box be better?)
					VectorSubtract(vertex, projectorigin, direction);
					ratio = projectdistance / VectorLength(direction);
					VectorMA(projectorigin, ratio, direction, (outvertex3f + 3));
				}
				outvertex3f += 6;
				outvertices += 2;
			}
		}

		// output the sides (facing outward from this triangle)
		if (!side[0])
		{
			remappedelement[0] = vertexremap[element[0]];
			remappedelement[1] = vertexremap[element[1]];
			outelement3i[0] = remappedelement[1];
			outelement3i[1] = remappedelement[0];
			outelement3i[2] = remappedelement[0] + 1;
			outelement3i[3] = remappedelement[1];
			outelement3i[4] = remappedelement[0] + 1;
			outelement3i[5] = remappedelement[1] + 1;

			outelement3i += 6;
			outtriangles += 2;
		}
		if (!side[1])
		{
			remappedelement[1] = vertexremap[element[1]];
			remappedelement[2] = vertexremap[element[2]];
			outelement3i[0] = remappedelement[2];
			outelement3i[1] = remappedelement[1];
			outelement3i[2] = remappedelement[1] + 1;
			outelement3i[3] = remappedelement[2];
			outelement3i[4] = remappedelement[1] + 1;
			outelement3i[5] = remappedelement[2] + 1;

			outelement3i += 6;
			outtriangles += 2;
		}
		if (!side[2])
		{
			remappedelement[0] = vertexremap[element[0]];
			remappedelement[2] = vertexremap[element[2]];
			outelement3i[0] = remappedelement[0];
			outelement3i[1] = remappedelement[2];
			outelement3i[2] = remappedelement[2] + 1;
			outelement3i[3] = remappedelement[0];
			outelement3i[4] = remappedelement[2] + 1;
			outelement3i[5] = remappedelement[0] + 1;

			outelement3i += 6;
			outtriangles += 2;
		}
	}
	if (outnumvertices)
		*outnumvertices = outvertices;
	return outtriangles;
}

void R_Shadow_MarkVolumeFromBox(int firsttriangle, int numtris, const float *invertex3f, const int *elements, const vec3_t projectorigin, const vec3_t projectdirection, const vec3_t lightmins, const vec3_t lightmaxs, const vec3_t surfacemins, const vec3_t surfacemaxs)
{
	int t, tend;
	const int *e;
	const float *v[3];
	float normal[3];
	if (!BoxesOverlap(lightmins, lightmaxs, surfacemins, surfacemaxs))
		return;
	tend = firsttriangle + numtris;
	if (BoxInsideBox(surfacemins, surfacemaxs, lightmins, lightmaxs))
	{
		// surface box entirely inside light box, no box cull
		if (projectdirection)
		{
			for (t = firsttriangle, e = elements + t * 3;t < tend;t++, e += 3)
			{
				TriangleNormal(invertex3f + e[0] * 3, invertex3f + e[1] * 3, invertex3f + e[2] * 3, normal);
				if (r_shadow_frontsidecasting.integer == (DotProduct(normal, projectdirection) < .0f))
					shadowmarklist[numshadowmark++] = t;
			}
		}
		else
		{
			for (t = firsttriangle, e = elements + t * 3;t < tend;t++, e += 3)
				if (r_shadow_frontsidecasting.integer == PointInfrontOfTriangle(projectorigin, invertex3f + e[0] * 3, invertex3f + e[1] * 3, invertex3f + e[2] * 3))
					shadowmarklist[numshadowmark++] = t;
		}
	}
	else
	{
		// surface box not entirely inside light box, cull each triangle
		if (projectdirection)
		{
			for (t = firsttriangle, e = elements + t * 3;t < tend;t++, e += 3)
			{
				v[0] = invertex3f + e[0] * 3;
				v[1] = invertex3f + e[1] * 3;
				v[2] = invertex3f + e[2] * 3;
				TriangleNormal(v[0], v[1], v[2], normal);
				if (r_shadow_frontsidecasting.integer == (DotProduct(normal, projectdirection) < 0)
				 && TriangleBBoxOverlapsBox(v[0], v[1], v[2], lightmins, lightmaxs))
					shadowmarklist[numshadowmark++] = t;
			}
		}
		else
		{
			for (t = firsttriangle, e = elements + t * 3;t < tend;t++, e += 3)
			{
				v[0] = invertex3f + e[0] * 3;
				v[1] = invertex3f + e[1] * 3;
				v[2] = invertex3f + e[2] * 3;
				if (r_shadow_frontsidecasting.integer == PointInfrontOfTriangle(projectorigin, v[0], v[1], v[2])
				 && TriangleBBoxOverlapsBox(v[0], v[1], v[2], lightmins, lightmaxs))
					shadowmarklist[numshadowmark++] = t;
			}
		}
	}
}

static bool R_Shadow_UseZPass(vec3_t mins, vec3_t maxs)
{
	return false;
}

void R_Shadow_VolumeFromList(int numverts, int numtris, const float *invertex3f, const int *elements, const int *neighbors, const vec3_t projectorigin, const vec3_t projectdirection, float projectdistance, int nummarktris, const int *marktris, vec3_t trismins, vec3_t trismaxs)
{
	int i, tris, outverts;
	if (unlikely(projectdistance < 0.1f))
	{
		Con_Printf("R_Shadow_Volume: projectdistance %f\n", projectdistance);
		return;
	}
	if (!numverts || !nummarktris)
		return;
	// make sure shadowelements is big enough for this volume
	if (maxshadowtriangles < nummarktris*8 || maxshadowvertices < numverts*2)
		R_Shadow_ResizeShadowArrays(numverts, nummarktris, 2, 8);

	if (maxvertexupdate < numverts)
	{
		maxvertexupdate = numverts;
		if (vertexupdate)
			Mem_Free(vertexupdate);
		if (vertexremap)
			Mem_Free(vertexremap);
		vertexupdate = (int *)Mem_Alloc(r_main_mempool, maxvertexupdate * sizeof(int));
		vertexremap = (int *)Mem_Alloc(r_main_mempool, maxvertexupdate * sizeof(int));
		vertexupdatenum = 0;
	}
	vertexupdatenum++;
	if (vertexupdatenum == 0)
	{
		vertexupdatenum = 1;
		memset(vertexupdate, 0, maxvertexupdate * sizeof(int));
		memset(vertexremap, 0, maxvertexupdate * sizeof(int));
	}

	for (i = 0;i < nummarktris;i++)
		shadowmark[marktris[i]] = shadowmarkcount;

	if (r_shadow_compilingrtlight)
	{
		// if we're compiling an rtlight, capture the mesh
		//tris = R_Shadow_ConstructShadowVolume_ZPass(numverts, numtris, elements, neighbors, invertex3f, &outverts, shadowelements, shadowvertex3f, projectorigin, projectdirection, projectdistance, nummarktris, marktris);
		//Mod_ShadowMesh_AddMesh(r_main_mempool, r_shadow_compilingrtlight->static_meshchain_shadow_zpass, NULL, NULL, NULL, shadowvertex3f, NULL, NULL, NULL, NULL, tris, shadowelements);
		tris = R_Shadow_ConstructShadowVolume_ZFail(numverts, numtris, elements, neighbors, invertex3f, &outverts, shadowelements, shadowvertex3f, projectorigin, projectdirection, projectdistance, nummarktris, marktris);
		Mod_ShadowMesh_AddMesh(r_main_mempool, r_shadow_compilingrtlight->static_meshchain_shadow_zfail, nullptr, nullptr, nullptr, shadowvertex3f, nullptr, nullptr, nullptr, nullptr, tris, shadowelements);
	}
	else if (r_shadow_rendermode == R_SHADOW_RENDERMODE_VISIBLEVOLUMES)
	{
		tris = R_Shadow_ConstructShadowVolume_ZFail(numverts, numtris, elements, neighbors, invertex3f, &outverts, shadowelements, shadowvertex3f, projectorigin, projectdirection, projectdistance, nummarktris, marktris);
		R_Mesh_PrepareVertices_Vertex3f(outverts, shadowvertex3f, nullptr, 0);
		R_Mesh_Draw(0, outverts, 0, tris, shadowelements, nullptr, 0, nullptr, nullptr, 0);
	}
	else
	{
		// decide which type of shadow to generate and set stencil mode
		R_Shadow_RenderMode_StencilShadowVolumes(R_Shadow_UseZPass(trismins, trismaxs));
		// generate the sides or a solid volume, depending on type
		if (r_shadow_rendermode >= R_SHADOW_RENDERMODE_ZPASS_STENCIL && r_shadow_rendermode <= R_SHADOW_RENDERMODE_ZPASS_STENCILTWOSIDE)
			tris = R_Shadow_ConstructShadowVolume_ZPass(numverts, numtris, elements, neighbors, invertex3f, &outverts, shadowelements, shadowvertex3f, projectorigin, projectdirection, projectdistance, nummarktris, marktris);
		else
			tris = R_Shadow_ConstructShadowVolume_ZFail(numverts, numtris, elements, neighbors, invertex3f, &outverts, shadowelements, shadowvertex3f, projectorigin, projectdirection, projectdistance, nummarktris, marktris);
		r_refdef.stats[r_stat_lights_dynamicshadowtriangles] += tris;
		r_refdef.stats[r_stat_lights_shadowtriangles] += tris;
		if (r_shadow_rendermode == R_SHADOW_RENDERMODE_ZPASS_STENCIL)
		{
			// increment stencil if frontface is infront of depthbuffer
			GL_CullFace(r_refdef.view.cullface_front);
			R_SetStencil(true, 255, GL_KEEP, GL_KEEP, GL_DECR, GL_ALWAYS, 128, 255);
			R_Mesh_Draw(0, outverts, 0, tris, shadowelements, nullptr, 0, nullptr, nullptr, 0);
			// decrement stencil if backface is infront of depthbuffer
			GL_CullFace(r_refdef.view.cullface_back);
			R_SetStencil(true, 255, GL_KEEP, GL_KEEP, GL_INCR, GL_ALWAYS, 128, 255);
		}
		else if (r_shadow_rendermode == R_SHADOW_RENDERMODE_ZFAIL_STENCIL)
		{
			// decrement stencil if backface is behind depthbuffer
			GL_CullFace(r_refdef.view.cullface_front);
			R_SetStencil(true, 255, GL_KEEP, GL_DECR, GL_KEEP, GL_ALWAYS, 128, 255);
			R_Mesh_Draw(0, outverts, 0, tris, shadowelements, nullptr, 0, nullptr, nullptr, 0);
			// increment stencil if frontface is behind depthbuffer
			GL_CullFace(r_refdef.view.cullface_back);
			R_SetStencil(true, 255, GL_KEEP, GL_INCR, GL_KEEP, GL_ALWAYS, 128, 255);
		}
		R_Mesh_PrepareVertices_Vertex3f(outverts, shadowvertex3f, nullptr, 0);
		R_Mesh_Draw(0, outverts, 0, tris, shadowelements, nullptr, 0, nullptr, nullptr, 0);
	}
}

#include "renderer/shadows/Calculate_Side_Masks.hpp"
#include "renderer/shadows/Frustum_Cull_Sides.hpp"


int R_Shadow_ChooseSidesFromBox(int firsttriangle, int numtris, const float *invertex3f, const int *elements, const matrix4x4_t *worldtolight, const vec3_t projectorigin, const vec3_t projectdirection, const vec3_t lightmins, const vec3_t lightmaxs, const vec3_t surfacemins, const vec3_t surfacemaxs, int *totals)
{
	int t, tend;
	const int *e;
	const float *v[3];
	float normal[3];
	vec3_t p[3];
	float bias;
	int mask, surfacemask = 0;
	if (!BoxesOverlap(lightmins, lightmaxs, surfacemins, surfacemaxs))
		return 0;
	bias = r_shadow_shadowmapborder / (float)(r_shadow_shadowmapmaxsize - r_shadow_shadowmapborder);
	tend = firsttriangle + numtris;
	if (BoxInsideBox(surfacemins, surfacemaxs, lightmins, lightmaxs))
	{
		// surface box entirely inside light box, no box cull
		if (projectdirection)
		{
			for (t = firsttriangle, e = elements + t * 3;t < tend;t++, e += 3)
			{
				v[0] = invertex3f + e[0] * 3, v[1] = invertex3f + e[1] * 3, v[2] = invertex3f + e[2] * 3;
				TriangleNormal(v[0], v[1], v[2], normal);
				if (r_shadow_frontsidecasting.integer == (DotProduct(normal, projectdirection) < 0))
				{
					Matrix4x4_Transform(worldtolight, v[0], p[0]), Matrix4x4_Transform(worldtolight, v[1], p[1]), Matrix4x4_Transform(worldtolight, v[2], p[2]);
					mask = R_Shadow_CalcTriangleSideMask(p[0], p[1], p[2], bias);
					surfacemask |= mask;
					if(totals)
					{
						totals[0] += mask&1, totals[1] += (mask>>1)&1, totals[2] += (mask>>2)&1, totals[3] += (mask>>3)&1, totals[4] += (mask>>4)&1, totals[5] += mask>>5;
						shadowsides[numshadowsides] = mask;
						shadowsideslist[numshadowsides++] = t;
					}
				}
			}
		}
		else
		{
			for (t = firsttriangle, e = elements + t * 3;t < tend;t++, e += 3)
			{
				v[0] = invertex3f + e[0] * 3, v[1] = invertex3f + e[1] * 3,	v[2] = invertex3f + e[2] * 3;
				if (r_shadow_frontsidecasting.integer == PointInfrontOfTriangle(projectorigin, v[0], v[1], v[2]))
				{
					Matrix4x4_Transform(worldtolight, v[0], p[0]), Matrix4x4_Transform(worldtolight, v[1], p[1]), Matrix4x4_Transform(worldtolight, v[2], p[2]);
					mask = R_Shadow_CalcTriangleSideMask(p[0], p[1], p[2], bias);
					surfacemask |= mask;
					if(totals)
					{
						totals[0] += mask&1, totals[1] += (mask>>1)&1, totals[2] += (mask>>2)&1, totals[3] += (mask>>3)&1, totals[4] += (mask>>4)&1, totals[5] += mask>>5;
						shadowsides[numshadowsides] = mask;
						shadowsideslist[numshadowsides++] = t;
					}
				}
			}
		}
	}
	else
	{
		// surface box not entirely inside light box, cull each triangle
		if (projectdirection)
		{
			for (t = firsttriangle, e = elements + t * 3;t < tend;t++, e += 3)
			{
				v[0] = invertex3f + e[0] * 3, v[1] = invertex3f + e[1] * 3,	v[2] = invertex3f + e[2] * 3;
				TriangleNormal(v[0], v[1], v[2], normal);
				if (r_shadow_frontsidecasting.integer == (DotProduct(normal, projectdirection) < 0)
				 && TriangleBBoxOverlapsBox(v[0], v[1], v[2], lightmins, lightmaxs))
				{
					Matrix4x4_Transform(worldtolight, v[0], p[0]), Matrix4x4_Transform(worldtolight, v[1], p[1]), Matrix4x4_Transform(worldtolight, v[2], p[2]);
					mask = R_Shadow_CalcTriangleSideMask(p[0], p[1], p[2], bias);
					surfacemask |= mask;
					if(totals)
					{
						totals[0] += mask&1, totals[1] += (mask>>1)&1, totals[2] += (mask>>2)&1, totals[3] += (mask>>3)&1, totals[4] += (mask>>4)&1, totals[5] += mask>>5;
						shadowsides[numshadowsides] = mask;
						shadowsideslist[numshadowsides++] = t;
					}
				}
			}
		}
		else
		{
			for (t = firsttriangle, e = elements + t * 3;t < tend;t++, e += 3)
			{
				v[0] = invertex3f + e[0] * 3, v[1] = invertex3f + e[1] * 3, v[2] = invertex3f + e[2] * 3;
				if (r_shadow_frontsidecasting.integer == PointInfrontOfTriangle(projectorigin, v[0], v[1], v[2])
				 && TriangleBBoxOverlapsBox(v[0], v[1], v[2], lightmins, lightmaxs))
				{
					Matrix4x4_Transform(worldtolight, v[0], p[0]), Matrix4x4_Transform(worldtolight, v[1], p[1]), Matrix4x4_Transform(worldtolight, v[2], p[2]);
					mask = R_Shadow_CalcTriangleSideMask(p[0], p[1], p[2], bias);
					surfacemask |= mask;
					if(totals)
					{
						totals[0] += mask&1, totals[1] += (mask>>1)&1, totals[2] += (mask>>2)&1, totals[3] += (mask>>3)&1, totals[4] += (mask>>4)&1, totals[5] += mask>>5;
						shadowsides[numshadowsides] = mask;
						shadowsideslist[numshadowsides++] = t;
					}
				}
			}
		}
	}
	return surfacemask;
}

void R_Shadow_ShadowMapFromList(int numverts, int numtris, const float *vertex3f, const int *elements, int numsidetris, const int *sidetotals, const unsigned char *sides, const int *sidetris)
{
	int outtriangles = 0;
	int *outelement3i[6];
	if (!numverts || !numsidetris || !r_shadow_compilingrtlight)
		return;
	outtriangles = sidetotals[0] + sidetotals[1] + sidetotals[2] + sidetotals[3] + sidetotals[4] + sidetotals[5];
	// make sure shadowelements is big enough for this mesh
	if (maxshadowtriangles < outtriangles)
		R_Shadow_ResizeShadowArrays(0, outtriangles, 0, 1);

	// compute the offset and size of the separate index lists for each cubemap side
	outtriangles = 0;
	for (size32 i = 0; i < 6; i++)
	{
		outelement3i[i] = shadowelements + outtriangles * 3;
		r_shadow_compilingrtlight->static_meshchain_shadow_shadowmap->sideoffsets[i] = outtriangles;
		r_shadow_compilingrtlight->static_meshchain_shadow_shadowmap->sidetotals[i] = sidetotals[i];
		outtriangles += sidetotals[i];
	}

	// gather up the (sparse) triangles into separate index lists for each cubemap side
	for (size32 i = 0; i < numsidetris; i++)
	{
		const int *element = elements + sidetris[i] * 3;
		for (size32 j = 0; j < 6; j++)
		{
			if (sides[i] & (1 << j))
			{
				outelement3i[j][0] = element[0];
				outelement3i[j][1] = element[1];
				outelement3i[j][2] = element[2];
				outelement3i[j] += 3;
			}
		}
	}
			
	Mod_ShadowMesh_AddMesh(r_main_mempool, r_shadow_compilingrtlight->static_meshchain_shadow_shadowmap, nullptr, nullptr, nullptr, vertex3f, nullptr, nullptr, nullptr, nullptr, outtriangles, shadowelements);
}

#include "renderer/shadows/R_Shadow_MakeTextures.hpp"


void R_Shadow_ValidateCvars()
{
	if (r_shadow_texture3d.integer && !vid.support.ext_texture_3d)
		Cvar_SetValueQuick(&r_shadow_texture3d, 0);
	if (gl_ext_separatestencil.integer && !vid.support.ati_separate_stencil)
		Cvar_SetValueQuick(&gl_ext_separatestencil, 0);
	if (gl_ext_stenciltwoside.integer && !vid.support.ext_stencil_two_side)
		Cvar_SetValueQuick(&gl_ext_stenciltwoside, 0);
}

void R_Shadow_RenderMode_Begin()
{

	R_Shadow_ValidateCvars();

	if (!r_shadow_attenuation2dtexture
	 || (!r_shadow_attenuation3dtexture && r_shadow_texture3d.integer)
	 || r_shadow_lightattenuationdividebias.value != r_shadow_attendividebias
	 || r_shadow_lightattenuationlinearscale.value != r_shadow_attenlinearscale)
		R_Shadow_MakeTextures();

	CHECKGLERROR
	R_Mesh_ResetTextureState();
	GL_BlendFunc(GL_ONE, GL_ZERO);
	GL_DepthRange(.0f, 1.0f);
	GL_PolygonOffset(r_refdef.polygonfactor, r_refdef.polygonoffset);
	GL_DepthTest(true);
	GL_DepthMask(false);
	GL_Color(.0f, .0f, .0f, 1.0f);
	GL_Scissor(r_refdef.view.viewport.x, r_refdef.view.viewport.y, r_refdef.view.viewport.width, r_refdef.view.viewport.height);
	
	r_shadow_rendermode = R_SHADOW_RENDERMODE_NONE;

	if (gl_ext_separatestencil.integer && vid.support.ati_separate_stencil)
	{
		r_shadow_shadowingrendermode_zpass = R_SHADOW_RENDERMODE_ZPASS_SEPARATESTENCIL;
		r_shadow_shadowingrendermode_zfail = R_SHADOW_RENDERMODE_ZFAIL_SEPARATESTENCIL;
	}
	else if (gl_ext_stenciltwoside.integer && vid.support.ext_stencil_two_side)
	{
		r_shadow_shadowingrendermode_zpass = R_SHADOW_RENDERMODE_ZPASS_STENCILTWOSIDE;
		r_shadow_shadowingrendermode_zfail = R_SHADOW_RENDERMODE_ZFAIL_STENCILTWOSIDE;
	}
	else
	{
		r_shadow_shadowingrendermode_zpass = R_SHADOW_RENDERMODE_ZPASS_STENCIL;
		r_shadow_shadowingrendermode_zfail = R_SHADOW_RENDERMODE_ZFAIL_STENCIL;
	}

	switch(vid.renderpath)
	{
	case RENDERPATH_GL20:
#ifndef EXCLUDE_D3D_CODEPATHS
	case RENDERPATH_D3D9:
	case RENDERPATH_D3D10:
	case RENDERPATH_D3D11:
#endif
#ifndef NO_SOFTWARE_RENDERER
	case RENDERPATH_SOFT:
#endif
	case RENDERPATH_GLES2:
		r_shadow_lightingrendermode = R_SHADOW_RENDERMODE_LIGHT_GLSL;
		break;
	case RENDERPATH_GL11:
	case RENDERPATH_GL13:
	case RENDERPATH_GLES1:
		if (r_textureunits.integer >= 2 && vid.texunits >= 2 && r_shadow_texture3d.integer && r_shadow_attenuation3dtexture)
			r_shadow_lightingrendermode = R_SHADOW_RENDERMODE_LIGHT_VERTEX3DATTEN;
		else if (r_textureunits.integer >= 3 && vid.texunits >= 3)
			r_shadow_lightingrendermode = R_SHADOW_RENDERMODE_LIGHT_VERTEX2D1DATTEN;
		else if (r_textureunits.integer >= 2 && vid.texunits >= 2)
			r_shadow_lightingrendermode = R_SHADOW_RENDERMODE_LIGHT_VERTEX2DATTEN;
		else
			r_shadow_lightingrendermode = R_SHADOW_RENDERMODE_LIGHT_VERTEX;
		break;
	}

	CHECKGLERROR

	r_shadow_cullface_front = r_refdef.view.cullface_front;
	r_shadow_cullface_back = r_refdef.view.cullface_back;
}

void R_Shadow_RenderMode_ActiveLight(const rtlight_t *rtlight)
{
	rsurface.rtlight = rtlight;
}

void R_Shadow_RenderMode_Reset()
{
	R_Mesh_ResetTextureState();
	R_Mesh_SetRenderTargets(r_shadow_fb_fbo, r_shadow_fb_depthtexture, r_shadow_fb_colortexture, nullptr, nullptr, nullptr);
	R_SetViewport(&r_refdef.view.viewport);
	GL_Scissor(r_shadow_lightscissor[0], r_shadow_lightscissor[1], r_shadow_lightscissor[2], r_shadow_lightscissor[3]);
	GL_DepthRange(.0f, 1.0f);
	GL_DepthTest(true);
	GL_DepthMask(false);
	GL_DepthFunc(GL_LEQUAL);
	GL_PolygonOffset(r_refdef.polygonfactor, r_refdef.polygonoffset);CHECKGLERROR
	r_refdef.view.cullface_front = r_shadow_cullface_front;
	r_refdef.view.cullface_back = r_shadow_cullface_back;
	GL_CullFace(r_refdef.view.cullface_back);
	GL_Color(1.0f, 1.0f, 1.0f, 1.0f);
	GL_ColorMask(r_refdef.view.colormask[0], r_refdef.view.colormask[1], r_refdef.view.colormask[2], 1);
	GL_BlendFunc(GL_ONE, GL_ZERO);
	R_SetupShader_Generic_NoTexture(false, false);
	r_shadow_usingshadowmap2d = false;
	r_shadow_usingshadowmaportho = false;
	R_SetStencil(false, 255, GL_KEEP, GL_KEEP, GL_KEEP, GL_ALWAYS, 128, 255);
}

void R_Shadow_ClearStencil()
{
	GL_Clear(GL_STENCIL_BUFFER_BIT, nullptr, 1.0f, 128);
	r_refdef.stats[r_stat_lights_clears]++;
}

void R_Shadow_RenderMode_StencilShadowVolumes(bool zpass)
{
	r_shadow_rendermode_t mode = zpass ? r_shadow_shadowingrendermode_zpass : r_shadow_shadowingrendermode_zfail;
	if (r_shadow_rendermode == mode)
		return;
	R_Shadow_RenderMode_Reset();
	GL_DepthFunc(GL_LESS);
	GL_ColorMask(0, 0, 0, 0);
	GL_PolygonOffset(r_refdef.shadowpolygonfactor, r_refdef.shadowpolygonoffset);
	CHECKGLERROR
	GL_CullFace(GL_NONE);
	R_SetupShader_DepthOrShadow(false, false, false); // FIXME test if we have a skeletal model?
	r_shadow_rendermode = mode;
	switch(mode)
	{
	default:
		break;
	case R_SHADOW_RENDERMODE_ZPASS_STENCILTWOSIDE:
	case R_SHADOW_RENDERMODE_ZPASS_SEPARATESTENCIL:
		R_SetStencilSeparate(true, 255, GL_KEEP, GL_KEEP, GL_INCR, GL_KEEP, GL_KEEP, GL_DECR, GL_ALWAYS, GL_ALWAYS, 128, 255);
		break;
	case R_SHADOW_RENDERMODE_ZFAIL_STENCILTWOSIDE:
	case R_SHADOW_RENDERMODE_ZFAIL_SEPARATESTENCIL:
		R_SetStencilSeparate(true, 255, GL_KEEP, GL_INCR, GL_KEEP, GL_KEEP, GL_DECR, GL_KEEP, GL_ALWAYS, GL_ALWAYS, 128, 255);
		break;
	}
}

static void R_Shadow_MakeVSDCT()
{
	// maps to a 2x3 texture rectangle with normalized coordinates
	// +-
	// XX
	// YY
	// ZZ
	// stores abs(dir.xy), offset.xy/2.5
	unsigned char data[4*6] =
	{
		255, 0, 0x33, 0x33, // +X: <1, 0>, <0.5, 0.5>
		255, 0, 0x99, 0x33, // -X: <1, 0>, <1.5, 0.5>
		0, 255, 0x33, 0x99, // +Y: <0, 1>, <0.5, 1.5>
		0, 255, 0x99, 0x99, // -Y: <0, 1>, <1.5, 1.5>
		0,   0, 0x33, 0xFF, // +Z: <0, 0>, <0.5, 2.5>
		0,   0, 0x99, 0xFF, // -Z: <0, 0>, <1.5, 2.5>
	};
	r_shadow_shadowmapvsdcttexture = R_LoadTextureCubeMap(r_shadow_texturepool, "shadowmapvsdct", 1, data, TEXTYPE_RGBA, TEXF_FORCENEAREST | TEXF_CLAMP | TEXF_ALPHA, -1, nullptr);
}

static void R_Shadow_MakeShadowMap(int side, int size)
{
	switch (r_shadow_shadowmode)
	{
	case R_SHADOW_SHADOWMODE_SHADOWMAP2D:
		if (r_shadow_shadowmap2ddepthtexture) return;
		if (r_fb.usedepthtextures)
		{
			r_shadow_shadowmap2ddepthtexture = R_LoadTextureShadowMap2D(r_shadow_texturepool, "shadowmap", size*2, size*(vid.support.arb_texture_non_power_of_two ? 3 : 4), r_shadow_shadowmapdepthbits >= 24 ? (r_shadow_shadowmapsampler ? TEXTYPE_SHADOWMAP24_COMP : TEXTYPE_SHADOWMAP24_RAW) : (r_shadow_shadowmapsampler ? TEXTYPE_SHADOWMAP16_COMP : TEXTYPE_SHADOWMAP16_RAW), r_shadow_shadowmapsampler);
			r_shadow_shadowmap2ddepthbuffer = nullptr;
			r_shadow_fbo2d = R_Mesh_CreateFramebufferObject(r_shadow_shadowmap2ddepthtexture, nullptr, nullptr, nullptr, nullptr);
		}
		else
		{
			r_shadow_shadowmap2ddepthtexture = R_LoadTexture2D(r_shadow_texturepool, "shadowmaprendertarget", size*2, size*(vid.support.arb_texture_non_power_of_two ? 3 : 4), nullptr, TEXTYPE_COLORBUFFER, TEXF_RENDERTARGET | TEXF_FORCENEAREST | TEXF_CLAMP | TEXF_ALPHA, -1, nullptr);
			r_shadow_shadowmap2ddepthbuffer = R_LoadTextureRenderBuffer(r_shadow_texturepool, "shadowmap", size*2, size*(vid.support.arb_texture_non_power_of_two ? 3 : 4), r_shadow_shadowmapdepthbits >= 24 ? TEXTYPE_DEPTHBUFFER24 : TEXTYPE_DEPTHBUFFER16);
			r_shadow_fbo2d = R_Mesh_CreateFramebufferObject(r_shadow_shadowmap2ddepthbuffer, r_shadow_shadowmap2ddepthtexture, nullptr, nullptr, nullptr);
		}
		break;
	default:
		return;
	}
}

static void R_Shadow_RenderMode_ShadowMap(int side, int clear, int size)
{
	r_viewport_t viewport;
	int flipped;
	GLuint fbo2d = 0;
	float clearcolor[4];
	float nearclip = r_shadow_shadowmapping_nearclip.value / rsurface.rtlight->radius;
	float farclip = 1.0f;
	float bias = r_shadow_shadowmapping_bias.value * nearclip * (1024.0f / size);// * rsurface.rtlight->radius;
	r_shadow_shadowmap_parameters[1] = -nearclip * farclip / (farclip - nearclip) - 0.5f * bias;
	r_shadow_shadowmap_parameters[3] = 0.5f + 0.5f * (farclip + nearclip) / (farclip - nearclip);
	r_shadow_shadowmapside = side;
	r_shadow_shadowmapsize = size;

	r_shadow_shadowmap_parameters[0] = 0.5f * (size - r_shadow_shadowmapborder);
	r_shadow_shadowmap_parameters[2] = r_shadow_shadowmapvsdct ? 2.5f*size : size;
	R_Viewport_InitRectSideView(&viewport, &rsurface.rtlight->matrix_lighttoworld, side, size, r_shadow_shadowmapborder, nearclip, farclip, nullptr);
	if (r_shadow_rendermode == R_SHADOW_RENDERMODE_SHADOWMAP2D) goto init_done;

	// complex unrolled cube approach (more flexible)
	if (r_shadow_shadowmapvsdct && !r_shadow_shadowmapvsdcttexture)
		R_Shadow_MakeVSDCT();
	if (!r_shadow_shadowmap2ddepthtexture)
		R_Shadow_MakeShadowMap(side, r_shadow_shadowmapmaxsize);
	fbo2d = r_shadow_fbo2d;
	r_shadow_shadowmap_texturescale[0] = 1.0f / R_TextureWidth(r_shadow_shadowmap2ddepthtexture);
	r_shadow_shadowmap_texturescale[1] = 1.0f / R_TextureHeight(r_shadow_shadowmap2ddepthtexture);
	r_shadow_rendermode = R_SHADOW_RENDERMODE_SHADOWMAP2D;

	R_Mesh_ResetTextureState();
	R_Shadow_RenderMode_Reset();
	if (r_shadow_shadowmap2ddepthbuffer)
		R_Mesh_SetRenderTargets(fbo2d, r_shadow_shadowmap2ddepthbuffer, r_shadow_shadowmap2ddepthtexture, nullptr, nullptr, nullptr);
	else
		R_Mesh_SetRenderTargets(fbo2d, r_shadow_shadowmap2ddepthtexture, nullptr, nullptr, nullptr, nullptr);
	R_SetupShader_DepthOrShadow(true, r_shadow_shadowmap2ddepthbuffer != nullptr, false); // FIXME test if we have a skeletal model?
	GL_PolygonOffset(r_shadow_shadowmapping_polygonfactor.value, r_shadow_shadowmapping_polygonoffset.value);
	GL_DepthMask(true);
	GL_DepthTest(true);

init_done:
	R_SetViewport(&viewport);
	flipped = (side & 1) ^ (side >> 2);
	r_refdef.view.cullface_front = flipped ? r_shadow_cullface_back : r_shadow_cullface_front;
	r_refdef.view.cullface_back = flipped ? r_shadow_cullface_front : r_shadow_cullface_back;
	if (r_shadow_shadowmap2ddepthbuffer)
	{
		// completely different meaning than in depthtexture approach
		r_shadow_shadowmap_parameters[1] = 0;
		r_shadow_shadowmap_parameters[3] = -bias;
	}
	Vector4Set(clearcolor, 1.0f, 1.0f, 1.0f, 1.0f);
	if (r_shadow_shadowmap2ddepthbuffer)
		GL_ColorMask(1, 1, 1, 1);
	else
		GL_ColorMask(0, 0, 0, 0);
	switch(vid.renderpath)
	{
	case RENDERPATH_GL11:
	case RENDERPATH_GL13:
	case RENDERPATH_GL20:
#ifndef NO_SOFTWARE_RENDERER
	case RENDERPATH_SOFT:
#endif
	case RENDERPATH_GLES1:
	case RENDERPATH_GLES2:
		GL_CullFace(r_refdef.view.cullface_back);
		// OpenGL lets us scissor larger than the viewport, so go ahead and clear all views at once
		if ((clear & ((2 << side) - 1)) == (1 << side)) // only clear if the side is the first in the mask
		{
			// get tightest scissor rectangle that encloses all viewports in the clear mask
			int x1 = clear & 0x15 ? 0 : size;
			int x2 = clear & 0x2A ? 2 * size : size;
			int y1 = clear & 0x03 ? 0 : (clear & 0xC ? size : 2 * size);
			int y2 = clear & 0x30 ? 3 * size : (clear & 0xC ? 2 * size : size);
			GL_Scissor(x1, y1, x2 - x1, y2 - y1);
			if (clear)
			{
				if (r_shadow_shadowmap2ddepthbuffer)
					GL_Clear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT, clearcolor, 1.0f, 0);
				else
					GL_Clear(GL_DEPTH_BUFFER_BIT, clearcolor, 1.0f, 0);
			}
		}
		GL_Scissor(viewport.x, viewport.y, viewport.width, viewport.height);
		break;
#ifndef EXCLUDE_D3D_CODEPATHS
	case RENDERPATH_D3D9:
	case RENDERPATH_D3D10:
	case RENDERPATH_D3D11:
		// we invert the cull mode because we flip the projection matrix
		// NOTE: this actually does nothing because the DrawShadowMap code sets it to doublesided...
		GL_CullFace(r_refdef.view.cullface_front);
		// D3D considers it an error to use a scissor larger than the viewport...  clear just this view
		GL_Scissor(viewport.x, viewport.y, viewport.width, viewport.height);
		if (clear)
		{
			if (r_shadow_shadowmap2ddepthbuffer)
				GL_Clear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT, clearcolor, 1.0f, 0);
			else
				GL_Clear(GL_DEPTH_BUFFER_BIT, clearcolor, 1.0f, 0);
		}
		break;
#endif
	}
}

void R_Shadow_RenderMode_Lighting(bool stenciltest, bool transparent, bool shadowmapping)
{
	R_Mesh_ResetTextureState();
	if (transparent)
	{

		r_shadow_lightscissor =
		{
				r_refdef.view.viewport.x,
				r_refdef.view.viewport.y,
				r_refdef.view.viewport.width,
				r_refdef.view.viewport.height
		};
	}
	R_Shadow_RenderMode_Reset();
	GL_BlendFunc(GL_SRC_ALPHA, GL_ONE);
	if (!transparent)
		GL_DepthFunc(GL_EQUAL);
	// do global setup needed for the chosen lighting mode
	if (r_shadow_rendermode == R_SHADOW_RENDERMODE_LIGHT_GLSL)
		GL_ColorMask(r_refdef.view.colormask[0], r_refdef.view.colormask[1], r_refdef.view.colormask[2], 0);
	r_shadow_usingshadowmap2d = shadowmapping;
	r_shadow_rendermode = r_shadow_lightingrendermode;
	// only draw light where this geometry was already rendered AND the
	// stencil is 128 (values other than this mean shadow)
	if (stenciltest)
		R_SetStencil(true, 255, GL_KEEP, GL_KEEP, GL_KEEP, GL_EQUAL, 128, 255);
	else
		R_SetStencil(false, 255, GL_KEEP, GL_KEEP, GL_KEEP, GL_ALWAYS, 128, 255);
}

__align(mSimdAlign)
static const unsigned short bboxelements[36] =
{
	5, 1, 3, 5, 3, 7,
	6, 2, 0, 6, 0, 4,
	7, 3, 2, 7, 2, 6,
	4, 0, 1, 4, 1, 5,
	4, 5, 7, 4, 7, 6,
	1, 0, 2, 1, 2, 3,
};

__align(mSimdAlign)
static const float bboxpoints[8][3] =
{
	{-1,-1,-1},
	{ 1,-1,-1},
	{-1, 1,-1},
	{ 1, 1,-1},
	{-1,-1, 1},
	{ 1,-1, 1},
	{-1, 1, 1},
	{ 1, 1, 1},
};

void R_Shadow_RenderMode_DrawDeferredLight(bool stenciltest, bool shadowmapping)
{
	float vertex3f[8*3];
	const matrix4x4_t *matrix = &rsurface.rtlight->matrix_lighttoworld;
// do global setup needed for the chosen lighting mode
	R_Shadow_RenderMode_Reset();
	r_shadow_rendermode = r_shadow_lightingrendermode;
	R_EntityMatrix(reinterpret_cast<const matrix4x4_t*>(&identitymatrix));
	GL_BlendFunc(GL_SRC_ALPHA, GL_ONE);
	// only draw light where this geometry was already rendered AND the
	// stencil is 128 (values other than this mean shadow)
	R_SetStencil(stenciltest, 255, GL_KEEP, GL_KEEP, GL_KEEP, GL_EQUAL, 128, 255);
	if (rsurface.rtlight->specularscale > 0 && r_shadow_gloss.integer > 0)
		R_Mesh_SetRenderTargets(r_shadow_prepasslightingdiffusespecularfbo, r_shadow_prepassgeometrydepthbuffer, r_shadow_prepasslightingdiffusetexture, r_shadow_prepasslightingspeculartexture, nullptr, nullptr);
	else
		R_Mesh_SetRenderTargets(r_shadow_prepasslightingdiffusefbo, r_shadow_prepassgeometrydepthbuffer, r_shadow_prepasslightingdiffusetexture, nullptr, nullptr, nullptr);

	r_shadow_usingshadowmap2d = shadowmapping;

	// render the lighting
	R_SetupShader_DeferredLight(rsurface.rtlight);
	for (size_t i = 0; i < 8; i++)
		Matrix4x4_Transform(matrix, bboxpoints[i], vertex3f + i*3);
	GL_ColorMask(1, 1, 1, 1);
	GL_DepthMask(false);
	GL_DepthRange(.0f, 1.0f);
	GL_PolygonOffset(.0f, .0f);
	GL_DepthTest(true);
	GL_DepthFunc(GL_GREATER);
	GL_CullFace(r_refdef.view.cullface_back);
	R_Mesh_PrepareVertices_Vertex3f(8, vertex3f, nullptr, 0);
	R_Mesh_Draw(0, 8, 0, 12, nullptr, nullptr, 0, bboxelements, nullptr, 0);
}

void R_Shadow_UpdateBounceGridTexture()
{
#define MAXBOUNCEGRIDPARTICLESPERLIGHT 1048576
	dlight_t *light;
	int flag = r_refdef.scene.rtworld ? LIGHTFLAG_REALTIMEMODE : LIGHTFLAG_NORMALMODE;
	int bouncecount;
	int hitsupercontentsmask;
	int maxbounce;
	int numpixels;
	int resolution[3];
	int shootparticles;
	int shotparticles;
	int photoncount;
	int tex[3];
	trace_t cliptrace;
	//trace_t cliptrace2;
	//trace_t cliptrace3;
	unsigned char *pixel;
	unsigned char *pixels;
	float *highpixel;
	float *highpixels;
	unsigned int lightindex;
	unsigned int range;
	unsigned int range1;
	unsigned int range2;
	unsigned int seed = (unsigned int)(realtime * 1000.0f);
	vec3_t shotcolor;
	vec3_t baseshotcolor;
	vec3_t surfcolor;
	vec3_t clipend;
	vec3_t clipstart;
	vec3_t clipdiff;
	vec3_t ispacing;
	vec3_t maxs;
	vec3_t mins;
	vec3_t size;
	vec3_t spacing;
	vec3_t lightcolor;
	vec3_t steppos;
	vec3_t stepdelta;
	vec3_t cullmins, cullmaxs;
	vec_t radius;
	vec_t s;
	vec_t lightintensity;
	vec_t photonscaling;
	vec_t photonresidual;
	float m[16];
	float texlerp[2][3];
	float splatcolor[32];
	float pixelweight[8];
	float w;
	int c[4];
	int pixelindex[8];
	int corner;
	int pixelsperband;
	int pixelband;
	int pixelbands;
	int numsteps;
	int step;
	int x, y, z;
	rtlight_t *rtlight;
	r_shadow_bouncegrid_settings_t settings;
	bool enable = r_shadow_bouncegrid.integer != 0 && r_refdef.scene.worldmodel;
	bool allowdirectionalshading = false;
	switch(vid.renderpath)
	{
	case RENDERPATH_GL20:
		allowdirectionalshading = true;
		if (!vid.support.ext_texture_3d)
			return;
		break;
	case RENDERPATH_GLES2:
		// for performance reasons, do not use directional shading on GLES devices
		if (!vid.support.ext_texture_3d)
			return;
		break;
		// these renderpaths do not currently have the code to display the bouncegrid, so disable it on them...
	case RENDERPATH_GL11:
	case RENDERPATH_GL13:
	case RENDERPATH_GLES1:
#ifndef NO_SOFTWARE_RENDERER
	case RENDERPATH_SOFT:
#endif
#ifndef EXCLUDE_D3D_CODEPATHS
	case RENDERPATH_D3D9:
	case RENDERPATH_D3D10:
	case RENDERPATH_D3D11:
#endif
		return;
	}

	r_shadow_bouncegridintensity = r_shadow_bouncegrid_intensity.value;

	// see if there are really any lights to render...
	if (enable && r_shadow_bouncegrid_static.integer)
	{
		enable = false;
		range = (unsigned int)Mem_ExpandableArray_IndexRange(&r_shadow_worldlightsarray); // checked
		for (lightindex = 0;lightindex < range;lightindex++)
		{
			light = (dlight_t *) Mem_ExpandableArray_RecordAtIndex(&r_shadow_worldlightsarray, lightindex);
			if (!light || !(light->flags & flag))
				continue;
			rtlight = &light->rtlight;
			// when static, we skip styled lights because they tend to change...
			if (rtlight->style > 0)
				continue;
			VectorScale(rtlight->color, (rtlight->ambientscale + rtlight->diffusescale + rtlight->specularscale), lightcolor);
			if (!VectorLength2(lightcolor))
				continue;
			enable = true;
			break;
		}
	}

	if (!enable)
	{
		if (r_shadow_bouncegridtexture)
		{
			R_FreeTexture(r_shadow_bouncegridtexture);
			r_shadow_bouncegridtexture = nullptr;
		}
		if (r_shadow_bouncegridpixels)
			Mem_Free(r_shadow_bouncegridpixels);
		r_shadow_bouncegridpixels = nullptr;
		if (r_shadow_bouncegridhighpixels)
			Mem_Free(r_shadow_bouncegridhighpixels);
		r_shadow_bouncegridhighpixels = nullptr;
		r_shadow_bouncegridnumpixels = 0;
		r_shadow_bouncegriddirectional = false;
		return;
	}

	// build up a complete collection of the desired settings, so that memcmp can be used to compare parameters
	memset(&settings, 0, sizeof(settings));
	settings.staticmode                    = r_shadow_bouncegrid_static.integer != 0;
	settings.bounceanglediffuse            = r_shadow_bouncegrid_bounceanglediffuse.integer != 0;
	settings.directionalshading            = (r_shadow_bouncegrid_static.integer != 0 ? r_shadow_bouncegrid_static_directionalshading.integer != 0 : r_shadow_bouncegrid_directionalshading.integer != 0) && allowdirectionalshading;
	settings.dlightparticlemultiplier      = r_shadow_bouncegrid_dlightparticlemultiplier.value;
	settings.hitmodels                     = r_shadow_bouncegrid_hitmodels.integer != 0;
	settings.includedirectlighting         = r_shadow_bouncegrid_includedirectlighting.integer != 0 || r_shadow_bouncegrid.integer == 2;
	settings.lightradiusscale              = (r_shadow_bouncegrid_static.integer != 0 ? r_shadow_bouncegrid_static_lightradiusscale.value : r_shadow_bouncegrid_lightradiusscale.value);
	settings.maxbounce                     = (r_shadow_bouncegrid_static.integer != 0 ? r_shadow_bouncegrid_static_maxbounce.integer : r_shadow_bouncegrid_maxbounce.integer);
	settings.particlebounceintensity       = r_shadow_bouncegrid_particlebounceintensity.value;
	settings.particleintensity             = r_shadow_bouncegrid_particleintensity.value * 16384.0f * (settings.directionalshading ? 4.0f : 1.0f) / (r_shadow_bouncegrid_spacing.value * r_shadow_bouncegrid_spacing.value);
	settings.photons                       = r_shadow_bouncegrid_static.integer ? r_shadow_bouncegrid_static_photons.integer : r_shadow_bouncegrid_photons.integer;
	settings.spacing[0]                    = r_shadow_bouncegrid_spacing.value;
	settings.spacing[1]                    = r_shadow_bouncegrid_spacing.value;
	settings.spacing[2]                    = r_shadow_bouncegrid_spacing.value;
	settings.stablerandom                  = r_shadow_bouncegrid_stablerandom.integer;

	// bound the values for sanity
	settings.photons = bound(1, settings.photons, 1048576);
	settings.lightradiusscale = bound(0.0001f, settings.lightradiusscale, 1024.0f);
	settings.maxbounce = bound(0, settings.maxbounce, 16);
	settings.spacing[0] = bound(1, settings.spacing[0], 512);
	settings.spacing[1] = bound(1, settings.spacing[1], 512);
	settings.spacing[2] = bound(1, settings.spacing[2], 512);

	// get the spacing values
	spacing[0] = settings.spacing[0];
	spacing[1] = settings.spacing[1];
	spacing[2] = settings.spacing[2];
	ispacing[0] = 1.0f / spacing[0];
	ispacing[1] = 1.0f / spacing[1];
	ispacing[2] = 1.0f / spacing[2];

	// calculate texture size enclosing entire world bounds at the spacing
	VectorMA(r_refdef.scene.worldmodel->normalmins, -2.0f, spacing, mins);
	VectorMA(r_refdef.scene.worldmodel->normalmaxs, 2.0f, spacing, maxs);
	VectorSubtract(maxs, mins, size);
	// now we can calculate the resolution we want
	c[0] = (int)math::floorf(size[0] / spacing[0] + 0.5f);
	c[1] = (int)math::floorf(size[1] / spacing[1] + 0.5f);
	c[2] = (int)math::floorf(size[2] / spacing[2] + 0.5f);
	// figure out the exact texture size (honoring power of 2 if required)
	c[0] = bound(4, c[0], (int)vid.maxtexturesize_3d);
	c[1] = bound(4, c[1], (int)vid.maxtexturesize_3d);
	c[2] = bound(4, c[2], (int)vid.maxtexturesize_3d);
	if (vid.support.arb_texture_non_power_of_two)
	{
		resolution[0] = c[0];
		resolution[1] = c[1];
		resolution[2] = c[2];
	}
	else
	{
		for (resolution[0] = 4;resolution[0] < c[0];resolution[0]*=2) ;
		for (resolution[1] = 4;resolution[1] < c[1];resolution[1]*=2) ;
		for (resolution[2] = 4;resolution[2] < c[2];resolution[2]*=2) ;
	}
	size[0] = spacing[0] * resolution[0];
	size[1] = spacing[1] * resolution[1];
	size[2] = spacing[2] * resolution[2];

	// if dynamic we may or may not want to use the world bounds
	// if the dynamic size is smaller than the world bounds, use it instead
	if (!settings.staticmode && (r_shadow_bouncegrid_x.integer * r_shadow_bouncegrid_y.integer * r_shadow_bouncegrid_z.integer < resolution[0] * resolution[1] * resolution[2]))
	{
		// we know the resolution we want
		c[0] = r_shadow_bouncegrid_x.integer;
		c[1] = r_shadow_bouncegrid_y.integer;
		c[2] = r_shadow_bouncegrid_z.integer;
		// now we can calculate the texture size (power of 2 if required)
		c[0] = bound(4, c[0], (int)vid.maxtexturesize_3d);
		c[1] = bound(4, c[1], (int)vid.maxtexturesize_3d);
		c[2] = bound(4, c[2], (int)vid.maxtexturesize_3d);
		if (vid.support.arb_texture_non_power_of_two)
		{
			resolution[0] = c[0];
			resolution[1] = c[1];
			resolution[2] = c[2];
		}
		else
		{
			for (resolution[0] = 4;resolution[0] < c[0];resolution[0]*=2) ;
			for (resolution[1] = 4;resolution[1] < c[1];resolution[1]*=2) ;
			for (resolution[2] = 4;resolution[2] < c[2];resolution[2]*=2) ;
		}
		size[0] = spacing[0] * resolution[0];
		size[1] = spacing[1] * resolution[1];
		size[2] = spacing[2] * resolution[2];
		// center the rendering on the view
		mins[0] = math::floorf(r_refdef.view.origin[0] * ispacing[0] + 0.5f) * spacing[0] - 0.5f * size[0];
		mins[1] = math::floorf(r_refdef.view.origin[1] * ispacing[1] + 0.5f) * spacing[1] - 0.5f * size[1];
		mins[2] = math::floorf(r_refdef.view.origin[2] * ispacing[2] + 0.5f) * spacing[2] - 0.5f * size[2];
	}

	// recalculate the maxs in case the resolution was not satisfactory
	VectorAdd(mins, size, maxs);

	// if all the settings seem identical to the previous update, return
	if (r_shadow_bouncegridtexture && (settings.staticmode || realtime < r_shadow_bouncegridtime + r_shadow_bouncegrid_updateinterval.value) && !memcmp(&r_shadow_bouncegridsettings, &settings, sizeof(settings)))
		return;

	// store the new settings
	r_shadow_bouncegridsettings = settings;

	pixelbands = settings.directionalshading ? 8 : 1;
	pixelsperband = resolution[0]*resolution[1]*resolution[2];
	numpixels = pixelsperband*pixelbands;

	// we're going to update the bouncegrid, update the matrix...
	memset(m, 0, sizeof(m));
	m[0] = 1.0f / size[0];
	m[3] = -mins[0] * m[0];
	m[5] = 1.0f / size[1];
	m[7] = -mins[1] * m[5];
	m[10] = 1.0f / size[2];
	m[11] = -mins[2] * m[10];
	m[15] = 1.0f;
	Matrix4x4_FromArrayFloatD3D(&r_shadow_bouncegridmatrix, m);
	// reallocate pixels for this update if needed...
	if (r_shadow_bouncegridnumpixels != numpixels || !r_shadow_bouncegridpixels || !r_shadow_bouncegridhighpixels)
	{
		if (r_shadow_bouncegridtexture)
		{
			R_FreeTexture(r_shadow_bouncegridtexture);
			r_shadow_bouncegridtexture = nullptr;
		}
		r_shadow_bouncegridpixels = (unsigned char *)Mem_Realloc(r_main_mempool, r_shadow_bouncegridpixels, numpixels * sizeof(unsigned char[4]));
		r_shadow_bouncegridhighpixels = (float *)Mem_Realloc(r_main_mempool, r_shadow_bouncegridhighpixels, numpixels * sizeof(float[4]));
	}
	r_shadow_bouncegridnumpixels = numpixels;
	pixels = r_shadow_bouncegridpixels;
	highpixels = r_shadow_bouncegridhighpixels;//d
	x = pixelsperband*4;
	for (pixelband = 0;pixelband < pixelbands;pixelband++)
	{
		if (pixelband == 1)
			memset(pixels + pixelband * x, 128, x);
		else
			memset(pixels + pixelband * x, 0, x);
	}
	memset(highpixels, 0, numpixels * sizeof(float[4]));
	// figure out what we want to interact with
	if (settings.hitmodels)
		hitsupercontentsmask = SUPERCONTENTS_SOLID | SUPERCONTENTS_BODY;// | SUPERCONTENTS_LIQUIDSMASK;
	else
		hitsupercontentsmask = SUPERCONTENTS_SOLID;// | SUPERCONTENTS_LIQUIDSMASK;
	maxbounce = settings.maxbounce;
	// clear variables that produce warnings otherwise
	memset(splatcolor, 0, sizeof(splatcolor));
	// iterate world rtlights
	range = (unsigned int)Mem_ExpandableArray_IndexRange(&r_shadow_worldlightsarray); // checked
	range1 = settings.staticmode ? 0 : r_refdef.scene.numlights;
	range2 = range + range1;
	photoncount = 0;
	for (lightindex = 0;lightindex < range2;lightindex++)
	{
		if (lightindex < range)
		{
			light = (dlight_t *) Mem_ExpandableArray_RecordAtIndex(&r_shadow_worldlightsarray, lightindex);
			if (!light)
				continue;
			rtlight = &light->rtlight;
			VectorClear(rtlight->photoncolor);
			rtlight->photons = 0;
			if (!(light->flags & flag))
				continue;
			if (settings.staticmode)
			{
				// when static, we skip styled lights because they tend to change...
				if (rtlight->style > 0 && r_shadow_bouncegrid.integer != 2)
					continue;
			}
		}
		else
		{
			rtlight = r_refdef.scene.lights[lightindex - range];
			VectorClear(rtlight->photoncolor);
			rtlight->photons = 0;
		}
		// draw only visible lights (major speedup)
		radius = rtlight->radius * settings.lightradiusscale;
		cullmins[0] = rtlight->shadoworigin[0] - radius;
		cullmins[1] = rtlight->shadoworigin[1] - radius;
		cullmins[2] = rtlight->shadoworigin[2] - radius;
		cullmaxs[0] = rtlight->shadoworigin[0] + radius;
		cullmaxs[1] = rtlight->shadoworigin[1] + radius;
		cullmaxs[2] = rtlight->shadoworigin[2] + radius;
		if (R_CullBox(cullmins, cullmaxs))
			continue;
		if (r_refdef.scene.worldmodel
		 && r_refdef.scene.worldmodel->brush.BoxTouchingVisibleLeafs
		 && !r_refdef.scene.worldmodel->brush.BoxTouchingVisibleLeafs(r_refdef.scene.worldmodel, r_refdef.viewcache.world_leafvisible, cullmins, cullmaxs))
			continue;
		w = r_shadow_lightintensityscale.value * (rtlight->ambientscale + rtlight->diffusescale + rtlight->specularscale);
		if (w * VectorLength2(rtlight->color) == 0.0f)
			continue;
		w *= ((rtlight->style >= 0 && rtlight->style < MAX_LIGHTSTYLES) ? r_refdef.scene.rtlightstylevalue[rtlight->style] : 1);
		VectorScale(rtlight->color, w, rtlight->photoncolor);
		//if (!VectorLength2(rtlight->photoncolor))
		//	continue;
		// shoot particles from this light
		// use a calculation for the number of particles that will not
		// vary with lightstyle, otherwise we get randomized particle
		// distribution, the seeded random is only consistent for a
		// consistent number of particles on this light...
		s = rtlight->radius;
		lightintensity = VectorLength(rtlight->color) * (rtlight->ambientscale + rtlight->diffusescale + rtlight->specularscale);
		if (lightindex >= range)
			lightintensity *= settings.dlightparticlemultiplier;
		rtlight->photons = max(0.0f, lightintensity * s * s);
		photoncount += rtlight->photons;
	}
	photonscaling = (float)settings.photons / max(1, photoncount);
	photonresidual = 0.0f;
	for (lightindex = 0;lightindex < range2;lightindex++)
	{
		if (lightindex < range)
		{
			light = (dlight_t *) Mem_ExpandableArray_RecordAtIndex(&r_shadow_worldlightsarray, lightindex);
			if (!light)
				continue;
			rtlight = &light->rtlight;
		}
		else
			rtlight = r_refdef.scene.lights[lightindex - range];
		// skip a light with no photons
		if (rtlight->photons == 0.0f)
			continue;
		// skip a light with no photon color)
		if (VectorLength2(rtlight->photoncolor) == 0.0f)
			continue;
		photonresidual += rtlight->photons * photonscaling;
		shootparticles = (int)bound(0, photonresidual, MAXBOUNCEGRIDPARTICLESPERLIGHT);
		if (!shootparticles)
			continue;
		photonresidual -= shootparticles;
		radius = rtlight->radius * settings.lightradiusscale;
		s = settings.particleintensity / shootparticles;
		VectorScale(rtlight->photoncolor, s, baseshotcolor);
		r_refdef.stats[r_stat_bouncegrid_lights]++;
		r_refdef.stats[r_stat_bouncegrid_particles] += shootparticles;
		for (shotparticles = 0;shotparticles < shootparticles;shotparticles++)
		{
			if (settings.stablerandom > 0)
				seed = lightindex * 11937 + shotparticles;
			VectorCopy(baseshotcolor, shotcolor);
			VectorCopy(rtlight->shadoworigin, clipstart);
			if (settings.stablerandom < 0)
				VectorRandom(clipend);
			else
				VectorCheeseRandom(clipend);
			VectorMA(clipstart, radius, clipend, clipend);
			for (bouncecount = 0;;bouncecount++)
			{
				r_refdef.stats[r_stat_bouncegrid_traces]++;
				if (settings.staticmode)
				{
					// static mode fires a LOT of rays but none of them are identical, so they are not cached
					cliptrace = CL_TraceLine(clipstart, clipend, settings.staticmode ? MOVE_WORLDONLY : (settings.hitmodels ? MOVE_HITMODEL : MOVE_NOMONSTERS), nullptr, hitsupercontentsmask, collision_extendmovelength.value, true, false, nullptr, true, true);
				}
				else
				{
					// dynamic mode fires many rays and most will match the cache from the previous frame
					cliptrace = CL_Cache_TraceLineSurfaces(clipstart, clipend, settings.staticmode ? MOVE_WORLDONLY : (settings.hitmodels ? MOVE_HITMODEL : MOVE_NOMONSTERS), hitsupercontentsmask);
				}
				if (bouncecount > 0 || settings.includedirectlighting)
				{
					// calculate second order spherical harmonics values (average, slopeX, slopeY, slopeZ)
					// accumulate average shotcolor
					w = VectorLength(shotcolor);
					splatcolor[ 0] = shotcolor[0];
					splatcolor[ 1] = shotcolor[1];
					splatcolor[ 2] = shotcolor[2];
					splatcolor[ 3] = 0.0f;
					if (pixelbands > 1)
					{
						VectorSubtract(clipstart, cliptrace.endpos, clipdiff);
						VectorNormalize(clipdiff);
						// store bentnormal in case the shader has a use for it
						splatcolor[ 4] = clipdiff[0] * w;
						splatcolor[ 5] = clipdiff[1] * w;
						splatcolor[ 6] = clipdiff[2] * w;
						splatcolor[ 7] = w;
						// accumulate directional contributions (+X, +Y, +Z, -X, -Y, -Z)
						splatcolor[ 8] = shotcolor[0] * max(0.0f, clipdiff[0]);
						splatcolor[ 9] = shotcolor[0] * max(0.0f, clipdiff[1]);
						splatcolor[10] = shotcolor[0] * max(0.0f, clipdiff[2]);
						splatcolor[11] = 0.0f;
						splatcolor[12] = shotcolor[1] * max(0.0f, clipdiff[0]);
						splatcolor[13] = shotcolor[1] * max(0.0f, clipdiff[1]);
						splatcolor[14] = shotcolor[1] * max(0.0f, clipdiff[2]);
						splatcolor[15] = 0.0f;
						splatcolor[16] = shotcolor[2] * max(0.0f, clipdiff[0]);
						splatcolor[17] = shotcolor[2] * max(0.0f, clipdiff[1]);
						splatcolor[18] = shotcolor[2] * max(0.0f, clipdiff[2]);
						splatcolor[19] = 0.0f;
						splatcolor[20] = shotcolor[0] * max(0.0f, -clipdiff[0]);
						splatcolor[21] = shotcolor[0] * max(0.0f, -clipdiff[1]);
						splatcolor[22] = shotcolor[0] * max(0.0f, -clipdiff[2]);
						splatcolor[23] = 0.0f;
						splatcolor[24] = shotcolor[1] * max(0.0f, -clipdiff[0]);
						splatcolor[25] = shotcolor[1] * max(0.0f, -clipdiff[1]);
						splatcolor[26] = shotcolor[1] * max(0.0f, -clipdiff[2]);
						splatcolor[27] = 0.0f;
						splatcolor[28] = shotcolor[2] * max(0.0f, -clipdiff[0]);
						splatcolor[29] = shotcolor[2] * max(0.0f, -clipdiff[1]);
						splatcolor[30] = shotcolor[2] * max(0.0f, -clipdiff[2]);
						splatcolor[31] = 0.0f;
					}
					// calculate the number of steps we need to traverse this distance
					VectorSubtract(cliptrace.endpos, clipstart, stepdelta);
					numsteps = (int)(VectorLength(stepdelta) * ispacing[0]);
					numsteps = bound(1, numsteps, 1024);
					w = 1.0f / numsteps;
					VectorScale(stepdelta, w, stepdelta);
					VectorMA(clipstart, 0.5f, stepdelta, steppos);
					for (step = 0;step < numsteps;step++)
					{
						r_refdef.stats[r_stat_bouncegrid_splats]++;
						// figure out which texture pixel this is in
						texlerp[1][0] = ((steppos[0] - mins[0]) * ispacing[0]) - 0.5f;
						texlerp[1][1] = ((steppos[1] - mins[1]) * ispacing[1]) - 0.5f;
						texlerp[1][2] = ((steppos[2] - mins[2]) * ispacing[2]) - 0.5f;
						tex[0] = (int)math::floorf(texlerp[1][0]);
						tex[1] = (int)math::floorf(texlerp[1][1]);
						tex[2] = (int)math::floorf(texlerp[1][2]);
						if (tex[0] >= 1 && tex[1] >= 1 && tex[2] >= 1 && tex[0] < resolution[0] - 2 && tex[1] < resolution[1] - 2 && tex[2] < resolution[2] - 2)
						{
							// it is within bounds...  do the real work now
							// calculate the lerp factors
							texlerp[1][0] -= tex[0];
							texlerp[1][1] -= tex[1];
							texlerp[1][2] -= tex[2];
							texlerp[0][0] = 1.0f - texlerp[1][0];
							texlerp[0][1] = 1.0f - texlerp[1][1];
							texlerp[0][2] = 1.0f - texlerp[1][2];
							// calculate individual pixel indexes and weights
							pixelindex[0] = (((tex[2]  )*resolution[1]+tex[1]  )*resolution[0]+tex[0]  );pixelweight[0] = (texlerp[0][0]*texlerp[0][1]*texlerp[0][2]);
							pixelindex[1] = (((tex[2]  )*resolution[1]+tex[1]  )*resolution[0]+tex[0]+1);pixelweight[1] = (texlerp[1][0]*texlerp[0][1]*texlerp[0][2]);
							pixelindex[2] = (((tex[2]  )*resolution[1]+tex[1]+1)*resolution[0]+tex[0]  );pixelweight[2] = (texlerp[0][0]*texlerp[1][1]*texlerp[0][2]);
							pixelindex[3] = (((tex[2]  )*resolution[1]+tex[1]+1)*resolution[0]+tex[0]+1);pixelweight[3] = (texlerp[1][0]*texlerp[1][1]*texlerp[0][2]);
							pixelindex[4] = (((tex[2]+1)*resolution[1]+tex[1]  )*resolution[0]+tex[0]  );pixelweight[4] = (texlerp[0][0]*texlerp[0][1]*texlerp[1][2]);
							pixelindex[5] = (((tex[2]+1)*resolution[1]+tex[1]  )*resolution[0]+tex[0]+1);pixelweight[5] = (texlerp[1][0]*texlerp[0][1]*texlerp[1][2]);
							pixelindex[6] = (((tex[2]+1)*resolution[1]+tex[1]+1)*resolution[0]+tex[0]  );pixelweight[6] = (texlerp[0][0]*texlerp[1][1]*texlerp[1][2]);
							pixelindex[7] = (((tex[2]+1)*resolution[1]+tex[1]+1)*resolution[0]+tex[0]+1);pixelweight[7] = (texlerp[1][0]*texlerp[1][1]*texlerp[1][2]);
							// update the 8 pixels...
							for (pixelband = 0;pixelband < pixelbands;pixelband++)
							{
								for (corner = 0;corner < 8;corner++)
								{
									// calculate address for pixel
									w = pixelweight[corner];
									pixel = pixels + 4 * pixelindex[corner] + pixelband * pixelsperband * 4;
									highpixel = highpixels + 4 * pixelindex[corner] + pixelband * pixelsperband * 4;
									// add to the high precision pixel color
									highpixel[0] += (splatcolor[pixelband*4+0]*w);
									highpixel[1] += (splatcolor[pixelband*4+1]*w);
									highpixel[2] += (splatcolor[pixelband*4+2]*w);
									highpixel[3] += (splatcolor[pixelband*4+3]*w);
									// flag the low precision pixel as needing to be updated
									pixel[3] = 255;
				
								}
							}
						}
						VectorAdd(steppos, stepdelta, steppos);
					}
				}
				if (cliptrace.fraction >= 1.0f)
					break;
				r_refdef.stats[r_stat_bouncegrid_hits]++;
				if (bouncecount >= maxbounce)
					break;
				// scale down shot color by bounce intensity and texture color (or 50% if no texture reported)
				// also clamp the resulting color to never add energy, even if the user requests extreme values
				if (cliptrace.hittexture && cliptrace.hittexture->currentskinframe)
					VectorCopy(cliptrace.hittexture->currentskinframe->avgcolor, surfcolor);
				else
					VectorSet(surfcolor, 0.5f, 0.5f, 0.5f);
				VectorScale(surfcolor, settings.particlebounceintensity, surfcolor);
				surfcolor[0] = min(surfcolor[0], 1.0f);
				surfcolor[1] = min(surfcolor[1], 1.0f);
				surfcolor[2] = min(surfcolor[2], 1.0f);
				VectorMultiply(shotcolor, surfcolor, shotcolor);
				if (VectorLength2(baseshotcolor) == 0.0f)
					break;
				r_refdef.stats[r_stat_bouncegrid_bounces]++;
				if (settings.bounceanglediffuse)
				{
					// random direction, primarily along plane normal
					s = VectorDistance(cliptrace.endpos, clipend);
					if (settings.stablerandom < 0)
						VectorRandom(clipend);
					else
						VectorCheeseRandom(clipend);
					VectorMA(cliptrace.plane.normal, 0.95f, clipend, clipend);
					VectorNormalize(clipend);
					VectorScale(clipend, s, clipend);
				}
				else
				{
					// reflect the remaining portion of the line across plane normal
					VectorSubtract(clipend, cliptrace.endpos, clipdiff);
					VectorReflect(clipdiff, 1.0, cliptrace.plane.normal, clipend);
				}
				// calculate the new line start and end
				VectorCopy(cliptrace.endpos, clipstart);
				VectorAdd(clipstart, clipend, clipend);
			}
		}
	}
	// generate pixels array from highpixels array
	// skip first and last columns, rows, and layers as these are blank
	// the pixel[3] value was written above, so we can use it to detect only pixels that need to be calculated
	for (pixelband = 0;pixelband < pixelbands;pixelband++)
	{
		for (z = 1;z < resolution[2]-1;z++)
		{
			for (y = 1;y < resolution[1]-1;y++)
			{
				for (x = 1, pixelindex[0] = ((pixelband*resolution[2]+z)*resolution[1]+y)*resolution[0]+x, pixel = pixels + 4*pixelindex[0], highpixel = highpixels + 4*pixelindex[0];x < resolution[0]-1;x++, pixel += 4, highpixel += 4)
				{
					// only convert pixels that were hit by photons
					if (pixel[3] == 255)
					{
						// normalize the bentnormal...
						if (pixelband == 1)
						{
							VectorNormalize(highpixel);
							c[0] = (int)(highpixel[0]*128.0f+128.0f);
							c[1] = (int)(highpixel[1]*128.0f+128.0f);
							c[2] = (int)(highpixel[2]*128.0f+128.0f);
							c[3] = (int)(highpixel[3]*128.0f+128.0f);
						}
						else
						{
							c[0] = (int)(highpixel[0]*256.0f);
							c[1] = (int)(highpixel[1]*256.0f);
							c[2] = (int)(highpixel[2]*256.0f);
							c[3] = (int)(highpixel[3]*256.0f);
						}
						pixel[2] = (unsigned char)bound(0, c[0], 255);
						pixel[1] = (unsigned char)bound(0, c[1], 255);
						pixel[0] = (unsigned char)bound(0, c[2], 255);
						pixel[3] = (unsigned char)bound(0, c[3], 255);
					}
				}
			}
		}
	}
	if (r_shadow_bouncegridtexture && r_shadow_bouncegridresolution[0] == resolution[0] && r_shadow_bouncegridresolution[1] == resolution[1] && r_shadow_bouncegridresolution[2] == resolution[2] && r_shadow_bouncegriddirectional == settings.directionalshading)
		R_UpdateTexture(r_shadow_bouncegridtexture, pixels, 0, 0, 0, resolution[0], resolution[1], resolution[2]*pixelbands);
	else
	{
		VectorCopy(resolution, r_shadow_bouncegridresolution);
		r_shadow_bouncegriddirectional = settings.directionalshading;
		if (r_shadow_bouncegridtexture)
			R_FreeTexture(r_shadow_bouncegridtexture);
		r_shadow_bouncegridtexture = R_LoadTexture3D(r_shadow_texturepool, "bouncegrid", resolution[0], resolution[1], resolution[2]*pixelbands, pixels, TEXTYPE_BGRA, TEXF_CLAMP | TEXF_ALPHA | TEXF_FORCELINEAR, 0, nullptr);
	}
	r_shadow_bouncegridtime = realtime;
}

void R_Shadow_RenderMode_VisibleShadowVolumes()
{
	R_Shadow_RenderMode_Reset();
	GL_BlendFunc(GL_ONE, GL_ONE);
	GL_DepthRange(0.0f, 1.0f);
	GL_DepthTest(r_showshadowvolumes.integer < 2);
	GL_Color(0.0f, 0.0125f * r_refdef.view.colorscale, 0.1f * r_refdef.view.colorscale, 1.0f);
	GL_PolygonOffset(r_refdef.shadowpolygonfactor, r_refdef.shadowpolygonoffset);CHECKGLERROR
	GL_CullFace(GL_NONE);
	r_shadow_rendermode = R_SHADOW_RENDERMODE_VISIBLEVOLUMES;
}

void R_Shadow_RenderMode_VisibleLighting(bool stenciltest, bool transparent)
{
	R_Shadow_RenderMode_Reset();
	GL_BlendFunc(GL_ONE, GL_ONE);
	GL_DepthRange(.0f, 1.0f);
	GL_DepthTest(r_showlighting.integer < 2);
	GL_Color(.1f * r_refdef.view.colorscale, .0125f * r_refdef.view.colorscale, .0f, 1.0f);
	if (!transparent)
		GL_DepthFunc(GL_EQUAL);
	R_SetStencil(stenciltest, 255, GL_KEEP, GL_KEEP, GL_KEEP, GL_EQUAL, 128, 255);
	r_shadow_rendermode = R_SHADOW_RENDERMODE_VISIBLELIGHTING;
}

void R_Shadow_RenderMode_End()
{
	R_Shadow_RenderMode_Reset();
	R_Shadow_RenderMode_ActiveLight(nullptr);
	GL_DepthMask(true);
	GL_Scissor(r_refdef.view.viewport.x, r_refdef.view.viewport.y, r_refdef.view.viewport.width, r_refdef.view.viewport.height);
	r_shadow_rendermode = R_SHADOW_RENDERMODE_NONE;
}

__align(mSimdAlign)
int bboxedges[12][2] =
{
	// top
	{0, 1}, // +X
	{0, 2}, // +Y
	{1, 3}, // Y, +X
	{2, 3}, // X, +Y
	// bottom
	{4, 5}, // +X
	{4, 6}, // +Y
	{5, 7}, // Y, +X
	{6, 7}, // X, +Y
	// verticals
	{0, 4}, // +Z
	{1, 5}, // X, +Z
	{2, 6}, // Y, +Z
	{3, 7}, // XY, +Z
};

bool R_Shadow_ScissorForBBox(const float *mins, const float *maxs)
{
	if (!r_shadow_scissor.integer || r_shadow_usingdeferredprepass || r_trippy.integer)
	{
		/*r_shadow_lightscissor[0] = r_refdef.view.viewport.x;
		r_shadow_lightscissor[1] = r_refdef.view.viewport.y;
		r_shadow_lightscissor[2] = r_refdef.view.viewport.width;
		r_shadow_lightscissor[3] = r_refdef.view.viewport.height;*/
		r_shadow_lightscissor =
		{
				r_refdef.view.viewport.x,
				r_refdef.view.viewport.y,
				r_refdef.view.viewport.width,
				r_refdef.view.viewport.height
		};
		return false;
	}
	if(R_ScissorForBBox(mins, maxs, reinterpret_cast<int*>(&r_shadow_lightscissor)))
		return true; // invisible
	if(r_shadow_lightscissor[0] != r_refdef.view.viewport.x
	|| r_shadow_lightscissor[1] != r_refdef.view.viewport.y
	|| r_shadow_lightscissor[2] != r_refdef.view.viewport.width
	|| r_shadow_lightscissor[3] != r_refdef.view.viewport.height)
		r_refdef.stats[r_stat_lights_scissored]++;
	return false;
}

#include "renderer/shadows/R_Shadow_RenderLighting.hpp"

#include "renderer/shadows/RTLight_Update.hpp"

#include "renderer/shadows/RTLight_Compiler.hpp"

static void R_Shadow_ComputeShadowCasterCullingPlanes(rtlight_t *rtlight)
{
	int i, j;
	mplane_t plane;
	// reset the count of frustum planes
	// see rtlight->cached_frustumplanes definition for how much this array
	// can hold
	rtlight->cached_numfrustumplanes = 0;
#if !NO_SILLY_CVARS
	if (r_trippy.integer)
		return;
#endif
	// haven't implemented a culling path for ortho rendering
	if (!r_refdef.view.useperspective)
	{
		// check if the light is on screen and copy the 4 planes if it is
		for (i = 0;i < 4;i++)
			if (PlaneDiff(rtlight->shadoworigin, &r_refdef.view.frustum[i]) < -0.03125f)
				break;
		if (i == 4)
			for (i = 0;i < 4;i++)
				rtlight->cached_frustumplanes[rtlight->cached_numfrustumplanes++] = r_refdef.view.frustum[i];
		return;
	}

	// generate a deformed frustum that includes the light origin, this is
	// used to cull shadow casting surfaces that can not possibly cast a
	// shadow onto the visible light-receiving surfaces, which can be a
	// performance gain
	//
	// if the light origin is onscreen the result will be 4 planes exactly
	// if the light origin is offscreen on only one axis the result will
	// be exactly 5 planes (split-side case)
	// if the light origin is offscreen on two axes the result will be
	// exactly 4 planes (stretched corner case)
	for (i = 0;i < 4;i++)
	{
		// quickly reject standard frustum planes that put the light
		// origin outside the frustum
		if (PlaneDiff(rtlight->shadoworigin, &r_refdef.view.frustum[i]) < -0.03125f)
			continue;
		// copy the plane
		rtlight->cached_frustumplanes[rtlight->cached_numfrustumplanes++] = r_refdef.view.frustum[i];
	}
	// if all the standard frustum planes were accepted, the light is onscreen
	// otherwise we need to generate some more planes below...
	if (rtlight->cached_numfrustumplanes < 4)
	{
		// at least one of the stock frustum planes failed, so we need to
		// create one or two custom planes to enclose the light origin
		for (i = 0;i < 4;i++)
		{
			// create a plane using the view origin and light origin, and a
			// single point from the frustum corner set
			TriangleNormal(r_refdef.view.origin, r_refdef.view.frustumcorner[i], rtlight->shadoworigin, plane.normal);
			VectorNormalize(plane.normal);
			plane.dist = DotProduct(r_refdef.view.origin, plane.normal);
			// see if this plane is backwards and flip it if so
			for (j = 0;j < 4;j++)
				if (j != i && DotProduct(r_refdef.view.frustumcorner[j], plane.normal) - plane.dist < -0.03125f)
					break;
			if (j < 4)
			{
				VectorNegate(plane.normal, plane.normal);
				plane.dist *= -1;
				// flipped plane, test again to see if it is now valid
				for (j = 0;j < 4;j++)
					if (j != i && DotProduct(r_refdef.view.frustumcorner[j], plane.normal) - plane.dist < -0.03125f)
						break;
				// if the plane is still not valid, then it is dividing the
				// frustum and has to be rejected
				if (j < 4)
					continue;
			}
			// we have created a valid plane, compute extra info
			PlaneClassify(&plane);
			// copy the plane
			rtlight->cached_frustumplanes[rtlight->cached_numfrustumplanes++] = plane;

			// if we've found 5 frustum planes then we have constructed a
			// proper split-side case and do not need to keep searching for
			// planes to enclose the light origin
			if (rtlight->cached_numfrustumplanes == 5)
				break;

		}
	}

}

#include "renderer/shadows/R_Shadow_DrawWorldShadow.hpp"

static void R_Shadow_DrawEntityShadow(entity_render_t *ent)
{
	vec3_t relativeshadoworigin, relativeshadowmins, relativeshadowmaxs;
	vec_t relativeshadowradius;
	RSurf_ActiveModelEntity(ent, false, false, false);
	Matrix4x4_Transform(&ent->inversematrix, rsurface.rtlight->shadoworigin, relativeshadoworigin);
	// we need to re-init the shader for each entity because the matrix changed
	relativeshadowradius = rsurface.rtlight->radius / ent->scale;
	relativeshadowmins[0] = relativeshadoworigin[0] - relativeshadowradius;
	relativeshadowmins[1] = relativeshadoworigin[1] - relativeshadowradius;
	relativeshadowmins[2] = relativeshadoworigin[2] - relativeshadowradius;
	relativeshadowmaxs[0] = relativeshadoworigin[0] + relativeshadowradius;
	relativeshadowmaxs[1] = relativeshadoworigin[1] + relativeshadowradius;
	relativeshadowmaxs[2] = relativeshadoworigin[2] + relativeshadowradius;
	switch (r_shadow_rendermode)
	{
	case R_SHADOW_RENDERMODE_SHADOWMAP2D:
		ent->model->DrawShadowMap(r_shadow_shadowmapside, ent, relativeshadoworigin, nullptr, relativeshadowradius, ent->model->nummodelsurfaces, ent->model->sortedmodelsurfaces, nullptr, relativeshadowmins, relativeshadowmaxs);
		break;
	default:
		ent->model->DrawShadowVolume(ent, relativeshadoworigin, nullptr, relativeshadowradius, ent->model->nummodelsurfaces, ent->model->sortedmodelsurfaces, relativeshadowmins, relativeshadowmaxs);
		break;
	}
	rsurface.entity = nullptr; // used only by R_GetCurrentTexture and RSurf_ActiveWorldEntity/RSurf_ActiveModelEntity
}

void R_Shadow_SetupEntityLight(const entity_render_t *ent)
{
	// set up properties for rendering light onto this entity
	RSurf_ActiveModelEntity(ent, true, true, false);
	Matrix4x4_Concat(&rsurface.entitytolight, &rsurface.rtlight->matrix_worldtolight, &ent->matrix);
	Matrix4x4_Concat(&rsurface.entitytoattenuationxyz, &matrix_attenuationxyz, &rsurface.entitytolight);
	Matrix4x4_Concat(&rsurface.entitytoattenuationz, &matrix_attenuationz, &rsurface.entitytolight);
	Matrix4x4_Transform(&ent->inversematrix, rsurface.rtlight->shadoworigin, rsurface.entitylightorigin);
}

static void R_Shadow_DrawWorldLight(int numsurfaces, int *surfacelist, const unsigned char *lighttrispvs)
{
	if (!r_refdef.scene.worldmodel->DrawLight)
		return;

	// set up properties for rendering light onto this entity
	RSurf_ActiveWorldEntity();
	rsurface.entitytolight = rsurface.rtlight->matrix_worldtolight;
	Matrix4x4_Concat(&rsurface.entitytoattenuationxyz, &matrix_attenuationxyz, &rsurface.entitytolight);
	Matrix4x4_Concat(&rsurface.entitytoattenuationz, &matrix_attenuationz, &rsurface.entitytolight);
	VectorCopy(rsurface.rtlight->shadoworigin, rsurface.entitylightorigin);

	r_refdef.scene.worldmodel->DrawLight(r_refdef.scene.worldentity, numsurfaces, surfacelist, lighttrispvs);

	rsurface.entity = nullptr; // used only by R_GetCurrentTexture and RSurf_ActiveWorldEntity/RSurf_ActiveModelEntity
}

static void R_Shadow_DrawEntityLight(entity_render_t *ent)
{
	dp_model_t *model = ent->model;
	if (!model->DrawLight)
		return;

	R_Shadow_SetupEntityLight(ent);

	model->DrawLight(ent, model->nummodelsurfaces, model->sortedmodelsurfaces, nullptr);

	rsurface.entity = nullptr; // used only by R_GetCurrentTexture and RSurf_ActiveWorldEntity/RSurf_ActiveModelEntity
}

#include "renderer/shadows/R_Shadow_PrepareLight.hpp"

#include "renderer/shadows/R_Shadow_DrawLight.hpp"

static void R_Shadow_FreeDeferred()
{
	R_Mesh_DestroyFramebufferObject(r_shadow_prepassgeometryfbo);
	r_shadow_prepassgeometryfbo = 0;

	R_Mesh_DestroyFramebufferObject(r_shadow_prepasslightingdiffusespecularfbo);
	r_shadow_prepasslightingdiffusespecularfbo = 0;

	R_Mesh_DestroyFramebufferObject(r_shadow_prepasslightingdiffusefbo);
	r_shadow_prepasslightingdiffusefbo = 0;

	if (r_shadow_prepassgeometrydepthbuffer)
		R_FreeTexture(r_shadow_prepassgeometrydepthbuffer);
	r_shadow_prepassgeometrydepthbuffer = nullptr;

	if (r_shadow_prepassgeometrynormalmaptexture)
		R_FreeTexture(r_shadow_prepassgeometrynormalmaptexture);
	r_shadow_prepassgeometrynormalmaptexture = nullptr;

	if (r_shadow_prepasslightingdiffusetexture)
		R_FreeTexture(r_shadow_prepasslightingdiffusetexture);
	r_shadow_prepasslightingdiffusetexture = nullptr;

	if (r_shadow_prepasslightingspeculartexture)
		R_FreeTexture(r_shadow_prepasslightingspeculartexture);
	r_shadow_prepasslightingspeculartexture = nullptr;
}

void R_Shadow_DrawPrepass()
{
	int i;
	int flag;
	int lnum;
	size_t lightindex;
	dlight_t *light;
	size_t range;
	entity_render_t *ent;
	float clearcolor[4];

	R_Mesh_ResetTextureState();
	GL_DepthMask(true);
	GL_ColorMask(1, 1, 1, 1);
	GL_BlendFunc(GL_ONE, GL_ZERO);
	GL_Color(1.0f, 1.0f, 1.0f, 1.0f);
	GL_DepthTest(true);
	R_Mesh_SetRenderTargets(r_shadow_prepassgeometryfbo, r_shadow_prepassgeometrydepthbuffer, r_shadow_prepassgeometrynormalmaptexture, nullptr, nullptr, nullptr);
	Vector4Set(clearcolor, .5f, .5f, .5f, 1.0f);
	GL_Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, clearcolor, 1.0f, 0);
#if ENABLE_TIMEREPORT
	if (r_timereport_active)
		R_TimeReport("prepasscleargeom");
#endif
	if (cl.csqc_vidvars.drawworld && r_refdef.scene.worldmodel && r_refdef.scene.worldmodel->DrawPrepass)
		r_refdef.scene.worldmodel->DrawPrepass(r_refdef.scene.worldentity);
#if ENABLE_TIMEREPORT
	if (r_timereport_active)
		R_TimeReport("prepassworld");
#endif
	for (i = 0;i < r_refdef.scene.numentities;i++)
	{
		if (!r_refdef.viewcache.entityvisible[i])
			continue;
		ent = r_refdef.scene.entities[i];
		if (ent->model && ent->model->DrawPrepass != nullptr)
			ent->model->DrawPrepass(ent);
	}
#if ENABLE_TIMEREPORT
	if (r_timereport_active)
		R_TimeReport("prepassmodels");
#endif
	GL_DepthMask(false);
	GL_ColorMask(1, 1, 1, 1);
	GL_Color(1.0f, 1.0f, 1.0f, 1.0f);
	GL_DepthTest(true);
	R_Mesh_SetRenderTargets(r_shadow_prepasslightingdiffusespecularfbo, r_shadow_prepassgeometrydepthbuffer, r_shadow_prepasslightingdiffusetexture, r_shadow_prepasslightingspeculartexture, nullptr, nullptr);
	Vector4Set(clearcolor, .0f, .0f, .0f, .0f);
	GL_Clear(GL_COLOR_BUFFER_BIT, clearcolor, 1.0f, 0);
#if ENABLE_TIMEREPORT
	if (r_timereport_active)
		R_TimeReport("prepassclearlit");
#endif
	R_Shadow_RenderMode_Begin();

	flag = r_refdef.scene.rtworld ? LIGHTFLAG_REALTIMEMODE : LIGHTFLAG_NORMALMODE;
	if (r_shadow_debuglight.integer >= 0)
	{
		lightindex = r_shadow_debuglight.integer;
		light = (dlight_t *) Mem_ExpandableArray_RecordAtIndex(&r_shadow_worldlightsarray, lightindex);
		if (light && (light->flags & flag) && light->rtlight.draw)
			R_Shadow_DrawLight(&light->rtlight);
	}
	else
	{
		range = Mem_ExpandableArray_IndexRange(&r_shadow_worldlightsarray); // checked
		for (lightindex = 0;lightindex < range;lightindex++)
		{
			light = (dlight_t *) Mem_ExpandableArray_RecordAtIndex(&r_shadow_worldlightsarray, lightindex);
			if (light && (light->flags & flag) && light->rtlight.draw)
				R_Shadow_DrawLight(&light->rtlight);
		}
	}
	if (r_refdef.scene.rtdlight)
		for (lnum = 0;lnum < r_refdef.scene.numlights;lnum++)
			if (r_refdef.scene.lights[lnum]->draw)
				R_Shadow_DrawLight(r_refdef.scene.lights[lnum]);

	R_Shadow_RenderMode_End();
#if ENABLE_TIMEREPORT
	if (r_timereport_active)
		R_TimeReport("prepasslights");
#endif
}

void R_Shadow_DrawLightSprites();
void R_Shadow_PrepareLights(int fbo, rtexture_t *depthtexture, rtexture_t *colortexture)
{
	int flag;
	int lnum;
	size_t lightindex;
	dlight_t *light;
	size_t range;
	float f;

	if (r_shadow_shadowmapmaxsize != bound(1, r_shadow_shadowmapping_maxsize.integer, (int)vid.maxtexturesize_2d / 4) ||
		(r_shadow_shadowmode != R_SHADOW_SHADOWMODE_STENCIL) != (r_shadow_shadowmapping.integer || r_shadow_deferred.integer) ||
		r_shadow_shadowmapvsdct != (r_shadow_shadowmapping_vsdct.integer != 0 && vid.renderpath == RENDERPATH_GL20) || 
		r_shadow_shadowmapfilterquality != r_shadow_shadowmapping_filterquality.integer || 
		r_shadow_shadowmapshadowsampler != (vid.support.arb_shadow && r_shadow_shadowmapping_useshadowsampler.integer) || 
		r_shadow_shadowmapdepthbits != r_shadow_shadowmapping_depthbits.integer || 
		r_shadow_shadowmapborder != bound(0, r_shadow_shadowmapping_bordersize.integer, 16) ||
		r_shadow_shadowmapdepthtexture != r_fb.usedepthtextures)
		R_Shadow_FreeShadowMaps();

	r_shadow_fb_fbo = fbo;
	r_shadow_fb_depthtexture = depthtexture;
	r_shadow_fb_colortexture = colortexture;

	r_shadow_usingshadowmaportho = false;

	switch (vid.renderpath)
	{
	case RENDERPATH_GL20:
#ifndef EXCLUDE_D3D_CODEPATHS
	case RENDERPATH_D3D9:
	case RENDERPATH_D3D10:
	case RENDERPATH_D3D11:
#endif
#ifndef NO_SOFTWARE_RENDERER
	case RENDERPATH_SOFT:
#endif
#ifndef USE_GLES2
		if (!r_shadow_deferred.integer || r_shadow_shadowmode == R_SHADOW_SHADOWMODE_STENCIL || !vid.support.ext_framebuffer_object || vid.maxdrawbuffers < 2)
		{
			r_shadow_usingdeferredprepass = false;
			if (r_shadow_prepass_width)
				R_Shadow_FreeDeferred();
			r_shadow_prepass_width = r_shadow_prepass_height = 0;
			break;
		}

		if (r_shadow_prepass_width != vid.width || r_shadow_prepass_height != vid.height)
		{
			R_Shadow_FreeDeferred();

			r_shadow_usingdeferredprepass = true;
			r_shadow_prepass_width = vid.width;
			r_shadow_prepass_height = vid.height;
			r_shadow_prepassgeometrydepthbuffer = R_LoadTextureRenderBuffer(r_shadow_texturepool, "prepassgeometrydepthbuffer", vid.width, vid.height, TEXTYPE_DEPTHBUFFER24);
			r_shadow_prepassgeometrynormalmaptexture = R_LoadTexture2D(r_shadow_texturepool, "prepassgeometrynormalmap", vid.width, vid.height, nullptr, TEXTYPE_COLORBUFFER32F, TEXF_RENDERTARGET | TEXF_CLAMP | TEXF_ALPHA | TEXF_FORCENEAREST, -1, nullptr);
			r_shadow_prepasslightingdiffusetexture = R_LoadTexture2D(r_shadow_texturepool, "prepasslightingdiffuse", vid.width, vid.height, nullptr, TEXTYPE_COLORBUFFER16F, TEXF_RENDERTARGET | TEXF_CLAMP | TEXF_ALPHA | TEXF_FORCENEAREST, -1, nullptr);
			r_shadow_prepasslightingspeculartexture = R_LoadTexture2D(r_shadow_texturepool, "prepasslightingspecular", vid.width, vid.height, nullptr, TEXTYPE_COLORBUFFER16F, TEXF_RENDERTARGET | TEXF_CLAMP | TEXF_ALPHA | TEXF_FORCENEAREST, -1, nullptr);

			// set up the geometry pass fbo (depth + normalmap)
			r_shadow_prepassgeometryfbo = R_Mesh_CreateFramebufferObject(r_shadow_prepassgeometrydepthbuffer, r_shadow_prepassgeometrynormalmaptexture, nullptr, nullptr, nullptr);
			R_Mesh_SetRenderTargets(r_shadow_prepassgeometryfbo, r_shadow_prepassgeometrydepthbuffer, r_shadow_prepassgeometrynormalmaptexture, nullptr, nullptr, nullptr);
			// render depth into a renderbuffer and other important properties into the normalmap texture

			// set up the lighting pass fbo (diffuse + specular)
			r_shadow_prepasslightingdiffusespecularfbo = R_Mesh_CreateFramebufferObject(r_shadow_prepassgeometrydepthbuffer, r_shadow_prepasslightingdiffusetexture, r_shadow_prepasslightingspeculartexture, nullptr, nullptr);
			R_Mesh_SetRenderTargets(r_shadow_prepasslightingdiffusespecularfbo, r_shadow_prepassgeometrydepthbuffer, r_shadow_prepasslightingdiffusetexture, r_shadow_prepasslightingspeculartexture, nullptr, nullptr);
			// render diffuse into one texture and specular into another,
			// with depth and normalmap bound as textures,
			// with depth bound as attachment as well

			// set up the lighting pass fbo (diffuse)
			r_shadow_prepasslightingdiffusefbo = R_Mesh_CreateFramebufferObject(r_shadow_prepassgeometrydepthbuffer, r_shadow_prepasslightingdiffusetexture, nullptr, nullptr, nullptr);
			R_Mesh_SetRenderTargets(r_shadow_prepasslightingdiffusefbo, r_shadow_prepassgeometrydepthbuffer, r_shadow_prepasslightingdiffusetexture, nullptr, nullptr, nullptr);
			// render diffuse into one texture,
			// with depth and normalmap bound as textures,
			// with depth bound as attachment as well
		}
#endif
		break;
	case RENDERPATH_GL11:
	case RENDERPATH_GL13:
	case RENDERPATH_GLES1:
	case RENDERPATH_GLES2:
		r_shadow_usingdeferredprepass = false;
		break;
	}

	R_Shadow_EnlargeLeafSurfaceTrisBuffer(r_refdef.scene.worldmodel->brush.num_leafs, r_refdef.scene.worldmodel->num_surfaces, r_refdef.scene.worldmodel->brush.shadowmesh ? r_refdef.scene.worldmodel->brush.shadowmesh->numtriangles : r_refdef.scene.worldmodel->surfmesh.num_triangles, r_refdef.scene.worldmodel->surfmesh.num_triangles);

	flag = r_refdef.scene.rtworld ? LIGHTFLAG_REALTIMEMODE : LIGHTFLAG_NORMALMODE;
	if (r_shadow_debuglight.integer >= 0)
	{
		lightindex = r_shadow_debuglight.integer;
		light = (dlight_t *) Mem_ExpandableArray_RecordAtIndex(&r_shadow_worldlightsarray, lightindex);
		if (light)
			R_Shadow_PrepareLight(&light->rtlight);
	}
	else
	{
		range = Mem_ExpandableArray_IndexRange(&r_shadow_worldlightsarray); // checked
		for (lightindex = 0;lightindex < range;lightindex++)
		{
			light = (dlight_t *) Mem_ExpandableArray_RecordAtIndex(&r_shadow_worldlightsarray, lightindex);
			if (light && (light->flags & flag))
				R_Shadow_PrepareLight(&light->rtlight);
		}
	}
	if (r_refdef.scene.rtdlight)
	{
		for (lnum = 0;lnum < r_refdef.scene.numlights;lnum++)
			R_Shadow_PrepareLight(r_refdef.scene.lights[lnum]);
	}
	else if(gl_flashblend.integer)
	{
		for (lnum = 0;lnum < r_refdef.scene.numlights;lnum++)
		{
			rtlight_t *rtlight = r_refdef.scene.lights[lnum];
			f = ((rtlight->style >= 0 && rtlight->style < MAX_LIGHTSTYLES) ? r_refdef.scene.lightstylevalue[rtlight->style] : 1) * r_shadow_lightintensityscale.value;
			VectorScale(rtlight->color, f, rtlight->currentcolor);
		}
	}

	if (r_editlights.integer)
		R_Shadow_DrawLightSprites();
}

void R_Shadow_DrawLights()
{
	R_Shadow_RenderMode_Begin();
	
	{
		if (r_shadow_debuglight.integer >= 0)
		{
			const size_t lightindex = r_shadow_debuglight.integer;
			dlight_t *RESTRICT const light = (dlight_t *) Mem_ExpandableArray_RecordAtIndex(&r_shadow_worldlightsarray, lightindex);
			if (light != nullptr)
				R_Shadow_DrawLight(&light->rtlight);
		}
		else
		{
			const int flag = r_refdef.scene.rtworld ? LIGHTFLAG_REALTIMEMODE : LIGHTFLAG_NORMALMODE;
			const size_t range = Mem_ExpandableArray_IndexRange(&r_shadow_worldlightsarray); // checked
			for (size_t lightindex = 0; lightindex < range; lightindex++)
			{
				dlight_t *RESTRICT const light = (dlight_t *) Mem_ExpandableArray_RecordAtIndex(&r_shadow_worldlightsarray, lightindex);
				if (light != nullptr && (light->flags & flag))
					R_Shadow_DrawLight(&light->rtlight);
			}
		}
	}
	if (r_refdef.scene.rtdlight)
	{
		const size32 numLights = r_refdef.scene.numlights;
		for (size32 lightIndex = 0; lightIndex < numLights; lightIndex++)
			R_Shadow_DrawLight(r_refdef.scene.lights[lightIndex]);
	}
	R_Shadow_RenderMode_End();
}

#define MAX_MODELSHADOWS 1024
static int r_shadow_nummodelshadows;
static entity_render_t *r_shadow_modelshadows[MAX_MODELSHADOWS];

void R_Shadow_PrepareModelShadows()
{
	prvm_vec3_t prvmshadowdir, prvmshadowfocus;
	vec3_t shadowdir, shadowforward, shadowright, shadoworigin, shadowfocus, shadowmins, shadowmaxs;

	r_shadow_nummodelshadows = 0;
	if (!r_refdef.scene.numentities)
		return;

	switch (r_shadow_shadowmode)
	{
	case R_SHADOW_SHADOWMODE_SHADOWMAP2D:
		if (r_shadows.integer >= 2) 
			break;
		// fall through
	case R_SHADOW_SHADOWMODE_STENCIL:
		if (!vid.stencil)
			return;
		{
			for (size_t i = 0; i < r_refdef.scene.numentities; i++)
			{
				entity_render_t *RESTRICT const ent = r_refdef.scene.entities[i];
				if (ent->model && ent->model->DrawShadowVolume != nullptr && (!ent->model->brush.submodel || r_shadows_castfrombmodels.integer) && (ent->flags & RENDER_SHADOW))
				{
					if (r_shadow_nummodelshadows >= MAX_MODELSHADOWS)
						break;
					r_shadow_modelshadows[r_shadow_nummodelshadows++] = ent;
					R_AnimCache_GetEntity(ent, false, false);
				}
			}
		}
		return;
	default:
		return;
	}

	float radius 	=	.5f * 
	({
		const float size	=	2.0f * r_shadow_shadowmapmaxsize;
		const float scale 	=	r_shadow_shadowmapping_precision.value * r_shadows_shadowmapscale.value;
		size / scale;
	});
	
	Math_atov(r_shadows_throwdirection.string, prvmshadowdir);
	VectorCopy(prvmshadowdir, shadowdir);
	VectorNormalize(shadowdir);
	
	float dot1 			= DotProduct(r_refdef.view.forward, shadowdir);
	const float dot2 	= DotProduct(r_refdef.view.up, shadowdir);
	
	if (math::fabs(dot1) <= math::fabs(dot2))
		VectorMA(r_refdef.view.forward, -dot1, shadowdir, shadowforward);
	else
		VectorMA(r_refdef.view.up, -dot2, shadowdir, shadowforward);
		
	VectorNormalize(shadowforward);
	CrossProduct(shadowdir, shadowforward, shadowright);
	Math_atov(r_shadows_focus.string, prvmshadowfocus);
	
	VectorCopy(prvmshadowfocus, shadowfocus);
	VectorM(shadowfocus[0], r_refdef.view.right, shadoworigin);
	VectorMA(shadoworigin, shadowfocus[1], r_refdef.view.up, shadoworigin);
	VectorMA(shadoworigin, -shadowfocus[2], r_refdef.view.forward, shadoworigin);
	VectorAdd(shadoworigin, r_refdef.view.origin, shadoworigin);
	
	if (shadowfocus[0] || shadowfocus[1] || shadowfocus[2])
		dot1 = 1.0f;
	VectorMA(shadoworigin, (1.0f - math::fabs(dot1)) * radius, shadowforward, shadoworigin);

	shadowmins[0] = shadoworigin[0] - r_shadows_throwdistance.value *
	fabsf(shadowdir[0]) - radius * (fabsf(shadowforward[0]) + fabsf(shadowright[0]));
	shadowmins[1] = shadoworigin[1] - r_shadows_throwdistance.value * fabsf(shadowdir[1]) - radius * (fabsf(shadowforward[1]) + fabsf(shadowright[1]));
	shadowmins[2] = shadoworigin[2] - r_shadows_throwdistance.value * fabsf(shadowdir[2]) - radius * (fabsf(shadowforward[2]) + fabsf(shadowright[2]));
	shadowmaxs[0] = shadoworigin[0] + r_shadows_throwdistance.value * fabsf(shadowdir[0]) + radius * (fabsf(shadowforward[0]) + fabsf(shadowright[0]));
	shadowmaxs[1] = shadoworigin[1] + r_shadows_throwdistance.value * fabsf(shadowdir[1]) + radius * (fabsf(shadowforward[1]) + fabsf(shadowright[1]));
	shadowmaxs[2] = shadoworigin[2] + r_shadows_throwdistance.value * fabsf(shadowdir[2]) + radius * (fabsf(shadowforward[2]) + fabsf(shadowright[2]));

	{
		const size32 numEntities = r_refdef.scene.numentities;
		entity_render_t** RESTRICT const entities = r_refdef.scene.entities;

		for (size32 i = 0; i < numEntities; i++)
		{
			entity_render_t *RESTRICT const ent = entities[i];
			if (!BoxesOverlap(ent->mins, ent->maxs, shadowmins, shadowmaxs))
				continue;
			// cast shadows from anything of the map (submodels are optional)
			if (ent->model && ent->model->DrawShadowMap != nullptr && (!ent->model->brush.submodel || r_shadows_castfrombmodels.integer) && (ent->flags & RENDER_SHADOW))
			{
				if (r_shadow_nummodelshadows >= MAX_MODELSHADOWS)
					break;
				r_shadow_modelshadows[r_shadow_nummodelshadows++] = ent;
				R_AnimCache_GetEntity(ent, false, false);
			}
		}
	}
}

#include "renderer/shadows/R_DrawModel.hpp"

static void R_BeginCoronaQuery(rtlight_t *rtlight, float scale, bool usequery)
{
	vec3_t centerorigin;
#if defined(GL_SAMPLES_PASSED_ARB) && !defined(USE_GLES2)
	float vertex3f[12];
#endif
	// if it's too close, skip it
	if (VectorLength(rtlight->currentcolor) < (1.0f / 256.0f))
		return;
	float zdist = (DotProduct(rtlight->shadoworigin, r_refdef.view.forward) - DotProduct(r_refdef.view.origin, r_refdef.view.forward));
	if (zdist < 32.0f)
 		return;
	if (usequery && r_numqueries + 2 <= r_maxqueries)
	{
		rtlight->corona_queryindex_allpixels = r_queries[r_numqueries++];
		rtlight->corona_queryindex_visiblepixels = r_queries[r_numqueries++];
		// we count potential samples in the middle of the screen, we count actual samples at the light location, this allows counting potential samples of off-screen lights
		VectorMA(r_refdef.view.origin, zdist, r_refdef.view.forward, centerorigin);

		switch(vid.renderpath)
		{
		case RENDERPATH_GL11:
		case RENDERPATH_GL13:
		case RENDERPATH_GL20:
		case RENDERPATH_GLES1:
		case RENDERPATH_GLES2:
#if defined(GL_SAMPLES_PASSED_ARB) && !defined(USE_GLES2)
			CHECKGLERROR
			// NOTE: GL_DEPTH_TEST must be enabled or ATI won't count samples, so use GL_DepthFunc instead
			qglBeginQueryARB(GL_SAMPLES_PASSED_ARB, rtlight->corona_queryindex_allpixels);
			GL_DepthFunc(GL_ALWAYS);
			R_CalcSprite_Vertex3f(vertex3f, centerorigin, r_refdef.view.right, r_refdef.view.up, scale, -scale, -scale, scale);
			R_Mesh_PrepareVertices_Vertex3f(4, vertex3f, nullptr, 0);
			R_Mesh_Draw(0, 4, 0, 2, polygonelement3i, nullptr, 0, polygonelement3s, nullptr, 0);
			qglEndQueryARB(GL_SAMPLES_PASSED_ARB);
			GL_DepthFunc(GL_LEQUAL);
			qglBeginQueryARB(GL_SAMPLES_PASSED_ARB, rtlight->corona_queryindex_visiblepixels);
			R_CalcSprite_Vertex3f(vertex3f, rtlight->shadoworigin, r_refdef.view.right, r_refdef.view.up, scale, -scale, -scale, scale);
			R_Mesh_PrepareVertices_Vertex3f(4, vertex3f, nullptr, 0);
			R_Mesh_Draw(0, 4, 0, 2, polygonelement3i, nullptr, 0, polygonelement3s, nullptr, 0);
			qglEndQueryARB(GL_SAMPLES_PASSED_ARB);
			CHECKGLERROR
#endif
			break;
#ifndef EXCLUDE_D3D_CODEPATHS
		case RENDERPATH_D3D9:
			Con_DPrintf("FIXME D3D9 %s:%i %s\n", __FILE__, __LINE__, __FUNCTION__);
			break;
		case RENDERPATH_D3D10:
			Con_DPrintf("FIXME D3D10 %s:%i %s\n", __FILE__, __LINE__, __FUNCTION__);
			break;
		case RENDERPATH_D3D11:
			Con_DPrintf("FIXME D3D11 %s:%i %s\n", __FILE__, __LINE__, __FUNCTION__);
			break;
#endif
#ifndef NO_SOFTWARE_RENDERER
		case RENDERPATH_SOFT:
			break;
#endif
		}
	}
	rtlight->corona_visibility = bound(0, (zdist - 32) / 32, 1);
}

__align(mSimdAlign)
static float spritetexcoord2f[4*2] = {0, 1, 0, 0, 1, 0, 1, 1};

static void R_DrawCorona(rtlight_t *rtlight, float cscale, float scale)
{
	vec3_t color;
	unsigned int occlude = 0;
	GLint allpixels = 0, visiblepixels = 0;

	// now we have to check the query result
	if (rtlight->corona_queryindex_visiblepixels)
	{
		switch(vid.renderpath)
		{
		case RENDERPATH_GL11:
		case RENDERPATH_GL13:
		case RENDERPATH_GL20:
		case RENDERPATH_GLES1:
		case RENDERPATH_GLES2:
#if defined(GL_SAMPLES_PASSED_ARB) && !defined(USE_GLES2)
			CHECKGLERROR
			// See if we can use the GPU-side method to prevent implicit sync
			if (vid.support.arb_query_buffer_object) {
#define BUFFER_OFFSET(i)    (reinterpret_cast<GLint*>(reinterpret_cast<char*>(0) + (i)))
				if (!r_shadow_occlusion_buf) {
					qglGenBuffersARB(1, &r_shadow_occlusion_buf);
					qglBindBufferARB(GL_QUERY_BUFFER_ARB, r_shadow_occlusion_buf);
					qglBufferDataARB(GL_QUERY_BUFFER_ARB, 8, nullptr, GL_DYNAMIC_COPY);
				} else {
					qglBindBufferARB(GL_QUERY_BUFFER_ARB, r_shadow_occlusion_buf);
				}
				qglGetQueryObjectivARB(rtlight->corona_queryindex_visiblepixels, GL_QUERY_RESULT_ARB, BUFFER_OFFSET(0));
				qglGetQueryObjectivARB(rtlight->corona_queryindex_allpixels, GL_QUERY_RESULT_ARB, BUFFER_OFFSET(4));
				qglBindBufferBase(GL_UNIFORM_BUFFER, 0, r_shadow_occlusion_buf);
				occlude = MATERIALFLAG_OCCLUDE;
			} else {
				qglGetQueryObjectivARB(rtlight->corona_queryindex_visiblepixels, GL_QUERY_RESULT_ARB, &visiblepixels);
				qglGetQueryObjectivARB(rtlight->corona_queryindex_allpixels, GL_QUERY_RESULT_ARB, &allpixels); 
				if (visiblepixels < 1 || allpixels < 1)
					return;
				rtlight->corona_visibility *= bound(0, (float)visiblepixels / (float)allpixels, 1);
			}
			cscale *= rtlight->corona_visibility;
			CHECKGLERROR
			break;
#else
			return;
#endif
#ifndef EXCLUDE_D3D_CODEPATHS
		case RENDERPATH_D3D9:
			Con_DPrintf("FIXME D3D9 %s:%i %s\n", __FILE__, __LINE__, __FUNCTION__);
			return;
		case RENDERPATH_D3D10:
			Con_DPrintf("FIXME D3D10 %s:%i %s\n", __FILE__, __LINE__, __FUNCTION__);
			return;
		case RENDERPATH_D3D11:
			Con_DPrintf("FIXME D3D11 %s:%i %s\n", __FILE__, __LINE__, __FUNCTION__);
			return;
#endif
#ifndef NO_SOFTWARE_RENDERER
		case RENDERPATH_SOFT:
			return;
#endif
		default:
			return;
		}
	}
	else
	{
		// FIXME: these traces should scan all render entities instead of cl.world
		if (CL_TraceLine(r_refdef.view.origin, rtlight->shadoworigin, MOVE_NOMONSTERS, nullptr, SUPERCONTENTS_SOLID, collision_extendmovelength.value, true, false, nullptr, false, true).fraction < 1)
			return;
	}
	VectorScale(rtlight->currentcolor, cscale, color);
	if (VectorLength(color) > (1.0f / 256.0f))
	{
		float vertex3f[12];
		bool negated = (color[0] + color[1] + color[2] < .0f) && vid.support.ext_blend_subtract;
		if(negated)
		{
			VectorNegate(color, color);
			GL_BlendEquationSubtract(true);
		}
		R_CalcSprite_Vertex3f(vertex3f, rtlight->shadoworigin, r_refdef.view.right, r_refdef.view.up, scale, -scale, -scale, scale);
		RSurf_ActiveCustomEntity(reinterpret_cast<const matrix4x4_t*>(&identitymatrix),
				reinterpret_cast<const matrix4x4_t*>(&identitymatrix),
		RENDER_NODEPTHTEST, 0, color[0], color[1], color[2], 1, 4, vertex3f,
		spritetexcoord2f, nullptr, nullptr, nullptr, nullptr, 2, polygonelement3i,
		polygonelement3s, false, false);
		R_DrawCustomSurface(r_shadow_lightcorona,
				reinterpret_cast<const matrix4x4_t*>(&identitymatrix),
		MATERIALFLAG_ADD | MATERIALFLAG_BLENDED | MATERIALFLAG_FULLBRIGHT | MATERIALFLAG_NOCULLFACE |
		MATERIALFLAG_NODEPTHTEST | occlude, 0, 4, 0, 2, false, false);
		if(negated)
			GL_BlendEquationSubtract(false);
	}
}

void R_Shadow_DrawCoronas()
{
	int i, flag;
	bool usequery = false;
	size_t lightindex;
	dlight_t *light;
	rtlight_t *rtlight;
	size_t range;
	if (r_coronas.value < (1.0f / 256.0f) && !gl_flashblend.integer)
		return;
	if (r_fb.water.renderingscene)
		return;
	flag = r_refdef.scene.rtworld ? LIGHTFLAG_REALTIMEMODE : LIGHTFLAG_NORMALMODE;
	R_EntityMatrix(reinterpret_cast<const matrix4x4_t*>(&identitymatrix));

	range = Mem_ExpandableArray_IndexRange(&r_shadow_worldlightsarray); // checked

	// check occlusion of coronas
	// use GL_ARB_occlusion_query if available
	// otherwise use raytraces
	r_numqueries = 0;
	switch (vid.renderpath)
	{
	case RENDERPATH_GL11:
	case RENDERPATH_GL13:
	case RENDERPATH_GL20:
	case RENDERPATH_GLES1:
	case RENDERPATH_GLES2:
		usequery = vid.support.arb_occlusion_query && r_coronas_occlusionquery.integer;
#if defined(GL_SAMPLES_PASSED_ARB) && !defined(USE_GLES2)
		if (usequery)
		{
			GL_ColorMask(0, 0, 0, 0);
			if (r_maxqueries < ((unsigned int)range + r_refdef.scene.numlights) * 2)
			if (r_maxqueries < MAX_OCCLUSION_QUERIES)
			{
				i = r_maxqueries;
				r_maxqueries = ((unsigned int)range + r_refdef.scene.numlights) * 4;
				r_maxqueries = min(r_maxqueries, MAX_OCCLUSION_QUERIES);
				CHECKGLERROR
				qglGenQueriesARB(r_maxqueries - i, r_queries + i);
				CHECKGLERROR
			}
			RSurf_ActiveWorldEntity();
			GL_BlendFunc(GL_ONE, GL_ZERO);
			GL_CullFace(GL_NONE);
			GL_DepthMask(false);
			GL_DepthRange(.0f, 1.0f);
			GL_PolygonOffset(.0f, .0f);
			GL_DepthTest(true);
			R_Mesh_ResetTextureState();
			R_SetupShader_Generic_NoTexture(false, false);
		}
#endif
		break;
#ifndef EXCLUDE_D3D_CODEPATHS
	case RENDERPATH_D3D9:
		usequery = false;
		break;
	case RENDERPATH_D3D10:
		Con_DPrintf("FIXME D3D10 %s:%i %s\n", __FILE__, __LINE__, __FUNCTION__);
		break;
	case RENDERPATH_D3D11:
		Con_DPrintf("FIXME D3D11 %s:%i %s\n", __FILE__, __LINE__, __FUNCTION__);
		break;
#endif
#ifndef NO_SOFTWARE_RENDERER
	case RENDERPATH_SOFT:
		usequery = false;
		break;
#endif
	}
	for (lightindex = 0;lightindex < range;lightindex++)
	{
		light = (dlight_t *) Mem_ExpandableArray_RecordAtIndex(&r_shadow_worldlightsarray, lightindex);
		if (!light)
			continue;
		rtlight = &light->rtlight;
		rtlight->corona_visibility = 0;
		rtlight->corona_queryindex_visiblepixels = 0;
		rtlight->corona_queryindex_allpixels = 0;
		if (!(rtlight->flags & flag))
			continue;
		if (rtlight->corona <= 0)
			continue;
		if (r_shadow_debuglight.integer >= 0 && r_shadow_debuglight.integer != (int)lightindex)
			continue;
		R_BeginCoronaQuery(rtlight, rtlight->radius * rtlight->coronasizescale * r_coronas_occlusionsizescale.value, usequery);
	}
	for (i = 0;i < r_refdef.scene.numlights;i++)
	{
		rtlight = r_refdef.scene.lights[i];
		rtlight->corona_visibility = 0;
		rtlight->corona_queryindex_visiblepixels = 0;
		rtlight->corona_queryindex_allpixels = 0;
		if (!(rtlight->flags & flag))
			continue;
		if (rtlight->corona <= 0)
			continue;
		R_BeginCoronaQuery(rtlight, rtlight->radius * rtlight->coronasizescale * r_coronas_occlusionsizescale.value, usequery);
	}
	if (usequery)
		GL_ColorMask(r_refdef.view.colormask[0], r_refdef.view.colormask[1], r_refdef.view.colormask[2], 1);

	// now draw the coronas using the query data for intensity info
	for (lightindex = 0;lightindex < range;lightindex++)
	{
		light = (dlight_t *) Mem_ExpandableArray_RecordAtIndex(&r_shadow_worldlightsarray, lightindex);
		if (!light)
			continue;
		rtlight = &light->rtlight;
		if (rtlight->corona_visibility <= 0)
			continue;
		R_DrawCorona(rtlight, rtlight->corona * r_coronas.value * 0.25f, rtlight->radius * rtlight->coronasizescale);
	}
	for (i = 0;i < r_refdef.scene.numlights;i++)
	{
		rtlight = r_refdef.scene.lights[i];
		if (rtlight->corona_visibility <= 0)
			continue;
		if (gl_flashblend.integer)
			R_DrawCorona(rtlight, rtlight->corona, rtlight->radius * rtlight->coronasizescale * 2.0f);
		else
			R_DrawCorona(rtlight, rtlight->corona * r_coronas.value * 0.25f, rtlight->radius * rtlight->coronasizescale);
	}
}

#include "renderer/shadows/R_Shadow_WorldLight.hpp"

#include "renderer/shadows/R_Shadow_EditLights.hpp"

/*
=============================================================================

LIGHT SAMPLING

=============================================================================
*/

void R_LightPoint(float *color, const vec3_t p, const int flags)
{
	int i, numlights, flag;
	float f, relativepoint[3], dist, dist2, lightradius2;
	vec3_t diffuse, n;
	rtlight_t *light;
	dlight_t *dlight;

	if (unlikely(r_fullbright.integer != 0))
	{
		VectorSet(color, 1.0f, 1.0f, 1.0f);
		return;
	}

	VectorClear(color);

	if (flags & LP_LIGHTMAP)
	{
		if (!r_fullbright.integer && r_refdef.scene.worldmodel && r_refdef.scene.worldmodel->lit && r_refdef.scene.worldmodel->brush.LightPoint)
		{
			VectorClear(diffuse);
			r_refdef.scene.worldmodel->brush.LightPoint(r_refdef.scene.worldmodel, p, color, diffuse, n);
			VectorAdd(color, diffuse, color);
		}
		else
			VectorSet(color, 1.0f, 1.0f, 1.0f);
		color[0] += r_refdef.scene.ambient;
		color[1] += r_refdef.scene.ambient;
		color[2] += r_refdef.scene.ambient;
	}

	if (flags & LP_RTWORLD)
	{
		flag = r_refdef.scene.rtworld ? LIGHTFLAG_REALTIMEMODE : LIGHTFLAG_NORMALMODE;
		numlights = (int)Mem_ExpandableArray_IndexRange(&r_shadow_worldlightsarray);
		for (i = 0; i < numlights; i++)
		{
			dlight = (dlight_t *) Mem_ExpandableArray_RecordAtIndex(&r_shadow_worldlightsarray, i);
			if (!dlight)
				continue;
			light = &dlight->rtlight;
			if (!(light->flags & flag))
				continue;
			// sample
			lightradius2 = light->radius * light->radius;
			VectorSubtract(light->shadoworigin, p, relativepoint);
			dist2 = VectorLength2(relativepoint);
			if (dist2 >= lightradius2)
				continue;
			dist = math::sqrtf(dist2) / light->radius;
			f = dist < 1 ? (r_shadow_lightintensityscale.value * ((1.0f - dist) * r_shadow_lightattenuationlinearscale.value / (r_shadow_lightattenuationdividebias.value + dist*dist))) : 0;
			if (f <= 0)
				continue;
			// todo: add to both ambient and diffuse
			if (!light->shadow || CL_TraceLine(p, light->shadoworigin, MOVE_NOMONSTERS, nullptr, SUPERCONTENTS_SOLID, collision_extendmovelength.value, true, false, nullptr, false, true).fraction == 1)
				VectorMA(color, f, light->currentcolor, color);
		}
	}
	if (flags & LP_DYNLIGHT)
	{
		// sample dlights
		for (i = 0;i < r_refdef.scene.numlights;i++)
		{
			light = r_refdef.scene.lights[i];
			// sample
			lightradius2 = light->radius * light->radius;
			VectorSubtract(light->shadoworigin, p, relativepoint);
			dist2 = VectorLength2(relativepoint);
			if (dist2 >= lightradius2)
				continue;
			dist = sqrt(dist2) / light->radius;
			f = dist < 1 ? (r_shadow_lightintensityscale.value * ((1.0f - dist) * r_shadow_lightattenuationlinearscale.value / (r_shadow_lightattenuationdividebias.value + dist*dist))) : 0;
			if (f <= 0)
				continue;
			// todo: add to both ambient and diffuse
			if (!light->shadow || CL_TraceLine(p, light->shadoworigin, MOVE_NOMONSTERS, nullptr, SUPERCONTENTS_SOLID, collision_extendmovelength.value, true, false, nullptr, false, true).fraction == 1)
				VectorMA(color, f, light->color, color);
		}
	}
}

void R_CompleteLightPoint(vec3_t ambient, vec3_t diffuse, vec3_t lightdir, const vec3_t p, const int flags)
{
	int i, numlights, flag;
	rtlight_t *light;
	dlight_t *dlight;
	float relativepoint[3];
	float color[3];
	float dir[3];
	float dist;
	float dist2;
	float intensity;
	float sample[5*3];
	float lightradius2;

	if (unlikely(r_fullbright.integer != 0))
	{
		VectorSet(ambient, 1.0f, 1.0f, 1.0f);
		VectorClear(diffuse);
		VectorClear(lightdir);
		return;
	}

	if (flags == LP_LIGHTMAP)
	{
		VectorSet(ambient, r_refdef.scene.ambient, r_refdef.scene.ambient, r_refdef.scene.ambient);
		VectorClear(diffuse);
		VectorClear(lightdir);
		if (r_refdef.scene.worldmodel && r_refdef.scene.worldmodel->lit && r_refdef.scene.worldmodel->brush.LightPoint)
			r_refdef.scene.worldmodel->brush.LightPoint(r_refdef.scene.worldmodel, p, ambient, diffuse, lightdir);
		else
			VectorSet(ambient, 1.0f, 1.0f, 1.0f);
		return;
	}

	memset(sample, 0, sizeof(sample));
	VectorSet(sample, r_refdef.scene.ambient, r_refdef.scene.ambient, r_refdef.scene.ambient);

	if ((flags & LP_LIGHTMAP) && r_refdef.scene.worldmodel && r_refdef.scene.worldmodel->lit && r_refdef.scene.worldmodel->brush.LightPoint)
	{
		vec3_t tempambient;
		VectorClear(tempambient);
		VectorClear(color);
		VectorClear(relativepoint);
		r_refdef.scene.worldmodel->brush.LightPoint(r_refdef.scene.worldmodel, p, tempambient, color, relativepoint);
		VectorScale(tempambient, r_refdef.lightmapintensity, tempambient);
		VectorScale(color, r_refdef.lightmapintensity, color);
		VectorAdd(sample, tempambient, sample);
		VectorMA(sample    , 0.5f            , color, sample    );
		VectorMA(sample + 3, relativepoint[0], color, sample + 3);
		VectorMA(sample + 6, relativepoint[1], color, sample + 6);
		VectorMA(sample + 9, relativepoint[2], color, sample + 9);
		// calculate a weighted average light direction as well
		intensity = VectorLength(color);
		VectorMA(sample + 12, intensity, relativepoint, sample + 12);
	}

	if (flags & LP_RTWORLD)
	{
		flag = r_refdef.scene.rtworld ? LIGHTFLAG_REALTIMEMODE : LIGHTFLAG_NORMALMODE;
		numlights = (int)Mem_ExpandableArray_IndexRange(&r_shadow_worldlightsarray);
		for (i = 0; i < numlights; i++)
		{
			dlight = (dlight_t *) Mem_ExpandableArray_RecordAtIndex(&r_shadow_worldlightsarray, i);
			if (!dlight)
				continue;
			light = &dlight->rtlight;
			if (!(light->flags & flag))
				continue;
			// sample
			lightradius2 = light->radius * light->radius;
			VectorSubtract(light->shadoworigin, p, relativepoint);
			dist2 = VectorLength2(relativepoint);
			if (dist2 >= lightradius2)
				continue;
			dist = sqrt(dist2) / light->radius;
			intensity = min(1.0f, (1.0f - dist) * r_shadow_lightattenuationlinearscale.value / (r_shadow_lightattenuationdividebias.value + dist*dist)) * r_shadow_lightintensityscale.value;
			if (intensity <= 0.0f)
				continue;
			if (light->shadow && CL_TraceLine(p, light->shadoworigin, MOVE_NOMONSTERS, nullptr, SUPERCONTENTS_SOLID, collision_extendmovelength.value, true, false, nullptr, false, true).fraction < 1)
				continue;
			// scale down intensity to add to both ambient and diffuse
			//intensity *= 0.5f;
			VectorNormalize(relativepoint);
			VectorScale(light->currentcolor, intensity, color);
			VectorMA(sample    , 0.5f            , color, sample    );
			VectorMA(sample + 3, relativepoint[0], color, sample + 3);
			VectorMA(sample + 6, relativepoint[1], color, sample + 6);
			VectorMA(sample + 9, relativepoint[2], color, sample + 9);
			// calculate a weighted average light direction as well
			intensity *= VectorLength(color);
			VectorMA(sample + 12, intensity, relativepoint, sample + 12);
		}
		// FIXME: sample bouncegrid too!
	}

	if (flags & LP_DYNLIGHT)
	{
		// sample dlights
		for (i = 0;i < r_refdef.scene.numlights;i++)
		{
			light = r_refdef.scene.lights[i];
			// sample
			lightradius2 = light->radius * light->radius;
			VectorSubtract(light->shadoworigin, p, relativepoint);
			dist2 = VectorLength2(relativepoint);
			if (dist2 >= lightradius2)
				continue;
			dist = sqrt(dist2) / light->radius;
			intensity = (1.0f - dist) * r_shadow_lightattenuationlinearscale.value / (r_shadow_lightattenuationdividebias.value + dist*dist) * r_shadow_lightintensityscale.value;
			if (intensity <= 0.0f)
				continue;
			if (light->shadow && CL_TraceLine(p, light->shadoworigin, MOVE_NOMONSTERS, nullptr, SUPERCONTENTS_SOLID, collision_extendmovelength.value, true, false, nullptr, false, true).fraction < 1)
				continue;
			// scale down intensity to add to both ambient and diffuse
			//intensity *= 0.5f;
			VectorNormalize(relativepoint);
			VectorScale(light->currentcolor, intensity, color);
			VectorMA(sample    , 0.5f            , color, sample    );
			VectorMA(sample + 3, relativepoint[0], color, sample + 3);
			VectorMA(sample + 6, relativepoint[1], color, sample + 6);
			VectorMA(sample + 9, relativepoint[2], color, sample + 9);
			// calculate a weighted average light direction as well
			intensity *= VectorLength(color);
			VectorMA(sample + 12, intensity, relativepoint, sample + 12);
		}
	}

	// calculate the direction we'll use to reduce the sample to a directional light source
	VectorCopy(sample + 12, dir);
	//VectorSet(dir, sample[3] + sample[4] + sample[5], sample[6] + sample[7] + sample[8], sample[9] + sample[10] + sample[11]);
	VectorNormalize(dir);
	// extract the diffuse color along the chosen direction and scale it
	diffuse[0] = (dir[0]*sample[3] + dir[1]*sample[6] + dir[2]*sample[ 9] + sample[ 0]);
	diffuse[1] = (dir[0]*sample[4] + dir[1]*sample[7] + dir[2]*sample[10] + sample[ 1]);
	diffuse[2] = (dir[0]*sample[5] + dir[1]*sample[8] + dir[2]*sample[11] + sample[ 2]);
	// subtract some of diffuse from ambient
	VectorMA(sample, -0.333f, diffuse, ambient);
	// store the normalized lightdir
	VectorCopy(dir, lightdir);
}
