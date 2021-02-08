#version 450

layout(location = 0) in vec3	Position;
layout(location = 1) in vec3	Normal;
layout(location = 2) in vec2	Texcoord;

layout(location = 0) out vec3	WorldPos;
layout(location = 1) out vec3	WorldNormal;
layout(location = 2) out vec2	Texc0;
layout(location = 3) out int	InstanceId;


layout(push_constant) uniform pc0
{
	vec4	Center;
	vec4	Size;
    vec4    TextureSize;
};


void main() 
{
	WorldPos	= Position - Center.xyz + 0.5f * Size.xyz;
	InstanceId	= gl_InstanceIndex;

	Texc0		= Texcoord;
	Texc0.y		= 1.f - Texc0.y;

	WorldNormal = Normal;
}
