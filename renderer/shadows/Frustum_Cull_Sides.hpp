#pragma once


static int R_Shadow_CullFrustumSides(rtlight_t *RESTRICT const rtlight, const float size, const float border)
{
	vector3f o, p, n;
	int sides = 0x3F;
	int masks[6] = 
	{ 
		3	<<	4, 
		3	<<	4, 
		3	<<	0, 
		3	<<	0, 
		3	<<	2, 
		3	<<	2 
	};

	float len;
	const float bias = border / (float)(size - border);

	const float scale = 
	({
		const float prescale = (size - 2.0f * border) / size;
		
		// check if cone enclosing side would cross frustum plane 
		2.0f / (prescale*prescale + 2.0f);
	});
	
	//Matrix4x4_OriginFromMatrix(&rtlight->matrix_lighttoworld, o);
	o[0] = rtlight->matrix_lighttoworld.m[0][3];
	o[1] = rtlight->matrix_lighttoworld.m[1][3];
	o[2] = rtlight->matrix_lighttoworld.m[2][3];
	{
		
		for (size_t i = 0; i < 5; i++)
		{
			if (PlaneDiff(o, &r_refdef.view.frustum[i]) > -.03125f)
				continue;

			//Matrix4x4_Transform3x3(&rtlight->matrix_worldtolight, r_refdef.view.frustum[i].normal, n);
			{
				vector3f frustumNormal = r_refdef.view.frustum[i].normal;
				const matrix4x4_t* RESTRICT const matWtl = &rtlight->matrix_worldtolight;
				
				n[0] = frustumNormal[0] * matWtl->m[0][0] + frustumNormal[1] * matWtl->m[0][1] + frustumNormal[2] * matWtl->m[0][2];
				n[1] = frustumNormal[0] * matWtl->m[1][0] + frustumNormal[1] * matWtl->m[1][1] + frustumNormal[2] * matWtl->m[1][2];
				n[2] = frustumNormal[0] * matWtl->m[2][0] + frustumNormal[1] * matWtl->m[2][1] + frustumNormal[2] * matWtl->m[2][2];	
			}
			len = scale*VectorLength2(n);
			
			{
				const vector3f nSquared = n * n;
				if(nSquared[0] > len) 
					sides &= n[0] < .0f ? ~(1<<0) : ~(2 << 0);
				if(nSquared[1] > len) 
					sides &= n[1] < .0f ? ~(1<<2) : ~(2 << 2);
				if(nSquared[2] > len) 
					sides &= n[2] < .0f ? ~(1<<4) : ~(2 << 4);
			}
		
		}
		if (PlaneDiff(o, &r_refdef.view.frustum[4]) >= r_refdef.farclip - r_refdef.nearclip + .03125f)
		{
			//Matrix4x4_Transform3x3(&rtlight->matrix_worldtolight, r_refdef.view.frustum[4].normal, n);
			{
				vector3f frustumNormal = r_refdef.view.frustum[4].normal;
				const matrix4x4_t* RESTRICT const matWtl = &rtlight->matrix_worldtolight;
				
				n[0] = frustumNormal[0] * matWtl->m[0][0] + frustumNormal[1] * matWtl->m[0][1] + frustumNormal[2] * matWtl->m[0][2];
				n[1] = frustumNormal[0] * matWtl->m[1][0] + frustumNormal[1] * matWtl->m[1][1] + frustumNormal[2] * matWtl->m[1][2];
				n[2] = frustumNormal[0] * matWtl->m[2][0] + frustumNormal[1] * matWtl->m[2][1] + frustumNormal[2] * matWtl->m[2][2];	
			}
			len = scale*VectorLength2(n);
			const vector3f nSquared = n * n;//
			if(nSquared[0] > len) 
				sides &= n[0] >= .0f ? ~(1<<0) : ~(2 << 0);
				
			if(nSquared[1] > len) 
				sides &= n[1] >= .0f ? ~(1<<2) : ~(2 << 2);
				
			if(nSquared[2] > len) 
				sides &= n[2] >= .0f ? ~(1<<4) : ~(2 << 4);
				
		}
	}
	// this next test usually clips off more sides than the former, but occasionally clips fewer/different ones, so do both and combine results
	// check if frustum corners/origin cross plane sides

	// infinite version, assumes frustum corners merely give direction and extend to infinite distance
	
	//Matrix4x4_Transform(&rtlight->matrix_worldtolight, r_refdef.view.origin, p);

	{
		const matrix4x4_t* RESTRICT const matWtl = &rtlight->matrix_worldtolight;
		const vector3f viewOrigin = r_refdef.view.origin;
		p[0] = viewOrigin[0] * matWtl->m[0][0] + viewOrigin[1] * matWtl->m[0][1] + viewOrigin[2] * matWtl->m[0][2] + matWtl->m[0][3];
		p[1] = viewOrigin[0] * matWtl->m[1][0] + viewOrigin[1] * matWtl->m[1][1] + viewOrigin[2] * matWtl->m[1][2] + matWtl->m[1][3];
		p[2] = viewOrigin[0] * matWtl->m[2][0] + viewOrigin[1] * matWtl->m[2][1] + viewOrigin[2] * matWtl->m[2][2] + matWtl->m[2][3];		
	}
	
	float dp;
	float dn;
	float ap;
	float an;
	dp = p[0] + p[1];
	dn = p[0] - p[1];
	ap = math::fabsf(dp);
	an = math::fabsf(dn);
	masks[0] |= ap <= bias*an ? 0x3F : (dp >= .0f ? (1<<0)|(1<<2) : (2<<0)|(2<<2));
	masks[1] |= an <= bias*ap ? 0x3F : (dn >= .0f ? (1<<0)|(2<<2) : (2<<0)|(1<<2));
	dp = p[1] + p[2];
	dn = p[1] - p[2];
	ap = math::fabsf(dp);
	an = math::fabsf(dn);
	masks[2] |= ap <= bias*an ? 0x3F : (dp >= .0f ? (1<<2)|(1<<4) : (2<<2)|(2<<4));
	masks[3] |= an <= bias*ap ? 0x3F : (dn >= .0f ? (1<<2)|(2<<4) : (2<<2)|(1<<4));
	dp = p[2] + p[0];
	dn = p[2] - p[0];
	ap = math::fabsf(dp);
	an = math::fabsf(dn);
	masks[4] |= ap <= bias*an ? 0x3F : (dp >= .0f ? (1<<4)|(1<<0) : (2<<4)|(2<<0));
	masks[5] |= an <= bias*ap ? 0x3F : (dn >= .0f ? (1<<4)|(2<<0) : (2<<4)|(1<<0));
	{
		const matrix4x4_t* RESTRICT const matWtl = &rtlight->matrix_worldtolight;
		for (size_t i = 0; i < 4; i++)
		{

			//Matrix4x4_Transform(&rtlight->matrix_worldtolight, r_refdef.view.frustumcorner[i], n);
			{
				const vector3f corner = r_refdef.view.frustumcorner[i];
				n[0] = corner[0] * matWtl->m[0][0] + corner[1] * matWtl->m[0][1] + corner[2] * matWtl->m[0][2] + matWtl->m[0][3];
				n[1] = corner[0] * matWtl->m[1][0] + corner[1] * matWtl->m[1][1] + corner[2] * matWtl->m[1][2] + matWtl->m[1][3];
				n[2] = corner[0] * matWtl->m[2][0] + corner[1] * matWtl->m[2][1] + corner[2] * matWtl->m[2][2] + matWtl->m[2][3];				
			}
			VectorSubtract(n, p, n);
			dp = n[0] + n[1];
			dn = n[0] - n[1];
			ap = math::fabsf(dp);
			an = math::fabsf(dn);
			if(ap > .0f) 
				masks[0] |= dp >= .0f ? (1<<0)|(1<<2) : (2<<0)|(2<<2);
			if(an > .0f) 
				masks[1] |= dn >= .0f ? (1<<0)|(2<<2) : (2<<0)|(1<<2);
			dp = n[1] + n[2];
			dn = n[1] - n[2];
			ap = math::fabsf(dp);
			an = math::fabsf(dn);
			
			if(ap > .0f) 
				masks[2] |= dp >= .0f ? (1<<2)|(1<<4) : (2<<2)|(2<<4);
				
			if(an > .0f) 
				masks[3] |= dn >= .0f ? (1<<2)|(2<<4) : (2<<2)|(1<<4);
				
			dp = n[2] + n[0];
			dn = n[2] - n[0];
			ap = math::fabsf(dp);
			an = math::fabsf(dn);
			
			if(ap > .0f) 
				masks[4] |= dp >= .0f ? (1<<4)|(1<<0) : (2<<4)|(2<<0);
				
			if(an > .0f) 
				masks[5] |= dn >= .0f ? (1<<4)|(2<<0) : (2<<4)|(1<<0);
		}
	}
	return sides & masks[0] & masks[1] & masks[2] & masks[3] & masks[4] & masks[5];
}
