#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout(location= 0) in struct
{
	vec3	Normal;
	vec3	Tangent;
	vec3	Bitangent;
	vec3	WorldPos;
	vec2	Texcoords;
	vec3	CurrPos;
	vec3	LastPos;
} interp;


layout (binding = 1, std140) uniform cb1
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


layout(location = 0) out vec4 AlbedoTarget;
layout(location = 1) out vec4 NormalTarget;
layout(location = 2) out vec4 InfoTarget;
layout(location = 3) out float EmissiveTarget;
layout(location = 4) out precise vec2 Velocity;


layout(binding = 2) uniform texture2D	MaterialTex[];
layout(binding = 3) uniform sampler		samp;


vec3 EncodeNormal(in vec3 v) 
{
	float s;
	s = sign(v.z) * 2.f - 1.f;
	v *= s;

	vec2 p = v.xy * (1.0 / (abs(v.x) + abs(v.y) + v.z));
	return vec3(p.x + p.y, p.x - p.y, s) * 0.5f + 0.5f.xxx;
}



layout(early_fragment_tests) in;
void main( void )
{
	if (DiffuseTextureID == 0xffffffff)
		AlbedoTarget.rgb		= Color.rgb;
	else
		AlbedoTarget.rgb		= texture(sampler2D(MaterialTex[DiffuseTextureID], samp), interp.Texcoords).rgb;

	EmissiveTarget = pow(Emissive * (1.f / 2500.f), 0.25f);

	vec3 normal;
	float roughness;

	if (NormalTextureID == 0xffffffff)
	{
		normal = normalize(interp.Normal);
		roughness = 1.f;
	}

	else
	{
		vec3 VN = normalize(interp.Normal);
		vec3 VT = normalize(interp.Tangent);
		vec3 VB = normalize(interp.Bitangent);

		vec4 normalTex = texture(sampler2D(MaterialTex[NormalTextureID], samp), interp.Texcoords);
		roughness = 1.f - normalTex.a;

		vec3 NTex;
		NTex.xy		= BumpHeight * (normalTex.xy - 0.5f.xx) * vec2(1, -1);
		float fdotz = 1.f - dot(NTex.xy, NTex.xy);
		NTex.z		= sqrt(max(fdotz, 0.f));

		normal = normalize(NTex.z * VN - NTex.x * VT - NTex.y * VB);
	}

	NormalTarget.rga	= vec3(normal.rg, sign(normal.b)) * 0.5f + 0.5f;
	NormalTarget.b		= Roughness * roughness;

	InfoTarget.r		= Metalness;
	InfoTarget.g		= Reflectivity;

	Velocity	= interp.CurrPos.xy / interp.CurrPos.z - interp.LastPos.xy / interp.LastPos.z;
}
