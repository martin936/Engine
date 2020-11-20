// shadertype=glsl

#version 450

in struct PS_INPUT
{
	vec2 uv;
} interp;


layout (location = 0) out vec4 Color;

layout(location = 0) uniform sampler2D Base;


uniform uint PassCount;


void main( void )
{
	vec2 size = textureSize(Base, 0);
	vec2 pixelSize = 1.f / size;
	vec2 dir;

	if (PassCount == 0U)
		dir = vec2(1.f,0.f);

	else
		dir = vec2(0.f, 1.f);

	dir *= pixelSize;
	
	vec4 color = vec4(0.44198f.xxx, 1.f)  * textureLod(Base, interp.uv, 0.f);
	color.rgb += 0.27901f * textureLod(Base, interp.uv - dir, 0.f).rgb;
	color.rgb += 0.27901f * textureLod(Base, interp.uv + dir, 0.f).rgb;

	Color	= color;
}
