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


layout(push_constant) uniform pc0
{
	vec4 m_Eye;
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



void main( void )
{
	vec4 albedo;

	if (DiffuseTextureID == 0xffffffff)
		albedo		= Color;
	else
		albedo		= texture(sampler2D(MaterialTex[DiffuseTextureID], samp), interp.Texcoords);

	if (albedo.a < 1.f)
		discard;

	AlbedoTarget.rgb	= albedo.rgb;
	
	EmissiveTarget		= pow(Emissive * (1.f / 2500.f), 0.25f);

	vec3 normal;
	float roughness;

	vec3 pos	= interp.WorldPos;
	vec3 view	= normalize(m_Eye.xyz - pos.xyz);

	vec3 VN		= normalize(interp.Normal);

	VN *= sign(dot(view, VN));

	if (NormalTextureID == 0xffffffff)
	{
		normal = VN;
		roughness = 1.f;
	}

	else
	{
		vec3 VT = normalize(interp.Tangent);
		vec3 VB = normalize(interp.Bitangent);

		vec4 normalTex = texture(sampler2D(MaterialTex[NormalTextureID], samp), interp.Texcoords);
		roughness = 1.f - normalTex.a;

		vec3 NTex;
		NTex.xy		= BumpHeight * (normalTex.xy - 0.5f.xx);
		float fdotz = 1.f - dot(NTex.xy, NTex.xy);
		NTex.z		= sqrt(max(fdotz, 0.f));

		normal = normalize(NTex.z * VN - NTex.x * VT - NTex.y * VB);
	}

	NormalTarget.rga	= EncodeNormal(normal);
	NormalTarget.b		= Roughness * roughness;

	InfoTarget.r		= Metalness;
	InfoTarget.g		= Reflectivity;

	Velocity	= interp.CurrPos.xy / interp.CurrPos.z - interp.LastPos.xy / interp.LastPos.z;
}
