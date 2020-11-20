// shadertype=glsl

#version 450

layout(location = 0) in vec3 Position;

layout (std140) uniform cb0
{
	mat4	WorldViewProj;
};

out struct VS_OUTPUT
{
	vec3	Pos;

} interp;

void main() 
{
	vec4 pos = WorldViewProj * vec4( Position, 1.f );

	interp.Pos = pos.xyz * 0.5f + 0.5f;
    
    gl_Position = pos;
}
