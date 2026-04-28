// shadertype=glsl

#version 450

layout(location = 0) in vec3 position;

layout(location = 0) out vec2 uv;

void main(void)
{
	gl_Position = vec4(position, 1.f);
	uv = position.xy * 0.5f + 0.5f;
	uv.y = 1.f - uv.y;
}
