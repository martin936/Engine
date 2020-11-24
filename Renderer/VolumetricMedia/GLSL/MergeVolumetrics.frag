#version 450
#extension GL_EXT_samplerless_texture_functions : require

layout(location = 0) out vec4 color;

layout(binding = 0) uniform texture2D VolumetricData;


void main( void )
{
	color = texelFetch(VolumetricData, ivec2(gl_FragCoord.xy), 0);
}
