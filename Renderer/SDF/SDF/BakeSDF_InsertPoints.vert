#version 450

layout(location = 0) in vec3 Position;
layout(location = 1) in vec3 Normal;

layout(location = 0) out vec3 WorldPos;
layout(location = 1) out vec3 WorldNormal;


layout(push_constant) uniform pc0
{
	vec4	Center;
	vec4	Size;
};


void main() 
{
	WorldPos	= Position - Center.xyz + 0.5f * Size.xyz;
	WorldNormal = Normal;

	vec3 pos = 2.f * (Position - Center.xyz) / Size.xyz;

	if (gl_InstanceIndex == 0)
		gl_Position = vec4(pos.xy, 0.5f, 1.f);

	else if (gl_InstanceIndex == 1)
		gl_Position = vec4(pos.xz, 0.5f, 1.f);

	else
		gl_Position = vec4(pos.zy, 0.5f, 1.f);
}
