#pragma once

// transparent
#define TEXF_ALPHA 0x00000001
// mipmapped
#define TEXF_MIPMAP 0x00000002
// multiply RGB by A channel before uploading
#define TEXF_RGBMULTIPLYBYALPHA 0x00000004
// indicates texture coordinates should be clamped rather than wrapping
#define TEXF_CLAMP 0x00000020
// indicates texture should be uploaded using GL_NEAREST or GL_NEAREST_MIPMAP_NEAREST mode
#define TEXF_FORCENEAREST 0x00000040
// indicates texture should be uploaded using GL_LINEAR or GL_LINEAR_MIPMAP_NEAREST or GL_LINEAR_MIPMAP_LINEAR mode
#define TEXF_FORCELINEAR 0x00000080
// indicates texture should be affected by gl_picmip and gl_max_size cvar
#define TEXF_PICMIP 0x00000100
// indicates texture should be compressed if possible
#define TEXF_COMPRESS 0x00000200
// use this flag to block R_PurgeTexture from freeing a texture (only used by r_texture_white and similar which may be used in skinframe_t)
#define TEXF_PERSISTENT 0x00000400
// indicates texture should use GL_COMPARE_R_TO_TEXTURE mode
#define TEXF_COMPARE 0x00000800
// indicates texture should use lower precision where supported
#define TEXF_LOWPRECISION 0x00001000
// indicates texture should support R_UpdateTexture on small regions, actual uploads may be delayed until R_Mesh_TexBind if gl_nopartialtextureupdates is on
#define TEXF_ALLOWUPDATES 0x00002000
// indicates texture should be affected by gl_picmip_world and r_picmipworld (maybe others in the future) instead of gl_picmip_other
#define TEXF_ISWORLD 0x00004000
// indicates texture should be affected by gl_picmip_sprites and r_picmipsprites (maybe others in the future) instead of gl_picmip_other
#define TEXF_ISSPRITE 0x00008000
// indicates the texture will be used as a render target (D3D hint)
#define TEXF_RENDERTARGET 0x0010000
// used for checking if textures mismatch
#define TEXF_IMPORTANTBITS (TEXF_ALPHA | TEXF_MIPMAP | TEXF_RGBMULTIPLYBYALPHA | TEXF_CLAMP | TEXF_FORCENEAREST | TEXF_FORCELINEAR | TEXF_PICMIP | TEXF_COMPRESS | TEXF_COMPARE | TEXF_LOWPRECISION | TEXF_RENDERTARGET)
// set as a flag to force the texture to be reloaded
#define TEXF_FORCE_RELOAD 0x80000000

typedef enum textype_e
{
	// 8bit paletted
	TEXTYPE_PALETTE,
	// 32bit RGBA
	TEXTYPE_RGBA,
	// 32bit BGRA (preferred format due to faster uploads on most hardware)
	TEXTYPE_BGRA,
	// 8bit ALPHA (used for freetype fonts)
	TEXTYPE_ALPHA,
	// 4x4 block compressed 15bit color (4 bits per pixel)
	TEXTYPE_DXT1,
	// 4x4 block compressed 15bit color plus 1bit alpha (4 bits per pixel)
	TEXTYPE_DXT1A,
	// 4x4 block compressed 15bit color plus 8bit alpha (8 bits per pixel)
	TEXTYPE_DXT3,
	// 4x4 block compressed 15bit color plus 8bit alpha (8 bits per pixel)
	TEXTYPE_DXT5,

	// default compressed type for GLES2
	TEXTYPE_ETC1,

	// 8bit paletted in sRGB colorspace
	TEXTYPE_SRGB_PALETTE,
	// 32bit RGBA in sRGB colorspace
	TEXTYPE_SRGB_RGBA,
	// 32bit BGRA (preferred format due to faster uploads on most hardware) in sRGB colorspace
	TEXTYPE_SRGB_BGRA,
	// 4x4 block compressed 15bit color (4 bits per pixel) in sRGB colorspace
	TEXTYPE_SRGB_DXT1,
	// 4x4 block compressed 15bit color plus 1bit alpha (4 bits per pixel) in sRGB colorspace
	TEXTYPE_SRGB_DXT1A,
	// 4x4 block compressed 15bit color plus 8bit alpha (8 bits per pixel) in sRGB colorspace
	TEXTYPE_SRGB_DXT3,
	// 4x4 block compressed 15bit color plus 8bit alpha (8 bits per pixel) in sRGB colorspace
	TEXTYPE_SRGB_DXT5,

	// this represents the same format as the framebuffer, for fast copies
	TEXTYPE_COLORBUFFER,
	// this represents an RGBA half_float texture (4 16bit floats)
	TEXTYPE_COLORBUFFER16F,
	// this represents an RGBA float texture (4 32bit floats)
	TEXTYPE_COLORBUFFER32F,
	// depth-stencil buffer (or texture)
	TEXTYPE_DEPTHBUFFER16,
	// depth-stencil buffer (or texture)
	TEXTYPE_DEPTHBUFFER24,
	// 32bit D24S8 buffer (24bit depth, 8bit stencil), not supported on OpenGL ES
	TEXTYPE_DEPTHBUFFER24STENCIL8,
	// shadowmap-friendly format with depth comparison (not supported on some hardware)
	TEXTYPE_SHADOWMAP16_COMP,
	// shadowmap-friendly format with raw reading (not supported on some hardware)
	TEXTYPE_SHADOWMAP16_RAW,
	// shadowmap-friendly format with depth comparison (not supported on some hardware)
	TEXTYPE_SHADOWMAP24_COMP,
	// shadowmap-friendly format with raw reading (not supported on some hardware)
	TEXTYPE_SHADOWMAP24_RAW,
}
textype_t;

// contents of this structure are private to gl_textures.c
typedef struct rtexturepool_s
{
	int useless;
}
rtexturepool_t;

typedef void (*updatecallback_t)(struct rtexture_t *rt, void *data);


struct textypeinfo_t
{
	const char *name;
	textype_t textype;
	int inputbytesperpixel;
	int internalbytesperpixel;
	float glinternalbytesperpixel;
	int glinternalformat;
	int glformat;
	int gltype;
};

#define mRTextureNonOpaque	1

struct rtexture_proto
{
		// d3d stuff the backend needs
	void 					*d3dtexture;
	void 					*d3dsurface;
	
	// dynamic texture stuff [11/22/2007 Black]
	updatecallback_t 		updatecallback;
	void 					*updatacallback_data;
	// --- [11/22/2007 Black]

	// stores backup copy of texture for deferred texture updates (gl_nopartialtextureupdates cvar)
	unsigned char 			*bufferpixels;
	// pointer to texturepool (check this to see if the texture is allocated)
	struct gltexturepool_t 	*pool;
	// pointer to next texture in texturepool chain
	struct gltexture_t 		*chain;
	// copy of the original texture(s) supplied to the upload function, for
	// delayed uploads (non-precached)
	unsigned char 			*inputtexels;
	// pointer to one of the textype_ structs
	textypeinfo_t 			*textype;
	// palette if the texture is TEXTYPE_PALETTE
	const unsigned int 		*palette;
	/*
		end of pointer fields
	*/
	int gltexturetypeenum; // used by R_Mesh_TexBind
	int texnum; // GL texture slot number
	int renderbuffernum; // GL renderbuffer slot number

#ifdef SUPPORTD3D
	bool d3disrendertargetsurface;
	bool d3disdepthstencilsurface;
	int d3dformat;
	int d3dusage;
	int d3dpool;
	int d3daddressu;
	int d3daddressv;
	int d3daddressw;
	int d3dmagfilter;
	int d3dminfilter;
	int d3dmipfilter;
	int d3dmaxmiplevelfilter;
	int d3dmipmaplodbias;
	int d3dmaxmiplevel;
#endif
	// original data size in *inputtexels
	int inputwidth;
	int inputheight;
	int inputdepth;

	// original data size in *inputtexels
	int inputdatasize;
	// flags supplied to the LoadTexture function
	// (might be altered to remove TEXF_ALPHA), and GLTEXF_ private flags
	int flags;
	// picmip level
	int miplevel;

	// one of the GLTEXTURETYPE_ values
	int texturetype;

	// actual stored texture size after gl_picmip and gl_max_size are applied
	// (power of 2 if vid.support.arb_texture_non_power_of_two is not supported)
	int tilewidth;
	int tileheight;
	int tiledepth;
	// 1 or 6 depending on texturetype
	int sides;
	// how many mipmap levels in this texture
	int miplevels;
	// bytes per pixel
	int bytesperpixel;
	// GL_RGB or GL_RGBA or GL_DEPTH_COMPONENT
	int glformat;
	// 3 or 4
	int glinternalformat;
	// GL_UNSIGNED_BYTE or GL_UNSIGNED_INT or GL_UNSIGNED_SHORT or GL_FLOAT
	int gltype;
	bool dirty; // indicates that R_RealGetTexture should be called
	bool glisdepthstencil; // indicates that FBO attachment has to be GL_DEPTH_STENCIL_ATTACHMENT
	bool buffermodified;
	char pad0;
};

struct rtexture_t : public rtexture_proto
{
	static constexpr size_t identifierLength = MAX_QPATH + 32;
	// name of the texture (this might be removed someday), no duplicates
	char identifier[identifierLength];
	
	char padding[padSizeSimd(sizeof(rtexture_proto) + identifierLength)];
};

static_assert(sizeof(rtexture_t) % mSimdAlign == 0);


// allocate a texture pool, to be used with R_LoadTexture
rtexturepool_t *R_AllocTexturePool();
// free a texture pool (textures can not be freed individually)
void R_FreeTexturePool(rtexturepool_t **rtexturepool);

// the color/normal/etc cvars should be checked by callers of R_LoadTexture* functions to decide whether to add TEXF_COMPRESS to the flags
extern cvar_t gl_texturecompression;
extern cvar_t gl_texturecompression_color;
extern cvar_t gl_texturecompression_normal;
extern cvar_t gl_texturecompression_gloss;
extern cvar_t gl_texturecompression_glow;
extern cvar_t gl_texturecompression_2d;
extern cvar_t gl_texturecompression_q3bsplightmaps;
extern cvar_t gl_texturecompression_q3bspdeluxemaps;
extern cvar_t gl_texturecompression_sky;
extern cvar_t gl_texturecompression_lightcubemaps;
extern cvar_t gl_texturecompression_reflectmask;
extern cvar_t r_texture_dds_load;
extern cvar_t r_texture_dds_save;

// add a texture to a pool and optionally precache (upload) it
// (note: data == NULL is perfectly acceptable)
// (note: palette must not be NULL if using TEXTYPE_PALETTE)
rtexture_t *R_LoadTexture2D(rtexturepool_t *rtexturepool, const char *identifier, int width, int height, const unsigned char *data, textype_t textype, int flags, int miplevel, const unsigned int *palette);
rtexture_t *R_LoadTexture3D(rtexturepool_t *rtexturepool, const char *identifier, int width, int height, int depth, const unsigned char *data, textype_t textype, int flags, int miplevel, const unsigned int *palette);
rtexture_t *R_LoadTextureCubeMap(rtexturepool_t *rtexturepool, const char *identifier, int width, const unsigned char *data, textype_t textype, int flags, int miplevel, const unsigned int *palette);
rtexture_t *R_LoadTextureShadowMap2D(rtexturepool_t *rtexturepool, const char *identifier, int width, int height, textype_t textype, bool filter);
rtexture_t *R_LoadTextureRenderBuffer(rtexturepool_t *rtexturepool, const char *identifier, int width, int height, textype_t textype);
rtexture_t *R_LoadTextureDDSFile(rtexturepool_t *rtexturepool, const char *filename, bool srgb, int flags, bool *hasalphaflag, float *avgcolor, int miplevel, bool optionaltexture);

// saves a texture to a DDS file
int R_SaveTextureDDSFile(rtexture_t *rt, const char *filename, bool skipuncompressed, bool hasalpha);

// free a texture
void R_FreeTexture(rtexture_t *rt);

// update a portion of the image data of a texture, used by lightmap updates
// and procedural textures such as video playback, actual uploads may be
// delayed by gl_nopartialtextureupdates cvar until R_Mesh_TexBind uses it
void R_UpdateTexture(rtexture_t *rt, const unsigned char *data, int x, int y, int z, int width, int height, int depth);

// returns the renderer dependent texture slot number (call this before each
// use, as a texture might not have been precached)
#define R_GetTexture(rt) ((rt) ? ((rt)->dirty ? R_RealGetTexture(rt) : (rt)->texnum) : r_texture_white->texnum)
int R_RealGetTexture (rtexture_t *rt);

// returns width of texture, as was specified when it was uploaded
int R_TextureWidth(rtexture_t *rt);

// returns height of texture, as was specified when it was uploaded
int R_TextureHeight(rtexture_t *rt);

// returns flags of texture, as was specified when it was uploaded
int R_TextureFlags(rtexture_t *rt);

// only frees the texture if TEXF_PERSISTENT is not set
// also resets the variable
void R_PurgeTexture(rtexture_t *prt);

// frees processing buffers each frame, and may someday animate procedural textures
void R_Textures_Frame();

// maybe rename this - sounds awful? [11/21/2007 Black]
void R_MarkDirtyTexture(rtexture_t *rt);
void R_MakeTextureDynamic(rtexture_t *rt, updatecallback_t updatecallback, void *data);

// Clear the texture's contents
void R_ClearTexture (rtexture_t *rt);

// returns the desired picmip level for given TEXF_ flags
int R_PicmipForFlags(int flags);

void R_TextureStats_Print(bool printeach, bool printpool, bool printtotal);
