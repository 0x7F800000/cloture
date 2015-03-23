#pragma once

CVar r_shadow_bumpscale_basetexture
{
	0,
	"r_shadow_bumpscale_basetexture",
	"0",
	"generate fake bumpmaps from diffuse textures at this bumpyness, try 4 to match tenebrae, higher values increase depth, requires r_restart to take effect"
};

CVar r_shadow_bumpscale_bumpmap
{
	0,
	"r_shadow_bumpscale_bumpmap",
	"4",
	"what magnitude to interpret _bump.tga textures as, higher values increase depth, requires r_restart to take effect"
};

CVar r_shadow_debuglight
{
	0,
	"r_shadow_debuglight",
	"-1",
	"renders only one light, for level design purposes or debugging"
};

CVar r_shadow_deferred
{
	CVAR_SAVE,
	"r_shadow_deferred",
	"0",
	"uses image-based lighting instead of geometry-based lighting, the method used renders a depth image and a normalmap image, renders lights into separate diffuse and specular images, and then combines this into the normal rendering, requires r_shadow_shadowmapping"
};

CVar r_shadow_usebihculling
{
	0,
	"r_shadow_usebihculling",
	"1",
	"use BIH (Bounding Interval Hierarchy) for culling lit surfaces instead of BSP (Binary Space Partitioning)"
};

CVar r_shadow_usenormalmap
{
	CVAR_SAVE,
	"r_shadow_usenormalmap",
	"1",
	"enables use of directional shading on lights"
};

CVar r_shadow_gloss
{
	CVAR_SAVE,
	"r_shadow_gloss",
	"1",
	"0 disables gloss (specularity) rendering, 1 uses gloss if textures are found, 2 forces a flat metallic specular effect on everything without textures (similar to tenebrae)"
};

CVar r_shadow_gloss2intensity
{
	0,
	"r_shadow_gloss2intensity",
	"0.125",
	"how bright the forced flat gloss should look if r_shadow_gloss is 2"
};

CVar r_shadow_glossintensity
{
	0,
	"r_shadow_glossintensity",
	"1",
	"how bright textured glossmaps should look if r_shadow_gloss is 1 or 2"
};

CVar r_shadow_glossexponent
{
	0,
	"r_shadow_glossexponent",
	"32",
	"how 'sharp' the gloss should appear (specular power)"
};

CVar r_shadow_gloss2exponent
{
	0,
	"r_shadow_gloss2exponent",
	"32",
	"same as r_shadow_glossexponent but for forced gloss (gloss 2) surfaces"
};

CVar r_shadow_glossexact
{
	0,
	"r_shadow_glossexact",
	"0",
	"use exact reflection math for gloss (slightly slower, but should look a tad better)"
};

CVar r_shadow_lightattenuationdividebias
{
	0,
	"r_shadow_lightattenuationdividebias",
	"1",
	"changes attenuation texture generation"
};

CVar r_shadow_lightattenuationlinearscale
{
	0,
	"r_shadow_lightattenuationlinearscale",
	"2",
	"changes attenuation texture generation"
};

CVar r_shadow_lightintensityscale
{
	0,
	"r_shadow_lightintensityscale",
	"1",
	"renders all world lights brighter or darker"
};

CVar r_shadow_lightradiusscale
{
	0,
	"r_shadow_lightradiusscale",
	"1",
	"renders all world lights larger or smaller"
};

CVar r_shadow_projectdistance
{
	0,
	"r_shadow_projectdistance",
	"0",
	"how far to cast shadows"
};

CVar r_shadow_frontsidecasting
{
	0,
	"r_shadow_frontsidecasting",
	"1",
	"whether to cast shadows from illuminated triangles (front side of model) or unlit triangles (back side of model)"
};

CVar r_shadow_realtime_dlight
{
	CVAR_SAVE,
	"r_shadow_realtime_dlight",
	"1",
	"enables rendering of dynamic lights such as explosions and rocket light"
};

CVar r_shadow_realtime_dlight_shadows
{
	CVAR_SAVE,
	"r_shadow_realtime_dlight_shadows",
	"1",
	"enables rendering of shadows from dynamic lights"
};

CVar r_shadow_realtime_dlight_svbspculling
{
	0,
	"r_shadow_realtime_dlight_svbspculling",
	"0",
	"enables svbsp optimization on dynamic lights (very slow!)"
};

CVar r_shadow_realtime_dlight_portalculling
{
	0,
	"r_shadow_realtime_dlight_portalculling",
	"0",
	"enables portal optimization on dynamic lights (slow!)"
};

CVar r_shadow_realtime_world
{
	CVAR_SAVE,
	"r_shadow_realtime_world",
	"0",
	"enables rendering of full world lighting (whether loaded from the map, or a .rtlights file, or a .ent file, or a .lights file produced by hlight)"
};

CVar r_shadow_realtime_world_lightmaps
{
	CVAR_SAVE,
	"r_shadow_realtime_world_lightmaps",
	"0",
	"brightness to render lightmaps when using full world lighting, try 0.5 for a tenebrae-like appearance"
};

CVar r_shadow_realtime_world_shadows
{
	CVAR_SAVE,
	"r_shadow_realtime_world_shadows",
	"1",
	"enables rendering of shadows from world lights"
};

CVar r_shadow_realtime_world_compile
{
	0,
	"r_shadow_realtime_world_compile",
	"1",
	"enables compilation of world lights for higher performance rendering"
};

CVar r_shadow_realtime_world_compileshadow
{
	0,
	"r_shadow_realtime_world_compileshadow",
	"1",
	"enables compilation of shadows from world lights for higher performance rendering"
};

CVar r_shadow_realtime_world_compilesvbsp
{
	0,
	"r_shadow_realtime_world_compilesvbsp",
	"1",
	"enables svbsp optimization during compilation (slower than compileportalculling but more exact)"
};

CVar r_shadow_realtime_world_compileportalculling
{
	0,
	"r_shadow_realtime_world_compileportalculling",
	"1",
	"enables portal-based culling optimization during compilation (overrides compilesvbsp)"
};

CVar r_shadow_scissor
{
	0,
	"r_shadow_scissor",
	"1",
	"use scissor optimization of light rendering (restricts rendering to the portion of the screen affected by the light)"
};

CVar r_shadow_shadowmapping
{
	CVAR_SAVE,
	"r_shadow_shadowmapping",
	"1",
	"enables use of shadowmapping (depth texture sampling) instead of stencil shadow volumes, requires gl_fbo 1"
};

CVar r_shadow_shadowmapping_filterquality
{
	CVAR_SAVE,
	"r_shadow_shadowmapping_filterquality",
	"-1",
	"shadowmap filter modes: -1 = auto-select, 0 = no filtering, 1 = bilinear, 2 = bilinear 2x2 blur (fast), 3 = 3x3 blur (moderate), 4 = 4x4 blur (slow)"
};

CVar r_shadow_shadowmapping_useshadowsampler
{
	CVAR_SAVE,
	"r_shadow_shadowmapping_useshadowsampler",
	"1",
	"whether to use sampler2DShadow if available"
};

CVar r_shadow_shadowmapping_depthbits
{
	CVAR_SAVE,
	"r_shadow_shadowmapping_depthbits",
	"24",
	"requested minimum shadowmap texture depth bits"
};

CVar r_shadow_shadowmapping_vsdct
{
	CVAR_SAVE,
	"r_shadow_shadowmapping_vsdct",
	"1",
	"enables use of virtual shadow depth cube texture"
};

CVar r_shadow_shadowmapping_minsize
{
	CVAR_SAVE,
	"r_shadow_shadowmapping_minsize",
	"32",
	"shadowmap size limit"
};

CVar r_shadow_shadowmapping_maxsize
{
	CVAR_SAVE,
	"r_shadow_shadowmapping_maxsize",
	"512",
	"shadowmap size limit"
};

CVar r_shadow_shadowmapping_precision
{
	CVAR_SAVE,
	"r_shadow_shadowmapping_precision",
	"1",
	"makes shadowmaps have a maximum resolution of this number of pixels per light source radius unit such that, for example, at precision 0.5 a light with radius 200 will have a maximum resolution of 100 pixels"
};

CVar r_shadow_shadowmapping_bordersize
{
	CVAR_SAVE,
	"r_shadow_shadowmapping_bordersize",
	"4",
	"shadowmap size bias for filtering"
};

CVar r_shadow_shadowmapping_nearclip
{
	CVAR_SAVE,
	"r_shadow_shadowmapping_nearclip",
	"1",
	"shadowmap nearclip in world units"
};

CVar r_shadow_shadowmapping_bias
{
	CVAR_SAVE,
	"r_shadow_shadowmapping_bias",
	"0.03",
	"shadowmap bias parameter (this is multiplied by nearclip * 1024 / lodsize)"
};

CVar r_shadow_shadowmapping_polygonfactor
{
	CVAR_SAVE,
	"r_shadow_shadowmapping_polygonfactor",
	"2",
	"slope-dependent shadowmapping bias"
};

CVar r_shadow_shadowmapping_polygonoffset
{
	CVAR_SAVE,
	"r_shadow_shadowmapping_polygonoffset",
	"0",
	"constant shadowmapping bias"
};

CVar r_shadow_sortsurfaces
{
	0,
	"r_shadow_sortsurfaces",
	"1",
	"improve performance by sorting illuminated surfaces by texture"
};

CVar r_shadow_polygonfactor
{
	0,
	"r_shadow_polygonfactor",
	"0",
	"how much to enlarge shadow volume polygons when rendering (should be 0!)"
};

CVar r_shadow_polygonoffset
{
	0,
	"r_shadow_polygonoffset",
	"1",
	"how much to push shadow volumes into the distance when rendering, to reduce chances of zfighting artifacts (should not be less than 0)"
};

CVar r_shadow_texture3d
{
	0,
	"r_shadow_texture3d",
	"1",
	"use 3D voxel textures for spherical attenuation rather than cylindrical (does not affect OpenGL 2.0 render path)"
};

CVar r_shadow_bouncegrid
{
	CVAR_SAVE,
	"r_shadow_bouncegrid",
	"0",
	"perform particle tracing for indirect lighting (Global Illumination / radiosity) using a 3D texture covering the scene, only active on levels with realtime lights active (r_shadow_realtime_world is usually required for these)"
};

CVar r_shadow_bouncegrid_bounceanglediffuse
{
	CVAR_SAVE,
	"r_shadow_bouncegrid_bounceanglediffuse",
	"0",
	"use random bounce direction rather than true reflection, makes some corner areas dark"
};

CVar r_shadow_bouncegrid_directionalshading
{
	CVAR_SAVE,
	"r_shadow_bouncegrid_directionalshading",
	"0",
	"use diffuse shading rather than ambient,3D texture becomes 8x as many pixels to hold the additional data"
};

CVar r_shadow_bouncegrid_dlightparticlemultiplier
{
	CVAR_SAVE,
	"r_shadow_bouncegrid_dlightparticlemultiplier",
	"0",
	"if set to a high value like 16 this can make dlights look great, but 0 is recommended for performance reasons"
};

CVar r_shadow_bouncegrid_hitmodels
{
	CVAR_SAVE,
	"r_shadow_bouncegrid_hitmodels",
	"0",
	"enables hitting character model geometry (SLOW)"
};

CVar r_shadow_bouncegrid_includedirectlighting
{
	CVAR_SAVE,
	"r_shadow_bouncegrid_includedirectlighting",
	"0",
	"allows direct lighting to be recorded, not just indirect (gives an effect somewhat like r_shadow_realtime_world_lightmaps)"
};

CVar r_shadow_bouncegrid_intensity
{
	CVAR_SAVE,
	"r_shadow_bouncegrid_intensity",
	"4",
	"overall brightness of bouncegrid texture"
};

CVar r_shadow_bouncegrid_lightradiusscale
{
	CVAR_SAVE,
	"r_shadow_bouncegrid_lightradiusscale",
	"4",
	"particles stop at this fraction of light radius (can be more than 1)"
};

CVar r_shadow_bouncegrid_maxbounce
{
	CVAR_SAVE,
	"r_shadow_bouncegrid_maxbounce",
	"2",
	"maximum number of bounces for a particle (minimum is 0)"
};

CVar r_shadow_bouncegrid_particlebounceintensity
{
	CVAR_SAVE,
	"r_shadow_bouncegrid_particlebounceintensity",
	"1",
	"amount of energy carried over after each bounce, this is a multiplier of texture color and the result is clamped to 1 or less, to prevent adding energy on each bounce"
};

CVar r_shadow_bouncegrid_particleintensity
{
	CVAR_SAVE,
	"r_shadow_bouncegrid_particleintensity",
	"1",
	"brightness of particles contributing to bouncegrid texture"
};

CVar r_shadow_bouncegrid_photons
{
	CVAR_SAVE,
	"r_shadow_bouncegrid_photons",
	"2000",
	"total photons to shoot per update, divided proportionately between lights"
};

CVar r_shadow_bouncegrid_spacing
{
	CVAR_SAVE,
	"r_shadow_bouncegrid_spacing",
	"64",
	"unit size of bouncegrid pixel"
};

CVar r_shadow_bouncegrid_stablerandom
{
	CVAR_SAVE,
	"r_shadow_bouncegrid_stablerandom",
	"1",
	"make particle distribution consistent from frame to frame"
};

CVar r_shadow_bouncegrid_static
{
	CVAR_SAVE,
	"r_shadow_bouncegrid_static",
	"1",
	"use static radiosity solution (high quality) rather than dynamic (splotchy)"
};

CVar r_shadow_bouncegrid_static_directionalshading
{
	CVAR_SAVE,
	"r_shadow_bouncegrid_static_directionalshading",
	"1",
	"whether to use directionalshading when in static mode"
};

CVar r_shadow_bouncegrid_static_lightradiusscale
{
	CVAR_SAVE,
	"r_shadow_bouncegrid_static_lightradiusscale",
	"10",
	"particles stop at this fraction of light radius (can be more than 1) when in static mode"
};

CVar r_shadow_bouncegrid_static_maxbounce
{
	CVAR_SAVE,
	"r_shadow_bouncegrid_static_maxbounce",
	"5",
	"maximum number of bounces for a particle (minimum is 0) in static mode"
};

CVar r_shadow_bouncegrid_static_photons
{
	CVAR_SAVE,
	"r_shadow_bouncegrid_static_photons",
	"25000",
	"photons value to use when in static mode"
};

CVar r_shadow_bouncegrid_updateinterval
{
	CVAR_SAVE,
	"r_shadow_bouncegrid_updateinterval",
	"0",
	"update bouncegrid texture once per this many seconds, useful values are 0, 0.05, or 1000000"
};

CVar r_shadow_bouncegrid_x
{
	CVAR_SAVE,
	"r_shadow_bouncegrid_x",
	"64",
	"maximum texture size of bouncegrid on X axis"
};

CVar r_shadow_bouncegrid_y
{
	CVAR_SAVE,
	"r_shadow_bouncegrid_y",
	"64",
	"maximum texture size of bouncegrid on Y axis"
};

CVar r_shadow_bouncegrid_z
{
	CVAR_SAVE,
	"r_shadow_bouncegrid_z",
	"32",
	"maximum texture size of bouncegrid on Z axis"
};

CVar r_coronas
{
	CVAR_SAVE,
	"r_coronas",
	"0",
	"brightness of corona flare effects around certain lights, 0 disables corona effects"
};

CVar r_coronas_occlusionsizescale
{
	CVAR_SAVE,
	"r_coronas_occlusionsizescale",
	"0.1",
	"size of light source for corona occlusion checksum the proportion of hidden pixels controls corona intensity"
};

CVar r_coronas_occlusionquery
{
	CVAR_SAVE,
	"r_coronas_occlusionquery",
	"0",
	"use GL_ARB_occlusion_query extension if supported (fades coronas according to visibility) - bad performance (synchronous rendering) - worse on multi-gpu!"
};

CVar gl_flashblend
{
	CVAR_SAVE,
	"gl_flashblend",
	"0",
	"render bright coronas for dynamic lights instead of actual lighting, fast but ugly"
};

CVar gl_ext_separatestencil
{
	0,
	"gl_ext_separatestencil",
	"1",
	"make use of OpenGL 2.0 glStencilOpSeparate or GL_ATI_separate_stencil extension"
};

CVar gl_ext_stenciltwoside
{
	0,
	"gl_ext_stenciltwoside",
	"1",
	"make use of GL_EXT_stenciltwoside extension (NVIDIA only)"
};

CVar r_editlights
{
	0,
	"r_editlights",
	"0",
	"enables .rtlights file editing mode"
};

CVar r_editlights_cursordistance
{
	0,
	"r_editlights_cursordistance",
	"1024",
	"maximum distance of cursor from eye"
};

CVar r_editlights_cursorpushback
{
	0,
	"r_editlights_cursorpushback",
	"0",
	"how far to pull the cursor back toward the eye"
};

CVar r_editlights_cursorpushoff
{
	0,
	"r_editlights_cursorpushoff",
	"4",
	"how far to push the cursor off the impacted surface"
};

CVar r_editlights_cursorgrid
{
	0,
	"r_editlights_cursorgrid",
	"4",
	"snaps cursor to this grid size"
};

CVar r_editlights_quakelightsizescale
{
	CVAR_SAVE,
	"r_editlights_quakelightsizescale",
	"1",
	"changes size of light entities loaded from a map"
};

CVar r_editlights_drawproperties
{
	0,
	"r_editlights_drawproperties",
	"1",
	"draw properties of currently selected light"
};

CVar r_editlights_current_origin
{
	0,
	"r_editlights_current_origin",
	"0 0 0",
	"origin of selected light"
};

CVar r_editlights_current_angles
{
	0,
	"r_editlights_current_angles",
	"0 0 0",
	"angles of selected light"
};

CVar r_editlights_current_color
{
	0,
	"r_editlights_current_color",
	"1 1 1",
	"color of selected light"
};

CVar r_editlights_current_radius
{
	0,
	"r_editlights_current_radius",
	"0",
	"radius of selected light"
};

CVar r_editlights_current_corona
{
	0,
	"r_editlights_current_corona",
	"0",
	"corona intensity of selected light"
};

CVar r_editlights_current_coronasize
{
	0,
	"r_editlights_current_coronasize",
	"0",
	"corona size of selected light"
};

CVar r_editlights_current_style
{
	0,
	"r_editlights_current_style",
	"0",
	"style of selected light"
};

CVar r_editlights_current_shadows
{
	0,
	"r_editlights_current_shadows",
	"0",
	"shadows flag of selected light"
};

CVar r_editlights_current_cubemap
{
	0,
	"r_editlights_current_cubemap",
	"0",
	"cubemap of selected light"
};

CVar r_editlights_current_ambient
{
	0,
	"r_editlights_current_ambient",
	"0",
	"ambient intensity of selected light"
};

CVar r_editlights_current_diffuse
{
	0,
	"r_editlights_current_diffuse",
	"1",
	"diffuse intensity of selected light"
};

CVar r_editlights_current_specular
{
	0,
	"r_editlights_current_specular",
	"1",
	"specular intensity of selected light"
};

CVar r_editlights_current_normalmode
{
	0,
	"r_editlights_current_normalmode",
	"0",
	"normalmode flag of selected light"
};

CVar r_editlights_current_realtimemode
{
	0,
	"r_editlights_current_realtimemode",
	"0",
	"realtimemode flag of selected light"
};
