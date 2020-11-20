// shadertype=glsl

#version 450

layout (location = 0) out vec4 Color;

layout (std140) uniform cb0
{
	vec4	ModulateColor;
};


layout(location = 0) uniform sampler2D Sprite;

in vec2 Texc0;

void main()
{
	vec4 spriteColor = texture(Sprite, Texc0);

	spriteColor.rgb /= max(1e-3f, spriteColor.a);

	Color = pow(ModulateColor, vec4(vec3(2.2f), 1.f)) * spriteColor;
}
