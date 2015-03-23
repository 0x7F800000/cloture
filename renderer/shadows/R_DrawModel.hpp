#pragma once


void R_DrawModelShadowMaps(int fbo, rtexture_t *depthtexture, rtexture_t *colortexture)
{
	int i;
	float relativethrowdistance, scale, size, radius, nearclip, farclip, bias, dot1, dot2;
	entity_render_t *ent;
	vec3_t relativelightorigin;
	vec3_t relativelightdirection, relativeforward, relativeright;
	vec3_t relativeshadowmins, relativeshadowmaxs;
	vec3_t shadowdir, shadowforward, shadowright, shadoworigin, shadowfocus;
	prvm_vec3_t prvmshadowdir, prvmshadowfocus;
	float m[12];
	matrix4x4_t shadowmatrix, cameramatrix, mvpmatrix, invmvpmatrix, scalematrix, texmatrix;
	r_viewport_t viewport;
	GLuint shadowfbo = 0;
	float clearcolor[4];

	if (!r_shadow_nummodelshadows)
		return;

	switch (r_shadow_shadowmode)
	{
	case R_SHADOW_SHADOWMODE_SHADOWMAP2D:
		break;
	default:
		return;
	}

	r_shadow_fb_fbo = fbo;
	r_shadow_fb_depthtexture = depthtexture;
	r_shadow_fb_colortexture = colortexture;

	R_ResetViewRendering3D(fbo, depthtexture, colortexture);
	R_Shadow_RenderMode_Begin();
	R_Shadow_RenderMode_ActiveLight(nullptr);

	switch (r_shadow_shadowmode)
	{
	case R_SHADOW_SHADOWMODE_SHADOWMAP2D:
		if (!r_shadow_shadowmap2ddepthtexture)
			R_Shadow_MakeShadowMap(0, r_shadow_shadowmapmaxsize);
		shadowfbo = r_shadow_fbo2d;
		r_shadow_shadowmap_texturescale[0] = 1.0f / R_TextureWidth(r_shadow_shadowmap2ddepthtexture);
		r_shadow_shadowmap_texturescale[1] = 1.0f / R_TextureHeight(r_shadow_shadowmap2ddepthtexture);
		r_shadow_rendermode = R_SHADOW_RENDERMODE_SHADOWMAP2D;
		break;
	default:
		break;
	}

	size = 2*r_shadow_shadowmapmaxsize;
	scale = (r_shadow_shadowmapping_precision.value * r_shadows_shadowmapscale.value) / size;
	radius = 0.5f / scale;
	nearclip = -r_shadows_throwdistance.value;
	farclip = r_shadows_throwdistance.value;
	bias = (r_shadows_shadowmapbias.value < 0) ? r_shadow_shadowmapping_bias.value : r_shadows_shadowmapbias.value * r_shadow_shadowmapping_nearclip.value / (2 * r_shadows_throwdistance.value) * (1024.0f / size);

	r_shadow_shadowmap_parameters[0] = size;
	r_shadow_shadowmap_parameters[1] = size;
	r_shadow_shadowmap_parameters[2] = 1.0;
	r_shadow_shadowmap_parameters[3] = bound(0.0f, 1.0f - r_shadows_darken.value, 1.0f);

	Math_atov(r_shadows_throwdirection.string, prvmshadowdir);
	VectorCopy(prvmshadowdir, shadowdir);
	VectorNormalize(shadowdir);
	Math_atov(r_shadows_focus.string, prvmshadowfocus);
	VectorCopy(prvmshadowfocus, shadowfocus);
	VectorM(shadowfocus[0], r_refdef.view.right, shadoworigin);
	VectorMA(shadoworigin, shadowfocus[1], r_refdef.view.up, shadoworigin);
	VectorMA(shadoworigin, -shadowfocus[2], r_refdef.view.forward, shadoworigin);
	VectorAdd(shadoworigin, r_refdef.view.origin, shadoworigin);
	dot1 = DotProduct(r_refdef.view.forward, shadowdir);
	dot2 = DotProduct(r_refdef.view.up, shadowdir);
	if (math::fabsf(dot1) <= math::fabsf(dot2)) 
		VectorMA(r_refdef.view.forward, -dot1, shadowdir, shadowforward);
	else
		VectorMA(r_refdef.view.up, -dot2, shadowdir, shadowforward);
	VectorNormalize(shadowforward);
	VectorM(scale, shadowforward, &m[0]);
	if (shadowfocus[0] || shadowfocus[1] || shadowfocus[2])
		dot1 = 1;
	m[3] = math::fabsf(dot1) * 0.5f - DotProduct(shadoworigin, &m[0]);
	CrossProduct(shadowdir, shadowforward, shadowright);
	VectorM(scale, shadowright, &m[4]);
	m[7] = 0.5f - DotProduct(shadoworigin, &m[4]);
	VectorM(1.0f / (farclip - nearclip), shadowdir, &m[8]);
	m[11] = 0.5f - DotProduct(shadoworigin, &m[8]);
	Matrix4x4_FromArray12FloatD3D(&shadowmatrix, m);
	Matrix4x4_Invert_Full(&cameramatrix, &shadowmatrix);
	R_Viewport_InitOrtho(&viewport, &cameramatrix, 0, 0, size, size, 0, 0, 1, 1, 0, -1, nullptr); 

	VectorMA(shadoworigin, (1.0f - math::fabsf(dot1)) * radius, shadowforward, shadoworigin);

	if (r_shadow_shadowmap2ddepthbuffer)
		R_Mesh_SetRenderTargets(shadowfbo, r_shadow_shadowmap2ddepthbuffer, r_shadow_shadowmap2ddepthtexture, nullptr, nullptr, nullptr);
	else
		R_Mesh_SetRenderTargets(shadowfbo, r_shadow_shadowmap2ddepthtexture, nullptr, nullptr, nullptr, nullptr);
	R_SetupShader_DepthOrShadow(true, r_shadow_shadowmap2ddepthbuffer != nullptr, false); // FIXME test if we have a skeletal model?
	GL_PolygonOffset(r_shadow_shadowmapping_polygonfactor.value, r_shadow_shadowmapping_polygonoffset.value);
	GL_DepthMask(true);
	GL_DepthTest(true);
	R_SetViewport(&viewport);
	GL_Scissor(viewport.x, viewport.y, min(viewport.width + r_shadow_shadowmapborder, 2*r_shadow_shadowmapmaxsize), viewport.height + r_shadow_shadowmapborder);
	Vector4Set(clearcolor, 1.0f, 1.0f, 1.0f, 1.0f);
	// in D3D9 we have to render to a color texture shadowmap
	// in GL we render directly to a depth texture only
	if (r_shadow_shadowmap2ddepthbuffer)
		GL_Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, clearcolor, 1.0f, 0);
	else
		GL_Clear(GL_DEPTH_BUFFER_BIT, clearcolor, 1.0f, 0);
	// render into a slightly restricted region so that the borders of the
	// shadowmap area fade away, rather than streaking across everything
	// outside the usable area
	GL_Scissor(viewport.x + r_shadow_shadowmapborder, viewport.y + r_shadow_shadowmapborder, viewport.width - 2*r_shadow_shadowmapborder, viewport.height - 2*r_shadow_shadowmapborder);

	for (i = 0;i < r_shadow_nummodelshadows;i++)
	{
		ent = r_shadow_modelshadows[i];
		relativethrowdistance = r_shadows_throwdistance.value * Matrix4x4_ScaleFromMatrix(&ent->inversematrix);
		Matrix4x4_Transform(&ent->inversematrix, shadoworigin, relativelightorigin);
		Matrix4x4_Transform3x3(&ent->inversematrix, shadowdir, relativelightdirection);
		Matrix4x4_Transform3x3(&ent->inversematrix, shadowforward, relativeforward);
		Matrix4x4_Transform3x3(&ent->inversematrix, shadowright, relativeright);
		relativeshadowmins[0] = relativelightorigin[0] - r_shadows_throwdistance.value * fabsf(relativelightdirection[0]) - radius * (fabsf(relativeforward[0]) + fabsf(relativeright[0]));
		relativeshadowmins[1] = relativelightorigin[1] - r_shadows_throwdistance.value * fabsf(relativelightdirection[1]) - radius * (fabsf(relativeforward[1]) + fabsf(relativeright[1]));
		relativeshadowmins[2] = relativelightorigin[2] - r_shadows_throwdistance.value * fabsf(relativelightdirection[2]) - radius * (fabsf(relativeforward[2]) + fabsf(relativeright[2]));
		relativeshadowmaxs[0] = relativelightorigin[0] + r_shadows_throwdistance.value * fabsf(relativelightdirection[0]) + radius * (fabsf(relativeforward[0]) + fabsf(relativeright[0]));
		relativeshadowmaxs[1] = relativelightorigin[1] + r_shadows_throwdistance.value * fabsf(relativelightdirection[1]) + radius * (fabsf(relativeforward[1]) + fabsf(relativeright[1]));
		relativeshadowmaxs[2] = relativelightorigin[2] + r_shadows_throwdistance.value * fabsf(relativelightdirection[2]) + radius * (fabsf(relativeforward[2]) + fabsf(relativeright[2]));
		RSurf_ActiveModelEntity(ent, false, false, false);
		ent->model->DrawShadowMap(0, ent, relativelightorigin, relativelightdirection, relativethrowdistance, ent->model->nummodelsurfaces, ent->model->sortedmodelsurfaces, nullptr, relativeshadowmins, relativeshadowmaxs);
		rsurface.entity = nullptr; // used only by R_GetCurrentTexture and RSurf_ActiveWorldEntity/RSurf_ActiveModelEntity
	}


	R_Shadow_RenderMode_End();

	Matrix4x4_Concat(&mvpmatrix, &r_refdef.view.viewport.projectmatrix, &r_refdef.view.viewport.viewmatrix);
	Matrix4x4_Invert_Full(&invmvpmatrix, &mvpmatrix);
	Matrix4x4_CreateScale3(&scalematrix, size, -size, 1); 
	Matrix4x4_AdjustOrigin(&scalematrix, 0, size, -0.5f * bias);
	Matrix4x4_Concat(&texmatrix, &scalematrix, &shadowmatrix);
	Matrix4x4_Concat(&r_shadow_shadowmapmatrix, &texmatrix, &invmvpmatrix);

	switch (vid.renderpath)
	{
	case RENDERPATH_GL11:
	case RENDERPATH_GL13:
	case RENDERPATH_GL20:
#ifndef NO_SOFTWARE_RENDERER
	case RENDERPATH_SOFT:
#endif
	case RENDERPATH_GLES1:
	case RENDERPATH_GLES2:
		break;
#ifndef EXCLUDE_D3D_CODEPATHS
	case RENDERPATH_D3D9:
	case RENDERPATH_D3D10:
	case RENDERPATH_D3D11:
#ifdef MATRIX4x4_OPENGLORIENTATION
		r_shadow_shadowmapmatrix.m[0][0]	*= -1.0f;
		r_shadow_shadowmapmatrix.m[0][1]	*= -1.0f;
		r_shadow_shadowmapmatrix.m[0][2]	*= -1.0f;
		r_shadow_shadowmapmatrix.m[0][3]	*= -1.0f;
#else
		r_shadow_shadowmapmatrix.m[0][0]	*= -1.0f;
		r_shadow_shadowmapmatrix.m[1][0]	*= -1.0f;
		r_shadow_shadowmapmatrix.m[2][0]	*= -1.0f;
		r_shadow_shadowmapmatrix.m[3][0]	*= -1.0f;
#endif
		break;
#endif
	}

	r_shadow_usingshadowmaportho = true;
	switch (r_shadow_shadowmode)
	{
	case R_SHADOW_SHADOWMODE_SHADOWMAP2D:
		r_shadow_usingshadowmap2d = true;
		break;
	default:
		break;
	}
}

void R_DrawModelShadows(int fbo, rtexture_t *depthtexture, rtexture_t *colortexture)
{
	int i;
	float relativethrowdistance;
	entity_render_t *ent;
	vec3_t relativelightorigin;
	vec3_t relativelightdirection;
	vec3_t relativeshadowmins, relativeshadowmaxs;
	vec3_t tmp, shadowdir;
	prvm_vec3_t prvmshadowdir;

	if (!r_shadow_nummodelshadows || (r_shadow_shadowmode != R_SHADOW_SHADOWMODE_STENCIL && r_shadows.integer != 1))
		return;

	r_shadow_fb_fbo = fbo;
	r_shadow_fb_depthtexture = depthtexture;
	r_shadow_fb_colortexture = colortexture;

	R_ResetViewRendering3D(fbo, depthtexture, colortexture);

	R_Shadow_RenderMode_Begin();
	R_Shadow_RenderMode_ActiveLight(nullptr);

	r_shadow_lightscissor =
	{
			r_refdef.view.x,
			vid.height - r_refdef.view.y - r_refdef.view.height,
			r_refdef.view.width,
			r_refdef.view.height
	};
	R_Shadow_RenderMode_StencilShadowVolumes(false);

	// get shadow dir
	if (r_shadows.integer == 2)
	{
		Math_atov(r_shadows_throwdirection.string, prvmshadowdir);
		VectorCopy(prvmshadowdir, shadowdir);
		VectorNormalize(shadowdir);
	}

	R_Shadow_ClearStencil();

	for (i = 0;i < r_shadow_nummodelshadows;i++)
	{
		ent = r_shadow_modelshadows[i];

		// cast shadows from anything of the map (submodels are optional)
		relativethrowdistance = r_shadows_throwdistance.value * Matrix4x4_ScaleFromMatrix(&ent->inversematrix);
		VectorSet(relativeshadowmins, -relativethrowdistance, -relativethrowdistance, -relativethrowdistance);
		VectorSet(relativeshadowmaxs, relativethrowdistance, relativethrowdistance, relativethrowdistance);
		if (r_shadows.integer == 2) // 2: simpler mode, throw shadows always in same direction
			Matrix4x4_Transform3x3(&ent->inversematrix, shadowdir, relativelightdirection);
		else
		{
			if(ent->entitynumber != 0)
			{
				if(ent->entitynumber >= MAX_EDICTS) // csqc entity
				{
					// FIXME handle this
					VectorNegate(ent->modellight_lightdir, relativelightdirection);
				}
				else
				{
					// networked entity - might be attached in some way (then we should use the parent's light direction, to not tear apart attached entities)
					int entnum, entnum2, recursion;
					entnum = entnum2 = ent->entitynumber;
					for(recursion = 32; recursion > 0; --recursion)
					{
						entnum2 = cl.entities[entnum].state_current.tagentity;
						if(entnum2 >= 1 && entnum2 < cl.num_entities && cl.entities_active[entnum2])
							entnum = entnum2;
						else
							break;
					}
					if(recursion && recursion != 32) // if we followed a valid non-empty attachment chain
					{
						VectorNegate(cl.entities[entnum].render.modellight_lightdir, relativelightdirection);
						// transform into modelspace of OUR entity
						Matrix4x4_Transform3x3(&cl.entities[entnum].render.matrix, relativelightdirection, tmp);
						Matrix4x4_Transform3x3(&ent->inversematrix, tmp, relativelightdirection);
					}
					else
						VectorNegate(ent->modellight_lightdir, relativelightdirection);
				}
			}
			else
				VectorNegate(ent->modellight_lightdir, relativelightdirection);
		}

		VectorScale(relativelightdirection, -relativethrowdistance, relativelightorigin);
		RSurf_ActiveModelEntity(ent, false, false, false);
		ent->model->DrawShadowVolume(ent, relativelightorigin, relativelightdirection, relativethrowdistance, ent->model->nummodelsurfaces, ent->model->sortedmodelsurfaces, relativeshadowmins, relativeshadowmaxs);
		rsurface.entity = nullptr; // used only by R_GetCurrentTexture and RSurf_ActiveWorldEntity/RSurf_ActiveModelEntity
	}

	// not really the right mode, but this will disable any silly stencil features
	R_Shadow_RenderMode_End();

	R_ResetViewRendering2D(fbo, depthtexture, colortexture);

	// set up a darkening blend on shadowed areas
	GL_BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	GL_Color(.0f, .0f, .0f, r_shadows_darken.value);

	R_SetStencil(true, 255, GL_KEEP, GL_KEEP, GL_KEEP, GL_NOTEQUAL, 128, 255);

	// apply the blend to the shadowed areas
	R_Mesh_PrepareVertices_Generic_Arrays(4, r_screenvertex3f, nullptr, nullptr);
	R_SetupShader_Generic_NoTexture(false, true);
	R_Mesh_Draw(0, 4, 0, 2, polygonelement3i, nullptr, 0, polygonelement3s, nullptr, 0);

	// restore the viewport
	R_SetViewport(&r_refdef.view.viewport);

}
