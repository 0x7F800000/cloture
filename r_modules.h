#pragma once

void R_Modules_Init();
void R_RegisterModule(const char *name, void(*start)(), void(*shutdown)(), void(*newmap)(), void(*devicelost)(), void(*devicerestored)());
void R_Modules_Start();
void R_Modules_Shutdown();
void R_Modules_NewMap();
void R_Modules_Restart();
void R_Modules_DeviceLost();
void R_Modules_DeviceRestored();

