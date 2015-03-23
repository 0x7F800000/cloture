#pragma once


static void R_Shadow_SelectLight(dlight_t *light)
{
	if (r_shadow_selectedlight)
		r_shadow_selectedlight->selected = false;
	r_shadow_selectedlight = light;
	if (r_shadow_selectedlight)
		r_shadow_selectedlight->selected = true;
}

static void R_Shadow_DrawCursor_TransparentCallback(const entity_render_t *ent, const rtlight_t *rtlight, int numsurfaces, int *surfacelist)
{
	// this is never batched (there can be only one)
	float vertex3f[12];
	R_CalcSprite_Vertex3f(vertex3f, r_editlights_cursorlocation, r_refdef.view.right, r_refdef.view.up, EDLIGHTSPRSIZE, -EDLIGHTSPRSIZE, -EDLIGHTSPRSIZE, EDLIGHTSPRSIZE);
	RSurf_ActiveCustomEntity(
			reinterpret_cast<const matrix4x4_t*>(&identitymatrix),
			reinterpret_cast<const matrix4x4_t*>(&identitymatrix), 0, 0, 1, 1, 1, 1, 4,
	vertex3f, spritetexcoord2f, nullptr, nullptr, nullptr,
	nullptr, 2, polygonelement3i, polygonelement3s,
	false, false);
	R_DrawCustomSurface(r_editlights_sprcursor,
			reinterpret_cast<const matrix4x4_t*>(&identitymatrix),
			MATERIALFLAG_NODEPTHTEST | MATERIALFLAG_ALPHA | MATERIALFLAG_BLENDED | MATERIALFLAG_FULLBRIGHT |
			MATERIALFLAG_NOCULLFACE, 0, 4, 0, 2, false, false);
}

static void R_Shadow_DrawLightSprite_TransparentCallback(const entity_render_t *ent, const rtlight_t *rtlight, int numsurfaces, int *surfacelist)
{
	float intensity;
	float s;
	vec3_t spritecolor;
	skinframe_t *skinframe;
	float vertex3f[12];

	// this is never batched (due to the ent parameter changing every time)
	// so numsurfaces == 1 and surfacelist[0] == lightnumber
	const dlight_t *light = (dlight_t *)ent;
	s = EDLIGHTSPRSIZE;

	R_CalcSprite_Vertex3f(vertex3f, light->origin, r_refdef.view.right, r_refdef.view.up, s, -s, -s, s);

	intensity = 0.5f;
	VectorScale(light->color, intensity, spritecolor);
	if (VectorLength(spritecolor) < 0.1732f)
		VectorSet(spritecolor, 0.1f, 0.1f, 0.1f);
	if (VectorLength(spritecolor) > 1.0f)
		VectorNormalize(spritecolor);

	// draw light sprite
	if (light->cubemapname[0] && !light->shadow)
		skinframe = r_editlights_sprcubemapnoshadowlight;
	else if (light->cubemapname[0])
		skinframe = r_editlights_sprcubemaplight;
	else if (!light->shadow)
		skinframe = r_editlights_sprnoshadowlight;
	else
		skinframe = r_editlights_sprlight;

	RSurf_ActiveCustomEntity(reinterpret_cast<const matrix4x4_t*>(&identitymatrix),
			reinterpret_cast<const matrix4x4_t*>(&identitymatrix), 0, 0, spritecolor[0], spritecolor[1], spritecolor[2], 1, 4, vertex3f, spritetexcoord2f, nullptr, nullptr, nullptr, nullptr, 2, polygonelement3i, polygonelement3s, false, false);
	R_DrawCustomSurface(skinframe, reinterpret_cast<const matrix4x4_t*>(&identitymatrix), MATERIALFLAG_ALPHA | MATERIALFLAG_BLENDED | MATERIALFLAG_FULLBRIGHT | MATERIALFLAG_NOCULLFACE, 0, 4, 0, 2, false, false);

	// draw selection sprite if light is selected
	if (light->selected)
	{
		RSurf_ActiveCustomEntity(reinterpret_cast<const matrix4x4_t*>(&identitymatrix),
				reinterpret_cast<const matrix4x4_t*>(&identitymatrix), 0, 0, 1, 1, 1, 1, 4, vertex3f, spritetexcoord2f, nullptr, nullptr, nullptr, nullptr, 2, polygonelement3i, polygonelement3s, false, false);
		R_DrawCustomSurface(r_editlights_sprselection,
				reinterpret_cast<const matrix4x4_t*>(&identitymatrix), MATERIALFLAG_ALPHA | MATERIALFLAG_BLENDED | MATERIALFLAG_FULLBRIGHT | MATERIALFLAG_NOCULLFACE, 0, 4, 0, 2, false, false);
		// VorteX todo: add normalmode/realtime mode light overlay sprites?
	}
}

void R_Shadow_DrawLightSprites()
{
	size_t lightindex;
	dlight_t *light;
	size_t range = Mem_ExpandableArray_IndexRange(&r_shadow_worldlightsarray); // checked
	for (lightindex = 0;lightindex < range;lightindex++)
	{
		light = (dlight_t *) Mem_ExpandableArray_RecordAtIndex(&r_shadow_worldlightsarray, lightindex);
		if (light)
			R_MeshQueue_AddTransparent(TRANSPARENTSORT_DISTANCE, light->origin, R_Shadow_DrawLightSprite_TransparentCallback, (entity_render_t *)light, 5, &light->rtlight);
	}
	if (!r_editlights_lockcursor)
		R_MeshQueue_AddTransparent(TRANSPARENTSORT_DISTANCE, r_editlights_cursorlocation, R_Shadow_DrawCursor_TransparentCallback, nullptr, 0, nullptr);
}

int R_Shadow_GetRTLightInfo(unsigned int lightindex, float *origin, float *radius, float *color)
{
	unsigned int range;
	dlight_t *light;
	rtlight_t *rtlight;
	range = (unsigned int)Mem_ExpandableArray_IndexRange(&r_shadow_worldlightsarray);
	if (lightindex >= range)
		return -1;
	light = (dlight_t *) Mem_ExpandableArray_RecordAtIndex(&r_shadow_worldlightsarray, lightindex);
	if (!light)
		return 0;
	rtlight = &light->rtlight;

	VectorCopy(rtlight->shadoworigin, origin);
	*radius = rtlight->radius;
	VectorCopy(rtlight->color, color);
	return 1;
}

static void R_Shadow_SelectLightInView()
{
	float bestrating, rating, temp[3];
	dlight_t *best;
	size_t lightindex;
	dlight_t *light;
	size_t range = Mem_ExpandableArray_IndexRange(&r_shadow_worldlightsarray); // checked
	best = nullptr;
	bestrating = 0;

	if (r_editlights_lockcursor)
		return;
	for (lightindex = 0;lightindex < range;lightindex++)
	{
		light = (dlight_t *) Mem_ExpandableArray_RecordAtIndex(&r_shadow_worldlightsarray, lightindex);
		if (!light)
			continue;
		VectorSubtract(light->origin, r_refdef.view.origin, temp);
		rating = (DotProduct(temp, r_refdef.view.forward) / sqrt(DotProduct(temp, temp)));
		if (rating >= 0.95f)
		{
			rating /= (1.0f + 0.0625f * math::sqrtf(DotProduct(temp, temp)));
			if (bestrating < rating && CL_TraceLine(light->origin, r_refdef.view.origin, MOVE_NOMONSTERS, nullptr, SUPERCONTENTS_SOLID, collision_extendmovelength.value, true, false, nullptr, false, true).fraction == 1.0f)
			{
				bestrating = rating;
				best = light;
			}
		}
	}
	R_Shadow_SelectLight(best);
}

void R_Shadow_LoadWorldLights()
{
	int n, a, style, shadow, flags;
	char tempchar, *lightsstring, *s, *t, name[MAX_QPATH], cubemapname[MAX_QPATH];
	float origin[3], radius, color[3], angles[3], corona, coronasizescale, ambientscale, diffusescale, specularscale;
	if (cl.worldmodel == nullptr)
	{
		Con_Print("No map loaded.\n");
		return;
	}
	dpsnprintf(name, sizeof(name), "%s.rtlights", cl.worldnamenoextension);
	lightsstring = (char *)FS_LoadFile(name, tempmempool, false, nullptr);
	if (lightsstring)
	{
		s = lightsstring;
		n = 0;
		while (*s)
		{

			t = s;
			while (*s && *s != '\n' && *s != '\r')
				s++;
			if (!*s)
				break;
			tempchar = *s;
			shadow = true;
			// check for modifier flags
			if (*t == '!')
			{
				shadow = false;
				t++;
			}
			*s = 0;
#if _MSC_VER >= 1400
#define sscanf sscanf_s
#endif
			cubemapname[sizeof(cubemapname)-1] = 0;
#if MAX_QPATH != 128
#error update this code if MAX_QPATH changes
#endif
			a = sscanf(t, "%f %f %f %f %f %f %f %d %127s %f %f %f %f %f %f %f %f %i", &origin[0], &origin[1], &origin[2], &radius, &color[0], &color[1], &color[2], &style, cubemapname
#if _MSC_VER >= 1400
, sizeof(cubemapname)
#endif
, &corona, &angles[0], &angles[1], &angles[2], &coronasizescale, &ambientscale, &diffusescale, &specularscale, &flags);
			*s = tempchar;
			if (a < 18)
				flags = LIGHTFLAG_REALTIMEMODE;
			if (a < 17)
				specularscale = 1;
			if (a < 16)
				diffusescale = 1;
			if (a < 15)
				ambientscale = 0;
			if (a < 14)
				coronasizescale = 0.25f;
			if (a < 13)
				VectorClear(angles);
			if (a < 10)
				corona = 0;
			if (a < 9 || !strcmp(cubemapname, "\"\""))
				cubemapname[0] = 0;
			// remove quotes on cubemapname
			if (cubemapname[0] == '"' && cubemapname[strlen(cubemapname) - 1] == '"')
			{
				size_t namelen;
				namelen = strlen(cubemapname) - 2;
				memmove(cubemapname, cubemapname + 1, namelen);
				cubemapname[namelen] = '\0';
			}
			if (a < 8)
			{
				Con_Printf("found %d parameters on line %i, should be 8 or more parameters (origin[0] origin[1] origin[2] radius color[0] color[1] color[2] style \"cubemapname\" corona angles[0] angles[1] angles[2] coronasizescale ambientscale diffusescale specularscale flags)\n", a, n + 1);
				break;
			}
			R_Shadow_UpdateWorldLight(R_Shadow_NewWorldLight(), origin, angles, color, radius, corona, style, shadow, cubemapname, coronasizescale, ambientscale, diffusescale, specularscale, flags);
			if (*s == '\r')
				s++;
			if (*s == '\n')
				s++;
			n++;
		}
		if (*s)
			Con_Printf("invalid rtlights file \"%s\"\n", name);
		Mem_Free(lightsstring);
	}
}

void R_Shadow_SaveWorldLights()
{
	size_t lightindex;
	dlight_t *light;
	size_t bufchars, bufmaxchars;
	char *buf, *oldbuf;
	char name[MAX_QPATH];
	char line[MAX_INPUTLINE];
	size_t range = Mem_ExpandableArray_IndexRange(&r_shadow_worldlightsarray); // checked, assuming the dpsnprintf mess doesn't screw it up...
	// I hate lines which are 3 times my screen size :( --blub
	if (!range)
		return;
	if (cl.worldmodel == nullptr)
	{
		Con_Print("No map loaded.\n");
		return;
	}
	dpsnprintf(name, sizeof(name), "%s.rtlights", cl.worldnamenoextension);
	bufchars = bufmaxchars = 0;
	buf = nullptr;
	for (lightindex = 0;lightindex < range;lightindex++)
	{
		light = (dlight_t *) Mem_ExpandableArray_RecordAtIndex(&r_shadow_worldlightsarray, lightindex);
		if (!light)
			continue;
		if (light->coronasizescale != 0.25f || light->ambientscale != 0 || light->diffusescale != 1 || light->specularscale != 1 || light->flags != LIGHTFLAG_REALTIMEMODE)
			dpsnprintf(line, sizeof(line), "%s%f %f %f %f %f %f %f %d \"%s\" %f %f %f %f %f %f %f %f %i\n", light->shadow ? "" : "!", light->origin[0], light->origin[1], light->origin[2], light->radius, light->color[0], light->color[1], light->color[2], light->style, light->cubemapname, light->corona, light->angles[0], light->angles[1], light->angles[2], light->coronasizescale, light->ambientscale, light->diffusescale, light->specularscale, light->flags);
		else if (light->cubemapname[0] || light->corona || light->angles[0] || light->angles[1] || light->angles[2])
			dpsnprintf(line, sizeof(line), "%s%f %f %f %f %f %f %f %d \"%s\" %f %f %f %f\n", light->shadow ? "" : "!", light->origin[0], light->origin[1], light->origin[2], light->radius, light->color[0], light->color[1], light->color[2], light->style, light->cubemapname, light->corona, light->angles[0], light->angles[1], light->angles[2]);
		else
			dpsnprintf(line, sizeof(line), "%s%f %f %f %f %f %f %f %d\n", light->shadow ? "" : "!", light->origin[0], light->origin[1], light->origin[2], light->radius, light->color[0], light->color[1], light->color[2], light->style);
		if (bufchars + strlen(line) > bufmaxchars)
		{
			bufmaxchars = bufchars + strlen(line) + 2048;
			oldbuf = buf;
			buf = (char *)Mem_Alloc(tempmempool, bufmaxchars);
			if (oldbuf)
			{
				if (bufchars)
					memcpy(buf, oldbuf, bufchars);
				Mem_Free(oldbuf);
			}
		}
		if (strlen(line))
		{
			memcpy(buf + bufchars, line, strlen(line));
			bufchars += strlen(line);
		}
	}
	if (bufchars)
		FS_WriteFile(name, buf, (fs_offset_t)bufchars);
	if (buf)
		Mem_Free(buf);
}

void R_Shadow_LoadLightsFile()
{
	int n, a, style;
	char tempchar, *lightsstring, *s, *t, name[MAX_QPATH];
	float origin[3], radius, color[3], subtract, spotdir[3], spotcone, falloff, distbias;
	if (cl.worldmodel == nullptr)
	{
		Con_Print("No map loaded.\n");
		return;
	}
	dpsnprintf(name, sizeof(name), "%s.lights", cl.worldnamenoextension);
	lightsstring = (char *)FS_LoadFile(name, tempmempool, false, nullptr);
	if (lightsstring)
	{
		s = lightsstring;
		n = 0;
		while (*s)
		{
			t = s;
			while (*s && *s != '\n' && *s != '\r')
				s++;
			if (!*s)
				break;
			tempchar = *s;
			*s = 0;
			a = sscanf(t, "%f %f %f %f %f %f %f %f %f %f %f %f %f %d", &origin[0], &origin[1], &origin[2], &falloff, &color[0], &color[1], &color[2], &subtract, &spotdir[0], &spotdir[1], &spotdir[2], &spotcone, &distbias, &style);
			*s = tempchar;
			if (a < 14)
			{
				Con_Printf("invalid lights file, found %d parameters on line %i, should be 14 parameters (origin[0] origin[1] origin[2] falloff light[0] light[1] light[2] subtract spotdir[0] spotdir[1] spotdir[2] spotcone distancebias style)\n", a, n + 1);
				break;
			}
			radius = math::sqrtf(DotProduct(color, color) / (falloff * falloff * 8192.0f * 8192.0f));
			radius = bound(15, radius, 4096);
			VectorScale(color, (2.0f / (8388608.0f)), color);
			R_Shadow_UpdateWorldLight(R_Shadow_NewWorldLight(), origin, vec3_origin, color, radius, 0, style, true, nullptr, 0.25, 0, 1, 1, LIGHTFLAG_REALTIMEMODE);
			if (*s == '\r')
				s++;
			if (*s == '\n')
				s++;
			n++;
		}
		if (*s)
			Con_Printf("invalid lights file \"%s\"\n", name);
		Mem_Free(lightsstring);
	}
}

// tyrlite/hmap2 light types in the delay field
typedef enum lighttype_e {LIGHTTYPE_MINUSX, LIGHTTYPE_RECIPX, LIGHTTYPE_RECIPXX, LIGHTTYPE_NONE, LIGHTTYPE_SUN, LIGHTTYPE_MINUSXX} lighttype_t;

void R_Shadow_LoadWorldLightsFromMap_LightArghliteTyrlite()
{
	int entnum;
	int style;
	int islight;
	int skin;
	int pflags;
	//int effects;
	int type;
	int n;
	char *entfiledata;
	const char *data;
	float origin[3], angles[3], radius, color[3], light[4], fadescale, lightscale, originhack[3], overridecolor[3], vec[4];
	char key[256], value[MAX_INPUTLINE];
	char vabuf[1024];

	if (cl.worldmodel == nullptr)
	{
		Con_Print("No map loaded.\n");
		return;
	}
	// try to load a .ent file first
	dpsnprintf(key, sizeof(key), "%s.ent", cl.worldnamenoextension);
	data = entfiledata = (char *)FS_LoadFile(key, tempmempool, true, nullptr);
	// and if that is not found, fall back to the bsp file entity string
	if (!data)
		data = cl.worldmodel->brush.entities;
	if (!data)
		return;
	for (entnum = 0;COM_ParseToken_Simple(&data, false, false, true) && com_token[0] == '{';entnum++)
	{
		type = LIGHTTYPE_MINUSX;
		origin[0] = origin[1] = origin[2] = 0;
		originhack[0] = originhack[1] = originhack[2] = 0;
		angles[0] = angles[1] = angles[2] = 0;
		color[0] = color[1] = color[2] = 1;
		light[0] = light[1] = light[2] = 1;light[3] = 300;
		overridecolor[0] = overridecolor[1] = overridecolor[2] = 1;
		fadescale = 1;
		lightscale = 1;
		style = 0;
		skin = 0;
		pflags = 0;
		//effects = 0;
		islight = false;
		while (1)
		{
			if (!COM_ParseToken_Simple(&data, false, false, true))
				break; // error
			if (com_token[0] == '}')
				break; // end of entity
			if (com_token[0] == '_')
				strlcpy(key, com_token + 1, sizeof(key));
			else
				strlcpy(key, com_token, sizeof(key));
			while (key[strlen(key)-1] == ' ') // remove trailing spaces
				key[strlen(key)-1] = 0;
			if (!COM_ParseToken_Simple(&data, false, false, true))
				break; // error
			strlcpy(value, com_token, sizeof(value));

			// now that we have the key pair worked out...
			if (!strcmp("light", key))
			{
				n = sscanf(value, "%f %f %f %f", &vec[0], &vec[1], &vec[2], &vec[3]);
				if (n == 1)
				{
					// quake
					light[0] = vec[0] * (1.0f / 256.0f);
					light[1] = vec[0] * (1.0f / 256.0f);
					light[2] = vec[0] * (1.0f / 256.0f);
					light[3] = vec[0];
				}
				else if (n == 4)
				{
					// halflife
					light[0] = vec[0] * (1.0f / 255.0f);
					light[1] = vec[1] * (1.0f / 255.0f);
					light[2] = vec[2] * (1.0f / 255.0f);
					light[3] = vec[3];
				}
			}
			else if (!strcmp("delay", key))
				type = atoi(value);
			else if (!strcmp("origin", key))
				sscanf(value, "%f %f %f", &origin[0], &origin[1], &origin[2]);
			else if (!strcmp("angle", key))
				angles[0] = 0, angles[1] = atof(value), angles[2] = 0;
			else if (!strcmp("angles", key))
				sscanf(value, "%f %f %f", &angles[0], &angles[1], &angles[2]);
			else if (!strcmp("color", key))
				sscanf(value, "%f %f %f", &color[0], &color[1], &color[2]);
			else if (!strcmp("wait", key))
				fadescale = atof(value);
			else if (!strcmp("classname", key))
			{
				if (!strncmp(value, "light", 5))
				{
					islight = true;
					if (!strcmp(value, "light_fluoro"))
					{
						originhack[0] = 0;
						originhack[1] = 0;
						originhack[2] = 0;
						overridecolor[0] = 1;
						overridecolor[1] = 1;
						overridecolor[2] = 1;
					}
					if (!strcmp(value, "light_fluorospark"))
					{
						originhack[0] = 0;
						originhack[1] = 0;
						originhack[2] = 0;
						overridecolor[0] = 1;
						overridecolor[1] = 1;
						overridecolor[2] = 1;
					}
					if (!strcmp(value, "light_globe"))
					{
						originhack[0] = 0;
						originhack[1] = 0;
						originhack[2] = 0;
						overridecolor[0] = 1;
						overridecolor[1] = 0.8;
						overridecolor[2] = 0.4;
					}
					if (!strcmp(value, "light_flame_large_yellow"))
					{
						originhack[0] = 0;
						originhack[1] = 0;
						originhack[2] = 0;
						overridecolor[0] = 1;
						overridecolor[1] = 0.5;
						overridecolor[2] = 0.1;
					}
					if (!strcmp(value, "light_flame_small_yellow"))
					{
						originhack[0] = 0;
						originhack[1] = 0;
						originhack[2] = 0;
						overridecolor[0] = 1;
						overridecolor[1] = 0.5;
						overridecolor[2] = 0.1;
					}
					if (!strcmp(value, "light_torch_small_white"))
					{
						originhack[0] = 0;
						originhack[1] = 0;
						originhack[2] = 0;
						overridecolor[0] = 1;
						overridecolor[1] = 0.5;
						overridecolor[2] = 0.1;
					}
					if (!strcmp(value, "light_torch_small_walltorch"))
					{
						originhack[0] = 0;
						originhack[1] = 0;
						originhack[2] = 0;
						overridecolor[0] = 1;
						overridecolor[1] = 0.5;
						overridecolor[2] = 0.1;
					}
				}
			}
			else if (!strcmp("style", key))
				style = atoi(value);
			else if (!strcmp("skin", key))
				skin = (int)atof(value);
			else if (!strcmp("pflags", key))
				pflags = (int)atof(value);
			//else if (!strcmp("effects", key))
			//	effects = (int)atof(value);
			else if (cl.worldmodel->type == mod_brushq3)
			{
				if (!strcmp("scale", key))
					lightscale = atof(value);
				if (!strcmp("fade", key))
					fadescale = atof(value);
			}
		}
		if (!islight)
			continue;
		if (lightscale <= 0)
			lightscale = 1;
		if (fadescale <= 0)
			fadescale = 1;
		if (color[0] == color[1] && color[0] == color[2])
		{
			color[0] *= overridecolor[0];
			color[1] *= overridecolor[1];
			color[2] *= overridecolor[2];
		}
		radius = light[3] * r_editlights_quakelightsizescale.value * lightscale / fadescale;
		color[0] = color[0] * light[0];
		color[1] = color[1] * light[1];
		color[2] = color[2] * light[2];
		switch (type)
		{
		case LIGHTTYPE_MINUSX:
			break;
		case LIGHTTYPE_RECIPX:
			radius *= 2;
			VectorScale(color, (1.0f / 16.0f), color);
			break;
		case LIGHTTYPE_RECIPXX:
			radius *= 2;
			VectorScale(color, (1.0f / 16.0f), color);
			break;
		default:
		case LIGHTTYPE_NONE:
			break;
		case LIGHTTYPE_SUN:
			break;
		case LIGHTTYPE_MINUSXX:
			break;
		}
		VectorAdd(origin, originhack, origin);
		if (radius >= 1)
			R_Shadow_UpdateWorldLight(R_Shadow_NewWorldLight(), origin, angles, color, radius, (pflags & PFLAGS_CORONA) != 0, style, (pflags & PFLAGS_NOSHADOW) == 0, skin >= 16 ? va(vabuf, sizeof(vabuf), "cubemaps/%i", skin) : nullptr, 0.25, 0, 1, 1, LIGHTFLAG_REALTIMEMODE);
	}
	if (entfiledata)
		Mem_Free(entfiledata);
}


static void R_Shadow_SetCursorLocationForView()
{
	vec_t dist, push;
	vec3_t dest, endpos;
	trace_t trace;
	VectorMA(r_refdef.view.origin, r_editlights_cursordistance.value, r_refdef.view.forward, dest);
	trace = CL_TraceLine(r_refdef.view.origin, dest, MOVE_NOMONSTERS, nullptr, SUPERCONTENTS_SOLID, collision_extendmovelength.value, true, false, nullptr, false, true);
	if (trace.fraction < 1)
	{
		dist = trace.fraction * r_editlights_cursordistance.value;
		push = r_editlights_cursorpushback.value;
		if (push > dist)
			push = dist;
		push = -push;
		VectorMA(trace.endpos, push, r_refdef.view.forward, endpos);
		VectorMA(endpos, r_editlights_cursorpushoff.value, trace.plane.normal, endpos);
	}
	else
	{
		VectorClear( endpos );
	}
	r_editlights_cursorlocation[0] = floor(endpos[0] / r_editlights_cursorgrid.value + 0.5f) * r_editlights_cursorgrid.value;
	r_editlights_cursorlocation[1] = floor(endpos[1] / r_editlights_cursorgrid.value + 0.5f) * r_editlights_cursorgrid.value;
	r_editlights_cursorlocation[2] = floor(endpos[2] / r_editlights_cursorgrid.value + 0.5f) * r_editlights_cursorgrid.value;
}

void R_Shadow_UpdateWorldLightSelection()
{
	if (unlikely(r_editlights.integer != 0))
	{
		R_Shadow_SetCursorLocationForView();
		R_Shadow_SelectLightInView();
	}
	else
		R_Shadow_SelectLight(nullptr);
}

static void R_Shadow_EditLights_Clear_f()
{
	R_Shadow_ClearWorldLights();
}

void R_Shadow_EditLights_Reload_f()
{
	if (!cl.worldmodel)
		return;
	strlcpy(r_shadow_mapname, cl.worldname, sizeof(r_shadow_mapname));
	R_Shadow_ClearWorldLights();
	R_Shadow_LoadWorldLights();
	if (!Mem_ExpandableArray_IndexRange(&r_shadow_worldlightsarray))
	{
		R_Shadow_LoadLightsFile();
		if (!Mem_ExpandableArray_IndexRange(&r_shadow_worldlightsarray))
			R_Shadow_LoadWorldLightsFromMap_LightArghliteTyrlite();
	}
}

static void R_Shadow_EditLights_Save_f()
{
	if (!cl.worldmodel)
		return;
	R_Shadow_SaveWorldLights();
}

static void R_Shadow_EditLights_ImportLightEntitiesFromMap_f()
{
	R_Shadow_ClearWorldLights();
	R_Shadow_LoadWorldLightsFromMap_LightArghliteTyrlite();
}

static void R_Shadow_EditLights_ImportLightsFile_f()
{
	R_Shadow_ClearWorldLights();
	R_Shadow_LoadLightsFile();
}

static void R_Shadow_EditLights_Spawn_f()
{
	vec3_t color;
	if (!r_editlights.integer)
	{
		Con_Print("Cannot spawn light when not in editing mode.  Set r_editlights to 1.\n");
		return;
	}
	if (Cmd_Argc() != 1)
	{
		Con_Print("r_editlights_spawn does not take parameters\n");
		return;
	}
	color[0] = color[1] = color[2] = 1.0f;
	R_Shadow_UpdateWorldLight(R_Shadow_NewWorldLight(), r_editlights_cursorlocation, vec3_origin, color, 200, 0, 0, true, nullptr, 0.25, 0, 1, 1, LIGHTFLAG_REALTIMEMODE);
}

static void R_Shadow_EditLights_Edit_f()
{
	vec3_t origin, angles, color;
	vec_t radius, corona, coronasizescale, ambientscale, diffusescale, specularscale;
	int style, shadows, flags, normalmode, realtimemode;
	char cubemapname[MAX_INPUTLINE];
	if (!r_editlights.integer)
	{
		Con_Print("Cannot spawn light when not in editing mode.  Set r_editlights to 1.\n");
		return;
	}
	if (!r_shadow_selectedlight)
	{
		Con_Print("No selected light.\n");
		return;
	}
	VectorCopy(r_shadow_selectedlight->origin, origin);
	VectorCopy(r_shadow_selectedlight->angles, angles);
	VectorCopy(r_shadow_selectedlight->color, color);
	radius = r_shadow_selectedlight->radius;
	style = r_shadow_selectedlight->style;
	if (r_shadow_selectedlight->cubemapname)
		strlcpy(cubemapname, r_shadow_selectedlight->cubemapname, sizeof(cubemapname));
	else
		cubemapname[0] = 0;
	shadows = r_shadow_selectedlight->shadow;
	corona = r_shadow_selectedlight->corona;
	coronasizescale = r_shadow_selectedlight->coronasizescale;
	ambientscale = r_shadow_selectedlight->ambientscale;
	diffusescale = r_shadow_selectedlight->diffusescale;
	specularscale = r_shadow_selectedlight->specularscale;
	flags = r_shadow_selectedlight->flags;
	normalmode = (flags & LIGHTFLAG_NORMALMODE) != 0;
	realtimemode = (flags & LIGHTFLAG_REALTIMEMODE) != 0;
	if (!strcmp(Cmd_Argv(1), "origin"))
	{
		if (Cmd_Argc() != 5)
		{
			Con_Printf("usage: r_editlights_edit %s x y z\n", Cmd_Argv(1));
			return;
		}
		origin[0] = atof(Cmd_Argv(2));
		origin[1] = atof(Cmd_Argv(3));
		origin[2] = atof(Cmd_Argv(4));
	}
	else if (!strcmp(Cmd_Argv(1), "originscale"))
	{
		if (Cmd_Argc() != 5)
		{
			Con_Printf("usage: r_editlights_edit %s x y z\n", Cmd_Argv(1));
			return;
		}
		origin[0] *= atof(Cmd_Argv(2));
		origin[1] *= atof(Cmd_Argv(3));
		origin[2] *= atof(Cmd_Argv(4));
	}
	else if (!strcmp(Cmd_Argv(1), "originx"))
	{
		if (Cmd_Argc() != 3)
		{
			Con_Printf("usage: r_editlights_edit %s value\n", Cmd_Argv(1));
			return;
		}
		origin[0] = atof(Cmd_Argv(2));
	}
	else if (!strcmp(Cmd_Argv(1), "originy"))
	{
		if (Cmd_Argc() != 3)
		{
			Con_Printf("usage: r_editlights_edit %s value\n", Cmd_Argv(1));
			return;
		}
		origin[1] = atof(Cmd_Argv(2));
	}
	else if (!strcmp(Cmd_Argv(1), "originz"))
	{
		if (Cmd_Argc() != 3)
		{
			Con_Printf("usage: r_editlights_edit %s value\n", Cmd_Argv(1));
			return;
		}
		origin[2] = atof(Cmd_Argv(2));
	}
	else if (!strcmp(Cmd_Argv(1), "move"))
	{
		if (Cmd_Argc() != 5)
		{
			Con_Printf("usage: r_editlights_edit %s x y z\n", Cmd_Argv(1));
			return;
		}
		origin[0] += atof(Cmd_Argv(2));
		origin[1] += atof(Cmd_Argv(3));
		origin[2] += atof(Cmd_Argv(4));
	}
	else if (!strcmp(Cmd_Argv(1), "movex"))
	{
		if (Cmd_Argc() != 3)
		{
			Con_Printf("usage: r_editlights_edit %s value\n", Cmd_Argv(1));
			return;
		}
		origin[0] += atof(Cmd_Argv(2));
	}
	else if (!strcmp(Cmd_Argv(1), "movey"))
	{
		if (Cmd_Argc() != 3)
		{
			Con_Printf("usage: r_editlights_edit %s value\n", Cmd_Argv(1));
			return;
		}
		origin[1] += atof(Cmd_Argv(2));
	}
	else if (!strcmp(Cmd_Argv(1), "movez"))
	{
		if (Cmd_Argc() != 3)
		{
			Con_Printf("usage: r_editlights_edit %s value\n", Cmd_Argv(1));
			return;
		}
		origin[2] += atof(Cmd_Argv(2));
	}
	else if (!strcmp(Cmd_Argv(1), "angles"))
	{
		if (Cmd_Argc() != 5)
		{
			Con_Printf("usage: r_editlights_edit %s x y z\n", Cmd_Argv(1));
			return;
		}
		angles[0] = atof(Cmd_Argv(2));
		angles[1] = atof(Cmd_Argv(3));
		angles[2] = atof(Cmd_Argv(4));
	}
	else if (!strcmp(Cmd_Argv(1), "anglesx"))
	{
		if (Cmd_Argc() != 3)
		{
			Con_Printf("usage: r_editlights_edit %s value\n", Cmd_Argv(1));
			return;
		}
		angles[0] = atof(Cmd_Argv(2));
	}
	else if (!strcmp(Cmd_Argv(1), "anglesy"))
	{
		if (Cmd_Argc() != 3)
		{
			Con_Printf("usage: r_editlights_edit %s value\n", Cmd_Argv(1));
			return;
		}
		angles[1] = atof(Cmd_Argv(2));
	}
	else if (!strcmp(Cmd_Argv(1), "anglesz"))
	{
		if (Cmd_Argc() != 3)
		{
			Con_Printf("usage: r_editlights_edit %s value\n", Cmd_Argv(1));
			return;
		}
		angles[2] = atof(Cmd_Argv(2));
	}
	else if (!strcmp(Cmd_Argv(1), "color"))
	{
		if (Cmd_Argc() != 5)
		{
			Con_Printf("usage: r_editlights_edit %s red green blue\n", Cmd_Argv(1));
			return;
		}
		color[0] = atof(Cmd_Argv(2));
		color[1] = atof(Cmd_Argv(3));
		color[2] = atof(Cmd_Argv(4));
	}
	else if (!strcmp(Cmd_Argv(1), "radius"))
	{
		if (Cmd_Argc() != 3)
		{
			Con_Printf("usage: r_editlights_edit %s value\n", Cmd_Argv(1));
			return;
		}
		radius = atof(Cmd_Argv(2));
	}
	else if (!strcmp(Cmd_Argv(1), "colorscale"))
	{
		if (Cmd_Argc() == 3)
		{
			double scale = atof(Cmd_Argv(2));
			color[0] *= scale;
			color[1] *= scale;
			color[2] *= scale;
		}
		else
		{
			if (Cmd_Argc() != 5)
			{
				Con_Printf("usage: r_editlights_edit %s red green blue  (OR grey instead of red green blue)\n", Cmd_Argv(1));
				return;
			}
			color[0] *= atof(Cmd_Argv(2));
			color[1] *= atof(Cmd_Argv(3));
			color[2] *= atof(Cmd_Argv(4));
		}
	}
	else if (!strcmp(Cmd_Argv(1), "radiusscale") || !strcmp(Cmd_Argv(1), "sizescale"))
	{
		if (Cmd_Argc() != 3)
		{
			Con_Printf("usage: r_editlights_edit %s value\n", Cmd_Argv(1));
			return;
		}
		radius *= atof(Cmd_Argv(2));
	}
	else if (!strcmp(Cmd_Argv(1), "style"))
	{
		if (Cmd_Argc() != 3)
		{
			Con_Printf("usage: r_editlights_edit %s value\n", Cmd_Argv(1));
			return;
		}
		style = atoi(Cmd_Argv(2));
	}
	else if (!strcmp(Cmd_Argv(1), "cubemap"))
	{
		if (Cmd_Argc() > 3)
		{
			Con_Printf("usage: r_editlights_edit %s value\n", Cmd_Argv(1));
			return;
		}
		if (Cmd_Argc() == 3)
			strlcpy(cubemapname, Cmd_Argv(2), sizeof(cubemapname));
		else
			cubemapname[0] = 0;
	}
	else if (!strcmp(Cmd_Argv(1), "shadows"))
	{
		if (Cmd_Argc() != 3)
		{
			Con_Printf("usage: r_editlights_edit %s value\n", Cmd_Argv(1));
			return;
		}
		shadows = Cmd_Argv(2)[0] == 'y' || Cmd_Argv(2)[0] == 'Y' || Cmd_Argv(2)[0] == 't' || atoi(Cmd_Argv(2));
	}
	else if (!strcmp(Cmd_Argv(1), "corona"))
	{
		if (Cmd_Argc() != 3)
		{
			Con_Printf("usage: r_editlights_edit %s value\n", Cmd_Argv(1));
			return;
		}
		corona = atof(Cmd_Argv(2));
	}
	else if (!strcmp(Cmd_Argv(1), "coronasize"))
	{
		if (Cmd_Argc() != 3)
		{
			Con_Printf("usage: r_editlights_edit %s value\n", Cmd_Argv(1));
			return;
		}
		coronasizescale = atof(Cmd_Argv(2));
	}
	else if (!strcmp(Cmd_Argv(1), "ambient"))
	{
		if (Cmd_Argc() != 3)
		{
			Con_Printf("usage: r_editlights_edit %s value\n", Cmd_Argv(1));
			return;
		}
		ambientscale = atof(Cmd_Argv(2));
	}
	else if (!strcmp(Cmd_Argv(1), "diffuse"))
	{
		if (Cmd_Argc() != 3)
		{
			Con_Printf("usage: r_editlights_edit %s value\n", Cmd_Argv(1));
			return;
		}
		diffusescale = atof(Cmd_Argv(2));
	}
	else if (!strcmp(Cmd_Argv(1), "specular"))
	{
		if (Cmd_Argc() != 3)
		{
			Con_Printf("usage: r_editlights_edit %s value\n", Cmd_Argv(1));
			return;
		}
		specularscale = atof(Cmd_Argv(2));
	}
	else if (!strcmp(Cmd_Argv(1), "normalmode"))
	{
		if (Cmd_Argc() != 3)
		{
			Con_Printf("usage: r_editlights_edit %s value\n", Cmd_Argv(1));
			return;
		}
		normalmode = Cmd_Argv(2)[0] == 'y' || Cmd_Argv(2)[0] == 'Y' || Cmd_Argv(2)[0] == 't' || atoi(Cmd_Argv(2));
	}
	else if (!strcmp(Cmd_Argv(1), "realtimemode"))
	{
		if (Cmd_Argc() != 3)
		{
			Con_Printf("usage: r_editlights_edit %s value\n", Cmd_Argv(1));
			return;
		}
		realtimemode = Cmd_Argv(2)[0] == 'y' || Cmd_Argv(2)[0] == 'Y' || Cmd_Argv(2)[0] == 't' || atoi(Cmd_Argv(2));
	}
	else
	{
		Con_Print("usage: r_editlights_edit [property] [value]\n");
		Con_Print("Selected light's properties:\n");
		Con_Printf("Origin       : %f %f %f\n", r_shadow_selectedlight->origin[0], r_shadow_selectedlight->origin[1], r_shadow_selectedlight->origin[2]);
		Con_Printf("Angles       : %f %f %f\n", r_shadow_selectedlight->angles[0], r_shadow_selectedlight->angles[1], r_shadow_selectedlight->angles[2]);
		Con_Printf("Color        : %f %f %f\n", r_shadow_selectedlight->color[0], r_shadow_selectedlight->color[1], r_shadow_selectedlight->color[2]);
		Con_Printf("Radius       : %f\n", r_shadow_selectedlight->radius);
		Con_Printf("Corona       : %f\n", r_shadow_selectedlight->corona);
		Con_Printf("Style        : %i\n", r_shadow_selectedlight->style);
		Con_Printf("Shadows      : %s\n", r_shadow_selectedlight->shadow ? "yes" : "no");
		Con_Printf("Cubemap      : %s\n", r_shadow_selectedlight->cubemapname);
		Con_Printf("CoronaSize   : %f\n", r_shadow_selectedlight->coronasizescale);
		Con_Printf("Ambient      : %f\n", r_shadow_selectedlight->ambientscale);
		Con_Printf("Diffuse      : %f\n", r_shadow_selectedlight->diffusescale);
		Con_Printf("Specular     : %f\n", r_shadow_selectedlight->specularscale);
		Con_Printf("NormalMode   : %s\n", (r_shadow_selectedlight->flags & LIGHTFLAG_NORMALMODE) ? "yes" : "no");
		Con_Printf("RealTimeMode : %s\n", (r_shadow_selectedlight->flags & LIGHTFLAG_REALTIMEMODE) ? "yes" : "no");
		return;
	}
	flags = (normalmode ? LIGHTFLAG_NORMALMODE : 0) | (realtimemode ? LIGHTFLAG_REALTIMEMODE : 0);
	R_Shadow_UpdateWorldLight(r_shadow_selectedlight, origin, angles, color, radius, corona, style, shadows, cubemapname, coronasizescale, ambientscale, diffusescale, specularscale, flags);
}

static void R_Shadow_EditLights_EditAll_f()
{
	size_t lightindex;
	dlight_t *light, *oldselected;
	size_t range;

	if (!r_editlights.integer)
	{
		Con_Print("Cannot edit lights when not in editing mode. Set r_editlights to 1.\n");
		return;
	}

	oldselected = r_shadow_selectedlight;
	// EditLights doesn't seem to have a "remove" command or something so:
	range = Mem_ExpandableArray_IndexRange(&r_shadow_worldlightsarray); // checked
	for (lightindex = 0;lightindex < range;lightindex++)
	{
		light = (dlight_t *) Mem_ExpandableArray_RecordAtIndex(&r_shadow_worldlightsarray, lightindex);
		if (!light)
			continue;
		R_Shadow_SelectLight(light);
		R_Shadow_EditLights_Edit_f();
	}
	// return to old selected (to not mess editing once selection is locked)
	R_Shadow_SelectLight(oldselected);
}

void R_Shadow_EditLights_DrawSelectedLightProperties()
{
	int lightnumber, lightcount;
	size_t lightindex, range;
	dlight_t *light;
	char temp[256];
	float x, y;

	if (!r_editlights.integer)
		return;

	// update cvars so QC can query them
	if (r_shadow_selectedlight)
	{
		dpsnprintf(temp, sizeof(temp), "%f %f %f", r_shadow_selectedlight->origin[0], r_shadow_selectedlight->origin[1], r_shadow_selectedlight->origin[2]);
		Cvar_SetQuick(&r_editlights_current_origin, temp);
		dpsnprintf(temp, sizeof(temp), "%f %f %f", r_shadow_selectedlight->angles[0], r_shadow_selectedlight->angles[1], r_shadow_selectedlight->angles[2]);
		Cvar_SetQuick(&r_editlights_current_angles, temp);
		dpsnprintf(temp, sizeof(temp), "%f %f %f", r_shadow_selectedlight->color[0], r_shadow_selectedlight->color[1], r_shadow_selectedlight->color[2]);
		Cvar_SetQuick(&r_editlights_current_color, temp);
		Cvar_SetValueQuick(&r_editlights_current_radius, r_shadow_selectedlight->radius);
		Cvar_SetValueQuick(&r_editlights_current_corona, r_shadow_selectedlight->corona);
		Cvar_SetValueQuick(&r_editlights_current_coronasize, r_shadow_selectedlight->coronasizescale);
		Cvar_SetValueQuick(&r_editlights_current_style, r_shadow_selectedlight->style);
		Cvar_SetValueQuick(&r_editlights_current_shadows, r_shadow_selectedlight->shadow);
		Cvar_SetQuick(&r_editlights_current_cubemap, r_shadow_selectedlight->cubemapname);
		Cvar_SetValueQuick(&r_editlights_current_ambient, r_shadow_selectedlight->ambientscale);
		Cvar_SetValueQuick(&r_editlights_current_diffuse, r_shadow_selectedlight->diffusescale);
		Cvar_SetValueQuick(&r_editlights_current_specular, r_shadow_selectedlight->specularscale);
		Cvar_SetValueQuick(&r_editlights_current_normalmode, (r_shadow_selectedlight->flags & LIGHTFLAG_NORMALMODE) ? 1 : 0);
		Cvar_SetValueQuick(&r_editlights_current_realtimemode, (r_shadow_selectedlight->flags & LIGHTFLAG_REALTIMEMODE) ? 1 : 0);
	}

	// draw properties on screen
	if (!r_editlights_drawproperties.integer)
		return;
	x = vid_conwidth.value - 240;
	y = 5;
	DrawQ_Pic(x-5, y-5, nullptr, 250, 155, 0, 0, 0, 0.75, 0);
	lightnumber = -1;
	lightcount = 0;
	range = Mem_ExpandableArray_IndexRange(&r_shadow_worldlightsarray); // checked
	for (lightindex = 0;lightindex < range;lightindex++)
	{
		light = (dlight_t *) Mem_ExpandableArray_RecordAtIndex(&r_shadow_worldlightsarray, lightindex);
		if (!light)
			continue;
		if (light == r_shadow_selectedlight)
			lightnumber = (int)lightindex;
		lightcount++;
	}
	dpsnprintf(temp, sizeof(temp), "Cursor origin: %.0f %.0f %.0f", r_editlights_cursorlocation[0], r_editlights_cursorlocation[1], r_editlights_cursorlocation[2]); DrawQ_String(x, y, temp, 0, 8, 8, 1, 1, 1, 1, 0, nullptr, false, FONT_DEFAULT);y += 8;
	dpsnprintf(temp, sizeof(temp), "Total lights : %i active (%i total)", lightcount, (int)Mem_ExpandableArray_IndexRange(&r_shadow_worldlightsarray)); DrawQ_String(x, y, temp, 0, 8, 8, 1, 1, 1, 1, 0, nullptr, false, FONT_DEFAULT);y += 8;
	y += 8;
	if (r_shadow_selectedlight == nullptr)
		return;
	dpsnprintf(temp, sizeof(temp), "Light #%i properties:", lightnumber);DrawQ_String(x, y, temp, 0, 8, 8, 1, 1, 1, 1, 0, nullptr, true, FONT_DEFAULT);y += 8;
	dpsnprintf(temp, sizeof(temp), "Origin       : %.0f %.0f %.0f\n", r_shadow_selectedlight->origin[0], r_shadow_selectedlight->origin[1], r_shadow_selectedlight->origin[2]);DrawQ_String(x, y, temp, 0, 8, 8, 1, 1, 1, 1, 0, nullptr, true, FONT_DEFAULT);y += 8;
	dpsnprintf(temp, sizeof(temp), "Angles       : %.0f %.0f %.0f\n", r_shadow_selectedlight->angles[0], r_shadow_selectedlight->angles[1], r_shadow_selectedlight->angles[2]);DrawQ_String(x, y, temp, 0, 8, 8, 1, 1, 1, 1, 0, nullptr, true, FONT_DEFAULT);y += 8;
	dpsnprintf(temp, sizeof(temp), "Color        : %.2f %.2f %.2f\n", r_shadow_selectedlight->color[0], r_shadow_selectedlight->color[1], r_shadow_selectedlight->color[2]);DrawQ_String(x, y, temp, 0, 8, 8, 1, 1, 1, 1, 0, nullptr, true, FONT_DEFAULT);y += 8;
	dpsnprintf(temp, sizeof(temp), "Radius       : %.0f\n", r_shadow_selectedlight->radius);DrawQ_String(x, y, temp, 0, 8, 8, 1, 1, 1, 1, 0, nullptr, true, FONT_DEFAULT);y += 8;
	dpsnprintf(temp, sizeof(temp), "Corona       : %.0f\n", r_shadow_selectedlight->corona);DrawQ_String(x, y, temp, 0, 8, 8, 1, 1, 1, 1, 0, nullptr, true, FONT_DEFAULT);y += 8;
	dpsnprintf(temp, sizeof(temp), "Style        : %i\n", r_shadow_selectedlight->style);DrawQ_String(x, y, temp, 0, 8, 8, 1, 1, 1, 1, 0, nullptr, true, FONT_DEFAULT);y += 8;
	dpsnprintf(temp, sizeof(temp), "Shadows      : %s\n", r_shadow_selectedlight->shadow ? "yes" : "no");DrawQ_String(x, y, temp, 0, 8, 8, 1, 1, 1, 1, 0, nullptr, true, FONT_DEFAULT);y += 8;
	dpsnprintf(temp, sizeof(temp), "Cubemap      : %s\n", r_shadow_selectedlight->cubemapname);DrawQ_String(x, y, temp, 0, 8, 8, 1, 1, 1, 1, 0, nullptr, true, FONT_DEFAULT);y += 8;
	dpsnprintf(temp, sizeof(temp), "CoronaSize   : %.2f\n", r_shadow_selectedlight->coronasizescale);DrawQ_String(x, y, temp, 0, 8, 8, 1, 1, 1, 1, 0, nullptr, true, FONT_DEFAULT);y += 8;
	dpsnprintf(temp, sizeof(temp), "Ambient      : %.2f\n", r_shadow_selectedlight->ambientscale);DrawQ_String(x, y, temp, 0, 8, 8, 1, 1, 1, 1, 0, nullptr, true, FONT_DEFAULT);y += 8;
	dpsnprintf(temp, sizeof(temp), "Diffuse      : %.2f\n", r_shadow_selectedlight->diffusescale);DrawQ_String(x, y, temp, 0, 8, 8, 1, 1, 1, 1, 0, nullptr, true, FONT_DEFAULT);y += 8;
	dpsnprintf(temp, sizeof(temp), "Specular     : %.2f\n", r_shadow_selectedlight->specularscale);DrawQ_String(x, y, temp, 0, 8, 8, 1, 1, 1, 1, 0, nullptr, true, FONT_DEFAULT);y += 8;
	dpsnprintf(temp, sizeof(temp), "NormalMode   : %s\n", (r_shadow_selectedlight->flags & LIGHTFLAG_NORMALMODE) ? "yes" : "no");DrawQ_String(x, y, temp, 0, 8, 8, 1, 1, 1, 1, 0, nullptr, true, FONT_DEFAULT);y += 8;
	dpsnprintf(temp, sizeof(temp), "RealTimeMode : %s\n", (r_shadow_selectedlight->flags & LIGHTFLAG_REALTIMEMODE) ? "yes" : "no");DrawQ_String(x, y, temp, 0, 8, 8, 1, 1, 1, 1, 0, nullptr, true, FONT_DEFAULT);y += 8;
}

static void R_Shadow_EditLights_ToggleShadow_f()
{
	if (!r_editlights.integer)
	{
		Con_Print("Cannot spawn light when not in editing mode.  Set r_editlights to 1.\n");
		return;
	}
	if (!r_shadow_selectedlight)
	{
		Con_Print("No selected light.\n");
		return;
	}
	R_Shadow_UpdateWorldLight(r_shadow_selectedlight, r_shadow_selectedlight->origin, r_shadow_selectedlight->angles, r_shadow_selectedlight->color, r_shadow_selectedlight->radius, r_shadow_selectedlight->corona, r_shadow_selectedlight->style, !r_shadow_selectedlight->shadow, r_shadow_selectedlight->cubemapname, r_shadow_selectedlight->coronasizescale, r_shadow_selectedlight->ambientscale, r_shadow_selectedlight->diffusescale, r_shadow_selectedlight->specularscale, r_shadow_selectedlight->flags);
}

static void R_Shadow_EditLights_ToggleCorona_f()
{
	if (!r_editlights.integer)
	{
		Con_Print("Cannot spawn light when not in editing mode.  Set r_editlights to 1.\n");
		return;
	}
	if (!r_shadow_selectedlight)
	{
		Con_Print("No selected light.\n");
		return;
	}
	R_Shadow_UpdateWorldLight(r_shadow_selectedlight, r_shadow_selectedlight->origin, r_shadow_selectedlight->angles, r_shadow_selectedlight->color, r_shadow_selectedlight->radius, !r_shadow_selectedlight->corona, r_shadow_selectedlight->style, r_shadow_selectedlight->shadow, r_shadow_selectedlight->cubemapname, r_shadow_selectedlight->coronasizescale, r_shadow_selectedlight->ambientscale, r_shadow_selectedlight->diffusescale, r_shadow_selectedlight->specularscale, r_shadow_selectedlight->flags);
}

static void R_Shadow_EditLights_Remove_f()
{
	if (!r_editlights.integer)
	{
		Con_Print("Cannot remove light when not in editing mode.  Set r_editlights to 1.\n");
		return;
	}
	if (!r_shadow_selectedlight)
	{
		Con_Print("No selected light.\n");
		return;
	}
	R_Shadow_FreeWorldLight(r_shadow_selectedlight);
	r_shadow_selectedlight = nullptr;
}

static void R_Shadow_EditLights_Help_f()
{
	Con_Print(
"Documentation on r_editlights system:\n"
"Settings:\n"
"r_editlights : enable/disable editing mode\n"
"r_editlights_cursordistance : maximum distance of cursor from eye\n"
"r_editlights_cursorpushback : push back cursor this far from surface\n"
"r_editlights_cursorpushoff : push cursor off surface this far\n"
"r_editlights_cursorgrid : snap cursor to grid of this size\n"
"r_editlights_quakelightsizescale : imported quake light entity size scaling\n"
"Commands:\n"
"r_editlights_help : this help\n"
"r_editlights_clear : remove all lights\n"
"r_editlights_reload : reload .rtlights, .lights file, or entities\n"
"r_editlights_lock : lock selection to current light, if already locked - unlock\n"
"r_editlights_save : save to .rtlights file\n"
"r_editlights_spawn : create a light with default settings\n"
"r_editlights_edit command : edit selected light - more documentation below\n"
"r_editlights_remove : remove selected light\n"
"r_editlights_toggleshadow : toggles on/off selected light's shadow property\n"
"r_editlights_importlightentitiesfrommap : reload light entities\n"
"r_editlights_importlightsfile : reload .light file (produced by hlight)\n"
"Edit commands:\n"
"origin x y z : set light location\n"
"originx x: set x component of light location\n"
"originy y: set y component of light location\n"
"originz z: set z component of light location\n"
"move x y z : adjust light location\n"
"movex x: adjust x component of light location\n"
"movey y: adjust y component of light location\n"
"movez z: adjust z component of light location\n"
"angles x y z : set light angles\n"
"anglesx x: set x component of light angles\n"
"anglesy y: set y component of light angles\n"
"anglesz z: set z component of light angles\n"
"color r g b : set color of light (can be brighter than 1 1 1)\n"
"radius radius : set radius (size) of light\n"
"colorscale grey : multiply color of light (1 does nothing)\n"
"colorscale r g b : multiply color of light (1 1 1 does nothing)\n"
"radiusscale scale : multiply radius (size) of light (1 does nothing)\n"
"sizescale scale : multiply radius (size) of light (1 does nothing)\n"
"originscale x y z : multiply origin of light (1 1 1 does nothing)\n"
"style style : set lightstyle of light (flickering patterns, switches, etc)\n"
"cubemap basename : set filter cubemap of light\n"
"shadows 1/0 : turn on/off shadows\n"
"corona n : set corona intensity\n"
"coronasize n : set corona size (0-1)\n"
"ambient n : set ambient intensity (0-1)\n"
"diffuse n : set diffuse intensity (0-1)\n"
"specular n : set specular intensity (0-1)\n"
"normalmode 1/0 : turn on/off rendering of this light in rtworld 0 mode\n"
"realtimemode 1/0 : turn on/off rendering of this light in rtworld 1 mode\n"
"<nothing> : print light properties to console\n"
	);
}

static void R_Shadow_EditLights_CopyInfo_f()
{
	if (!r_editlights.integer)
	{
		Con_Print("Cannot copy light info when not in editing mode.  Set r_editlights to 1.\n");
		return;
	}
	if (!r_shadow_selectedlight)
	{
		Con_Print("No selected light.\n");
		return;
	}
	VectorCopy(r_shadow_selectedlight->angles, r_shadow_bufferlight.angles);
	VectorCopy(r_shadow_selectedlight->color, r_shadow_bufferlight.color);
	r_shadow_bufferlight.radius = r_shadow_selectedlight->radius;
	r_shadow_bufferlight.style = r_shadow_selectedlight->style;
	if (r_shadow_selectedlight->cubemapname)
		strlcpy(r_shadow_bufferlight.cubemapname, r_shadow_selectedlight->cubemapname, sizeof(r_shadow_bufferlight.cubemapname));
	else
		r_shadow_bufferlight.cubemapname[0] = 0;
	r_shadow_bufferlight.shadow = r_shadow_selectedlight->shadow;
	r_shadow_bufferlight.corona = r_shadow_selectedlight->corona;
	r_shadow_bufferlight.coronasizescale = r_shadow_selectedlight->coronasizescale;
	r_shadow_bufferlight.ambientscale = r_shadow_selectedlight->ambientscale;
	r_shadow_bufferlight.diffusescale = r_shadow_selectedlight->diffusescale;
	r_shadow_bufferlight.specularscale = r_shadow_selectedlight->specularscale;
	r_shadow_bufferlight.flags = r_shadow_selectedlight->flags;
}

static void R_Shadow_EditLights_PasteInfo_f()
{
	if (!r_editlights.integer)
	{
		Con_Print("Cannot paste light info when not in editing mode.  Set r_editlights to 1.\n");
		return;
	}
	if (!r_shadow_selectedlight)
	{
		Con_Print("No selected light.\n");
		return;
	}
	R_Shadow_UpdateWorldLight(r_shadow_selectedlight, r_shadow_selectedlight->origin, r_shadow_bufferlight.angles, r_shadow_bufferlight.color, r_shadow_bufferlight.radius, r_shadow_bufferlight.corona, r_shadow_bufferlight.style, r_shadow_bufferlight.shadow, r_shadow_bufferlight.cubemapname, r_shadow_bufferlight.coronasizescale, r_shadow_bufferlight.ambientscale, r_shadow_bufferlight.diffusescale, r_shadow_bufferlight.specularscale, r_shadow_bufferlight.flags);
}

static void R_Shadow_EditLights_Lock_f()
{
	if (!r_editlights.integer)
	{
		Con_Print("Cannot lock on light when not in editing mode.  Set r_editlights to 1.\n");
		return;
	}
	if (r_editlights_lockcursor)
	{
		r_editlights_lockcursor = false;
		return;
	}
	if (!r_shadow_selectedlight)
	{
		Con_Print("No selected light to lock on.\n");
		return;
	}
	r_editlights_lockcursor = true;
}

static void R_Shadow_EditLights_Init()
{
	Cvar_RegisterVariable(&r_editlights);
	Cvar_RegisterVariable(&r_editlights_cursordistance);
	Cvar_RegisterVariable(&r_editlights_cursorpushback);
	Cvar_RegisterVariable(&r_editlights_cursorpushoff);
	Cvar_RegisterVariable(&r_editlights_cursorgrid);
	Cvar_RegisterVariable(&r_editlights_quakelightsizescale);
	Cvar_RegisterVariable(&r_editlights_drawproperties);
	Cvar_RegisterVariable(&r_editlights_current_origin);
	Cvar_RegisterVariable(&r_editlights_current_angles);
	Cvar_RegisterVariable(&r_editlights_current_color);
	Cvar_RegisterVariable(&r_editlights_current_radius);
	Cvar_RegisterVariable(&r_editlights_current_corona);
	Cvar_RegisterVariable(&r_editlights_current_coronasize);
	Cvar_RegisterVariable(&r_editlights_current_style);
	Cvar_RegisterVariable(&r_editlights_current_shadows);
	Cvar_RegisterVariable(&r_editlights_current_cubemap);
	Cvar_RegisterVariable(&r_editlights_current_ambient);
	Cvar_RegisterVariable(&r_editlights_current_diffuse);
	Cvar_RegisterVariable(&r_editlights_current_specular);
	Cvar_RegisterVariable(&r_editlights_current_normalmode);
	Cvar_RegisterVariable(&r_editlights_current_realtimemode);
	Cmd_AddCommand("r_editlights_help", R_Shadow_EditLights_Help_f, "prints documentation on console commands and variables in rtlight editing system");
	Cmd_AddCommand("r_editlights_clear", R_Shadow_EditLights_Clear_f, "removes all world lights (let there be darkness!)");
	Cmd_AddCommand("r_editlights_reload", R_Shadow_EditLights_Reload_f, "reloads rtlights file (or imports from .lights file or .ent file or the map itself)");
	Cmd_AddCommand("r_editlights_save", R_Shadow_EditLights_Save_f, "save .rtlights file for current level");
	Cmd_AddCommand("r_editlights_spawn", R_Shadow_EditLights_Spawn_f, "creates a light with default properties (let there be light!)");
	Cmd_AddCommand("r_editlights_edit", R_Shadow_EditLights_Edit_f, "changes a property on the selected light");
	Cmd_AddCommand("r_editlights_editall", R_Shadow_EditLights_EditAll_f, "changes a property on ALL lights at once (tip: use radiusscale and colorscale to alter these properties)");
	Cmd_AddCommand("r_editlights_remove", R_Shadow_EditLights_Remove_f, "remove selected light");
	Cmd_AddCommand("r_editlights_toggleshadow", R_Shadow_EditLights_ToggleShadow_f, "toggle on/off the shadow option on the selected light");
	Cmd_AddCommand("r_editlights_togglecorona", R_Shadow_EditLights_ToggleCorona_f, "toggle on/off the corona option on the selected light");
	Cmd_AddCommand("r_editlights_importlightentitiesfrommap", R_Shadow_EditLights_ImportLightEntitiesFromMap_f, "load lights from .ent file or map entities (ignoring .rtlights or .lights file)");
	Cmd_AddCommand("r_editlights_importlightsfile", R_Shadow_EditLights_ImportLightsFile_f, "load lights from .lights file (ignoring .rtlights or .ent files and map entities)");
	Cmd_AddCommand("r_editlights_copyinfo", R_Shadow_EditLights_CopyInfo_f, "store a copy of all properties (except origin) of the selected light");
	Cmd_AddCommand("r_editlights_pasteinfo", R_Shadow_EditLights_PasteInfo_f, "apply the stored properties onto the selected light (making it exactly identical except for origin)");
	Cmd_AddCommand("r_editlights_lock", R_Shadow_EditLights_Lock_f, "lock selection to current light, if already locked - unlock");
}

