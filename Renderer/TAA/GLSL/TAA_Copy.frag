#version 450
#extension GL_EXT_samplerless_texture_functions : require

layout(location = 0) out vec4 color1;
layout(location = 1) out vec4 color2;

layout(binding = 0) uniform texture2D Source;

void main( void )
{
	color1 = texelFetch(Source, ivec2(gl_FragCoord.xy), 0);
	color2 = color1;
}
