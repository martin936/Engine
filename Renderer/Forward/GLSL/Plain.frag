// shadertype=glsl

#version 450

in struct VS_OUTPUT
{
	vec3 Normal;

} interp;


layout(location = 0) out vec4 albedo;


void main( void )
{
	albedo 	= 1.f.xxxx;
}
