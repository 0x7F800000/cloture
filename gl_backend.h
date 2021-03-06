#pragma once

extern r_viewport_t gl_viewport;
extern matrix4x4_t gl_modelmatrix;
extern matrix4x4_t gl_viewmatrix;
extern matrix4x4_t gl_modelviewmatrix;
extern matrix4x4_t gl_projectionmatrix;
extern matrix4x4_t gl_modelviewprojectionmatrix;
extern float gl_modelview16f[16];
extern float gl_modelviewprojection16f[16];
extern bool gl_modelmatrixchanged;

extern cvar_t gl_vbo_dynamicvertex;
extern cvar_t gl_vbo_dynamicindex;

constexpr size_t POLYGONELEMENTS_MAXPOINTS = 258;

extern int polygonelement3i[(POLYGONELEMENTS_MAXPOINTS-2)*3];
extern unsigned short polygonelement3s[(POLYGONELEMENTS_MAXPOINTS-2)*3];

constexpr size_t QUADELEMENTS_MAXQUADS = 128;
extern int quadelement3i[QUADELEMENTS_MAXQUADS*6];
extern unsigned short quadelement3s[QUADELEMENTS_MAXQUADS*6];

void R_Viewport_TransformToScreen(const r_viewport_t *v, const vec4_t in, vec4_t out);
bool R_ScissorForBBox(const float *mins, const float *maxs, int *scissor);
void R_Viewport_InitOrtho(r_viewport_t *v, const matrix4x4_t *cameramatrix, int x, int y, int width, int height, float x1, float y1, float x2, float y2, float zNear, float zFar, const float *nearplane);
void R_Viewport_InitPerspective(r_viewport_t *v, const matrix4x4_t *cameramatrix, int x, int y, int width, int height, float frustumx, float frustumy, float zNear, float zFar, const float *nearplane);
void R_Viewport_InitPerspectiveInfinite(r_viewport_t *v, const matrix4x4_t *cameramatrix, int x, int y, int width, int height, float frustumx, float frustumy, float zNear, const float *nearplane);
void R_Viewport_InitCubeSideView(r_viewport_t *v, const matrix4x4_t *cameramatrix, int side, int size, float nearclip, float farclip, const float *nearplane);
void R_Viewport_InitRectSideView(r_viewport_t *v, const matrix4x4_t *cameramatrix, int side, int size, int border, float nearclip, float farclip, const float *nearplane);
void R_SetViewport(const r_viewport_t *v);
void R_GetViewport(r_viewport_t *v);
void GL_Finish();

void GL_BlendFunc(int blendfunc1, int blendfunc2);
void GL_BlendEquationSubtract(bool negated);
void GL_DepthMask(int state);
void GL_DepthTest(int state);
void GL_DepthFunc(int state);
void GL_DepthRange(float nearfrac, float farfrac);
void R_SetStencilSeparate(bool enable, int writemask, int frontfail, int frontzfail, int frontzpass, int backfail, int backzfail, int backzpass, int frontcompare, int backcompare, int comparereference, int comparemask);
void R_SetStencil(bool enable, int writemask, int fail, int zfail, int zpass, int compare, int comparereference, int comparemask);
void GL_PolygonOffset(float planeoffset, float depthoffset);
void GL_CullFace(int state);
void GL_AlphaTest(int state);
void GL_AlphaToCoverage(bool state);
void GL_ColorMask(int r, int g, int b, int a);
void GL_Color(float cr, float cg, float cb, float ca);
void GL_ActiveTexture(unsigned int num);
void GL_ClientActiveTexture(unsigned int num);
void GL_Scissor(int x, int y, int width, int height);
void GL_ScissorTest(int state);
void GL_Clear(int mask, const float *colorvalue, float depthvalue, int stencilvalue);
void GL_ReadPixelsBGRA(int x, int y, int width, int height, unsigned char *outpixels);
int R_Mesh_CreateFramebufferObject(rtexture_t *depthtexture, rtexture_t *colortexture, rtexture_t *colortexture2, rtexture_t *colortexture3, rtexture_t *colortexture4);
void R_Mesh_DestroyFramebufferObject(int fbo);
void R_Mesh_SetRenderTargets(int fbo, rtexture_t *depthtexture, rtexture_t *colortexture, rtexture_t *colortexture2, rtexture_t *colortexture3, rtexture_t *colortexture4);

unsigned int GL_Backend_CompileProgram(int vertexstrings_count, const char **vertexstrings_list, int geometrystrings_count, const char **geometrystrings_list, int fragmentstrings_count, const char **fragmentstrings_list);
void GL_Backend_FreeProgram(unsigned int prog);

extern cvar_t gl_paranoid;
extern cvar_t gl_printcheckerror;

// adds console variables and registers the render module (only call from GL_Init)
void gl_backend_init();

// starts mesh rendering for the frame
void R_Mesh_Start();

// ends mesh rendering for the frame
// (only valid after R_Mesh_Start)
void R_Mesh_Finish();


// vertex buffer and index buffer creation/updating/freeing
r_meshbuffer_t *R_Mesh_CreateMeshBuffer(const void *data, size_t size, const char *name, bool isindexbuffer, bool isuniformbuffer, bool isdynamic, bool isindex16);
void R_Mesh_UpdateMeshBuffer(r_meshbuffer_t *buffer, const void *data, size_t size, bool subdata, size_t offset);
void R_Mesh_DestroyMeshBuffer(r_meshbuffer_t *buffer);
void GL_Mesh_ListVBOs(bool printeach);

void R_Mesh_PrepareVertices_Vertex3f(int numvertices, const float *vertex3f, const r_meshbuffer_t *buffer, int bufferoffset);

r_vertexgeneric_t *R_Mesh_PrepareVertices_Generic_Lock(int numvertices);
bool R_Mesh_PrepareVertices_Generic_Unlock();
void R_Mesh_PrepareVertices_Generic_Arrays(int numvertices, const float *vertex3f, const float *color4f, const float *texcoord2f);
void R_Mesh_PrepareVertices_Generic(int numvertices, const r_vertexgeneric_t *vertex, const r_meshbuffer_t *vertexbuffer, int bufferoffset);

r_vertexmesh_t *R_Mesh_PrepareVertices_Mesh_Lock(int numvertices);
bool R_Mesh_PrepareVertices_Mesh_Unlock(); // if this returns false, you need to prepare the mesh again!
void R_Mesh_PrepareVertices_Mesh_Arrays(int numvertices, const float *vertex3f, const float *svector3f, const float *tvector3f, const float *normal3f, const float *color4f, const float *texcoordtexture2f, const float *texcoordlightmap2f);
void R_Mesh_PrepareVertices_Mesh(int numvertices, const r_vertexmesh_t *vertex, const r_meshbuffer_t *buffer, int bufferoffset);

// sets up the requested vertex transform matrix
void R_EntityMatrix(const matrix4x4_t *matrix);
// sets the vertex array pointer
void R_Mesh_VertexPointer(int components, int gltype, size_t stride, const void *pointer, const r_meshbuffer_t *vertexbuffer, size_t bufferoffset);
// sets the color array pointer (GL_Color only works when this is NULL)
void R_Mesh_ColorPointer(int components, int gltype, size_t stride, const void *pointer, const r_meshbuffer_t *vertexbuffer, size_t bufferoffset);
// sets the texcoord array pointer for an array unit, if GL_UNSIGNED_BYTE | 0x80000000 is specified it will be an unnormalized type (integer values)
void R_Mesh_TexCoordPointer(unsigned int unitnum, int components, int gltype, size_t stride, const void *pointer, const r_meshbuffer_t *vertexbuffer, size_t bufferoffset);
// returns current texture bound to this identifier
int R_Mesh_TexBound(unsigned int unitnum, int id);
// copies a section of the framebuffer to a 2D texture
void R_Mesh_CopyToTexture(rtexture_t *tex, int tx, int ty, int sx, int sy, int width, int height);
// bind a given texture to a given image unit
void R_Mesh_TexBind(unsigned int unitnum, rtexture_t *tex);
// sets the texcoord matrix for a texenv unit, can be NULL or blank (will use identity)
void R_Mesh_TexMatrix(unsigned int unitnum, const matrix4x4_t *matrix);
// sets the combine state for a texenv unit
void R_Mesh_TexCombine(unsigned int unitnum, int combinergb, int combinealpha, int rgbscale, int alphascale);
// set up a blank texture state (unbinds all textures, texcoord pointers, and resets combine settings)
void R_Mesh_ResetTextureState();
// before a texture is freed, make sure there are no references to it
void R_Mesh_ClearBindingsForTexture(int texnum);

// renders a mesh
void R_Mesh_Draw(int firstvertex, int numvertices, int firsttriangle, int numtriangles, const int *element3i, const r_meshbuffer_t *element3i_indexbuffer, int element3i_bufferoffset, const unsigned short *element3s, const r_meshbuffer_t *element3s_indexbuffer, int element3s_bufferoffset);

// saves a section of the rendered frame to a .tga or .jpg file
bool SCR_ScreenShot(char *filename, unsigned char *buffer1, unsigned char *buffer2, int x, int y, int width, int height, bool flipx, bool flipy, bool flipdiagonal, bool jpeg, bool png, bool gammacorrect, bool keep_alpha);
// used by R_Envmap_f and internally in backend, clears the frame
void R_ClearScreen(bool fogcolor);


#define		GL_INLINE	__forceinline
namespace cloture::engine::renderer::gl
{
	static GL_INLINE void finish()
	{
		GL_Finish();
	}
	static GL_INLINE void blendFunc(const int f1, const int f2)
	{
		GL_BlendFunc(f1, f2);
	}
	static GL_INLINE  void blendEquationSubtract(const bool negated)
	{
		GL_BlendEquationSubtract(negated);
	}
	static GL_INLINE  void depthMask(const int state)
	{
		GL_DepthMask(state);
	}
	static GL_INLINE  void depthTest(const int state)
	{
		GL_DepthTest(state);
	}
	static GL_INLINE  void depthFunc(const int state)
	{
		GL_DepthFunc(state);
	}
	static GL_INLINE  void depthRange(const float nearfrac, const float farfrac)
	{
		GL_DepthRange(nearfrac, farfrac);
	}
	static GL_INLINE void polygonOffset(const float planeoffset, const float depthoffset)
	{
		GL_PolygonOffset(planeoffset, depthoffset);
	}
	static GL_INLINE void cullFace(const int state)
	{
		GL_CullFace(state);
	}
	static GL_INLINE void alphaTest(const int state)
	{
		GL_AlphaTest(state);
	}
	static GL_INLINE void alphaToCoverage(const bool state)
	{
		GL_AlphaToCoverage(state);
	}
	static GL_INLINE void colorMask(const int r, const int g, const int b, const int a)
	{
		GL_ColorMask(r, g, b, a);
	}
	static GL_INLINE void color(const float cr, const float cg, const float cb, const float ca)
	{
		GL_Color(cr, cg, cb, ca);
	}
	static GL_INLINE void activeTexture(const unsigned int num)
	{
		GL_ActiveTexture(num);
	}
	static GL_INLINE void clientActiveTexture(const unsigned int num)
	{
		GL_ClientActiveTexture(num);
	}
	static GL_INLINE void scissor(const int x, const int y, const int width, const int height)
	{
		GL_Scissor(x, y, width, height);
	}
	static GL_INLINE void scissorTest(const int state)
	{
		GL_ScissorTest(state);
	}
	static GL_INLINE void clear(const int mask, const float *colorvalue, const float depthvalue, const int stencilvalue)
	{
		GL_Clear(mask, colorvalue, depthvalue, stencilvalue);
	}
	static GL_INLINE void readPixelsBGRA(const int x, const int y, const int width, const int height, unsigned char *outpixels)
	{
		GL_ReadPixelsBGRA(x, y, width, height, outpixels);
	}

}
