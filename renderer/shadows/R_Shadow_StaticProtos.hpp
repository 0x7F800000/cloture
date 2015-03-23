#pragma once

static void R_Shadow_EditLights_Init();
static void R_Shadow_FreeDeferred();

void R_Shadow_UncompileWorldLights();
void R_Shadow_ClearWorldLights();
void R_Shadow_SaveWorldLights();
void R_Shadow_LoadWorldLights();
void R_Shadow_LoadLightsFile();
void R_Shadow_LoadWorldLightsFromMap_LightArghliteTyrlite();
void R_Shadow_EditLights_Reload_f();
void R_Shadow_ValidateCvars();
static void R_Shadow_MakeTextures();
void R_Shadow_DrawLightSprites();
