#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout (location = 0) in vec2 Texcoord;


layout(binding = 0) uniform texture2D	MaterialTex[];
layout(binding = 1) uniform sampler		samp;


layout (binding = 2, std140) uniform cb2
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


void main( void )
{
	float alpha;

	if (DiffuseTextureID == 0xffffffff)
		alpha		= Color.a;
	else
		alpha		= texture(sampler2D(MaterialTex[DiffuseTextureID], samp), Texcoord).a;

	if (alpha > 0.5f)
        discard;
}
