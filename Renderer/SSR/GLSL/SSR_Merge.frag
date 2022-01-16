#version 450
#extension GL_EXT_samplerless_texture_functions : require


layout(binding = 10, std140) uniform cb10
{
	mat4	m_View;
	mat4	m_Proj;
	mat4	m_ViewProj;
	mat4	m_InvView;
	mat4	m_InvViewProj;
	
	mat4	m_LastView;
	mat4	m_LastProj;
	mat4	m_LastViewProj;
	mat4	m_LastInvView;
	mat4	m_LastInvViewProj;

	vec4	m_Eye;
};


layout(push_constant) uniform pc0
{
	uint TemporalOffset;
};

layout(binding = 0) uniform texture2D	DepthMap;
layout(binding = 1) uniform texture2D	NormalMap;
layout(binding = 2) uniform texture2D	InfoMap;
layout(binding = 3) uniform texture2D	AlbedoMap;
layout(binding = 4) uniform texture2D	ColorMap;
layout(binding = 5) uniform texture2D	BRDFMap;
layout(binding = 6) uniform utexture2D	Sobol16;
layout(binding = 7) uniform utexture3D	OwenScrambling16;
layout(binding = 8) uniform utexture3D	OwenRanking16;
layout(binding = 9) uniform sampler sampLinear;

layout (location = 0) out vec4 Color;


float OwenScrambledSobol(uint pixel_i, uint pixel_j, uint sampleIndex, uint sampleDimension)
{
	// wrap arguments
	pixel_i = pixel_i & 127;
	pixel_j = pixel_j & 127;
	sampleIndex = sampleIndex & 255;
	sampleDimension = sampleDimension & 255;

	// xor index based on optimized ranking
	uint rankedSampleIndex = sampleIndex ^ texelFetch(OwenRanking16, ivec3(pixel_i, pixel_j, sampleDimension & 7), 0).r;

	// fetch value in sequence
	uint value = texelFetch(Sobol16, ivec2(sampleDimension, rankedSampleIndex), 0).r;

	// If the dimension is optimized, xor sequence value based on optimized scrambling
	value = value ^ texelFetch(OwenScrambling16, ivec3(pixel_i, pixel_j, sampleDimension & 7), 0).r;

	// convert to float and return
	float v = (0.5f+value)/256.0f;
	return v;
}


vec3 DecodeNormal(in vec3 e) 
{
	e = e * 2.f - 1.f;
	
	vec2 temp = vec2(e.x + e.y, e.x - e.y) * 0.5;
	vec3 v = vec3(temp, 1.0 - abs(temp.x) - abs(temp.y));

	return normalize(v) * sign(e.z);
}


void main( void )
{
	float depth = texelFetch(DepthMap, ivec2(gl_FragCoord.xy), 0).r;

	if (depth == 0.f)
		discard;

	vec2 size = textureSize(DepthMap, 0).xy;

	vec2 texCoords = gl_FragCoord.xy / size;

	vec4 pos = m_InvViewProj * vec4(texCoords.xy * vec2(2.f, -2.f) - vec2(1.f, -1.f), depth, 1.f);
	pos /= pos.w;

	vec4 normalTex		= texelFetch(NormalMap, ivec2(gl_FragCoord.xy), 0);
	vec4 infoTex		= texelFetch(InfoMap, ivec2(gl_FragCoord.xy), 0);
	vec3 albedo			= texelFetch(AlbedoMap, ivec2(gl_FragCoord.xy), 0).rgb;

	vec3 normal			= DecodeNormal(normalTex.rga);
	float roughness		= normalTex.b * normalTex.b;
	vec3 fresnel		= (0.16f * infoTex.g * infoTex.g).xxx;
	float metallicity	= infoTex.r;

	if (metallicity > 0.f)
		fresnel = mix(fresnel, albedo.rgb, metallicity);

	vec3 view		= normalize(pos.xyz - m_Eye.xyz);

	vec4 base		= texelFetch(ColorMap, ivec2(gl_FragCoord.xy), 0);

	float w			= 0.382483f;
	vec3 color		= w * base.rgb;

	float radius	= 0*base.a;

	float angle = 3.141592f * OwenScrambledSobol(uint(gl_FragCoord.x), uint(gl_FragCoord.y), TemporalOffset, 0);
    float cos_a = cos(angle);
	float sin_a = sin(angle);

    vec2 dir	= vec2(sin_a, cos_a);
        
    vec2 offset = dir / size;
    float rng = OwenScrambledSobol(uint(gl_FragCoord.x), uint(gl_FragCoord.y), TemporalOffset, 1);
    texCoords += 0.33f * (rng - 0.5f) * radius * offset.yx;

    if (radius > 0.0f)
    {
		color = w * textureLod(sampler2D(ColorMap, sampLinear), texCoords, 0).rgb;

        float currRadius = 0.33f * radius;
		base = textureLod(sampler2D(ColorMap, sampLinear), texCoords + currRadius * offset, 0);
        float sampleRadius = base.w;
        float w1 = step(currRadius, sampleRadius) * 0.308758f;

        color += w1 * base.rgb;
        w += w1;

        base = textureLod(sampler2D(ColorMap, sampLinear), texCoords - currRadius * offset, 0);
        sampleRadius = base.w;
        w1 = step(currRadius, sampleRadius) * 0.308758f;

        color += w1 * base.rgb;
        w += w1;
    }

    color /= max(1e-3f, w);

	vec2 brdf		= textureLod(sampler2D(BRDFMap, sampLinear), vec2(dot(-view, normal), roughness), 0.f).rg;

	Color.rgb = (brdf.x * fresnel + brdf.y) * color;
}