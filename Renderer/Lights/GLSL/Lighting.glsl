#extension GL_EXT_samplerless_texture_functions : require
#extension GL_KHR_shader_subgroup_arithmetic : require

#define FP16_IRRADIANCE_PROBES	1


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


float Ylm(int index, vec3 dir)
{
	if (index == 0)
		return 0.282095f;

	else if (index == 1)
		return 0.4886025f * dir.y;

	else if (index == 2)
		return 0.4886025f * dir.z;

	else if (index == 3)
		return 0.4886025f * dir.x;

	else if (index == 4)
		return 1.092548f * dir.y * dir.x;

	else if (index == 5)
		return 1.092548f * dir.z * dir.y;

	else if (index == 6)
		return 0.315391f * (2 * dir.z * dir.z - dir.x * dir.x - dir.y * dir.y);

	else if (index == 7)
		return 1.092548f * dir.z * dir.x;

	else if (index == 8)
		return 0.546274 * (dir.x * dir.x - dir.y * dir.y);
			
	else if (index == 9)
		return 0.5900436f * (3 * dir.x * dir.x - dir.y * dir.y) * dir.y;

	else if (index == 10)
		return 2.890611f * dir.x * dir.y * dir.z;

	else if (index == 11)
		return 0.457045f * dir.y * (4 * dir.z * dir.z - dir.x * dir.x - dir.y * dir.y);

	else if (index == 12)
		return 0.373176f * dir.z * (2.f * dir.z * dir.z - 3 * dir.x * dir.x - 3 * dir.y * dir.y);

	else if (index == 13)
		return 0.457045f * dir.x * (4 * dir.z * dir.z - dir.x * dir.x - dir.y * dir.y);

	else if (index == 14)
		return 2.890611f * (dir.x * dir.x - dir.y * dir.y) * dir.z;

	else if (index == 15)
		return 0.5900436f * (dir.x * dir.x - 3.f * dir.y * dir.y) * dir.x;
}



vec3 ExtractSH(in texture2DArray shProbes, ivec3 puv, vec3 dir)
{
	vec3 res = 0.f.xxx;

	float	rcpWindow = 1.0f / 0.8f;
	vec2	Factors = vec2( sin( 3.1415926f * rcpWindow ) / (3.1415926f * rcpWindow), sin( 2.0 * 3.1415926f * rcpWindow ) / (2.0 * 3.1415926f * rcpWindow) );

	res += Ylm(0, dir) * texelFetch(shProbes, ivec3(puv.xy * 4, puv.z), 0).xyz;

	for (int i = 1; i < 4; i++)
		res += Factors.x * Ylm(i, dir) * texelFetch(shProbes, ivec3(puv.xy * 4 + ivec2(i & 3, i >> 2), puv.z), 0).xyz;

	for (int i = 4; i < 9; i++)
		res += Factors.y * Ylm(i, dir) * texelFetch(shProbes, ivec3(puv.xy * 4 + ivec2(i & 3, i >> 2), puv.z), 0).xyz;
	
	return max(0.f.xxx, res);
}


vec3 ExtractSHCoeff(in texture2DArray shProbes, ivec3 puv, int index)
{
	return texelFetch(shProbes, ivec3(puv.xy * 4 + ivec2(index & 3, index >> 2), puv.z), 0).xyz;
}


vec3 SHCone(in texture2DArray shProbes, ivec3 puv, vec3 dir, float cosThetaAO) 
{
    float   t2 = cosThetaAO * cosThetaAO;
    float   t3 = t2 * cosThetaAO;
    float   t4 = t3 * cosThetaAO;
    float   ct2 = 1.0 - t2; 

    float       c0 = 0.88622692545275801364908374167057 * ct2;          // 1/2 * sqrt(PI) * (1-t^2)
    float       c1 = 1.02332670794648848847955162488930 * (1.0-t3);     // sqrt(PI/3) * (1-t^3)
    float       c2 = 0.24770795610037568833406429782001 * (3.0 * (1.0-t4) - 2.0 * ct2); // 1/16 * sqrt(5*PI) * [3(1-t^4) - 2(1-t^2)]
    const float sqrt3 = 1.7320508075688772935274463415059;

    float   x = dir.x;
    float   y = dir.y;
    float   z = dir.z;

    return  max( 0.f.xxx, c0 * ExtractSHCoeff(shProbes, puv, 0)             // c0.L00
            + c1 * (ExtractSHCoeff(shProbes, puv, 1) * y + ExtractSHCoeff(shProbes, puv, 2) * z + ExtractSHCoeff(shProbes, puv, 3) * x)                     // c1.(L1-1.y + L10.z + L11.x)
            + c2 * (ExtractSHCoeff(shProbes, puv, 6) * (3.0*z*z - 1.0)                              // c2.L20.(3z²-1)
                + sqrt3 * (ExtractSHCoeff(shProbes, puv, 8) *(x*x - y*y)                           // sqrt(3).c2.L22.(x²-y²)
                    + 2.0 * (ExtractSHCoeff(shProbes, puv, 4) *x*y + ExtractSHCoeff(shProbes, puv, 5) *y*z + ExtractSHCoeff(shProbes, puv, 7) *z*x)))    // 2sqrt(3).c2.(L2-2.xy + L2-1.yz + L21.zx)
        ) / c0;
 }



#if FP16_IRRADIANCE_PROBES
vec3 InterpolateIrradiance(texture2DArray IrradianceField, sampler sampLinear, in vec3 texcoord)
{
	return textureLod(sampler2DArray(IrradianceField, sampLinear), texcoord, 0.f).rgb;
}
#else
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
#endif


float ExtractOcclusion(ivec3 puv, vec4 occlu0, vec4 occlu1)
{
	if ((puv.z & 1) == 0)
	{
		if ((puv.y & 1u) == 0u)
		{
			if ((puv.x & 1u) == 0u)
				return occlu0.r;
			else
				return occlu0.g;
		}

		else
		{
			if ((puv.x & 1u) == 0u)
				return occlu0.b;
			else
				return occlu0.a;
		}
	}

	else
	{
		if ((puv.y & 1u) == 0u)
		{
			if ((puv.x & 1u) == 0u)
				return occlu1.r;
			else
				return occlu1.g;
		}

		else
		{
			if ((puv.x & 1u) == 0u)
				return occlu1.b;
			else
				return occlu1.a;
		}
	}
}


float roughnessToConeHalfWidth(in float r)
{
    float m = 2.f / (r * r * r * r) - 2.f;

    if (m >= 1e8f)
        return 0.f;

    float cos_a = exp2(-2.035f / (m + 1.f));

    return sqrt(2.f * (1.f - cos_a) / (1.f - cos_a * cos_a) - 1.f);
}


#if FP16_IRRADIANCE_PROBES
void ComputeFullGI(out vec3 irradiance, out vec3 specular, texture2DArray IrradianceField, itexture2DArray ProbeMetadata, texture3D ProbeOcclusion0, texture3D ProbeOcclusion1, texture2DArray shProbes, sampler sampLinear, vec3 pos, vec3 coords, vec3 center, vec3 size, vec3 normal, vec3 view, float roughness)
#else
void ComputeFullGI(out vec3 irradiance, out vec3 specular, texture2DArray IrradianceField, itexture2DArray ProbeMetadata, texture3D ProbeOcclusion0, texture3D ProbeOcclusion1, texture2DArray shProbes, sampler sampLinear, vec3 pos, vec3 coords, vec3 center, vec3 size, vec3 normal, vec3 view, float roughness)
#endif
{
	ivec3 texSize = textureSize(IrradianceField, 0).xyz;
	ivec3 numProbes = texSize / ivec3(10, 10, 1);

	irradiance = 0.f.xxx;
	specular = 0.f.xxx;

	vec3 st = coords * numProbes - 0.5f.xxx;

	ivec3	iuv = ivec3(floor(st));
	vec3	fuv = fract(st);

	precise float w = 0.f;

	precise vec3 fbIrradiance = 0.f.xxx;
	precise vec3 fbSpecular = 0.f.xxx;
	precise float fbW = 0.f;

	vec4 occlusion0 = textureLod(sampler3D(ProbeOcclusion0, sampLinear), coords, 0);
	vec4 occlusion1 = textureLod(sampler3D(ProbeOcclusion1, sampLinear), coords, 0);

	float t = roughnessToConeHalfWidth(roughness);
	float cosCone = clamp(1.f / sqrt(1.f + t * t), 0.f, 0.999f);

	vec3 r = reflect(-view, normal);

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
		float distToProbe = length(probeToPoint);
		vec3 dir = probeToPoint / distToProbe;

		float wn = dot(dir, -normal);

		if (wn < 0.0f)
			continue;

		linw *= wn;

		vec2 pixcoord = puv.xy * 10.f + 1.f.xx;
		pixcoord += EncodeOct(normal) * 8.f;

		vec2 texcoord = pixcoord / texSize.xy;

		precise vec3 light = pow(InterpolateIrradiance(IrradianceField, sampLinear, vec3(texcoord, puv.z)), 2.5f.xxx);
		precise vec3 refl = SHCone(shProbes, puv, r, cosCone);

		fbSpecular += refl * linw;
		fbIrradiance += light * linw;
		fbW += linw;

		linw *= max(1e-4f, ExtractOcclusion(puv, occlusion0, occlusion1));

		irradiance += light * linw;
		specular += refl * linw;
		w += linw;
	}

	irradiance /= max(1e-6f, w);
	specular /= max(1e-6f, w);

	irradiance = mix(fbIrradiance / max(1e-6f, fbW), irradiance, clamp(smoothstep(1e-4f, 1e-2f, w), 0.f, 1.f));

	irradiance *= irradiance;
}


vec3 ComputeSHRadiance(itexture2DArray ProbeMetadata, in texture2DArray shProbes, texture3D ProbeOcclusion0, texture3D ProbeOcclusion1, sampler sampLinear, vec3 pos, vec3 coords, vec3 center, vec3 size, vec3 normal)
{
	ivec3 numProbes = textureSize(ProbeMetadata, 0).xyz;

	precise vec3 radiance = 0.f.xxx;

	vec3 st = coords * numProbes - 0.5f.xxx;

	ivec3	iuv = ivec3(floor(st));
	vec3	fuv = fract(st);

	precise float w = 0.f;

	precise vec3 fbIrradiance = 0.f.xxx;
	precise float fbW = 0.f;

	vec4 occlusion0 = textureLod(sampler3D(ProbeOcclusion0, sampLinear), coords, 0);
	vec4 occlusion1 = textureLod(sampler3D(ProbeOcclusion1, sampLinear), coords, 0);

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
		float distToProbe = length(probeToPoint);
		vec3 dir = probeToPoint / distToProbe;

		float wn = max(0.05f, dot(dir, -normal) * 0.5f + 0.5f);

		linw *= wn;

		precise vec3 light = ExtractSH(shProbes, puv, dir);

		fbIrradiance += light * linw;
		fbW += linw;

		linw *= max(1e-4f, ExtractOcclusion(puv, occlusion0, occlusion1));

		radiance += light * linw;
		w += linw;
	}

	radiance /= max(1e-6f, w);

	radiance = mix(fbIrradiance / max(1e-6f, fbW), radiance, clamp(smoothstep(1e-5f, 5e-3f, w), 0.f, 1.f));

	return radiance;
}


#if FP16_IRRADIANCE_PROBES
vec3 ComputeGI(texture2DArray IrradianceField, itexture2DArray ProbeMetadata, texture3D ProbeOcclusion0, texture3D ProbeOcclusion1, sampler sampLinear, vec3 pos, vec3 coords, vec3 center, vec3 size, vec3 normal, vec3 noise)
#else
vec3 ComputeGI(utexture2DArray IrradianceField, itexture2DArray ProbeMetadata, texture3D ProbeOcclusion0, texture3D ProbeOcclusion1, sampler sampLinear, vec3 pos, vec3 coords, vec3 center, vec3 size, vec3 normal, vec3 noise)
#endif
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

	vec4 occlusion0 = textureLod(sampler3D(ProbeOcclusion0, sampLinear), coords, 0);
	vec4 occlusion1 = textureLod(sampler3D(ProbeOcclusion1, sampLinear), coords, 0);

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
		float distToProbe = length(probeToPoint);
		vec3 dir = probeToPoint / distToProbe;

		float wn = max(0.05f, dot(dir, -normal) * 0.5f + 0.5f);

		linw *= wn;

		vec2 pixcoord = puv.xy * 10.f + 1.f.xx;
		pixcoord += EncodeOct(normal) * 8.f;

		vec2 texcoord = pixcoord / texSize.xy;

		precise vec3 light = pow(InterpolateIrradiance(IrradianceField, sampLinear, vec3(texcoord, puv.z)), 2.5f.xxx);

		fbIrradiance += light * linw;
		fbW += linw;

		linw *= max(1e-4f, ExtractOcclusion(puv, occlusion0, occlusion1));

		irradiance += light * linw;
		w += linw;
	}

	irradiance /= max(1e-6f, w);

	irradiance = mix(fbIrradiance / max(1e-6f, fbW), irradiance, clamp(smoothstep(1e-5f, 5e-3f, w), 0.f, 1.f));

	return irradiance * irradiance;
}


//#endif
