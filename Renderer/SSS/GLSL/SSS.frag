// shadertype=glsl

#version 450

in struct PS_INPUT
{
	vec2 uv;
} interp;


layout (std140) uniform cb0
{
	mat4 InvViewProj;
	vec4 Eye;
	vec2 Dir;
	vec2 Pixel;
};

#define DISTANCE_CUTOFF 0.01f


layout(location = 0) uniform sampler2D Irradiance;
layout(location = 1) uniform sampler2D WeightTex;
layout(location = 2) uniform sampler2D ZMap;
layout(location = 3) uniform sampler2D SSSParams;


layout(location = 0) out vec4 Color;


float InterleavedGradientNoise( vec2 seed )
{
	vec3 magic = vec3( 0.06711056f, 0.00583715f, 52.9829189f );
	return fract( magic.z * fract(dot(seed, magic.xy)) );
}

layout(early_fragment_tests) in;
void main(void)
{
	vec4 Params = textureLod(SSSParams, interp.uv, 0.f);

	Color = texture(Irradiance, interp.uv);

	float fZDist = textureLod(ZMap, interp.uv, 0.f).r * 2.f - 1.f;
	vec4 pos = InvViewProj * vec4(interp.uv * 2.f - 1.f, fZDist, 1.f);
	pos /= pos.w;

	float alpha = 0.5f * InterleavedGradientNoise( gl_FragCoord.xy );
	float sin_a = sin(alpha);
	float cos_a = cos(alpha);

	mat2 rot = mat2( cos_a, sin_a, -sin_a, cos_a );

	vec2 dir = rot * Dir;

	float dpos = length(dFdx(pos) * dir.x + dFdy(pos) * dir.y);

	float RadiusWS = Params.b * Params.b;
	int	Index = int(Color.a * 8.f) - 1;

	float radius = RadiusWS / dpos;
	
	if (radius > 1.f && Index >= 0)
	{
		radius *= abs(dot(Pixel, dir));

		vec2 coords;
		vec2 offset;
		vec4 weight;
		vec4 testSample;
		vec3 sum = texture(WeightTex, vec2(0.f, Index / 4.f)).rgb;
		float ZDistOffset = 0.f;
		vec4 PosOffset = 0.f.xxxx;
		vec2 TexCoords;

		Color.rgb *= sum;

		for (int i = -4; i < 0; i++)
		{
			offset = i * radius * dir;
			TexCoords	= interp.uv + offset;

			coords = vec2(abs(i) / 4.f, Index / 4.f);
			weight = texture(WeightTex, coords);

			testSample = texture(Irradiance, TexCoords);

			vec3 w = weight.rgb * (1.f - clamp(abs(testSample.a - Color.a) * 1e8f, 0.f, 1.f));

			Color.rgb += testSample.rgb * w;

			sum += w;
		}

		for (int i = 1; i <= 4; i++)
		{
			offset = i * radius * dir;
			TexCoords	= interp.uv + offset;

			coords = vec2(abs(i) / 4.f, Index / 4.f);
			weight = texture(WeightTex, coords);

			testSample = texture(Irradiance, TexCoords);

			vec3 w = weight.rgb * (1.f - clamp(abs(testSample.a - Color.a) * 1e8f, 0.f, 1.f));

			Color.rgb += testSample.rgb * w;

			sum += w;
		}

		Color.rgb /= sum;
	}	
}
