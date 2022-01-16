#version 450
#extension GL_EXT_samplerless_texture_functions : require

layout(binding = 0) uniform texture2D	Radiance;
layout(binding = 1) uniform sampler		sampLinear;

layout(location = 0) in vec2 Texc0;

layout(location = 0) out vec4 mipColor;


void main()
{
	vec3 color[4];
	vec4 r;
	vec4 g;
	vec4 b;

	r = textureGather(sampler2D(Radiance, sampLinear), Texc0, 0);
	g = textureGather(sampler2D(Radiance, sampLinear), Texc0, 1);
	b = textureGather(sampler2D(Radiance, sampLinear), Texc0, 2);

	color[0] = vec3(r.x, g.x, b.x);
	color[1] = vec3(r.y, g.y, b.y);
	color[2] = vec3(r.z, g.z, b.z);
	color[3] = vec3(r.w, g.w, b.w);

	float sumW = 0.f;
	vec3 c = 0.f.xxx;

	for (int i = 0; i < 4; i++)
	{
		float w = dot(color[i], 1.f.xxx) == 0.f ? 0.f : 1.f;
		c += color[i] * w;

		sumW += w;
	}

	mipColor = vec4(c / max(1.f, sumW), dot(c, 1.f.xxx) == 0.f ? 0.f : 1.f);
}
