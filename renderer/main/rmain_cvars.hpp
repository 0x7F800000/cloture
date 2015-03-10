#pragma once
/**
	rmain_cvars.hpp
	moved these out of gl_rmain to minimize the amount of code in there
	this file is included only once, and it is in gl_rmain
*/
cvar_t r_motionblur = {CVAR_SAVE, "r_motionblur", "0", "screen motionblur - value represents intensity, somewhere around 0.5 recommended - NOTE: bad performance on multi-gpu!"};
cvar_t r_damageblur = {CVAR_SAVE, "r_damageblur", "0", "screen motionblur based on damage - value represents intensity, somewhere around 0.5 recommended - NOTE: bad performance on multi-gpu!"};
cvar_t r_motionblur_averaging = {CVAR_SAVE, "r_motionblur_averaging", "0.1", "sliding average reaction time for velocity (higher = slower adaption to change)"};
cvar_t r_motionblur_randomize = {CVAR_SAVE, "r_motionblur_randomize", "0.1", "randomizing coefficient to workaround ghosting"};
cvar_t r_motionblur_minblur = {CVAR_SAVE, "r_motionblur_minblur", "0.5", "factor of blur to apply at all times (always have this amount of blur no matter what the other factors are)"};
cvar_t r_motionblur_maxblur = {CVAR_SAVE, "r_motionblur_maxblur", "0.9", "maxmimum amount of blur"};
cvar_t r_motionblur_velocityfactor = {CVAR_SAVE, "r_motionblur_velocityfactor", "1", "factoring in of player velocity to the blur equation - the faster the player moves around the map, the more blur they get"};
cvar_t r_motionblur_velocityfactor_minspeed = {CVAR_SAVE, "r_motionblur_velocityfactor_minspeed", "400", "lower value of velocity when it starts to factor into blur equation"};
cvar_t r_motionblur_velocityfactor_maxspeed = {CVAR_SAVE, "r_motionblur_velocityfactor_maxspeed", "800", "upper value of velocity when it reaches the peak factor into blur equation"};
cvar_t r_motionblur_mousefactor = {CVAR_SAVE, "r_motionblur_mousefactor", "2", "factoring in of mouse acceleration to the blur equation - the faster the player turns their mouse, the more blur they get"};
cvar_t r_motionblur_mousefactor_minspeed = {CVAR_SAVE, "r_motionblur_mousefactor_minspeed", "0", "lower value of mouse acceleration when it starts to factor into blur equation"};
cvar_t r_motionblur_mousefactor_maxspeed = {CVAR_SAVE, "r_motionblur_mousefactor_maxspeed", "50", "upper value of mouse acceleration when it reaches the peak factor into blur equation"};

// TODO do we want a r_equalize_entities cvar that works on all ents, or would that be a cheat?
cvar_t r_equalize_entities_fullbright = {CVAR_SAVE, "r_equalize_entities_fullbright", "0", "render fullbright entities by equalizing their lightness, not by not rendering light"};
cvar_t r_equalize_entities_minambient = {CVAR_SAVE, "r_equalize_entities_minambient", "0.5", "light equalizing: ensure at least this ambient/diffuse ratio"};
cvar_t r_equalize_entities_by = {CVAR_SAVE, "r_equalize_entities_by", "0.7", "light equalizing: exponent of dynamics compression (0 = no compression, 1 = full compression)"};
cvar_t r_equalize_entities_to = {CVAR_SAVE, "r_equalize_entities_to", "0.8", "light equalizing: target light level"};

cvar_t r_depthfirst = {CVAR_SAVE, "r_depthfirst", "0", "renders a depth-only version of the scene before normal rendering begins to eliminate overdraw, values: 0 = off, 1 = world depth, 2 = world and model depth"};
cvar_t r_useinfinitefarclip = {CVAR_SAVE, "r_useinfinitefarclip", "1", "enables use of a special kind of projection matrix that has an extremely large farclip"};
cvar_t r_farclip_base = {0, "r_farclip_base", "65536", "farclip (furthest visible distance) for rendering when r_useinfinitefarclip is 0"};
cvar_t r_farclip_world = {0, "r_farclip_world", "2", "adds map size to farclip multiplied by this value"};
cvar_t r_nearclip = {0, "r_nearclip", "1", "distance from camera of nearclip plane" };
cvar_t r_deformvertexes = {0, "r_deformvertexes", "1", "allows use of deformvertexes in shader files (can be turned off to check performance impact)"};
cvar_t r_transparent = {0, "r_transparent", "1", "allows use of transparent surfaces (can be turned off to check performance impact)"};
cvar_t r_transparent_alphatocoverage = {0, "r_transparent_alphatocoverage", "1", "enables GL_ALPHA_TO_COVERAGE antialiasing technique on alphablend and alphatest surfaces when using vid_samples 2 or higher"};
cvar_t r_transparent_sortsurfacesbynearest = {0, "r_transparent_sortsurfacesbynearest", "1", "sort entity and world surfaces by nearest point on bounding box instead of using the center of the bounding box, usually reduces sorting artifacts"};
cvar_t r_transparent_useplanardistance = {0, "r_transparent_useplanardistance", "0", "sort transparent meshes by distance from view plane rather than spherical distance to the chosen point"};
cvar_t r_showoverdraw = {0, "r_showoverdraw", "0", "shows overlapping geometry"};
cvar_t r_showbboxes = {0, "r_showbboxes", "0", "shows bounding boxes of server entities, value controls opacity scaling (1 = 10%,  10 = 100%)"};
cvar_t r_showsurfaces = {0, "r_showsurfaces", "0", "1 shows surfaces as different colors, or a value of 2 shows triangle draw order (for analyzing whether meshes are optimized for vertex cache)"};
cvar_t r_showtris = {0, "r_showtris", "0", "shows triangle outlines, value controls brightness (can be above 1)"};
cvar_t r_shownormals = {0, "r_shownormals", "0", "shows per-vertex surface normals and tangent vectors for bumpmapped lighting"};
cvar_t r_showlighting = {0, "r_showlighting", "0", "shows areas lit by lights, useful for finding out why some areas of a map render slowly (bright orange = lots of passes = slow), a value of 2 disables depth testing which can be interesting but not very useful"};
cvar_t r_showshadowvolumes = {0, "r_showshadowvolumes", "0", "shows areas shadowed by lights, useful for finding out why some areas of a map render slowly (bright blue = lots of passes = slow), a value of 2 disables depth testing which can be interesting but not very useful"};
cvar_t r_showcollisionbrushes = {0, "r_showcollisionbrushes", "0", "draws collision brushes in quake3 maps (mode 1), mode 2 disables rendering of world (trippy!)"};
cvar_t r_showcollisionbrushes_polygonfactor = {0, "r_showcollisionbrushes_polygonfactor", "-1", "expands outward the brush polygons a little bit, used to make collision brushes appear infront of walls"};
cvar_t r_showcollisionbrushes_polygonoffset = {0, "r_showcollisionbrushes_polygonoffset", "0", "nudges brush polygon depth in hardware depth units, used to make collision brushes appear infront of walls"};
cvar_t r_showdisabledepthtest = {0, "r_showdisabledepthtest", "0", "disables depth testing on r_show* cvars, allowing you to see what hidden geometry the graphics card is processing"};
cvar_t r_drawportals = {0, "r_drawportals", "0", "shows portals (separating polygons) in world interior in quake1 maps"};
cvar_t r_drawentities = {0, "r_drawentities","1", "draw entities (doors, players, projectiles, etc)"};
cvar_t r_draw2d = {0, "r_draw2d","1", "draw 2D stuff (dangerous to turn off)"};
cvar_t r_drawworld = {0, "r_drawworld","1", "draw world (most static stuff)"};
cvar_t r_drawviewmodel = {0, "r_drawviewmodel","1", "draw your weapon model"};
cvar_t r_drawexteriormodel = {0, "r_drawexteriormodel","1", "draw your player model (e.g. in chase cam, reflections)"};
cvar_t r_cullentities_trace = {0, "r_cullentities_trace", "1", "probabistically cull invisible entities"};
cvar_t r_cullentities_trace_samples = {0, "r_cullentities_trace_samples", "2", "number of samples to test for entity culling (in addition to center sample)"};
cvar_t r_cullentities_trace_tempentitysamples = {0, "r_cullentities_trace_tempentitysamples", "-1", "number of samples to test for entity culling of temp entities (including all CSQC entities), -1 disables trace culling on these entities to prevent flicker (pvs still applies)"};
cvar_t r_cullentities_trace_enlarge = {0, "r_cullentities_trace_enlarge", "0", "box enlargement for entity culling"};
cvar_t r_cullentities_trace_delay = {0, "r_cullentities_trace_delay", "1", "number of seconds until the entity gets actually culled"};
cvar_t r_sortentities = {0, "r_sortentities", "0", "sort entities before drawing (might be faster)"};
cvar_t r_speeds = {0, "r_speeds","0", "displays rendering statistics and per-subsystem timings"};
cvar_t r_fullbright = {0, "r_fullbright","0", "makes map very bright and renders faster"};

cvar_t r_fakelight = {0, "r_fakelight","0", "render 'fake' lighting instead of real lightmaps"};
cvar_t r_fakelight_intensity = {0, "r_fakelight_intensity","0.75", "fakelight intensity modifier"};
#define FAKELIGHT_ENABLED (r_fakelight.integer >= 2 || (r_fakelight.integer && r_refdef.scene.worldmodel && !r_refdef.scene.worldmodel->lit))

cvar_t r_wateralpha = {CVAR_SAVE, "r_wateralpha","1", "opacity of water polygons"};
cvar_t r_dynamic = {CVAR_SAVE, "r_dynamic","1", "enables dynamic lights (rocket glow and such)"};
cvar_t r_fullbrights = {CVAR_SAVE, "r_fullbrights", "1", "enables glowing pixels in quake textures (changes need r_restart to take effect)"};
cvar_t r_shadows = {CVAR_SAVE, "r_shadows", "0", "casts fake stencil shadows from models onto the world (rtlights are unaffected by this); when set to 2, always cast the shadows in the direction set by r_shadows_throwdirection, otherwise use the model lighting."};
cvar_t r_shadows_darken = {CVAR_SAVE, "r_shadows_darken", "0.5", "how much shadowed areas will be darkened"};
cvar_t r_shadows_throwdistance = {CVAR_SAVE, "r_shadows_throwdistance", "500", "how far to cast shadows from models"};
cvar_t r_shadows_throwdirection = {CVAR_SAVE, "r_shadows_throwdirection", "0 0 -1", "override throwing direction for r_shadows 2"};
cvar_t r_shadows_drawafterrtlighting = {CVAR_SAVE, "r_shadows_drawafterrtlighting", "0", "draw fake shadows AFTER realtime lightning is drawn. May be useful for simulating fast sunlight on large outdoor maps with only one noshadow rtlight. The price is less realistic appearance of dynamic light shadows."};
cvar_t r_shadows_castfrombmodels = {CVAR_SAVE, "r_shadows_castfrombmodels", "0", "do cast shadows from bmodels"};
cvar_t r_shadows_focus = {CVAR_SAVE, "r_shadows_focus", "0 0 0", "offset the shadowed area focus"};
cvar_t r_shadows_shadowmapscale = {CVAR_SAVE, "r_shadows_shadowmapscale", "1", "increases shadowmap quality (multiply global shadowmap precision) for fake shadows. Needs shadowmapping ON."};
cvar_t r_shadows_shadowmapbias = {CVAR_SAVE, "r_shadows_shadowmapbias", "-1", "sets shadowmap bias for fake shadows. -1 sets the value of r_shadow_shadowmapping_bias. Needs shadowmapping ON."};
cvar_t r_q1bsp_skymasking = {0, "r_q1bsp_skymasking", "1", "allows sky polygons in quake1 maps to obscure other geometry"};
cvar_t r_polygonoffset_submodel_factor = {0, "r_polygonoffset_submodel_factor", "0", "biases depth values of world submodels such as doors, to prevent z-fighting artifacts in Quake maps"};
cvar_t r_polygonoffset_submodel_offset = {0, "r_polygonoffset_submodel_offset", "14", "biases depth values of world submodels such as doors, to prevent z-fighting artifacts in Quake maps"};
cvar_t r_polygonoffset_decals_factor = {0, "r_polygonoffset_decals_factor", "0", "biases depth values of decals to prevent z-fighting artifacts"};
cvar_t r_polygonoffset_decals_offset = {0, "r_polygonoffset_decals_offset", "-14", "biases depth values of decals to prevent z-fighting artifacts"};
cvar_t r_fog_exp2 = {0, "r_fog_exp2", "0", "uses GL_EXP2 fog (as in Nehahra) rather than realistic GL_EXP fog"};
cvar_t r_fog_clear = {0, "r_fog_clear", "1", "clears renderbuffer with fog color before render starts"};
cvar_t r_drawfog = {CVAR_SAVE, "r_drawfog", "1", "allows one to disable fog rendering"};
cvar_t r_transparentdepthmasking = {CVAR_SAVE, "r_transparentdepthmasking", "0", "enables depth writes on transparent meshes whose materially is normally opaque, this prevents seeing the inside of a transparent mesh"};
cvar_t r_transparent_sortmindist = {CVAR_SAVE, "r_transparent_sortmindist", "0", "lower distance limit for transparent sorting"};
cvar_t r_transparent_sortmaxdist = {CVAR_SAVE, "r_transparent_sortmaxdist", "32768", "upper distance limit for transparent sorting"};
cvar_t r_transparent_sortarraysize = {CVAR_SAVE, "r_transparent_sortarraysize", "4096", "number of distance-sorting layers"};
cvar_t r_celshading = {CVAR_SAVE, "r_celshading", "0", "cartoon-style light shading (OpenGL 2.x only)"}; // FIXME remove OpenGL 2.x only once implemented for DX9
cvar_t r_celoutlines = {CVAR_SAVE, "r_celoutlines", "0", "cartoon-style outlines (requires r_shadow_deferred; OpenGL 2.x only)"}; // FIXME remove OpenGL 2.x only once implemented for DX9

cvar_t gl_fogenable = {0, "gl_fogenable", "0", "nehahra fog enable (for Nehahra compatibility only)"};
cvar_t gl_fogdensity = {0, "gl_fogdensity", "0.25", "nehahra fog density (recommend values below 0.1) (for Nehahra compatibility only)"};
cvar_t gl_fogred = {0, "gl_fogred","0.3", "nehahra fog color red value (for Nehahra compatibility only)"};
cvar_t gl_foggreen = {0, "gl_foggreen","0.3", "nehahra fog color green value (for Nehahra compatibility only)"};
cvar_t gl_fogblue = {0, "gl_fogblue","0.3", "nehahra fog color blue value (for Nehahra compatibility only)"};
cvar_t gl_fogstart = {0, "gl_fogstart", "0", "nehahra fog start distance (for Nehahra compatibility only)"};
cvar_t gl_fogend = {0, "gl_fogend","0", "nehahra fog end distance (for Nehahra compatibility only)"};
cvar_t gl_skyclip = {0, "gl_skyclip", "4608", "nehahra farclip distance - the real fog end (for Nehahra compatibility only)"};

cvar_t r_texture_dds_load = {CVAR_SAVE, "r_texture_dds_load", "0", "load compressed dds/filename.dds texture instead of filename.tga, if the file exists (requires driver support)"};
cvar_t r_texture_dds_save = {CVAR_SAVE, "r_texture_dds_save", "0", "save compressed dds/filename.dds texture when filename.tga is loaded, so that it can be loaded instead next time"};

cvar_t r_textureunits = {0, "r_textureunits", "32", "number of texture units to use in GL 1.1 and GL 1.3 rendering paths"};
static cvar_t gl_combine = {CVAR_READONLY, "gl_combine", "1", "indicates whether the OpenGL 1.3 rendering path is active"};
static cvar_t r_glsl = {CVAR_READONLY, "r_glsl", "1", "indicates whether the OpenGL 2.0 rendering path is active"};

cvar_t r_usedepthtextures = {CVAR_SAVE, "r_usedepthtextures", "1", "use depth texture instead of depth renderbuffer where possible, uses less video memory but may render slower (or faster) depending on hardware"};
cvar_t r_viewfbo = {CVAR_SAVE, "r_viewfbo", "0", "enables use of an 8bit (1) or 16bit (2) or 32bit (3) per component float framebuffer render, which may be at a different resolution than the video mode"};
cvar_t r_viewscale = {CVAR_SAVE, "r_viewscale", "1", "scaling factor for resolution of the fbo rendering method, must be > 0, can be above 1 for a costly antialiasing behavior, typical values are 0.5 for 1/4th as many pixels rendered, or 1 for normal rendering"};
cvar_t r_viewscale_fpsscaling = {CVAR_SAVE, "r_viewscale_fpsscaling", "0", "change resolution based on framerate"};
cvar_t r_viewscale_fpsscaling_min = {CVAR_SAVE, "r_viewscale_fpsscaling_min", "0.0625", "worst acceptable quality"};
cvar_t r_viewscale_fpsscaling_multiply = {CVAR_SAVE, "r_viewscale_fpsscaling_multiply", "5", "adjust quality up or down by the frametime difference from 1.0/target, multiplied by this factor"};
cvar_t r_viewscale_fpsscaling_stepsize = {CVAR_SAVE, "r_viewscale_fpsscaling_stepsize", "0.01", "smallest adjustment to hit the target framerate (this value prevents minute oscillations)"};
cvar_t r_viewscale_fpsscaling_stepmax = {CVAR_SAVE, "r_viewscale_fpsscaling_stepmax", "1.00", "largest adjustment to hit the target framerate (this value prevents wild overshooting of the estimate)"};
cvar_t r_viewscale_fpsscaling_target = {CVAR_SAVE, "r_viewscale_fpsscaling_target", "70", "desired framerate"};

cvar_t r_glsl_skeletal = {CVAR_SAVE, "r_glsl_skeletal", "1", "render skeletal models faster using a gpu-skinning technique"};
cvar_t r_glsl_deluxemapping = {CVAR_SAVE, "r_glsl_deluxemapping", "1", "use per pixel lighting on deluxemap-compiled q3bsp maps (or a value of 2 forces deluxemap shading even without deluxemaps)"};
cvar_t r_glsl_offsetmapping = {CVAR_SAVE, "r_glsl_offsetmapping", "0", "offset mapping effect (also known as parallax mapping or virtual displacement mapping)"};
cvar_t r_glsl_offsetmapping_steps = {CVAR_SAVE, "r_glsl_offsetmapping_steps", "2", "offset mapping steps (note: too high values may be not supported by your GPU)"};
cvar_t r_glsl_offsetmapping_reliefmapping = {CVAR_SAVE, "r_glsl_offsetmapping_reliefmapping", "0", "relief mapping effect (higher quality)"};
cvar_t r_glsl_offsetmapping_reliefmapping_steps = {CVAR_SAVE, "r_glsl_offsetmapping_reliefmapping_steps", "10", "relief mapping steps (note: too high values may be not supported by your GPU)"};
cvar_t r_glsl_offsetmapping_reliefmapping_refinesteps = {CVAR_SAVE, "r_glsl_offsetmapping_reliefmapping_refinesteps", "5", "relief mapping refine steps (these are a binary search executed as the last step as given by r_glsl_offsetmapping_reliefmapping_steps)"};
cvar_t r_glsl_offsetmapping_scale = {CVAR_SAVE, "r_glsl_offsetmapping_scale", "0.04", "how deep the offset mapping effect is"};
cvar_t r_glsl_offsetmapping_lod = {CVAR_SAVE, "r_glsl_offsetmapping_lod", "0", "apply distance-based level-of-detail correction to number of offsetmappig steps, effectively making it render faster on large open-area maps"};
cvar_t r_glsl_offsetmapping_lod_distance = {CVAR_SAVE, "r_glsl_offsetmapping_lod_distance", "32", "first LOD level distance, second level (-50% steps) is 2x of this, third (33%) - 3x etc."};
cvar_t r_glsl_postprocess = {CVAR_SAVE, "r_glsl_postprocess", "0", "use a GLSL postprocessing shader"};
cvar_t r_glsl_postprocess_uservec1 = {CVAR_SAVE, "r_glsl_postprocess_uservec1", "0 0 0 0", "a 4-component vector to pass as uservec1 to the postprocessing shader (only useful if default.glsl has been customized)"};
cvar_t r_glsl_postprocess_uservec2 = {CVAR_SAVE, "r_glsl_postprocess_uservec2", "0 0 0 0", "a 4-component vector to pass as uservec2 to the postprocessing shader (only useful if default.glsl has been customized)"};
cvar_t r_glsl_postprocess_uservec3 = {CVAR_SAVE, "r_glsl_postprocess_uservec3", "0 0 0 0", "a 4-component vector to pass as uservec3 to the postprocessing shader (only useful if default.glsl has been customized)"};
cvar_t r_glsl_postprocess_uservec4 = {CVAR_SAVE, "r_glsl_postprocess_uservec4", "0 0 0 0", "a 4-component vector to pass as uservec4 to the postprocessing shader (only useful if default.glsl has been customized)"};
cvar_t r_glsl_postprocess_uservec1_enable = {CVAR_SAVE, "r_glsl_postprocess_uservec1_enable", "1", "enables postprocessing uservec1 usage, creates USERVEC1 define (only useful if default.glsl has been customized)"};
cvar_t r_glsl_postprocess_uservec2_enable = {CVAR_SAVE, "r_glsl_postprocess_uservec2_enable", "1", "enables postprocessing uservec2 usage, creates USERVEC1 define (only useful if default.glsl has been customized)"};
cvar_t r_glsl_postprocess_uservec3_enable = {CVAR_SAVE, "r_glsl_postprocess_uservec3_enable", "1", "enables postprocessing uservec3 usage, creates USERVEC1 define (only useful if default.glsl has been customized)"};
cvar_t r_glsl_postprocess_uservec4_enable = {CVAR_SAVE, "r_glsl_postprocess_uservec4_enable", "1", "enables postprocessing uservec4 usage, creates USERVEC1 define (only useful if default.glsl has been customized)"};

cvar_t r_water = {CVAR_SAVE, "r_water", "0", "whether to use reflections and refraction on water surfaces (note: r_wateralpha must be set below 1)"};
cvar_t r_water_clippingplanebias = {CVAR_SAVE, "r_water_clippingplanebias", "1", "a rather technical setting which avoids black pixels around water edges"};
cvar_t r_water_resolutionmultiplier = {CVAR_SAVE, "r_water_resolutionmultiplier", "0.5", "multiplier for screen resolution when rendering refracted/reflected scenes, 1 is full quality, lower values are faster"};
cvar_t r_water_refractdistort = {CVAR_SAVE, "r_water_refractdistort", "0.01", "how much water refractions shimmer"};
cvar_t r_water_reflectdistort = {CVAR_SAVE, "r_water_reflectdistort", "0.01", "how much water reflections shimmer"};
cvar_t r_water_scissormode = {0, "r_water_scissormode", "3", "scissor (1) or cull (2) or both (3) water renders"};
cvar_t r_water_lowquality = {0, "r_water_lowquality", "0", "special option to accelerate water rendering, 1 disables shadows and particles, 2 disables all dynamic lights"};
cvar_t r_water_hideplayer = {CVAR_SAVE, "r_water_hideplayer", "0", "if set to 1 then player will be hidden in refraction views, if set to 2 then player will also be hidden in reflection views, player is always visible in camera views"};
cvar_t r_water_fbo = {CVAR_SAVE, "r_water_fbo", "1", "enables use of render to texture for water effects, otherwise copy to texture is used (slower)"};

cvar_t r_lerpsprites = {CVAR_SAVE, "r_lerpsprites", "0", "enables animation smoothing on sprites"};
cvar_t r_lerpmodels = {CVAR_SAVE, "r_lerpmodels", "1", "enables animation smoothing on models"};
cvar_t r_lerplightstyles = {CVAR_SAVE, "r_lerplightstyles", "0", "enable animation smoothing on flickering lights"};
cvar_t r_waterscroll = {CVAR_SAVE, "r_waterscroll", "1", "makes water scroll around, value controls how much"};

cvar_t r_bloom = {CVAR_SAVE, "r_bloom", "0", "enables bloom effect (makes bright pixels affect neighboring pixels)"};
cvar_t r_bloom_colorscale = {CVAR_SAVE, "r_bloom_colorscale", "1", "how bright the glow is"};

cvar_t r_bloom_brighten = {CVAR_SAVE, "r_bloom_brighten", "2", "how bright the glow is, after subtract/power"};
cvar_t r_bloom_blur = {CVAR_SAVE, "r_bloom_blur", "4", "how large the glow is"};
cvar_t r_bloom_resolution = {CVAR_SAVE, "r_bloom_resolution", "320", "what resolution to perform the bloom effect at (independent of screen resolution)"};
cvar_t r_bloom_colorexponent = {CVAR_SAVE, "r_bloom_colorexponent", "1", "how exaggerated the glow is"};
cvar_t r_bloom_colorsubtract = {CVAR_SAVE, "r_bloom_colorsubtract", "0.125", "reduces bloom colors by a certain amount"};
cvar_t r_bloom_scenebrightness = {CVAR_SAVE, "r_bloom_scenebrightness", "1", "global rendering brightness when bloom is enabled"};

cvar_t r_hdr_scenebrightness = {CVAR_SAVE, "r_hdr_scenebrightness", "1", "global rendering brightness"};
cvar_t r_hdr_glowintensity = {CVAR_SAVE, "r_hdr_glowintensity", "1", "how bright light emitting textures should appear"};
cvar_t r_hdr_irisadaptation = {CVAR_SAVE, "r_hdr_irisadaptation", "0", "adjust scene brightness according to light intensity at player location"};
cvar_t r_hdr_irisadaptation_multiplier = {CVAR_SAVE, "r_hdr_irisadaptation_multiplier", "2", "brightness at which value will be 1.0"};
cvar_t r_hdr_irisadaptation_minvalue = {CVAR_SAVE, "r_hdr_irisadaptation_minvalue", "0.5", "minimum value that can result from multiplier / brightness"};
cvar_t r_hdr_irisadaptation_maxvalue = {CVAR_SAVE, "r_hdr_irisadaptation_maxvalue", "4", "maximum value that can result from multiplier / brightness"};
cvar_t r_hdr_irisadaptation_value = {0, "r_hdr_irisadaptation_value", "1", "current value as scenebrightness multiplier, changes continuously when irisadaptation is active"};
cvar_t r_hdr_irisadaptation_fade_up = {CVAR_SAVE, "r_hdr_irisadaptation_fade_up", "0.1", "fade rate at which value adjusts to darkness"};
cvar_t r_hdr_irisadaptation_fade_down = {CVAR_SAVE, "r_hdr_irisadaptation_fade_down", "0.5", "fade rate at which value adjusts to brightness"};
cvar_t r_hdr_irisadaptation_radius = {CVAR_SAVE, "r_hdr_irisadaptation_radius", "15", "lighting within this many units of the eye is averaged"};

cvar_t r_smoothnormals_areaweighting = {0, "r_smoothnormals_areaweighting", "1", "uses significantly faster (and supposedly higher quality) area-weighted vertex normals and tangent vectors rather than summing normalized triangle normals and tangents"};

cvar_t developer_texturelogging = {0, "developer_texturelogging", "0", "produces a textures.log file containing names of skins and map textures the engine tried to load"};

cvar_t gl_lightmaps = {0, "gl_lightmaps", "0", "draws only lightmaps, no texture (for level designers), a value of 2 keeps normalmap shading"};

cvar_t r_test = {0, "r_test", "0", "internal development use only, leave it alone (usually does nothing anyway)"};

cvar_t r_batch_multidraw = {CVAR_SAVE, "r_batch_multidraw", "1", "issue multiple glDrawElements calls when rendering a batch of surfaces with the same texture (otherwise the index data is copied to make it one draw)"};
cvar_t r_batch_multidraw_mintriangles = {CVAR_SAVE, "r_batch_multidraw_mintriangles", "0", "minimum number of triangles to activate multidraw path (copying small groups of triangles may be faster)"};
cvar_t r_batch_debugdynamicvertexpath = {CVAR_SAVE, "r_batch_debugdynamicvertexpath", "0", "force the dynamic batching code path for debugging purposes"};
cvar_t r_batch_dynamicbuffer = {CVAR_SAVE, "r_batch_dynamicbuffer", "0", "use vertex/index buffers for drawing dynamic and copytriangles batches"};

cvar_t r_glsl_saturation = {CVAR_SAVE, "r_glsl_saturation", "1", "saturation multiplier (only working in glsl!)"};
cvar_t r_glsl_saturation_redcompensate = {CVAR_SAVE, "r_glsl_saturation_redcompensate", "0", "a 'vampire sight' addition to desaturation effect, does compensation for red color, r_glsl_restart is required"};

cvar_t r_glsl_vertextextureblend_usebothalphas = {CVAR_SAVE, "r_glsl_vertextextureblend_usebothalphas", "0", "use both alpha layers on vertex blended surfaces, each alpha layer sets amount of 'blend leak' on another layer, requires mod_q3shader_force_terrain_alphaflag on."};

cvar_t r_framedatasize = {CVAR_SAVE, "r_framedatasize", "0.5", "size of renderer data cache used during one frame (for skeletal animation caching, light processing, etc)"};
cvar_t r_buffermegs[R_BUFFERDATA_COUNT] =
{
	{CVAR_SAVE, "r_buffermegs_vertex", "4", "vertex buffer size for one frame"},
	{CVAR_SAVE, "r_buffermegs_index16", "1", "index buffer size for one frame (16bit indices)"},
	{CVAR_SAVE, "r_buffermegs_index32", "1", "index buffer size for one frame (32bit indices)"},
	{CVAR_SAVE, "r_buffermegs_uniform", "0.25", "uniform buffer size for one frame"},
};

extern cvar_t v_glslgamma;
extern cvar_t v_glslgamma_2d;
