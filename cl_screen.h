#pragma once
void SHOWLMP_decodehide();
void SHOWLMP_decodeshow();
void SHOWLMP_drawall();

extern cvar_t vid_conwidth;
extern cvar_t vid_conheight;
extern cvar_t vid_pixelheight;
extern cvar_t scr_screenshot_jpeg;
extern cvar_t scr_screenshot_jpeg_quality;
extern cvar_t scr_screenshot_png;
extern cvar_t scr_screenshot_gammaboost;
extern cvar_t scr_screenshot_name;

void CL_Screen_NewMap();
void CL_Screen_Init();
void CL_Screen_Shutdown();
void CL_UpdateScreen();

bool R_Stereo_Active();
bool R_Stereo_ColorMasking();
