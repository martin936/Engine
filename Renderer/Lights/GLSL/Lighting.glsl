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


vec3 GetProbePos(uvec3 coords, vec3 center, vec3 size, ivec3 numProbes)
{
	return center + vec3((coords.x + 0.5f) / numProbes.x - 0.5f, (coords.y + 0.5f) / numProbes.y - 0.5f, (coords.z + 0.5f) / numProbes.z - 0.5f) * size;
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


vec4 cubic(float v)
{
	vec4 n = vec4(1.0, 2.0, 3.0, 4.0) - v;
	vec4 s = n * n * n;
	float x = s.x;
	float y = s.y - 4.0 * s.x;
	float z = s.z - 4.0 * s.y + 6.0 * s.x;
	float w = 6.0 - x - y - z;
	return vec4(x, y, z, w);
}


vec2 filterDepth(texture2DArray FieldDepth_1, sampler sampLinear_1, in vec2 texcoord, in ivec3 probeID)
{
	vec2 texscale = 1.f / textureSize(FieldDepth_1, 0).xy;

	float fx = fract(texcoord.x);
	float fy = fract(texcoord.y);
	texcoord.x -= fx;
	texcoord.y -= fy;

	vec4 xcubic = cubic(fx);
	vec4 ycubic = cubic(fy);

	vec4 c = vec4(texcoord.x - 0.5, texcoord.x + 1.5, texcoord.y - 0.5, texcoord.y + 1.5);
	vec4 s = vec4(xcubic.x + xcubic.y, xcubic.z + xcubic.w, ycubic.x + ycubic.y, ycubic.z + ycubic.w);
	vec4 offset = c + vec4(xcubic.y, xcubic.w, ycubic.y, ycubic.w) / s;

	offset = clamp(offset, -0.5f.xxxx, 16.5f.xxxx);

	vec2 sample0 = textureLod(sampler2DArray(FieldDepth_1, sampLinear_1), vec3((vec2(offset.x, offset.z) + probeID.xy * 16.f + 1.f) * texscale.xy, probeID.z), 0).rg;
	vec2 sample1 = textureLod(sampler2DArray(FieldDepth_1, sampLinear_1), vec3((vec2(offset.y, offset.z) + probeID.xy * 16.f + 1.f) * texscale.xy, probeID.z), 0).rg;
	vec2 sample2 = textureLod(sampler2DArray(FieldDepth_1, sampLinear_1), vec3((vec2(offset.x, offset.w) + probeID.xy * 16.f + 1.f) * texscale.xy, probeID.z), 0).rg;
	vec2 sample3 = textureLod(sampler2DArray(FieldDepth_1, sampLinear_1), vec3((vec2(offset.y, offset.w) + probeID.xy * 16.f + 1.f) * texscale.xy, probeID.z), 0).rg;

	float sx = s.x / (s.x + s.y);
	float sy = s.z / (s.z + s.w);

	return mix(
		mix(sample3, sample2, sx),
		mix(sample1, sample0, sx), sy);
}


float BSplineCoeffs(ivec3 coord, vec3 t)
{
	float w = 1.f;

	w *= (coord.x == 0u) ? (1.f - t.x * t.x) : t.x * (2.f - t.x);
	w *= (coord.y == 0u) ? (1.f - t.y * t.y) : t.y * (2.f - t.y);
	w *= (coord.z == 0u) ? (1.f - t.z * t.z) : t.z * (2.f - t.z);

	return w;
}


vec3 BSplineDerivCoeffs(ivec3 coord, vec3 t)
{
	vec3 w = t * (1.f - t);

	w.x *= (coord.y == 0u) ? (1.f - t.y * t.y) : t.y * (2.f - t.y);
	w.x *= (coord.z == 0u) ? (1.f - t.z * t.z) : t.z * (2.f - t.z);
	w.x *= (coord.x == 0u) ? 1.f : -1.f;

	w.y *= (coord.x == 0u) ? (1.f - t.x) : t.x;
	w.y *= (coord.z == 0u) ? (1.f - t.z * t.z) : t.z * (2.f - t.z);
	w.y *= (coord.y == 0u) ? 1.f : -1.f;

	w.z *= (coord.x == 0u) ? (1.f - t.x) : t.x;
	w.z *= (coord.y == 0u) ? (1.f - t.y) : t.y;
	w.z *= (coord.z == 0u) ? 1.f : -1.f;

	return w;
}


vec3 ComputeSmoothGI(utexture2DArray IrradianceField, texture2DArray IrradianceGradient, texture2DArray FieldDepth, sampler sampLinear, vec3 pos, vec3 coords, vec3 center, vec3 size, vec3 normal, vec3 view, float MinCellAxis, float Bias)
{
	ivec3 texSize = textureSize(IrradianceField, 0).xyz;
	ivec3 depthTexSize = textureSize(FieldDepth, 0).xyz;
	ivec3 numProbes = texSize / ivec3(10, 10, 1);

	vec3 irradiance = 0.f.xxx;

	vec3 st = coords * numProbes - 0.5f.xxx;

	ivec3	iuv = ivec3(floor(st));
	vec3	fuv = fract(st);

	pos += (normal * 0.2f + view * 0.8f) * (0.75f * MinCellAxis) * Bias;

	float w = 0.f;

	for (uint id = 0U; id < 8U; id++)
	{
		ivec3 uv = ivec3(id & 1U, (id >> 1U) & 1U, id >> 2U);
		ivec3 puv = clamp(iuv + uv, ivec3(0), numProbes - ivec3(1));

		vec3 probeToPoint = pos - GetProbePos(puv, center, size, numProbes);
		vec3 dir = normalize(probeToPoint);

		//float wn = max(0.05f, dot(dir, -normal));

		//linw *= wn;

		vec2 pixcoord = puv.xy * 10.f + 1.f.xx;
		pixcoord += EncodeOct(normal) * 8.f;

		vec2 texcoord = pixcoord / texSize.xy;

		vec3 light = InterpolateIrradiance(IrradianceField, sampLinear, vec3(texcoord, puv.z));
		vec3 grad = textureLod(sampler2DArray(IrradianceGradient, sampLinear), vec3(texcoord, puv.z), 0).rgb;

		irradiance += BSplineCoeffs(uv, fuv) * light + dot(BSplineDerivCoeffs(uv, fuv), grad);

		/*float linw = (c.x + s.x * fuv.x) * (c.y + s.y * fuv.y) * (c.z + s.z * fuv.z);

		vec3 probeToPoint = pos - GetProbePos(puv, center, size, numProbes);
		vec3 dir = normalize(probeToPoint);

		float wn = max(0.05f, dot(dir, -normal));

		linw *= wn;

		vec2 pixcoord = puv.xy * 10.f + 1.f.xx;
		pixcoord += EncodeOct(normal) * 8.f;

		vec2 texcoord = pixcoord / texSize.xy;

		vec3 light = InterpolateIrradiance(IrradianceField, sampLinear, vec3(texcoord, puv.z));

		pixcoord = EncodeOct(dir);
		pixcoord = pixcoord * 16.f + puv.xy * 18.f + 1.f.xx;

		texcoord = pixcoord / depthTexSize.xy;

		vec2 depth = textureLod(sampler2DArray(FieldDepth, sampLinear), vec3(texcoord, puv.z), 0).rg;
		float distToProbe = length(probeToPoint);

		float variance = abs(depth.y - depth.x * depth.x);

		float t_sub_mean = distToProbe - depth.x;
		float chebychev = variance / (variance + t_sub_mean * t_sub_mean);

		linw *= (distToProbe <= depth.x) ? 1.f : max(chebychev, 0.f);

		irradiance += pow(light, 2.5f.xxx) * linw;
		w += linw;*/
	}

	//irradiance /= max(1e-3f, w);

	return irradiance;
}



vec3 ComputeGI(utexture2DArray IrradianceField, texture3D SDF, utexture2DArray ProbeMetadata, sampler sampLinear, vec3 pos, vec3 coords, vec3 center, vec3 size, vec3 normal, vec3 view, float MinCellAxis, float Bias)
{
	ivec3 texSize = textureSize(IrradianceField, 0).xyz;
	//ivec3 depthTexSize = textureSize(FieldDepth, 0).xyz;
	ivec3 numProbes = texSize / ivec3(10, 10, 1);

	vec3 irradiance = 0.f.xxx;

	vec3 st = coords * numProbes - 0.5f.xxx;

	ivec3	iuv = ivec3(floor(st));
	vec3	fuv = fract(st);

	pos += 0.05f * normal;//(normal * 0.2f + view * 0.8f) * (0.75f * MinCellAxis) * Bias;

	float w = 0.f;

	for (uint id = 0U; id < 8U; id++)
	{
		ivec3 puv = clamp(iuv + ivec3(id & 1U, (id >> 1U) & 1U, id >> 2U), ivec3(0), numProbes - ivec3(1));

		if ((texelFetch(ProbeMetadata, puv, 0).r & 1) == 0)
			continue;

		uvec3 c = 1U - (uvec3(id, id >> 1U, id >> 2U) & 1U);
		ivec3 s = ivec3(1) - 2 * ivec3(c);

		float linw = (c.x + s.x * fuv.x) * (c.y + s.y * fuv.y) * (c.z + s.z * fuv.z);

		vec3 probeToPoint = pos - GetProbePos(puv, center, size, numProbes);
		vec3 dir = normalize(probeToPoint);

		float wn = max(0.05f, dot(dir, -normal));

		linw *= wn;

		vec2 pixcoord = puv.xy * 10.f + 1.f.xx;
		pixcoord += EncodeOct(normal) * 8.f;

		vec2 texcoord = pixcoord / texSize.xy;

		vec3 light = InterpolateIrradiance(IrradianceField, sampLinear, vec3(texcoord, puv.z));

		linw *= SampleSDFVisibility(SDF, sampLinear, pos, -dir, length(probeToPoint));

		/*pixcoord = EncodeOct(dir);
		pixcoord = pixcoord * 16.f + puv.xy * 18.f + 1.f.xx;

		texcoord = pixcoord / depthTexSize.xy;

		vec2 depth = textureLod(sampler2DArray(FieldDepth, sampLinear), vec3(texcoord, puv.z), 0).rg;
		float distToProbe = length(probeToPoint);

		float variance = abs(depth.y - depth.x * depth.x);
		
		float t_sub_mean = 0.2f * distToProbe - depth.x;
		float chebychev = variance / (variance + t_sub_mean * t_sub_mean);
		
		linw *= (t_sub_mean <= 0.f) ? 1.f : max(chebychev, 0.f);*/

		irradiance += pow(light, 1.f.xxx) * linw;
		w += linw;
	}

	irradiance /= max(1e-3f, w);

	return pow(irradiance, 5.f.xxx);// *irradiance;
}

