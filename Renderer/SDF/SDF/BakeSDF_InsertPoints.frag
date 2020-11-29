#version 450
#extension GL_ARB_fragment_shader_interlock : require

layout (location = 0) in vec3 WorldPos;
layout (location = 1) in vec3 Normal;


layout(binding = 0, rgba32f) uniform image3D	Points;


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


void WritePixel(ivec3 coords)
{
	vec4 p;
	p.xyz = WorldPos;
	p.w = uintBitsToFloat(packHalf2x16(EncodeOct(Normal)));

	imageStore(Points, coords, p);
}


layout(early_fragment_tests) in;
void main( void )
{
	ivec3 size = imageSize(Points).xyz;

	ivec3 coords = ivec3((WorldPos / Size.xyz) * size);

	beginInvocationInterlockARB();

	vec4 point = imageLoad(Points, coords);

	if (floatBitsToUint(point.w) != 0)
	{
		vec3 cellCenter = ((coords + 0.5f) / size) * Size.xyz;

		vec3 p1 = point.xyz - cellCenter;
		vec3 p2 = WorldPos - cellCenter;
		
		if (dot(p1, p1) < dot(p2, p2))
			discard;
	}

	WritePixel(coords);

	endInvocationInterlockARB();
}
