#pragma once

r_shadow_rendermode_t r_shadow_rendermode 					= R_SHADOW_RENDERMODE_NONE;
r_shadow_rendermode_t r_shadow_lightingrendermode 			= R_SHADOW_RENDERMODE_NONE;
r_shadow_rendermode_t r_shadow_shadowingrendermode_zpass 	= R_SHADOW_RENDERMODE_NONE;
r_shadow_rendermode_t r_shadow_shadowingrendermode_zfail 	= R_SHADOW_RENDERMODE_NONE;

int r_shadow_shadowmapside;

__align(mSimdAlign)
float r_shadow_shadowmap_texturescale[2];

__align(mSimdAlign)
float r_shadow_shadowmap_parameters[4];

int r_shadow_cullface_front, r_shadow_cullface_back;
GLuint r_shadow_fbo2d;
r_shadow_shadowmode_t r_shadow_shadowmode;
int r_shadow_shadowmapfilterquality;
int r_shadow_shadowmapdepthbits;
int r_shadow_shadowmapmaxsize;



int r_shadow_shadowmappcf;
int r_shadow_shadowmapborder;
matrix4x4_t r_shadow_shadowmapmatrix;

__align(mSimdAlign)
vector4si r_shadow_lightscissor;


bool r_shadow_shadowmapvsdct;
bool r_shadow_shadowmapsampler;
bool r_shadow_shadowmapshadowsampler;
bool r_shadow_usingshadowmap2d;
bool r_shadow_usingshadowmaportho;
bool r_shadow_usingdeferredprepass;
bool r_shadow_shadowmapdepthtexture;

int maxshadowtriangles;
int *shadowelements;

int maxshadowvertices;
float *shadowvertex3f;

int maxshadowmark;
int numshadowmark;
int *shadowmark;
int *shadowmarklist;
int shadowmarkcount;

int maxshadowsides;
int numshadowsides;
uint8 *shadowsides;
int *shadowsideslist;

int maxvertexupdate;
int *vertexupdate;
int *vertexremap;
int vertexupdatenum;

int r_shadow_buffer_numleafpvsbytes;
uint8 *r_shadow_buffer_visitingleafpvs;
uint8 *r_shadow_buffer_leafpvs;
int *r_shadow_buffer_leaflist;

int r_shadow_buffer_numsurfacepvsbytes;
uint8 *r_shadow_buffer_surfacepvs;
int *r_shadow_buffer_surfacelist;
uint8 *r_shadow_buffer_surfacesides;

int r_shadow_buffer_numshadowtrispvsbytes;
uint8 *r_shadow_buffer_shadowtrispvs;
int r_shadow_buffer_numlighttrispvsbytes;
uint8 *r_shadow_buffer_lighttrispvs;

rtexturepool_t *r_shadow_texturepool;
rtexture_t *r_shadow_attenuationgradienttexture;
rtexture_t *r_shadow_attenuation2dtexture;
rtexture_t *r_shadow_attenuation3dtexture;
skinframe_t *r_shadow_lightcorona;
rtexture_t *r_shadow_shadowmap2ddepthbuffer;
rtexture_t *r_shadow_shadowmap2ddepthtexture;
rtexture_t *r_shadow_shadowmapvsdcttexture;
int r_shadow_shadowmapsize; // changes for each light based on distance
int r_shadow_shadowmaplod; // changes for each light based on distance

GLuint r_shadow_prepassgeometryfbo;
GLuint r_shadow_prepasslightingdiffusespecularfbo;
GLuint r_shadow_prepasslightingdiffusefbo;
int r_shadow_prepass_width;
int r_shadow_prepass_height;

rtexture_t *	r_shadow_prepassgeometrydepthbuffer;
rtexture_t *	r_shadow_prepassgeometrynormalmaptexture;
rtexture_t *	r_shadow_prepasslightingdiffusetexture;
rtexture_t *	r_shadow_prepasslightingspeculartexture;

// keep track of the provided framebuffer info
static int 			r_shadow_fb_fbo;
static rtexture_t *	r_shadow_fb_depthtexture;
static rtexture_t *	r_shadow_fb_colortexture;

// lights are reloaded when this changes
char r_shadow_mapname[MAX_QPATH];

// buffer for doing corona fading
unsigned int 		r_shadow_occlusion_buf = 0;

// used only for light filters (cubemaps)
rtexturepool_t *	r_shadow_filters_texturepool;

r_shadow_bouncegrid_settings_t r_shadow_bouncegridsettings;
rtexture_t *r_shadow_bouncegridtexture;
matrix4x4_t r_shadow_bouncegridmatrix;
vec_t r_shadow_bouncegridintensity;
bool r_shadow_bouncegriddirectional;
static double r_shadow_bouncegridtime;
static int r_shadow_bouncegridresolution[3];
static int r_shadow_bouncegridnumpixels;
static uint8 *r_shadow_bouncegridpixels;
static float *r_shadow_bouncegridhighpixels;

static float r_shadow_attendividebias; // r_shadow_lightattenuationdividebias
static float r_shadow_attenlinearscale; // r_shadow_lightattenuationlinearscale
static float r_shadow_attentable[ATTENTABLESIZE+1];

rtlight_t *r_shadow_compilingrtlight;
static memexpandablearray_t r_shadow_worldlightsarray;
dlight_t *r_shadow_selectedlight;
dlight_t r_shadow_bufferlight;
vec3_t r_editlights_cursorlocation;
bool r_editlights_lockcursor;

extern int con_vislines;

skinframe_t *r_editlights_sprcursor;
skinframe_t *r_editlights_sprlight;
skinframe_t *r_editlights_sprnoshadowlight;
skinframe_t *r_editlights_sprcubemaplight;
skinframe_t *r_editlights_sprcubemapnoshadowlight;
skinframe_t *r_editlights_sprselection;

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

//#define MAX_MODELSHADOWS 1024
static constexpr size32 	MAX_MODELSHADOWS = 1024;
static int 					r_shadow_nummodelshadows;
static entity_render_t *	r_shadow_modelshadows[MAX_MODELSHADOWS];


__align(mSimdAlign)
static float spritetexcoord2f[4*2] = {0, 1, 0, 0, 1, 0, 1, 1};


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
