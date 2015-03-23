#pragma once


int R_Shadow_CalcTriangleSideMask(const vec3_t p1, const vec3_t p2, const vec3_t p3, float bias)
{
	// p1, p2, p3 are in the cubemap's local coordinate system
	// bias = border/(size - border)
	int mask = 0x3F;

	float dp1 = p1[0] + p1[1], dn1 = p1[0] - p1[1], ap1 = math::fabsf(dp1), an1 = math::fabsf(dn1),
		  dp2 = p2[0] + p2[1], dn2 = p2[0] - p2[1], ap2 = math::fabsf(dp2), an2 = math::fabsf(dn2),
		  dp3 = p3[0] + p3[1], dn3 = p3[0] - p3[1], ap3 = math::fabsf(dp3), an3 = math::fabsf(dn3);
	if(ap1 > bias*an1 && ap2 > bias*an2 && ap3 > bias*an3)
		mask &= (3<<4)
			| (dp1 >= .0f ? (1<<0)|(1<<2) : (2<<0)|(2<<2))
			| (dp2 >= .0f ? (1<<0)|(1<<2) : (2<<0)|(2<<2))
			| (dp3 >= .0f ? (1<<0)|(1<<2) : (2<<0)|(2<<2));
	if(an1 > bias*ap1 && an2 > bias*ap2 && an3 > bias*ap3)
		mask &= (3<<4)
			| (dn1 >= .0f ? (1<<0)|(2<<2) : (2<<0)|(1<<2))
			| (dn2 >= .0f ? (1<<0)|(2<<2) : (2<<0)|(1<<2))			
			| (dn3 >= .0f ? (1<<0)|(2<<2) : (2<<0)|(1<<2));

	dp1 = p1[1] + p1[2], dn1 = p1[1] - p1[2], ap1 = math::fabsf(dp1), an1 = math::fabsf(dn1),
	dp2 = p2[1] + p2[2], dn2 = p2[1] - p2[2], ap2 = math::fabsf(dp2), an2 = math::fabsf(dn2),
	dp3 = p3[1] + p3[2], dn3 = p3[1] - p3[2], ap3 = math::fabsf(dp3), an3 = math::fabsf(dn3);
	if(ap1 > bias*an1 && ap2 > bias*an2 && ap3 > bias*an3)
		mask &= (3<<0)
			| (dp1 >= .0f ? (1<<2)|(1<<4) : (2<<2)|(2<<4))
			| (dp2 >= .0f ? (1<<2)|(1<<4) : (2<<2)|(2<<4))			
			| (dp3 >= .0f ? (1<<2)|(1<<4) : (2<<2)|(2<<4));
	if(an1 > bias*ap1 && an2 > bias*ap2 && an3 > bias*ap3)
		mask &= (3<<0)
			| (dn1 >= .0f ? (1<<2)|(2<<4) : (2<<2)|(1<<4))
			| (dn2 >= .0f ? (1<<2)|(2<<4) : (2<<2)|(1<<4))
			| (dn3 >= .0f ? (1<<2)|(2<<4) : (2<<2)|(1<<4));

	dp1 = p1[2] + p1[0], dn1 = p1[2] - p1[0], ap1 = math::fabsf(dp1), an1 = math::fabsf(dn1),
	dp2 = p2[2] + p2[0], dn2 = p2[2] - p2[0], ap2 = math::fabsf(dp2), an2 = math::fabsf(dn2),
	dp3 = p3[2] + p3[0], dn3 = p3[2] - p3[0], ap3 = math::fabsf(dp3), an3 = math::fabsf(dn3);
	if(ap1 > bias*an1 && ap2 > bias*an2 && ap3 > bias*an3)
		mask &= (3<<2)
			| (dp1 >= 0 ? (1<<4)|(1<<0) : (2<<4)|(2<<0))
			| (dp2 >= 0 ? (1<<4)|(1<<0) : (2<<4)|(2<<0))
			| (dp3 >= 0 ? (1<<4)|(1<<0) : (2<<4)|(2<<0));
	if(an1 > bias*ap1 && an2 > bias*ap2 && an3 > bias*ap3)
		mask &= (3<<2)
			| (dn1 >= 0 ? (1<<4)|(2<<0) : (2<<4)|(1<<0))
			| (dn2 >= 0 ? (1<<4)|(2<<0) : (2<<4)|(1<<0))
			| (dn3 >= 0 ? (1<<4)|(2<<0) : (2<<4)|(1<<0));

	return mask;
}

#if 1

static int R_Shadow_CalcBBoxSideMask(
	const vec3_t mins,
	const vec3_t maxs,
	const matrix4x4_t *worldtolight,
	const matrix4x4_t *radiustolight,
	float bias
)
{
	vec3_t center, radius, lightcenter, lightradius, pmin, pmax;
	float dp1, dn1, ap1, an1, dp2, dn2, ap2, an2;
	int mask = 0x3F;

	VectorSubtract(maxs, mins, radius);
	VectorScale(radius, 0.5f, radius);
	VectorAdd(mins, radius, center);
	Matrix4x4_Transform(worldtolight, center, lightcenter);
	Matrix4x4_Transform3x3(radiustolight, radius, lightradius);
	VectorSubtract(lightcenter, lightradius, pmin);
	VectorAdd(lightcenter, lightradius, pmax);

	dp1 = pmax[0] + pmax[1];
	dn1 = pmax[0] - pmin[1];
	ap1 = math::fabsf(dp1);
	an1 = math::fabsf(dn1);

	dp2 = pmin[0] + pmin[1];
	dn2 = pmin[0] - pmax[1];
	ap2 = math::fabsf(dp2);
	an2 = math::fabsf(dn2);

	if(ap1 > bias*an1 && ap2 > bias*an2)
		mask &= (3<<4)
			| (dp1 >= .0f ? (1<<0)|(1<<2) : (2<<0)|(2<<2))
			| (dp2 >= .0f ? (1<<0)|(1<<2) : (2<<0)|(2<<2));
	if(an1 > bias*ap1 && an2 > bias*ap2)
		mask &= (3<<4)
			| (dn1 >= .0f ? (1<<0)|(2<<2) : (2<<0)|(1<<2))
			| (dn2 >= .0f ? (1<<0)|(2<<2) : (2<<0)|(1<<2));

	dp1 = pmax[1] + pmax[2];
	dn1 = pmax[1] - pmin[2];
	ap1 = math::fabsf(dp1);
	an1 = math::fabsf(dn1);

	dp2 = pmin[1] + pmin[2];
	dn2 = pmin[1] - pmax[2];
	ap2 = math::fabsf(dp2);
	an2 = math::fabsf(dn2);

	if(ap1 > bias*an1 && ap2 > bias*an2)
		mask &= (3<<0)
			| (dp1 >= .0f ? (1<<2)|(1<<4) : (2<<2)|(2<<4))
			| (dp2 >= .0f ? (1<<2)|(1<<4) : (2<<2)|(2<<4));
	if(an1 > bias*ap1 && an2 > bias*ap2)
		mask &= (3<<0)
			| (dn1 >= .0f ? (1<<2)|(2<<4) : (2<<2)|(1<<4))
			| (dn2 >= .0f ? (1<<2)|(2<<4) : (2<<2)|(1<<4));

	dp1 = pmax[2] + pmax[0];
	dn1 = pmax[2] - pmin[0];
	ap1 = math::fabsf(dp1);
	an1 = math::fabsf(dn1);

	dp2 = pmin[2] + pmin[0];
	dn2 = pmin[2] - pmax[0];
	ap2 = math::fabsf(dp2);
	an2 = math::fabsf(dn2);

	if(ap1 > bias*an1 && ap2 > bias*an2)
		mask &= (3<<2)
			| (dp1 >= .0f ? (1<<4)|(1<<0) : (2<<4)|(2<<0))
			| (dp2 >= .0f ? (1<<4)|(1<<0) : (2<<4)|(2<<0));
	if(an1 > bias*ap1 && an2 > bias*ap2)
		mask &= (3<<2)
			| (dn1 >= .0f ? (1<<4)|(2<<0) : (2<<4)|(1<<0))
			| (dn2 >= .0f ? (1<<4)|(2<<0) : (2<<4)|(1<<0));

	return mask;
}

#else
static int R_Shadow_CalcBBoxSideMask(
	const vector3f 					mins,
	const vector3f 					maxs,
	const matrix4x4_t *RESTRICT		worldtolight,
	const matrix4x4_t *RESTRICT		radiustolight,
	float 							bias
)
{
	vector3f pmin, pmax;
	float dp1, dn1, ap1, an1, dp2, dn2, ap2, an2;

	{
		const vector3f radius = (maxs - mins) * .5f;
		const vector3f lightcenter = worldtolight->transform(mins + radius);
		const vector3f lightradius = radiustolight->transform3x3(radius);
		pmin = lightcenter - lightradius;
		pmax = lightcenter + lightradius;
	}

	int mask = 0x3F;
	{
		vecmask32x4 dSigns; 	//the sign masks of D

		/*
		 * result of bias * a shuffled and then compared to a
		 * if(an1 > bias*ap1 && an2 > bias*ap2)
		 * if(ap1 > bias*an1 && ap2 > bias*an2)
		 *
		 * the "true" elements in the vector are set to 0xFFFF
		 * the false ones are 0
		 */
		vecmask32x4 cmpResult;

		{
			const vector4f a =
			({

				const vector4f d  =
				{
						pmax[0] + pmax[1],
						pmax[0] - pmin[1],
						pmin[0] + pmin[1],
						pmin[0] - pmax[1]
				};

				const vector4ui signMasks =
				{
						math::fsignMask,
						math::fsignMask,
						math::fsignMask,
						math::fsignMask
				};

				dSigns = vector_cast<vecmask32x4>(d) & signMasks;
				//initialize a with the absolute value of d
				vector_cast<vector4f>(
						vector_cast<vector4ui>(d) & (~signMasks)
				);
			});

			const vector4f ashuf 	= vector4f(a[1], a[3], a[0], a[2]) * bias;

			cmpResult = a > ashuf;

		}

		/*
		 * cmpResult[0] == ap1 > bias*an1
		 * cmpResult[1] == ap2 > bias*an2
		 */

		/*
		 * shift the sign masks for dp1 and dp2 to the right by 31 bits
		 * and then AND them with 1
		 *
		 * then shift 0b0101 left by the result
		 *
		 * this emulates the original logic, as if dp1 was unsigned,
		 * 5 will not be shifted left at all
		 */

		{

			const vecmask32x2 orResult =
			({

				vecmask32x2 masker =
				{
						0b0101,
						0b0101
				};
				//get low 2 sign bits (dp1 and dp2)
				masker = vector_cast<vecmask32x2>(
				masker <<  ((vector_cast<vecmask32x2>(dSigns) >> 31) & 1)
				);//signsShift;

				const vecmask32x2 booleanAnd =
				({
					//get low 2 comparison results (ap1 > bias*an1 && ap2 > bias*an2)
					const vecmask32x2 cmpLow = vector_cast<vecmask32x2>(cmpResult);
					vecmask32x2(cmpLow[0] & cmpLow[1]);
				});
				booleanAnd & masker;
			});
			/*
			 * if(ap1 > bias*an1 && ap2 > bias*an2) is false,
			 * booleanAnd will be {0, 0}
			 *
			 * if the condition is true, it will be {0xFFFFFFFF, 0xFFFFFFFF}
			 */
			mask &= (0b0110000 | ~(cmpResult[0] & cmpResult[1]) | orResult[0] | orResult[1]);
		}

		{
			/*
			 * cmpResult[2] == an1 > bias*ap1
			 * cmpResult[3] == an2 > bias*ap2
			 */
			int cmpmask = ~(cmpResult[2] & cmpResult[3]);

			int builtmask = 0b0110000; //48
			vecmask32x2 basemask =
			{
					0b0110, //6
					0b0110
			};
			vecmask32x2 oneSigns =
			{
					dSigns[2] >> 31,
					dSigns[3] >> 31
			};
			vecmask32x2 threes =
			{
					3,
					3
			};
			vecmask32x2 builtmasks = (threes * oneSigns) + basemask;
			/*
			if(!dSigns[2])
				builtmask |= 0b01001;//9
			else
				builtmask |= 0b00110;//6

			if(!dSigns[3])
				builtmask |= 0b01001;//9
			else
				builtmask |= 0b0110;//6
			*/

			mask &= (0b0110000 | builtmasks[0] | builtmasks[1] | cmpmask);//builtmask;
			//}
		}
	}
	{
		dp1 = pmax[1] + pmax[2];
		dn1 = pmax[1] - pmin[2];
		ap1 = math::fabsf(dp1);
		an1 = math::fabsf(dn1);

		dp2 = pmin[1] + pmin[2];
		dn2 = pmin[1] - pmax[2];
		ap2 = math::fabsf(dp2);
		an2 = math::fabsf(dn2);

		if(ap1 > bias*an1 && ap2 > bias*an2)
			mask &= (3<<0)
				| (dp1 >= .0f ? (1<<2)|(1<<4) : (2<<2)|(2<<4))
				| (dp2 >= .0f ? (1<<2)|(1<<4) : (2<<2)|(2<<4));
		if(an1 > bias*ap1 && an2 > bias*ap2)
			mask &= (3<<0)
				| (dn1 >= .0f ? (1<<2)|(2<<4) : (2<<2)|(1<<4))
				| (dn2 >= .0f ? (1<<2)|(2<<4) : (2<<2)|(1<<4));
	}
	{
		dp1 = pmax[2] + pmax[0];
		dn1 = pmax[2] - pmin[0];
		ap1 = math::fabsf(dp1);
		an1 = math::fabsf(dn1);

		dp2 = pmin[2] + pmin[0];
		dn2 = pmin[2] - pmax[0];
		ap2 = math::fabsf(dp2);
		an2 = math::fabsf(dn2);

		if(ap1 > bias*an1 && ap2 > bias*an2)
			mask &= (3<<2)
				| (dp1 >= .0f ? (1<<4)|(1<<0) : (2<<4)|(2<<0))
				| (dp2 >= .0f ? (1<<4)|(1<<0) : (2<<4)|(2<<0));
		if(an1 > bias*ap1 && an2 > bias*ap2)
			mask &= (3<<2)
				| (dn1 >= .0f ? (1<<4)|(2<<0) : (2<<4)|(1<<0))
				| (dn2 >= .0f ? (1<<4)|(2<<0) : (2<<4)|(1<<0));
	}
	return mask;
}

#endif
#define R_Shadow_CalcEntitySideMask(ent, worldtolight, radiustolight, bias) R_Shadow_CalcBBoxSideMask((ent)->mins, (ent)->maxs, worldtolight, radiustolight, bias)

int R_Shadow_CalcSphereSideMask(const vec3_t p, float radius, float bias)
{
	// p is in the cubemap's local coordinate system
	// bias = border/(size - border)
	float dxyp = p[0] + p[1];
	float dxyn = p[0] - p[1];
	float axyp = math::fabsf(dxyp);
	float axyn = math::fabsf(dxyn);
	float dyzp = p[1] + p[2];
	float dyzn = p[1] - p[2];
	float ayzp = math::fabsf(dyzp);
	float ayzn = math::fabsf(dyzn);
	float dzxp = p[2] + p[0];
	float dzxn = p[2] - p[0];
	float azxp = math::fabsf(dzxp);
	float azxn = math::fabsf(dzxn);
	int mask = 0x3F;
	if(axyp > bias*axyn + radius) 
		mask &= dxyp < .0f ? ~((1<<0)|(1<<2)) : ~((2<<0)|(2<<2));
	if(axyn > bias*axyp + radius) 
		mask &= dxyn < .0f ? ~((1<<0)|(2<<2)) : ~((2<<0)|(1<<2));
	if(ayzp > bias*ayzn + radius) 
		mask &= dyzp < .0f ? ~((1<<2)|(1<<4)) : ~((2<<2)|(2<<4));
	if(ayzn > bias*ayzp + radius) 
		mask &= dyzn < .0f ? ~((1<<2)|(2<<4)) : ~((2<<2)|(1<<4));
	if(azxp > bias*azxn + radius) 
		mask &= dzxp < .0f ? ~((1<<4)|(1<<0)) : ~((2<<4)|(2<<0));
	if(azxn > bias*azxp + radius) 
		mask &= dzxn < .0f ? ~((1<<4)|(2<<0)) : ~((2<<4)|(1<<0));
	return mask;
}
