#pragma once

/**
	we gain an increase in performance by not checking for certain codepaths
	and ignoring the values of developer cvars
*/
#define 	NO_SOFTWARE_RENDERER
#define		EXCLUDE_D3D_CODEPATHS
#define		DEVELOPER_CVARS				0
#define		NO_SILLY_CVARS				1
#define		ENABLE_TIMEREPORT			0

/**
	we can enable use of __assume and unreachable() optimizations
	individually for each file
	
	This helps with debugging faulty assumptions
*/

/**
	set RENDERER_MAY_ASSUME to 0 to disable all assumptions in the renderer
*/
#define		RENDERER_MAY_ASSUME			1

#if RENDERER_MAY_ASSUME

	#define		BACKEND_MAY_ASSUME		1
	#define		RMAIN_MAY_ASSUME		1
	#define		RSHADOW_MAY_ASSUME		1
	#define		GLTEXTURES_MAY_ASSUME	1
#else
	#define		BACKEND_MAY_ASSUME		0
	#define		RMAIN_MAY_ASSUME		0
	#define		RSHADOW_MAY_ASSUME		0
	#define		GLTEXTURES_MAY_ASSUME	0
#endif
