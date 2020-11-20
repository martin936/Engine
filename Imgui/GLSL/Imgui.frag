#version 450

layout(location = 0) in vec2 tex;
layout(location = 1) in vec4 col;

layout(location = 0) out vec4 color;

layout(binding = 0) uniform texture2D Source;
layout(binding = 1) uniform sampler samp;


void main( void )
{
	color = col * texture(sampler2D(Source, samp), tex);
}
