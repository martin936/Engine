// shadertype=glsl

#version 450

layout(location = 0) out vec4 normal;
layout(location = 1) out float depth;

layout(std140) uniform cb0
{
	mat4 ViewProj;
};


in struct PS_INPUT
{
	vec3	WorldPos;
	float	WorldRadius;
} interp;


void main()
{
	vec3 N;
	N.xz = 2.f*gl_PointCoord-vec2(1.f);
	N.z = -N.z;
	float val = dot(N.xz, N.xz);
  
	if( val > 1.f )
		discard;

	N.y = -sqrt(1.0 - val);
 
	vec4 pixelPos = vec4(interp.WorldPos + N*interp.WorldRadius, 1.0);

	vec4 clipSpacePos = ViewProj * pixelPos;

	depth = 0.5f * clipSpacePos.z / clipSpacePos.w + 0.5f;

	normal 	= vec4(0.5f * N + 0.5f, 1.f - val);
}
