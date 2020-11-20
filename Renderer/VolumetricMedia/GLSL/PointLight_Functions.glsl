
//struct SPointLight
//{
//	vec3	Pos;
//	float	Radius;

//	vec4	Color;
//};


//struct SPointLightShadow
//{
//	vec3	Pos;
//	float	Radius;

//	vec4	Color;

//	vec4	Padding;

//	mat4	ShadowMatrix;
//	float	Near;
//	float	Far;
//	vec2	Padding;

//	vec2	DynamicIndex;
//	vec2	StaticIndex;
//};


#define m_Pos			m_Data[0].xyz
#define	m_Radius		m_Data[0].w

#define m_Color			m_Data[1].rgb



float smoothDistanceAtt ( float squaredDistance , float invSqrAttRadius )
{
	float factor = squaredDistance * invSqrAttRadius ;
	float smoothFactor = clamp(1.f - factor * factor, 0.f, 1.f);
	return smoothFactor * smoothFactor ;
}


float getDistanceAtt ( vec3 unormalizedLightVector , float invSqrAttRadius )
{
	float sqrDist = dot(unormalizedLightVector, unormalizedLightVector);
	float attenuation = 1.f / (max(sqrDist , 0.01f * 0.01f));
	attenuation *= smoothDistanceAtt(sqrDist , invSqrAttRadius);

	return attenuation ;
}

void Compute_PointLight(out vec3 Illuminance, out vec3 L, in vec3 Pos, in SLight light)
{
	vec3 unormalizedLightVector = light.m_Pos - Pos;
	L = normalize(unormalizedLightVector);

	float att = getDistanceAtt(unormalizedLightVector, 1.f / (light.m_Radius * light.m_Radius));

	Illuminance = att * light.m_Color;
}


float Compute_ShadowingFactor_Omni(in vec3 Pos, in SLightShadow light)
{
	vec3 vPosDP = (vec4(Pos, 1.0f) * light.m_ShadowMatrix).xyz;	
	
	float fLength = length(vPosDP);
	vPosDP /= fLength;
	
	float fSceneDepth;
	float fSum = 0.f;

	vec2 vTexCoords;
	float index;
	
	if(vPosDP.z >= 0.0f)
	{		
		 
		vTexCoords.x =  (vPosDP.x /  (1.0f + vPosDP.z)) * 0.5f + 0.5f; 
		vTexCoords.y =  (vPosDP.y /  (1.0f + vPosDP.z)) * 0.5f + 0.5f; 		
		
		index = light.m_DynamicIndex.x;
	}
	else
	{
		vTexCoords.x =  (vPosDP.x /  (1.0f - vPosDP.z)) * 0.5f + 0.5f; 
		vTexCoords.y =  (vPosDP.y /  (1.0f - vPosDP.z)) * 0.5f + 0.5f;

		index = light.m_DynamicIndex.y;
	}

	fSceneDepth = (fLength - light.m_Near) / (light.m_Far - light.m_Near);
	fSceneDepth -= 0.001f;

	vec2 texCoords;
	vec2 samples;
	vec2 lerpVals;
	vec4 sampleValues;


	if (vPosDP.z >= 0.0f || light.m_Params.x > 0.f)
	{
		texCoords	= vTexCoords.xy;
		samples		= floor(texCoords / ShadowPixelSize.xy + 0.5f.xx);
		lerpVals	= clamp(texCoords / ShadowPixelSize.xy + 0.5f.xx - samples, 0.f.xx, 1.f.xx);

		sampleValues = textureGather(ShadowMap, vec3(texCoords, index), fSceneDepth);

		fSum += mix(mix(sampleValues.w, sampleValues.z, lerpVals.x), mix(sampleValues.x, sampleValues.y, lerpVals.x), lerpVals.y);
	}

	else
		fSum = 1.f;

	return fSum;
}


void Compute_PointLightShadow(out vec3 Illuminance, out vec3 L, in vec3 Pos, in SLightShadow light)
{
	vec3 unormalizedLightVector = light.m_Pos - Pos;
	L = normalize(unormalizedLightVector);

	float att = getDistanceAtt(unormalizedLightVector, 1.f / (light.m_Radius * light.m_Radius));

	Illuminance = att * light.m_Color;

	if (light.m_DynamicIndex.x >= 0.f)
	{
		Illuminance *= Compute_ShadowingFactor_Omni(Pos, light);
	}
}
