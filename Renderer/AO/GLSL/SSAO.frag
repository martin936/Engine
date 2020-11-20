// shadertype=glsl

#version 450

in struct PS_INPUT
{
	vec2 uv;
} interp;


layout (std140) uniform cb0
{
	mat4	InvViewProj;
	mat4	ViewProj;
	mat4	LastViewProj;
	vec4	Kernel[16];
	float	FrameTick;
	float	KernelSize;
	vec2	Padding;
};

layout(location = 0) out vec4 BlendedSSAO;
layout(location = 1) out vec4 SSAO;


layout(location = 0) uniform sampler2D ZTex;
layout(location = 1) uniform sampler2D LastSSAOTex;
layout(location = 2) uniform sampler2D NormalTex;

#define RADIUS 1.5f
#define SAMPLES 16

float lerp( float a, float b, float t )
{
	return a*(1.f-t) + b*t;
}

vec3 lerp( vec3 a, vec3 b, float t )
{
	return a*(1.f-t) + b*t;
}

vec4 lerp( vec4 a, vec4 b, float t )
{
	return a*(1.f-t) + b*t;
}

float InterleavedGradientNoise( vec2 seed )
{
	vec3 magic = vec3( 0.06711056f, 0.00583715f, 52.9829189f );
	return fract( magic.z * fract(dot(seed, magic.xy)) );
}


void main( )
{
	SSAO = vec4(0.f);
	BlendedSSAO = vec4(0.f);

	float	ZDist 		= 2.f * texture( ZTex, interp.uv ).r - 1.f;
	vec4	NormalValue	= texture(NormalTex, interp.uv);

	vec3 n = normalize( 2.f * ( NormalValue.rgb - 0.5f ) );

	vec4 pos = InvViewProj * vec4( 2.f*interp.uv - 1.f , ZDist, 1.f );
	pos /= pos.w;

	vec4 LastProj = LastViewProj * pos;
	LastProj = LastProj / LastProj.w * 0.5f + 0.5f;

	float BlendFactor = 0.5f;

	if (LastProj.x < 0.f || LastProj.x > 1.f || LastProj.y < 0.f || LastProj.y > 1.f)
		BlendFactor = 0.f;

	vec4 LastSSAO = texture(LastSSAOTex, LastProj.xy);

	float alpha = 2.f * 3.141592f * (0.5f * InterleavedGradientNoise( gl_FragCoord.xy ) + FrameTick);

	vec2 rotation = vec2(cos(alpha), sin(alpha));

	mat3 rot = mat3(rotation.x, rotation.y, 0.f, 
					-rotation.y, rotation.x, 0.f, 
					0.f, 0.f, 1.f);

	vec3 tangent = normalize(lerp(vec3(0.f, 0.f, 1.f) - n * n.z, vec3(1.f, 0.f, 0.f), clamp((abs(n.z) - 0.9f)*100000.f, 0.f, 1.f)));
	vec3 bitangent = cross(n, tangent);
	mat3 tbn = KernelSize * mat3(tangent, bitangent, n) * rot;

	float fOcclusion = 0.f;
	vec4 PosOffset;
	vec4 PosProj;
	int i;

	vec4 color = vec4(0.f);

	for (i = 0; i < SAMPLES; i++)
	{
		PosOffset = vec4(tbn * Kernel[i].xyz, 0.f);
		PosOffset = pos + RADIUS * PosOffset;

		PosProj = ViewProj * PosOffset;
		PosProj = PosProj / PosProj.w * 0.5f + 0.5f;

		float	NewZDist 	= 2.f * texture( ZTex, PosProj.xy ).r - 1.f;
		
		PosOffset	= InvViewProj * vec4( 2.f*PosProj.xy - 1.f , NewZDist, 1.f );
		PosOffset	/= PosOffset.w;

		float factor = step(0.f, PosProj.x * (1.f- PosProj.x)) * step(0.f, PosProj.y * (1.f- PosProj.y));
		factor *= step(NewZDist, 2.f * PosProj.z - 1.f);

		fOcclusion += factor * step(length(pos.xyz - PosOffset.xyz), RADIUS); 
	}

	color /= max(1.f, fOcclusion);
	fOcclusion /= SAMPLES;

	SSAO = vec4(1.f - fOcclusion);
	BlendedSSAO = lerp(SSAO, LastSSAO, BlendFactor);
}
