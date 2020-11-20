// shadertype=glsl

#version 450

layout(location = 0) in vec4 position;


layout(std140) uniform cb0
{
	mat4 ViewProj;
};


out struct PS_INPUT
{
	vec3	WorldPos;
	float	WorldRadius;
} interp;


void main()
{
	gl_Position = ViewProj*vec4(position.xyz, 1.f);
	gl_PointSize = min(15.f + position.w * 20.f, 30.f);

	interp.WorldPos = position.xyz;
	interp.WorldRadius = 0.1f;
}
