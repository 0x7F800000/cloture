#pragma once


static void R_Shadow_MakeTextures_MakeCorona()
{
	float dx, dy;
	int x, y, a;
	unsigned char pixels[32][32][4];
	for (y = 0;y < 32;y++)
	{
		dy = (y - 15.5f) * (1.0f / 16.0f);
		for (x = 0;x < 32;x++)
		{
			dx = (x - 15.5f) * (1.0f / 16.0f);
			a = (int)(((1.0f / (dx * dx + dy * dy + 0.2f)) - (1.0f / (1.0f + 0.2))) * 32.0f / (1.0f / (1.0f + 0.2)));
			a = bound(0, a, 255);
			pixels[y][x][0] = a;
			pixels[y][x][1] = a;
			pixels[y][x][2] = a;
			pixels[y][x][3] = 255;
		}
	}
	r_shadow_lightcorona = R_SkinFrame_LoadInternalBGRA("lightcorona", TEXF_FORCELINEAR, &pixels[0][0][0], 32, 32, false);
}

static unsigned int R_Shadow_MakeTextures_SamplePoint(float x, float y, float z)
{
	float dist = math::sqrtf(x*x+y*y+z*z);
	float intensity = dist < 1.0f ? ((1.0f - dist) * 
	r_shadow_lightattenuationlinearscale.value / (r_shadow_lightattenuationdividebias.value + dist*dist)) : .0f;
	// note this code could suffer byte order issues except that it is multiplying by an integer that reads the same both ways
	return (unsigned char)bound(0, intensity * 256.0f, 255) * 0x01010101;
}

static void R_Shadow_MakeTextures()
{
	int x, y, z;
	float intensity, dist;
	unsigned int *data;
	R_Shadow_FreeShadowMaps();
	R_FreeTexturePool(&r_shadow_texturepool);
	r_shadow_texturepool = R_AllocTexturePool();
	r_shadow_attenlinearscale = r_shadow_lightattenuationlinearscale.value;
	r_shadow_attendividebias = r_shadow_lightattenuationdividebias.value;
	data = (unsigned int *)Mem_Alloc(tempmempool, max(max(ATTEN3DSIZE*ATTEN3DSIZE*ATTEN3DSIZE, ATTEN2DSIZE*ATTEN2DSIZE), ATTEN1DSIZE) * 4);
	// the table includes one additional value to avoid the need to clamp indexing due to minor math errors
	for (x = 0;x <= ATTENTABLESIZE;x++)
	{
		dist = (x + 0.5f) * (1.0f / ATTENTABLESIZE) * (1.0f / 0.9375);
		intensity = dist < 1 ? ((1.0f - dist) * r_shadow_lightattenuationlinearscale.value / (r_shadow_lightattenuationdividebias.value + dist*dist)) : 0;
		r_shadow_attentable[x] = bound(0, intensity, 1);
	}
	// 1D gradient texture
	for (x = 0;x < ATTEN1DSIZE;x++)
		data[x] = R_Shadow_MakeTextures_SamplePoint((x + 0.5f) * (1.0f / ATTEN1DSIZE) * (1.0f / 0.9375), 0, 0);
	r_shadow_attenuationgradienttexture = R_LoadTexture2D(r_shadow_texturepool, "attenuation1d", ATTEN1DSIZE, 1, (unsigned char *)data, TEXTYPE_BGRA, TEXF_CLAMP | TEXF_ALPHA | TEXF_FORCELINEAR, -1, nullptr);
	// 2D circle texture
	for (y = 0;y < ATTEN2DSIZE;y++)
		for (x = 0;x < ATTEN2DSIZE;x++)
			data[y*ATTEN2DSIZE+x] = R_Shadow_MakeTextures_SamplePoint(((x + 0.5f) * (2.0f / ATTEN2DSIZE) - 1.0f) * (1.0f / 0.9375), ((y + 0.5f) * (2.0f / ATTEN2DSIZE) - 1.0f) * (1.0f / 0.9375), 0);
	r_shadow_attenuation2dtexture = R_LoadTexture2D(r_shadow_texturepool, "attenuation2d", ATTEN2DSIZE, ATTEN2DSIZE, (unsigned char *)data, TEXTYPE_BGRA, TEXF_CLAMP | TEXF_ALPHA | TEXF_FORCELINEAR, -1, nullptr);
	// 3D sphere texture
	if (r_shadow_texture3d.integer && vid.support.ext_texture_3d)
	{
		for (z = 0;z < ATTEN3DSIZE;z++)
			for (y = 0;y < ATTEN3DSIZE;y++)
				for (x = 0;x < ATTEN3DSIZE;x++)
					data[(z*ATTEN3DSIZE+y)*ATTEN3DSIZE+x] = R_Shadow_MakeTextures_SamplePoint(((x + 0.5f) * (2.0f / ATTEN3DSIZE) - 1.0f) * (1.0f / 0.9375), ((y + 0.5f) * (2.0f / ATTEN3DSIZE) - 1.0f) * (1.0f / 0.9375), ((z + 0.5f) * (2.0f / ATTEN3DSIZE) - 1.0f) * (1.0f / 0.9375));
		r_shadow_attenuation3dtexture = R_LoadTexture3D(r_shadow_texturepool, "attenuation3d", ATTEN3DSIZE, ATTEN3DSIZE, ATTEN3DSIZE, (unsigned char *)data, TEXTYPE_BGRA, TEXF_CLAMP | TEXF_ALPHA | TEXF_FORCELINEAR, -1, nullptr);
	}
	else
		r_shadow_attenuation3dtexture = nullptr;
	Mem_Free(data);

	R_Shadow_MakeTextures_MakeCorona();

	// Editor light sprites
	r_editlights_sprcursor = R_SkinFrame_LoadInternal8bit("gfx/editlights/cursor", TEXF_ALPHA | TEXF_CLAMP, (const unsigned char *)
	"................"
	".3............3."
	"..5...2332...5.."
	"...7.3....3.7..."
	"....7......7...."
	"...3.7....7.3..."
	"..2...7..7...2.."
	"..3..........3.."
	"..3..........3.."
	"..2...7..7...2.."
	"...3.7....7.3..."
	"....7......7...."
	"...7.3....3.7..."
	"..5...2332...5.."
	".3............3."
	"................"
	, 16, 16, palette_bgra_embeddedpic, palette_bgra_embeddedpic);
	r_editlights_sprlight = R_SkinFrame_LoadInternal8bit("gfx/editlights/light", TEXF_ALPHA | TEXF_CLAMP, (const unsigned char *)
	"................"
	"................"
	"......1111......"
	"....11233211...."
	"...1234554321..."
	"...1356776531..."
	"..124677776421.."
	"..135777777531.."
	"..135777777531.."
	"..124677776421.."
	"...1356776531..."
	"...1234554321..."
	"....11233211...."
	"......1111......"
	"................"
	"................"
	, 16, 16, palette_bgra_embeddedpic, palette_bgra_embeddedpic);
	r_editlights_sprnoshadowlight = R_SkinFrame_LoadInternal8bit("gfx/editlights/noshadow", TEXF_ALPHA | TEXF_CLAMP, (const unsigned char *)
	"................"
	"................"
	"......1111......"
	"....11233211...."
	"...1234554321..."
	"...1356226531..."
	"..12462..26421.."
	"..1352....2531.."
	"..1352....2531.."
	"..12462..26421.."
	"...1356226531..."
	"...1234554321..."
	"....11233211...."
	"......1111......"
	"................"
	"................"
	, 16, 16, palette_bgra_embeddedpic, palette_bgra_embeddedpic);
	r_editlights_sprcubemaplight = R_SkinFrame_LoadInternal8bit("gfx/editlights/cubemaplight", TEXF_ALPHA | TEXF_CLAMP, (const unsigned char *)
	"................"
	"................"
	"......2772......"
	"....27755772...."
	"..277533335772.."
	"..753333333357.."
	"..777533335777.."
	"..735775577537.."
	"..733357753337.."
	"..733337733337.."
	"..753337733357.."
	"..277537735772.."
	"....27777772...."
	"......2772......"
	"................"
	"................"
	, 16, 16, palette_bgra_embeddedpic, palette_bgra_embeddedpic);
	r_editlights_sprcubemapnoshadowlight = R_SkinFrame_LoadInternal8bit("gfx/editlights/cubemapnoshadowlight", TEXF_ALPHA | TEXF_CLAMP, (const unsigned char *)
	"................"
	"................"
	"......2772......"
	"....27722772...."
	"..2772....2772.."
	"..72........27.."
	"..7772....2777.."
	"..7.27722772.7.."
	"..7...2772...7.."
	"..7....77....7.."
	"..72...77...27.."
	"..2772.77.2772.."
	"....27777772...."
	"......2772......"
	"................"
	"................"
	, 16, 16, palette_bgra_embeddedpic, palette_bgra_embeddedpic);
	r_editlights_sprselection = R_SkinFrame_LoadInternal8bit("gfx/editlights/selection", TEXF_ALPHA | TEXF_CLAMP, (unsigned char *)
	"................"
	".777752..257777."
	".742........247."
	".72..........27."
	".7............7."
	".5............5."
	".2............2."
	"................"
	"................"
	".2............2."
	".5............5."
	".7............7."
	".72..........27."
	".742........247."
	".777752..257777."
	"................"
	, 16, 16, palette_bgra_embeddedpic, palette_bgra_embeddedpic);
}
