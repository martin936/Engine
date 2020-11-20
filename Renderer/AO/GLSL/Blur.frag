// shadertype=glsl

#version 450

layout (std140) uniform cb0
{	
	mat4	InvViewProj;
	vec4	CamDir;
	vec2	dir;
	vec2	pixel;
};

layout(location = 0) out vec4 color;

layout(location = 0) uniform sampler2D Base;
layout(location = 1) uniform sampler2D ZMap;

#define DISTANCE_CUTOFF 0.1f

in struct PS_INPUT
{
	vec2 uv;
} interp;


float weights[5] = {0.06136f, 0.24477f, 0.38774f, 0.24477f, 0.06136f};


void main(void)
{
	float NormFactor = weights[2];
	color = weights[2] * texture(Base, interp.uv);
	float ZDist = 2.f * texture(ZMap, interp.uv).r - 1.f;
	vec4  pos	= InvViewProj * vec4(2.f*interp.uv - 1.f , ZDist, 1.f);
	pos		/= pos.w;

	vec2	TexCoords;
	float	ZDistOffset;
	vec4	PosOffset;

	for (int i = -2; i <= -1; i++)
	{
		TexCoords	= interp.uv + i * pixel * dir;
		ZDistOffset = 2.f * texture(ZMap, TexCoords).r - 1.f;
		PosOffset	= InvViewProj * vec4(2.f*TexCoords - 1.f , ZDistOffset, 1.f);
		PosOffset	/= PosOffset.w;

		float factor = step(abs(dot(pos.xyz - PosOffset.xyz, CamDir.xyz)), DISTANCE_CUTOFF);

		color += weights[i+2] * texture(Base, TexCoords) * factor;
		NormFactor += weights[i+2] * factor;
	}

	for (int i = 1; i <= 2; i++)
	{
		TexCoords	= interp.uv + i * pixel * dir;
		ZDistOffset = 2.f * texture(ZMap, TexCoords).r - 1.f;
		PosOffset	= InvViewProj * vec4(2.f*TexCoords - 1.f , ZDistOffset, 1.f);
		PosOffset	/= PosOffset.w;

		float factor = step(abs(dot(pos.xyz - PosOffset.xyz, CamDir.xyz)), DISTANCE_CUTOFF);

		color += weights[i+2] * texture(Base, TexCoords) * factor;
		NormFactor += weights[i+2] * factor;
	}
	
	color /= NormFactor;
}

