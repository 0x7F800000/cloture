#pragma once

/* These are VM built-ins that originate in the client-side programs support
   but are reused by the other programs (usually the menu). */

void VM_CL_setmodel ();
void VM_CL_precache_model ();
void VM_CL_setorigin ();

void VM_CL_R_AddDynamicLight ();
void VM_CL_R_ClearScene ();
void VM_CL_R_AddEntities ();
void VM_CL_R_AddEntity ();
void VM_CL_R_SetView ();
void VM_CL_R_RenderScene ();
void VM_CL_R_LoadWorldModel ();

void VM_CL_R_PolygonBegin ();
void VM_CL_R_PolygonVertex ();
void VM_CL_R_PolygonEnd ();

void VM_CL_setattachment();
void VM_CL_gettagindex();
void VM_CL_gettaginfo();
