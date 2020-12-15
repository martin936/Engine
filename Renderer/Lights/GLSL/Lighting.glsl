#extension GL_EXT_samplerless_texture_functions : require
#extension GL_KHR_shader_subgroup_arithmetic : require


struct SLight
{
	vec4		m_Pos;
	vec4		m_Color;
	vec4		m_Dir;
};

struct SLightShadow
{
	SLight		m_light;

	vec4		m_ShadowIndex;
	mat4		m_ShadowMatrix;
};


vec3 Fresnel_Schlick( vec3 f0, float f90, float cos_hv )
{
	return f0 + (f90 - f0) * pow( 1.f - cos_hv, 5.f );
}


float GGX( float roughness, float cos_nh )
{
	float roughness2 = roughness * roughness;

	float f = cos_nh * cos_nh * (roughness2 - 1.f) + 1.f;

	float res = 1.f;
	if (roughness2 > 0.f && f > 0.f)
		res = roughness2 / (f * f);

	return res;
}


float Smith_GGX( float roughness, float cos_nv, float cos_nl )
{
	float roughness2 = roughness*roughness;

	float lambda_ggxv = cos_nl * sqrt((-cos_nv * roughness2 + cos_nv) * cos_nv + roughness2);
	float lambda_ggxl = cos_nv * sqrt((-cos_nl * roughness2 + cos_nl) * cos_nl + roughness2);

	float res = 0.5f / (lambda_ggxv + lambda_ggxl);
	if (isnan(res) || isinf(res))
		res = 1.f;

	return clamp(res, 0.f, 1.f);
}


vec3 SpecularGGX( float roughness, vec3 n, vec3 l, vec3 v, vec3 f0 )
{
	vec3 h = normalize( v + l );

	float cos_nv = abs(dot(n, v)) + 1e-5f;
	float cos_hv = clamp(dot(v, h), 0.f, 1.f);
	float cos_nh = clamp(dot(n, h), 0.f, 1.f);
	float cos_nl = clamp(dot(n, l), 0.f, 1.f);

	vec3	F = Fresnel_Schlick( f0, 1.f, cos_hv );
	float	G = Smith_GGX( roughness, cos_nv, cos_nl );
	float	D = GGX( roughness, cos_nh );

    return clamp(F * D * G * (1.f / 3.141592f), 0.f, 1.f);
}


float DisneyDiffuse( vec3 n, vec3 l, vec3 v, float linearRoughness )
{

	vec3 h = normalize( v + l );

	float cos_nv = abs(dot(n, v)) + 1e-5f;
	float cos_hl = clamp(dot(l, h), 0.f, 1.f);
	float cos_nl = clamp(dot(n, l), 0.f, 1.f);

	float energyBias	= mix(0.f , 0.5f , linearRoughness );
	float energyFactor	= mix(1.f , 1.f / 1.51f , linearRoughness );
	float fd90			= energyBias + 2.f * cos_hl * cos_hl * linearRoughness ;
	float f0			= 1.f;
	float lightScatter	= Fresnel_Schlick ( f0.xxx , fd90 , cos_nl ).r;
	float viewScatter	= Fresnel_Schlick ( f0.xxx , fd90 , cos_nv ).r;

    return clamp(lightScatter * viewScatter * energyFactor, 0.f, 1.f);
}


float smoothDistanceAtt(float squaredDistance, float invSqrAttRadius)
{
	float factor = squaredDistance * invSqrAttRadius;
	float smoothFactor = clamp(1.0f - factor * factor, 0.f, 1.f);
	return smoothFactor * smoothFactor;
}


float getDistanceAtt(vec3 unormalizedLightVector, float invSqrAttRadius)
{
	float sqrDist = dot(unormalizedLightVector, unormalizedLightVector);
	float attenuation = 1.f / max(sqrDist, 0.01f);
	attenuation *= smoothDistanceAtt(sqrDist, invSqrAttRadius);

	return attenuation;
}


float getAngleAtt(vec3 normalizedLightVector, vec3 lightDir, float lightAngleScale, float lightAngleOffset)
{
	float cd = dot(-lightDir, normalizedLightVector);
	float attenuation = clamp((cd + lightAngleOffset) * lightAngleScale, 0.f, 1.f);

	// smooth the transition
	attenuation *= attenuation;

	return attenuation;
}


uint wang_hash(uint seed)
{
	seed = (seed ^ 61) ^ (seed >> 16);
	seed *= 9;
	seed = seed ^ (seed >> 4);
	seed *= 0x27d4eb2d;
	seed = seed ^ (seed >> 15);
	return seed;
}


float rand(uint seed)
{
	return float(wang_hash(seed)) / 0xffffffff;
}


// Returns ±1
vec2 signNotZero(vec2 v)
{
	return vec2((v.x >= 0.0) ? +1.0 : -1.0, (v.y >= 0.0) ? +1.0 : -1.0);
}

// Assume normalized input. Output is on [-1, 1] for each component.
vec2 EncodeOct(in vec3 v)
{
	// Project the sphere onto the octahedron, and then onto the xy plane
	vec2 p = v.xy * (1.0 / (abs(v.x) + abs(v.y) + abs(v.z)));
	// Reflect the folds of the lower hemisphere over the diagonals
	return ((v.z <= 0.0) ? ((1.0 - abs(p.yx)) * signNotZero(p)) : p) * 0.5f + 0.5f.xx;
}

vec3 DecodeOct(in vec2 e)
{
	e = e * 2.f - 1.f;
	vec3 v = vec3(e.xy, 1.0 - abs(e.x) - abs(e.y));
	if (v.z < 0) v.xy = (1.0 - abs(v.yx)) * signNotZero(v.xy);
	return normalize(v);
}


float ComputeSunShadow(mat4 ShadowMatrix, vec3 pos, texture2DArray shadowMap, sampler samp)
{
	vec4 shadowPos;

	shadowPos = ShadowMatrix * vec4(pos, 1);
	shadowPos.xy = shadowPos.xy * vec2(0.5f, -0.5f) + 0.5f.xx;
	shadowPos.z -= 4.f / 65536.f;

	float factor = 1.f;

	factor *= texture(sampler2DArrayShadow(shadowMap, samp), vec4(shadowPos.xy, 0, shadowPos.z)).r;
	factor *= texture(sampler2DArrayShadow(shadowMap, samp), vec4(shadowPos.xy, 1, shadowPos.z)).r;

	return factor;
}


float ComputeSunShadow(mat4 ShadowMatrix, vec3 pos, texture2DArray shadowMap, sampler samp, float bias)
{
	vec4 shadowPos;

	shadowPos = ShadowMatrix * vec4(pos, 1);
	shadowPos.xy = shadowPos.xy * vec2(0.5f, -0.5f) + 0.5f.xx;
	shadowPos.z -= bias / 65536.f;

	float factor = 1.f;

	factor *= texture(sampler2DArrayShadow(shadowMap, samp), vec4(shadowPos.xy, 0, shadowPos.z)).r;
	factor *= texture(sampler2DArrayShadow(shadowMap, samp), vec4(shadowPos.xy, 1, shadowPos.z)).r;

	return factor;
}


void ComputeLight(SLight light, vec3 pos, vec3 normal, out vec3 Illuminance, out vec3 l)
{
    vec3 unnormalizedLightVector = light.m_Pos.xyz - pos;
    l = normalize(unnormalizedLightVector);
    float fAttenuation = 1.f;

	float lightInvSqrAttRadius = light.m_Pos.w;
	fAttenuation *= getDistanceAtt(unnormalizedLightVector, lightInvSqrAttRadius);
    fAttenuation *= getAngleAtt(l, light.m_Dir.xyz, light.m_Color.w, light.m_Dir.w);

    float factor = dot(normal, l);

    Illuminance = light.m_Color.xyz * clamp(factor, 0.f, 1.f) * fAttenuation;
}


float ComputeShadow(SLightShadow light, vec3 pos, texture2DArray shadowMap, sampler samp)
{
	vec4 shadowPos;

	// Spot
	if (light.m_ShadowIndex.z < 0.5f)
	{
		shadowPos = light.m_ShadowMatrix * vec4(pos, 1);
		shadowPos.xyz = (shadowPos.xyz / shadowPos.w);
		shadowPos.xy = shadowPos.xy * vec2(0.5f, -0.5f) + 0.5f.xx;

		shadowPos.z -= 4.f / 65536.f;
	}

	// Omni
	else
	{
		vec3 d = pos - light.m_light.m_Pos.xyz;
		float depth = length(d);

		float n = light.m_ShadowMatrix[0][0];
		float f = light.m_ShadowMatrix[0][1];

		shadowPos.z = ((f + n - 2.f * (f * n) / depth) / (n - f)) * 0.5f + 0.5f;

		shadowPos.z += 1e-4f;

		shadowPos.xy = EncodeOct(normalize(d));
	}

	float factor = 1.f;

	if (light.m_ShadowIndex.x >= 0)
	{
		factor *= texture(sampler2DArrayShadow(shadowMap, samp), vec4(shadowPos.xy, light.m_ShadowIndex.x, shadowPos.z)).r;
	}

	if (light.m_ShadowIndex.y >= 0)
	{
		factor *= texture(sampler2DArrayShadow(shadowMap, samp), vec4(shadowPos.xy, light.m_ShadowIndex.y, shadowPos.z)).r;
	}

	return factor;
}


// ToRGBE - takes a float RGB value and converts it to a float RGB value with a shared exponent
vec4 ToRGBE(vec3 inColor)
{
	float base = max(inColor.r, max(inColor.g, inColor.b));
	int e;
	float m = frexp(base, e);
	return vec4(clamp(inColor.rgb / exp2(e), 0.f, 1.f), (e + 127) / 255.f);
}

// FromRGBE takes a float RGB value with a sahred exponent and converts it to a 
//	float RGB value
vec3 FromRGBE(vec4 inColor)
{
	return inColor.rgb * exp2((255.f * inColor.a) - 127);
}


vec3 GetProbePos(in itexture2DArray probeMetadata, in ivec3 coords, vec3 center, vec3 size, out bool enabled)
{
	ivec3 numProbes = textureSize(probeMetadata, 0).xyz;

	vec3 cellCenter = center + vec3((coords.x + 0.5f) / numProbes.x - 0.5f, (coords.y + 0.5f) / numProbes.y - 0.5f, (coords.z + 0.5f) / numProbes.z - 0.5f) * size;
	vec3 cellSize	= 0.5f * size / numProbes;

	ivec4 probeData = texelFetch(probeMetadata, coords, 0);
	vec3 relativePos = probeData.xyz * (1.f / 127.f);

	enabled = ((probeData.w & 1) == 1);

	return cellCenter + cellSize * relativePos;
}


vec3 InterpolateIrradiance(utexture2DArray IrradianceField, sampler sampLinear, in vec3 texcoord)
{
	ivec2 texSize = textureSize(IrradianceField, 0).xy;

	vec2 filterWeight = (texcoord.xy * vec2(texSize)) - vec2(0.5);
	texcoord.xy = (floor(filterWeight) + 0.5f) / texSize;

	filterWeight = clamp(filterWeight - floor(filterWeight), 0.f.xx, 1.f.xx);

	uvec4 Data = textureGather(usampler2DArray(IrradianceField, sampLinear), texcoord);

	vec3 tmp0 = mix(FromRGBE(unpackUnorm4x8(Data.x)), FromRGBE(unpackUnorm4x8(Data.y)), filterWeight.x);
	vec3 tmp1 = mix(FromRGBE(unpackUnorm4x8(Data.w)), FromRGBE(unpackUnorm4x8(Data.z)), filterWeight.x);

	return mix(tmp1, tmp0, filterWeight.y);
}


#ifdef SDF_H

vec3 ComputeGI(utexture2DArray IrradianceField, itexture2DArray ProbeMetadata, sampler sampLinear, vec3 pos, vec3 coords, vec3 center, vec3 size, vec3 normal, vec3 view)
{
	ivec3 texSize = textureSize(IrradianceField, 0).xyz;
	ivec3 numProbes = texSize / ivec3(10, 10, 1);

	precise vec3 irradiance = 0.f.xxx;

	vec3 st = coords * numProbes - 0.5f.xxx;

	ivec3	iuv = ivec3(floor(st));
	vec3	fuv = fract(st);

	precise float w = 0.f;

	precise vec3 fbIrradiance = 0.f.xxx;
	precise float fbW = 0.f;

	for (uint id = 0U; id < 8U; id++)
	{
		ivec3 puv = clamp(iuv + ivec3(id & 1U, (id >> 1U) & 1U, id >> 2U), ivec3(0), numProbes - ivec3(1));

		bool enabled;
		vec3 probePos	= GetProbePos(ProbeMetadata, puv, center, size, enabled);

		if (!enabled)
			continue;

		uvec3 c = 1U - (uvec3(id, id >> 1U, id >> 2U) & 1U);
		ivec3 s = ivec3(1) - 2 * ivec3(c);

		float linw = (c.x + s.x * fuv.x) * (c.y + s.y * fuv.y) * (c.z + s.z * fuv.z);

		vec3 probeToPoint = pos - probePos;
		vec3 dir = normalize(probeToPoint);

		float wn = dot(dir, -normal);

		if (wn < 0.0f)
			continue;

		linw *= wn;

		vec2 pixcoord = puv.xy * 10.f + 1.f.xx;
		pixcoord += EncodeOct(normal) * 8.f;

		vec2 texcoord = pixcoord / texSize.xy;

		vec3 light = InterpolateIrradiance(IrradianceField, sampLinear, vec3(texcoord, puv.z));

		fbIrradiance += light * linw;
		fbW += linw;

		linw *= SampleSDFVisibilityTarget(sampLinear, pos + 0.08f * view, probePos);

		irradiance += light * linw;
		w += linw;
	}

	if (w > 0.f)
		irradiance /= w;

	else
		irradiance = fbIrradiance / max(1e-6f, fbW);

	return irradiance * irradiance;
}


#endif
