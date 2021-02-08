#version 450

layout(location = 0) in vec3	Position;

layout(location = 0) out vec3	WorldPos;
layout(location = 1) out int	InstanceId;


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
}
