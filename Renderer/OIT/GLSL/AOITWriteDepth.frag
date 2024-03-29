#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout (location = 0) in vec2 Texcoord;


layout(binding = 1) uniform texture2D	MaterialTex[];
layout(binding = 2) uniform sampler		samp;


layout (binding = 3, std140) uniform cb3
{
	vec4	Color;

	float	Roughness;
	float	Emissive;
	float	BumpHeight;
	float	Reflectivity;

	float	Metalness;
	float	SSSProfileID;
	float	SSSRadius;
	float	SSSThickness;

	uint 	DiffuseTextureID;
	uint 	NormalTextureID;
	uint 	InfoTextureID;
	uint	SSSTextureID;
};


const float B3x3[9] =
{
    0.f, 0.777778f, 0.333333333f,
    0.666666667f, 0.555555556f, 0.22222222f,
    0.444444444f, 0.111111111f, 0.88888888f
};


void main( void )
{
	float alpha;

	if (DiffuseTextureID == 0xffffffff)
		alpha		= Color.a;
	else
		alpha		= texture(sampler2D(MaterialTex[DiffuseTextureID], samp), Texcoord).a;

	uvec2 pos = (uvec2(gl_FragCoord.xy) + uint(gl_FragCoord.z * 1e4f)) % 3U;

    float x   = B3x3[pos.y * 3 + pos.x];

	if (x > alpha || alpha < 0.02f)
        discard;
}
