#pragma once


static void R_Shadow_RenderLighting_Light_Vertex_Shading(int firstvertex, int numverts, const float *diffusecolor, const float *ambientcolor)
{
	int i;
	const float *vertex3f;
	const float *normal3f;
	float *color4f;
	float dist, dot, distintensity, shadeintensity, v[3], n[3];
	switch (r_shadow_rendermode)
	{
	case R_SHADOW_RENDERMODE_LIGHT_VERTEX3DATTEN:
	case R_SHADOW_RENDERMODE_LIGHT_VERTEX2D1DATTEN:
		if (VectorLength2(diffusecolor) > .0f)
		{
			for (i = 0, vertex3f = rsurface.batchvertex3f + 3*firstvertex, normal3f = rsurface.batchnormal3f + 3*firstvertex, color4f = rsurface.passcolor4f + 4 * firstvertex;i < numverts;i++, vertex3f += 3, normal3f += 3, color4f += 4)
			{
				Matrix4x4_Transform(&rsurface.entitytolight, vertex3f, v);
				Matrix4x4_Transform3x3(&rsurface.entitytolight, normal3f, n);
				if ((dot = DotProduct(n, v)) < .0f)
				{
					shadeintensity = -dot / math::sqrtf(VectorLength2(v) * VectorLength2(n));
					VectorMA(ambientcolor, shadeintensity, diffusecolor, color4f);
				}
				else
					VectorCopy(ambientcolor, color4f);
				if (r_refdef.fogenabled)
				{
					float f;
					f = RSurf_FogVertex(vertex3f);
					VectorScale(color4f, f, color4f);
				}
				color4f[3] = 1.0f;
			}
		}
		else
		{
			for (i = 0, vertex3f = rsurface.batchvertex3f + 3*firstvertex, color4f = rsurface.passcolor4f + 4 * firstvertex;i < numverts;i++, vertex3f += 3, color4f += 4)
			{
				VectorCopy(ambientcolor, color4f);
				if (r_refdef.fogenabled)
				{
					float f;
					Matrix4x4_Transform(&rsurface.entitytolight, vertex3f, v);
					f = RSurf_FogVertex(vertex3f);
					VectorScale(color4f + 4*i, f, color4f);
				}
				color4f[3] = 1.0f;
			}
		}
		break;
	case R_SHADOW_RENDERMODE_LIGHT_VERTEX2DATTEN:
		if (VectorLength2(diffusecolor) > .0f)
		{
			for (i = 0, vertex3f = rsurface.batchvertex3f + 3*firstvertex, normal3f = rsurface.batchnormal3f + 3*firstvertex, color4f = rsurface.passcolor4f + 4 * firstvertex;i < numverts;i++, vertex3f += 3, normal3f += 3, color4f += 4)
			{
				Matrix4x4_Transform(&rsurface.entitytolight, vertex3f, v);
				if ((dist = math::fabsf(v[2])) < 1.0f && (distintensity = r_shadow_attentable[(int)(dist * ATTENTABLESIZE)]))
				{
					Matrix4x4_Transform3x3(&rsurface.entitytolight, normal3f, n);
					if ((dot = DotProduct(n, v)) < .0f)
					{
						shadeintensity = -dot / math::sqrtf(VectorLength2(v) * VectorLength2(n));
						color4f[0] = (ambientcolor[0] + shadeintensity * diffusecolor[0]) * distintensity;
						color4f[1] = (ambientcolor[1] + shadeintensity * diffusecolor[1]) * distintensity;
						color4f[2] = (ambientcolor[2] + shadeintensity * diffusecolor[2]) * distintensity;
					}
					else
					{
						color4f[0] = ambientcolor[0] * distintensity;
						color4f[1] = ambientcolor[1] * distintensity;
						color4f[2] = ambientcolor[2] * distintensity;
					}
					if (r_refdef.fogenabled)
					{
						float f;
						f = RSurf_FogVertex(vertex3f);
						VectorScale(color4f, f, color4f);
					}
				}
				else
					VectorClear(color4f);
				color4f[3] = 1.0f;
			}
		}
		else
		{
			for (i = 0, vertex3f = rsurface.batchvertex3f + 3*firstvertex, color4f = rsurface.passcolor4f + 4 * firstvertex;i < numverts;i++, vertex3f += 3, color4f += 4)
			{
				Matrix4x4_Transform(&rsurface.entitytolight, vertex3f, v);
				if ((dist = math::fabsf(v[2])) < 1.0f && (distintensity = r_shadow_attentable[(int)(dist * ATTENTABLESIZE)]))
				{
					color4f[0] = ambientcolor[0] * distintensity;
					color4f[1] = ambientcolor[1] * distintensity;
					color4f[2] = ambientcolor[2] * distintensity;
					if (r_refdef.fogenabled)
					{
						float f;
						f = RSurf_FogVertex(vertex3f);
						VectorScale(color4f, f, color4f);
					}
				}
				else
					VectorClear(color4f);
				color4f[3] = 1.0f;
			}
		}
		break;
	case R_SHADOW_RENDERMODE_LIGHT_VERTEX:
		if (VectorLength2(diffusecolor) > .0f)
		{
			for (i = 0, vertex3f = rsurface.batchvertex3f + 3*firstvertex, normal3f = rsurface.batchnormal3f + 3*firstvertex, color4f = rsurface.passcolor4f + 4 * firstvertex;i < numverts;i++, vertex3f += 3, normal3f += 3, color4f += 4)
			{
				Matrix4x4_Transform(&rsurface.entitytolight, vertex3f, v);
				if ((dist = VectorLength(v)) < 1.0f && (distintensity = r_shadow_attentable[(int)(dist * ATTENTABLESIZE)]))
				{
					distintensity = (1.0f - dist) * r_shadow_lightattenuationlinearscale.value / (r_shadow_lightattenuationdividebias.value + dist*dist);
					Matrix4x4_Transform3x3(&rsurface.entitytolight, normal3f, n);
					if ((dot = DotProduct(n, v)) < .0f)
					{
						shadeintensity = -dot / math::sqrtf(VectorLength2(v) * VectorLength2(n));
						color4f[0] = (ambientcolor[0] + shadeintensity * diffusecolor[0]) * distintensity;
						color4f[1] = (ambientcolor[1] + shadeintensity * diffusecolor[1]) * distintensity;
						color4f[2] = (ambientcolor[2] + shadeintensity * diffusecolor[2]) * distintensity;
					}
					else
					{
						color4f[0] = ambientcolor[0] * distintensity;
						color4f[1] = ambientcolor[1] * distintensity;
						color4f[2] = ambientcolor[2] * distintensity;
					}
					if (r_refdef.fogenabled)
					{
						float f = RSurf_FogVertex(vertex3f);
						VectorScale(color4f, f, color4f);
					}
				}
				else
					VectorClear(color4f);
				color4f[3] = 1.0f;
			}
		}
		else
		{
			for (i = 0, vertex3f = rsurface.batchvertex3f + 3*firstvertex, color4f = rsurface.passcolor4f + 4 * firstvertex;i < numverts;i++, vertex3f += 3, color4f += 4)
			{
				Matrix4x4_Transform(&rsurface.entitytolight, vertex3f, v);
				if ((dist = VectorLength(v)) < 1 && (distintensity = r_shadow_attentable[(int)(dist * ATTENTABLESIZE)]))
				{
					distintensity = (1 - dist) * r_shadow_lightattenuationlinearscale.value / (r_shadow_lightattenuationdividebias.value + dist*dist);
					color4f[0] = ambientcolor[0] * distintensity;
					color4f[1] = ambientcolor[1] * distintensity;
					color4f[2] = ambientcolor[2] * distintensity;
					if (r_refdef.fogenabled)
					{
						float f;
						f = RSurf_FogVertex(vertex3f);
						VectorScale(color4f, f, color4f);
					}
				}
				else
					VectorClear(color4f);
				color4f[3] = 1;
			}
		}
		break;
	default:
		break;
	}
}

static void R_Shadow_RenderLighting_VisibleLighting(int texturenumsurfaces, const msurface_t **texturesurfacelist)
{
	// used to display how many times a surface is lit for level design purposes
	RSurf_PrepareVerticesForBatch(BATCHNEED_ARRAY_VERTEX | BATCHNEED_NOGAPS, texturenumsurfaces, texturesurfacelist);
	R_Mesh_PrepareVertices_Generic_Arrays(rsurface.batchnumvertices, rsurface.batchvertex3f, nullptr, nullptr);
	RSurf_DrawBatch();
}

static void R_Shadow_RenderLighting_Light_GLSL(int texturenumsurfaces, const msurface_t **texturesurfacelist, const vec3_t lightcolor, float ambientscale, float diffusescale, float specularscale)
{
	// ARB2 GLSL shader path (GFFX5200, Radeon 9500)
	R_SetupShader_Surface(lightcolor, false, ambientscale, diffusescale, specularscale, RSURFPASS_RTLIGHT, texturenumsurfaces, texturesurfacelist, nullptr, false);
	RSurf_DrawBatch();
}

static void R_Shadow_RenderLighting_Light_Vertex_Pass(int firstvertex, int numvertices, int numtriangles, const int *element3i, vec3_t diffusecolor2, vec3_t ambientcolor2)
{
	int renders;
	int i;
	int stop;
	int newfirstvertex;
	int newlastvertex;
	int newnumtriangles;
	int *newe;
	const int *e;
	float *c;
	int maxtriangles = 1024;
	int newelements[1024*3];
	R_Shadow_RenderLighting_Light_Vertex_Shading(firstvertex, numvertices, diffusecolor2, ambientcolor2);
	for (renders = 0;renders < 4;renders++)
	{
		stop = true;
		newfirstvertex = 0;
		newlastvertex = 0;
		newnumtriangles = 0;
		newe = newelements;
		// due to low fillrate on the cards this vertex lighting path is
		// designed for, we manually cull all triangles that do not
		// contain a lit vertex
		// this builds batches of triangles from multiple surfaces and
		// renders them at once
		for (i = 0, e = element3i;i < numtriangles;i++, e += 3)
		{
			if (VectorLength2(rsurface.passcolor4f + e[0] * 4) + VectorLength2(rsurface.passcolor4f + e[1] * 4) + VectorLength2(rsurface.passcolor4f + e[2] * 4) >= 0.01)
			{
				if (newnumtriangles)
				{
					newfirstvertex = min(newfirstvertex, e[0]);
					newlastvertex  = max(newlastvertex, e[0]);
				}
				else
				{
					newfirstvertex = e[0];
					newlastvertex = e[0];
				}
				newfirstvertex = min(newfirstvertex, e[1]);
				newlastvertex  = max(newlastvertex, e[1]);
				newfirstvertex = min(newfirstvertex, e[2]);
				newlastvertex  = max(newlastvertex, e[2]);
				newe[0] = e[0];
				newe[1] = e[1];
				newe[2] = e[2];
				newnumtriangles++;
				newe += 3;
				if (newnumtriangles >= maxtriangles)
				{
					R_Mesh_Draw(newfirstvertex, newlastvertex - newfirstvertex + 1, 0, newnumtriangles, newelements, nullptr, 0, nullptr, nullptr, 0);
					newnumtriangles = 0;
					newe = newelements;
					stop = false;
				}
			}
		}
		if (newnumtriangles >= 1)
		{
			R_Mesh_Draw(newfirstvertex, newlastvertex - newfirstvertex + 1, 0, newnumtriangles, newelements, nullptr, 0, nullptr, nullptr, 0);
			stop = false;
		}
		// if we couldn't find any lit triangles, exit early
		if (stop)
			break;
		// now reduce the intensity for the next overbright pass
		// we have to clamp to 0 here incase the drivers have improper
		// handling of negative colors
		// (some old drivers even have improper handling of >1 color)
		stop = true;
		for (i = 0, c = rsurface.passcolor4f + 4 * firstvertex;i < numvertices;i++, c += 4)
		{
			if (c[0] > 1.0f || c[1] > 1.0f || c[2] > 1.0f)
			{
				c[0] = max(.0f, c[0] - 1.0f);
				c[1] = max(.0f, c[1] - 1.0f);
				c[2] = max(.0f, c[2] - 1.0f);
				stop = false;
			}
			else
				VectorClear(c);
		}
		// another check...
		if (stop)
			break;
	}
}

static void R_Shadow_RenderLighting_Light_Vertex(int texturenumsurfaces, const msurface_t **texturesurfacelist, const vec3_t lightcolor, float ambientscale, float diffusescale)
{
	// OpenGL 1.1 path (anything)
	float ambientcolorbase[3], diffusecolorbase[3];
	float ambientcolorpants[3], diffusecolorpants[3];
	float ambientcolorshirt[3], diffusecolorshirt[3];
	const float *surfacecolor = rsurface.texture->dlightcolor;
	const float *surfacepants = rsurface.colormap_pantscolor;
	const float *surfaceshirt = rsurface.colormap_shirtcolor;
	rtexture_t *basetexture = rsurface.texture->basetexture;
	rtexture_t *pantstexture = rsurface.texture->pantstexture;
	rtexture_t *shirttexture = rsurface.texture->shirttexture;
	bool dopants = pantstexture && VectorLength2(surfacepants) >= (1.0f / 1048576.0f);
	bool doshirt = shirttexture && VectorLength2(surfaceshirt) >= (1.0f / 1048576.0f);
	ambientscale *= 2 * r_refdef.view.colorscale;
	diffusescale *= 2 * r_refdef.view.colorscale;
	ambientcolorbase[0] = lightcolor[0] * ambientscale * surfacecolor[0];ambientcolorbase[1] = lightcolor[1] * ambientscale * surfacecolor[1];ambientcolorbase[2] = lightcolor[2] * ambientscale * surfacecolor[2];
	diffusecolorbase[0] = lightcolor[0] * diffusescale * surfacecolor[0];diffusecolorbase[1] = lightcolor[1] * diffusescale * surfacecolor[1];diffusecolorbase[2] = lightcolor[2] * diffusescale * surfacecolor[2];
	ambientcolorpants[0] = ambientcolorbase[0] * surfacepants[0];ambientcolorpants[1] = ambientcolorbase[1] * surfacepants[1];ambientcolorpants[2] = ambientcolorbase[2] * surfacepants[2];
	diffusecolorpants[0] = diffusecolorbase[0] * surfacepants[0];diffusecolorpants[1] = diffusecolorbase[1] * surfacepants[1];diffusecolorpants[2] = diffusecolorbase[2] * surfacepants[2];
	ambientcolorshirt[0] = ambientcolorbase[0] * surfaceshirt[0];ambientcolorshirt[1] = ambientcolorbase[1] * surfaceshirt[1];ambientcolorshirt[2] = ambientcolorbase[2] * surfaceshirt[2];
	diffusecolorshirt[0] = diffusecolorbase[0] * surfaceshirt[0];diffusecolorshirt[1] = diffusecolorbase[1] * surfaceshirt[1];diffusecolorshirt[2] = diffusecolorbase[2] * surfaceshirt[2];
	RSurf_PrepareVerticesForBatch(BATCHNEED_ARRAY_VERTEX | (diffusescale > 0 ? BATCHNEED_ARRAY_NORMAL : 0) | BATCHNEED_ARRAY_TEXCOORD | BATCHNEED_NOGAPS, texturenumsurfaces, texturesurfacelist);
	rsurface.passcolor4f = (float *)R_FrameData_Alloc((rsurface.batchfirstvertex + rsurface.batchnumvertices) * sizeof(float[4]));
	R_Mesh_VertexPointer(3, GL_FLOAT, sizeof(float[3]), rsurface.batchvertex3f, rsurface.batchvertex3f_vertexbuffer, rsurface.batchvertex3f_bufferoffset);
	R_Mesh_ColorPointer(4, GL_FLOAT, sizeof(float[4]), rsurface.passcolor4f, 0, 0);
	R_Mesh_TexCoordPointer(0, 2, GL_FLOAT, sizeof(float[2]), rsurface.batchtexcoordtexture2f, rsurface.batchtexcoordtexture2f_vertexbuffer, rsurface.batchtexcoordtexture2f_bufferoffset);
	R_Mesh_TexBind(0, basetexture);
	R_Mesh_TexMatrix(0, &rsurface.texture->currenttexmatrix);
	R_Mesh_TexCombine(0, GL_MODULATE, GL_MODULATE, 1, 1);
	switch(r_shadow_rendermode)
	{
	case R_SHADOW_RENDERMODE_LIGHT_VERTEX3DATTEN:
		R_Mesh_TexBind(1, r_shadow_attenuation3dtexture);
		R_Mesh_TexMatrix(1, &rsurface.entitytoattenuationxyz);
		R_Mesh_TexCombine(1, GL_MODULATE, GL_MODULATE, 1, 1);
		R_Mesh_TexCoordPointer(1, 3, GL_FLOAT, sizeof(float[3]), rsurface.batchvertex3f, rsurface.batchvertex3f_vertexbuffer, rsurface.batchvertex3f_bufferoffset);
		break;
	case R_SHADOW_RENDERMODE_LIGHT_VERTEX2D1DATTEN:
		R_Mesh_TexBind(2, r_shadow_attenuation2dtexture);
		R_Mesh_TexMatrix(2, &rsurface.entitytoattenuationz);
		R_Mesh_TexCombine(2, GL_MODULATE, GL_MODULATE, 1, 1);
		R_Mesh_TexCoordPointer(2, 3, GL_FLOAT, sizeof(float[3]), rsurface.batchvertex3f, rsurface.batchvertex3f_vertexbuffer, rsurface.batchvertex3f_bufferoffset);
		// fall through
	case R_SHADOW_RENDERMODE_LIGHT_VERTEX2DATTEN:
		R_Mesh_TexBind(1, r_shadow_attenuation2dtexture);
		R_Mesh_TexMatrix(1, &rsurface.entitytoattenuationxyz);
		R_Mesh_TexCombine(1, GL_MODULATE, GL_MODULATE, 1, 1);
		R_Mesh_TexCoordPointer(1, 3, GL_FLOAT, sizeof(float[3]), rsurface.batchvertex3f, rsurface.batchvertex3f_vertexbuffer, rsurface.batchvertex3f_bufferoffset);
		break;
	case R_SHADOW_RENDERMODE_LIGHT_VERTEX:
		break;
	default:
		break;
	}
	R_Shadow_RenderLighting_Light_Vertex_Pass(rsurface.batchfirstvertex, rsurface.batchnumvertices, rsurface.batchnumtriangles, rsurface.batchelement3i + 3*rsurface.batchfirsttriangle, diffusecolorbase, ambientcolorbase);
	if (dopants)
	{
		R_Mesh_TexBind(0, pantstexture);
		R_Shadow_RenderLighting_Light_Vertex_Pass(rsurface.batchfirstvertex, rsurface.batchnumvertices, rsurface.batchnumtriangles, rsurface.batchelement3i + 3*rsurface.batchfirsttriangle, diffusecolorpants, ambientcolorpants);
	}
	if (doshirt)
	{
		R_Mesh_TexBind(0, shirttexture);
		R_Shadow_RenderLighting_Light_Vertex_Pass(rsurface.batchfirstvertex, rsurface.batchnumvertices, rsurface.batchnumtriangles, rsurface.batchelement3i + 3*rsurface.batchfirsttriangle, diffusecolorshirt, ambientcolorshirt);
	}
}

extern cvar_t gl_lightmaps;

void R_Shadow_RenderLighting(int texturenumsurfaces, const msurface_t **texturesurfacelist)
{
	//bool negated;
	//float lightcolor[3];
	
	//VectorCopy(rsurface.rtlight->currentcolor, lightcolor);
	vector3f lightColor 	= vector3f(rsurface.rtlight->currentcolor);
	
	float ambientscale 		= rsurface.rtlight->ambientscale	+	rsurface.texture->rtlightambient;
	float diffusescale 		= rsurface.rtlight->diffusescale	*	max(.0f, 1.0f - rsurface.texture->rtlightambient);
	float specularscale 	= rsurface.rtlight->specularscale	*	rsurface.texture->specularscale;
	
	if (!r_shadow_usenormalmap.integer)
	{
		ambientscale 	+= 1.0f * diffusescale;
		diffusescale 	= .0f;
		specularscale 	= .0f;
	}
	
	{
		const float colorVecLength = 
		({
			const vector3f lightColorSquared = lightColor * lightColor;
			lightColorSquared[0] + lightColorSquared[1] + lightColorSquared[2];
		});
		if ((ambientscale + diffusescale) * colorVecLength + specularscale * colorVecLength < (1.0f / 1048576.0f))
			return;
	}
	
	const bool negated = (lightColor[0] + lightColor[1] + lightColor[2] < .0f) && vid.support.ext_blend_subtract;
	if(negated)
	{
		
		lightColor = vector3f() - lightColor;
		//VectorNegate(lightcolor, lightcolor);
		
		GL_BlendEquationSubtract(true);
	}
	RSurf_SetupDepthAndCulling();
	switch (r_shadow_rendermode)
	{
	case R_SHADOW_RENDERMODE_VISIBLELIGHTING:
		GL_DepthTest(!(rsurface.texture->currentmaterialflags & MATERIALFLAG_NODEPTHTEST) && !r_showdisabledepthtest.integer);
		R_Shadow_RenderLighting_VisibleLighting(texturenumsurfaces, texturesurfacelist);
		break;
	case R_SHADOW_RENDERMODE_LIGHT_GLSL:
		R_Shadow_RenderLighting_Light_GLSL(texturenumsurfaces, texturesurfacelist, lightColor, ambientscale, diffusescale, specularscale);
		break;
	case R_SHADOW_RENDERMODE_LIGHT_VERTEX3DATTEN:
	case R_SHADOW_RENDERMODE_LIGHT_VERTEX2D1DATTEN:
	case R_SHADOW_RENDERMODE_LIGHT_VERTEX2DATTEN:
	case R_SHADOW_RENDERMODE_LIGHT_VERTEX:
		R_Shadow_RenderLighting_Light_Vertex(texturenumsurfaces, texturesurfacelist, lightColor, ambientscale, diffusescale);
		break;
	default:
	{
		r_unreachable();
		Con_Printf("R_Shadow_RenderLighting: unknown r_shadow_rendermode %i\n", r_shadow_rendermode);
		break;
	}
	}
	if(negated)
		GL_BlendEquationSubtract(false);
}
