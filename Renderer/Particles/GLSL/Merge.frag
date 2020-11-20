// shadertype=glsl

#version 450

layout(location = 0) out vec4 albedo;
layout(location = 1) out vec4 normal;
layout(location = 2) out vec4 info;

layout(std140) uniform cb0
{
	mat4 ViewProj;
};

in struct PS_INPUT
{
	vec2 uv;
} interp;


layout(location = 0) uniform sampler2D Base;
layout(location = 1) uniform sampler2D Depth;

void main()
{
	vec4 NormalTex = texture(Normal, interp.uv);

	if (NormalTex.a < 0.95f)
		discard;

	albedo = vec4(0.8f, 0.2f, 0.2f, 1.f);
	normal = vec4(NormalTex.rgb, 1.f);
	info = vec4(1.f, 0.3f, 0.f, 1.f);

	gl_FragDepth = texture(Depth, interp.uv).r;
}
