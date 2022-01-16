#version 450

layout(location = 0) in vec3	Position;

layout(location = 0) out vec3	WorldPos;


layout(push_constant) uniform pc0
{
	vec4	Center;
	vec4	Size;
};


void main() 
{
	WorldPos	= (Position - Center.xyz) / Size.xyz + 0.5f;

	if (gl_InstanceIndex == 0)
		gl_Position = vec4(WorldPos.xy * 2.f - 1.f, 0.5f, 1.f);

	else if (gl_InstanceIndex == 1)
		gl_Position = vec4(WorldPos.xz * 2.f - 1.f, 0.5f, 1.f);

	else if (gl_InstanceIndex == 2)
		gl_Position = vec4(WorldPos.yz * 2.f - 1.f, 0.5f, 1.f);
}
