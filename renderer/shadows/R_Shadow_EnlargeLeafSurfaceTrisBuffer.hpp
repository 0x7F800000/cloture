#pragma once


static void R_Shadow_EnlargeLeafSurfaceTrisBuffer(int numleafs, int numsurfaces, int numshadowtriangles, int numlighttriangles)
{
	int numleafpvsbytes = (((numleafs + 7) >> 3) + 255) & ~255;
	int numsurfacepvsbytes = (((numsurfaces + 7) >> 3) + 255) & ~255;
	int numshadowtrispvsbytes = (((numshadowtriangles + 7) >> 3) + 255) & ~255;
	int numlighttrispvsbytes = (((numlighttriangles + 7) >> 3) + 255) & ~255;
	
	if (r_shadow_buffer_numleafpvsbytes < numleafpvsbytes)
	{
		if (r_shadow_buffer_visitingleafpvs != nullptr)
			r_main_mempool->dealloc(r_shadow_buffer_visitingleafpvs);
			
		//Mem_Free(r_shadow_buffer_visitingleafpvs);
			
		if (r_shadow_buffer_leafpvs != nullptr)
			r_main_mempool->dealloc(r_shadow_buffer_leafpvs);
		
		//Mem_Free(r_shadow_buffer_leafpvs);
			
		if (r_shadow_buffer_leaflist != nullptr)
			r_main_mempool->dealloc(r_shadow_buffer_leaflist);
			
		//Mem_Free(r_shadow_buffer_leaflist);
			
		r_shadow_buffer_numleafpvsbytes = numleafpvsbytes;
		
		r_shadow_buffer_visitingleafpvs = r_main_mempool->alloc<uint8>(r_shadow_buffer_numleafpvsbytes);
		
		//(unsigned char *)Mem_Alloc(r_main_mempool, r_shadow_buffer_numleafpvsbytes);
		
		r_shadow_buffer_leafpvs = r_main_mempool->alloc<uint8>(r_shadow_buffer_numleafpvsbytes);
		//(unsigned char *)Mem_Alloc(r_main_mempool, r_shadow_buffer_numleafpvsbytes);
		
		r_shadow_buffer_leaflist = r_main_mempool->alloc<typeof(*r_shadow_buffer_leaflist)>(r_shadow_buffer_numleafpvsbytes * 8);
		//(int *)Mem_Alloc(r_main_mempool, r_shadow_buffer_numleafpvsbytes * 8 * sizeof(*r_shadow_buffer_leaflist));
	}
	if (r_shadow_buffer_numsurfacepvsbytes < numsurfacepvsbytes)
	{
		if (r_shadow_buffer_surfacepvs)
			Mem_Free(r_shadow_buffer_surfacepvs);
		if (r_shadow_buffer_surfacelist)
			Mem_Free(r_shadow_buffer_surfacelist);
		if (r_shadow_buffer_surfacesides)
			Mem_Free(r_shadow_buffer_surfacesides);
		r_shadow_buffer_numsurfacepvsbytes = numsurfacepvsbytes;
		r_shadow_buffer_surfacepvs = (unsigned char *)Mem_Alloc(r_main_mempool, r_shadow_buffer_numsurfacepvsbytes);
		r_shadow_buffer_surfacelist = (int *)Mem_Alloc(r_main_mempool, r_shadow_buffer_numsurfacepvsbytes * 8 * sizeof(*r_shadow_buffer_surfacelist));
		r_shadow_buffer_surfacesides = (unsigned char *)Mem_Alloc(r_main_mempool, r_shadow_buffer_numsurfacepvsbytes * 8 * sizeof(*r_shadow_buffer_surfacelist));
	}
	if (r_shadow_buffer_numshadowtrispvsbytes < numshadowtrispvsbytes)
	{
		if (r_shadow_buffer_shadowtrispvs)
			Mem_Free(r_shadow_buffer_shadowtrispvs);
		r_shadow_buffer_numshadowtrispvsbytes = numshadowtrispvsbytes;
		r_shadow_buffer_shadowtrispvs = (unsigned char *)Mem_Alloc(r_main_mempool, r_shadow_buffer_numshadowtrispvsbytes);
	}
	if (r_shadow_buffer_numlighttrispvsbytes < numlighttrispvsbytes)
	{
		if (r_shadow_buffer_lighttrispvs)
			Mem_Free(r_shadow_buffer_lighttrispvs);
		r_shadow_buffer_numlighttrispvsbytes = numlighttrispvsbytes;
		r_shadow_buffer_lighttrispvs = (unsigned char *)Mem_Alloc(r_main_mempool, r_shadow_buffer_numlighttrispvsbytes);
	}
}
