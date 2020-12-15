#version 450

layout(location = 0) in vec3 Position;
layout(location = 1) in vec3 Normal;
layout(location = 2) in vec2 Texcoord;

layout(location = 0) out vec3 WorldPos;
layout(location = 1) out vec3 WorldNormal;
layout(location = 2) out vec2 Texc0;


layout(push_constant) uniform pc0
{
	vec4	Center;
	vec4	Size;
};


void main() 
{
	WorldPos	= Position - Center.xyz + 0.5f * Size.xyz;
	Texc0		= Texcoord;
	Texc0.y = 1.f - Texc0.y;

	WorldNormal = Normal;

	vec3 pos = 2.f * (Position - Center.xyz) / Size.xyz;

	if (Size.w == 0.f)
		gl_Position = vec4(pos.xy, 0.5f, 1.f);

	else if (Size.w == 1.f)
		gl_Position = vec4(pos.xz, 0.5f, 1.f);

	else
		gl_Position = vec4(pos.zy, 0.5f, 1.f);
}
