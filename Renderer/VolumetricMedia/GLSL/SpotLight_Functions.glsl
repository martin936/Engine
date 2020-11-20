
//struct SSpotLight
//{
//	vec3	Pos;
//	float	Radius;

//	vec3	Color;
//	float	AngleScale;

//	vec3	Dir;
//	float	AngleOffset;
//};


#define m_AngleScale	m_Data[1].w
#define m_Dir			m_Data[2].xyz
#define m_AngleOffset	m_Data[2].w


float getAngleAtt(vec3 normalizedLightVector, vec3 lightDir, float lightAngleScale, float lightAngleOffset)
{
	// On the CPU
	// float lightAngleScale = 1.0 f / max (0.001f, ( cosInner - cosOuter ));
	// float lightAngleOffset = -cosOuter * angleScale ;

	float cd = dot ( lightDir , -normalizedLightVector );
	float attenuation = clamp((cd + lightAngleOffset) * lightAngleScale, 0.f, 1.f);
	// smooth the transition
	attenuation *= attenuation ;

	return attenuation ;
}



float Compute_ShadowingFactor_Spot(in vec3 Pos, in SLightShadow light)
{
	vec4 proj_pos = light.m_ShadowMatrix * vec4(Pos, 1.0f);
	proj_pos /= proj_pos.w;

	vec2 vTexCoords = proj_pos.xy * 0.5f + 0.5f.xx;
	float fSum = 0.f;

	vec2 texCoords;
	vec2 samples;
	vec2 lerpVals;
	vec4 sampleValues;

	proj_pos.z -= 0.0001f;
	proj_pos.z = proj_pos.z * 0.5f + 0.5f;

	texCoords	= vTexCoords.xy;
	samples		= floor(texCoords * ShadowPixelSize.xy + 0.5f.xx);
	lerpVals	= clamp(texCoords * ShadowPixelSize.xy + 0.5f.xx - samples, 0.f.xx, 1.f.xx);

	sampleValues = textureGather(ShadowMap, vec3(texCoords, light.m_DynamicIndex.x), proj_pos.z);

	fSum += mix(mix(sampleValues.w, sampleValues.z, lerpVals.x), mix(sampleValues.x, sampleValues.y, lerpVals.x), lerpVals.y);

	return fSum;
}



void Compute_SpotLight(out vec3 Illuminance, out vec3 L, in vec3 Pos, in SLight light)
{
	vec3 unormalizedLightVector = light.m_Pos - Pos;
	L = normalize(unormalizedLightVector);

	float att = getDistanceAtt(unormalizedLightVector, 1.f / (light.m_Radius * light.m_Radius));
	att *= getAngleAtt(L , light.m_Dir, light.m_AngleScale, light.m_AngleOffset);

	Illuminance = att * light.m_Color;
}



void Compute_SpotLightShadow(out vec3 Illuminance, out vec3 L, in vec3 Pos, in SLightShadow light)
{
	vec3 unormalizedLightVector = light.m_Pos - Pos;
	L = normalize(unormalizedLightVector);

	float att = getDistanceAtt(unormalizedLightVector, 1.f / (light.m_Radius * light.m_Radius));
	att *= getAngleAtt(L , light.m_Dir, light.m_AngleScale, light.m_AngleOffset);

	Illuminance = att * light.m_Color;

	if (light.m_DynamicIndex.x >= 0.f)
	{
		Illuminance *= Compute_ShadowingFactor_Spot(Pos, light);
	}
}

