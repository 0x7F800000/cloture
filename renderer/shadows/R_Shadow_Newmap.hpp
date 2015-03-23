#pragma once

static void r_shadow_newmap()
{
	if (r_shadow_bouncegridtexture) 
		R_FreeTexture(r_shadow_bouncegridtexture);
		
	r_shadow_bouncegridtexture = nullptr;
	
	if (r_shadow_lightcorona)                 
		R_SkinFrame_MarkUsed(r_shadow_lightcorona);
	if (r_editlights_sprcursor)               
		R_SkinFrame_MarkUsed(r_editlights_sprcursor);
	if (r_editlights_sprlight)                
		R_SkinFrame_MarkUsed(r_editlights_sprlight);
	if (r_editlights_sprnoshadowlight)        
		R_SkinFrame_MarkUsed(r_editlights_sprnoshadowlight);
	if (r_editlights_sprcubemaplight)         
		R_SkinFrame_MarkUsed(r_editlights_sprcubemaplight);
	if (r_editlights_sprcubemapnoshadowlight) 
		R_SkinFrame_MarkUsed(r_editlights_sprcubemapnoshadowlight);
	if (r_editlights_sprselection)            
		R_SkinFrame_MarkUsed(r_editlights_sprselection);
		
	if ( strncmp(cl.worldname, r_shadow_mapname, sizeof(r_shadow_mapname)) )
		R_Shadow_EditLights_Reload_f();
}
