#version 450
#extension GL_EXT_samplerless_texture_functions : require

layout(binding = 0) uniform texture2D Source;

void main( void )
{
	gl_FragDepth = texelFetch(Source, ivec2(gl_FragCoord.xy), 0).r;
}
