#pragma once


void R_RTLight_Update(rtlight_t *rtlight, int isstatic, matrix4x4_t *matrix, vec3_t color, int style, const char *cubemapname, int shadow, vec_t corona, vec_t coronasizescale, vec_t ambientscale, vec_t diffusescale, vec_t specularscale, int flags)
{
	matrix4x4_t tempmatrix = *matrix;
	Matrix4x4_Scale(&tempmatrix, r_shadow_lightradiusscale.value, 1);

	// if this light has been compiled before, free the associated data
	R_RTLight_Uncompile(rtlight);

	// clear it completely to avoid any lingering data
	memset(rtlight, 0, sizeof(*rtlight));

	// copy the properties
	rtlight->matrix_lighttoworld = tempmatrix;
	Matrix4x4_Invert_Simple(&rtlight->matrix_worldtolight, &tempmatrix);
	Matrix4x4_OriginFromMatrix(&tempmatrix, rtlight->shadoworigin);
	rtlight->radius = Matrix4x4_ScaleFromMatrix(&tempmatrix);
	VectorCopy(color, rtlight->color);
	rtlight->cubemapname[0] = 0;
	if (cubemapname && cubemapname[0])
		strlcpy(rtlight->cubemapname, cubemapname, sizeof(rtlight->cubemapname));
	rtlight->shadow = shadow;
	rtlight->corona = corona;
	rtlight->style = style;
	rtlight->isstatic = isstatic;
	rtlight->coronasizescale = coronasizescale;
	rtlight->ambientscale = ambientscale;
	rtlight->diffusescale = diffusescale;
	rtlight->specularscale = specularscale;
	rtlight->flags = flags;


	rtlight->cullmins[0] = rtlight->shadoworigin[0] - rtlight->radius;
	rtlight->cullmins[1] = rtlight->shadoworigin[1] - rtlight->radius;
	rtlight->cullmins[2] = rtlight->shadoworigin[2] - rtlight->radius;
	rtlight->cullmaxs[0] = rtlight->shadoworigin[0] + rtlight->radius;
	rtlight->cullmaxs[1] = rtlight->shadoworigin[1] + rtlight->radius;
	rtlight->cullmaxs[2] = rtlight->shadoworigin[2] + rtlight->radius;
}
