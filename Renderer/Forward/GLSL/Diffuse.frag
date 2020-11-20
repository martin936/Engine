// shadertype=glsl

#version 450

in struct VS_OUTPUT
{
	vec3 Normal;
	vec2 Texcoords;

} interp;

layout(location = 0) out vec4 albedo;

layout(location = 0) uniform sampler2D AlbedoTex;

void main( void )
{
	albedo 	= texture(AlbedoTex, interp.Texcoords);
}
