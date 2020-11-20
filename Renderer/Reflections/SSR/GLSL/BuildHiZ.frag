// shadertype=glsl

#version 450

in struct PS_INPUT
{
	vec2 uv;
} interp;


layout (location = 0) out float Depth;

layout(location = 0) uniform sampler2D ZMap;


uniform int level;

void main( void )
{
	//vec4 z = textureGather(ZMap, interp.uv);
	//Depth = min(z.x, min(z.y, min(z.z, z.w)));

	vec4 z;
	z.x = texelFetch(ZMap, ivec2(gl_FragCoord) * 2, level - 1).r;
	z.y = texelFetch(ZMap, ivec2(gl_FragCoord) * 2 + ivec2(1, 0), level - 1).r;
	z.z = texelFetch(ZMap, ivec2(gl_FragCoord) * 2 + ivec2(1, 1), level - 1).r;
	z.w = texelFetch(ZMap, ivec2(gl_FragCoord) * 2 + ivec2(0, 1), level - 1).r;

	Depth = min(z.x, min(z.y, min(z.z, z.w)));
}
