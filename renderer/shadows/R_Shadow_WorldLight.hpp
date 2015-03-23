#pragma once


static dlight_t *R_Shadow_NewWorldLight()
{
	return (dlight_t *)Mem_ExpandableArray_AllocRecord(&r_shadow_worldlightsarray);
}

static void R_Shadow_UpdateWorldLight(dlight_t *light, vec3_t origin, vec3_t angles, vec3_t color, vec_t radius, vec_t corona, int style, int shadowenable, const char *cubemapname, vec_t coronasizescale, vec_t ambientscale, vec_t diffusescale, vec_t specularscale, int flags)
{
	matrix4x4_t matrix;

	// note that style is no longer validated here, -1 is used for unstyled lights and >= MAX_LIGHTSTYLES is accepted for sake of editing rtlights files that might be out of bounds but perfectly formatted

	// validate parameters
	if (!cubemapname)
		cubemapname = "";

	// copy to light properties
	VectorCopy(origin, light->origin);
	light->angles[0] = angles[0] - 360.0f * math::floorf(angles[0] / 360.0f);
	light->angles[1] = angles[1] - 360.0f * math::floorf(angles[1] / 360.0f);
	light->angles[2] = angles[2] - 360.0f * math::floorf(angles[2] / 360.0f);

	light->color[0] = color[0];
	light->color[1] = color[1];
	light->color[2] = color[2];
	light->radius = max(radius, .0f);
	light->style = style;
	light->shadow = shadowenable;
	light->corona = corona;
	strlcpy(light->cubemapname, cubemapname, sizeof(light->cubemapname));
	light->coronasizescale = coronasizescale;
	light->ambientscale = ambientscale;
	light->diffusescale = diffusescale;
	light->specularscale = specularscale;
	light->flags = flags;

	// update renderable light data
	Matrix4x4_CreateFromQuakeEntity(&matrix, light->origin[0], light->origin[1], light->origin[2], light->angles[0], light->angles[1], light->angles[2], light->radius);
	R_RTLight_Update(&light->rtlight, true, &matrix, light->color, light->style, light->cubemapname[0] ? light->cubemapname : nullptr, light->shadow, light->corona, light->coronasizescale, light->ambientscale, light->diffusescale, light->specularscale, light->flags);
}

static void R_Shadow_FreeWorldLight(dlight_t *light)
{
	if (r_shadow_selectedlight == light)
		r_shadow_selectedlight = nullptr;
	R_RTLight_Uncompile(&light->rtlight);
	Mem_ExpandableArray_FreeRecord(&r_shadow_worldlightsarray, light);
}

void R_Shadow_ClearWorldLights()
{
	size_t lightindex;
	dlight_t *light;
	size_t range = Mem_ExpandableArray_IndexRange(&r_shadow_worldlightsarray); // checked
	for (lightindex = 0;lightindex < range;lightindex++)
	{
		light = (dlight_t *) Mem_ExpandableArray_RecordAtIndex(&r_shadow_worldlightsarray, lightindex);
		if (light)
			R_Shadow_FreeWorldLight(light);
	}
	r_shadow_selectedlight = nullptr;
}
