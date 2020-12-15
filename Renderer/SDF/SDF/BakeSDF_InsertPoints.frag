#version 450
#extension GL_ARB_fragment_shader_interlock : require
#extension GL_EXT_nonuniform_qualifier : require

layout (location = 0) in vec3 WorldPos;
layout (location = 1) in vec3 Normal;
layout (location = 2) in vec2 Texc0;


layout(binding = 0, rgba16ui)	uniform coherent volatile uimage3D	Points;
layout(binding = 1, rgba8)		uniform writeonly image3D			Albedo;

layout(binding = 2) uniform texture2D	MaterialTex[];
layout(binding = 3) uniform sampler		samp;


layout (binding = 4, std140) uniform cb4
{
	vec4	Color;
	vec4	Fresnel;

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
	vec4	Center;
	vec4	Size;
};


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


uvec3 packPosition(vec3 pos)
{
	return uvec3(pos * 65535.f / Size.xyz);
}


vec3 unpackPosition(uvec3 p)
{
	return Size.xyz * p * (1.f / 65535.f);
}


void WritePixel(ivec3 coords, bool inside)
{
	uvec4 p;
	p.xyz = packPosition(WorldPos);

	p.w = 64;

	if (inside)
		p.w |= 128;

	vec4 albedo;

	if (DiffuseTextureID == 0xffffffff)
		albedo		= Color;
	else
		albedo		= texture(sampler2D(MaterialTex[DiffuseTextureID], samp), Texc0);

	if (albedo.a < 0.3f)
		discard;

	albedo.a = pow(Emissive * (1.f / 2500.f), 0.25f);

	imageStore(Points, coords, p);
	imageStore(Albedo, coords, albedo);
}


layout(early_fragment_tests) in;
void main( void )
{
	ivec3 size = imageSize(Points).xyz;

	ivec3 coords = ivec3((WorldPos / Size.xyz) * size);

	beginInvocationInterlockARB();

	uvec4 point = imageLoad(Points, coords);

	precise vec3 cellCenter = ((coords + 0.5f) / size) * Size.xyz;
	precise vec3 p			= WorldPos - cellCenter;

	if (point.w != 0)
	{
		precise vec3 p1 = unpackPosition(point.xyz) - cellCenter;
		
		if (dot(p1, p1) < dot(p, p))
			discard;
	}

	bool inside = dot(p, normalize(Normal)) > 0.f;

	WritePixel(coords, inside);

	endInvocationInterlockARB();
}
