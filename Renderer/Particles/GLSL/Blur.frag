// shadertype=glsl

#version 450

layout (std140) uniform cb0
{	
	vec2	dir;
	vec2	pixel;
};

layout(location = 0) out vec4 color;
layout(location = 1) out float depth;


layout(location = 0) uniform sampler2D Base;
layout(location = 1) uniform sampler2D Depth;

#define DISTANCE_CUTOFF 0.1f

in struct PS_INPUT
{
	vec2 uv;
} interp;


float weights[11] = {0.084264, 0.088139, 0.091276, 0.093585, 0.094998, 0.095474, 0.094998, 0.093585, 0.091276, 0.088139, 0.084264};


void main(void)
{
	vec2	TexCoords;
	float	ZDistOffset;
	vec4	PosOffset;
	vec4	SampleTex = texture(Base, interp.uv);

	color = weights[5] * SampleTex;
	depth = weights[5] * texture(Depth, interp.uv).r;

	for (int i = -5; i <= -1; i++)
	{
		TexCoords	= interp.uv + i * pixel * dir;
		SampleTex		= texture(Base, TexCoords);

		color += weights[i+5] * SampleTex;
		depth += weights[i+5] * texture(Depth, TexCoords).r;
	}

	for (int i = 1; i <= 5; i++)
	{
		TexCoords	= interp.uv + i * pixel * dir;
		SampleTex		= texture(Base, TexCoords);

		color += weights[i+5] * SampleTex;
		depth += weights[i+5] * texture(Depth, TexCoords).r;
	}
}

