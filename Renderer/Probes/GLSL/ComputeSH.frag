// shadertype=glsl

#version 450

layout (location = 0) out float AlmYlm;

uniform samplerCube CubeMap[1];
uniform samplerCubeArray CubeMapArray[1];

#define EnvironmentMap	CubeMap[0]
#define SHLookup		CubeMapArray[0]

void main( void )
{
	float nIndex = gl_FragCoord.x * 3 + gl_FragCoord.y;
	vec3 view;
	vec2 uv;
	vec4 sum = vec4(0.f);

	float fNormalizationFactor = 1.f / (6.f * 512.f * 512.f);

	for (int i = 0; i < 512; i++)
	{
		for(int j = 0; j < 512; j++)
		{
			uv = (vec2(i, j) / 512.f) *- 2.f - 1.f;

			view = normalize(vec3(1.f, -uv.y, -uv.x));
			sum += texture(EnvironmentMap, view) * texture(SHLookup, vec4(view, nIndex));

			view = normalize(vec3(-1.f, -uv.y, uv.x));
			sum += texture(EnvironmentMap, view) * texture(SHLookup, vec4(view, nIndex));

			view = normalize(vec3(uv.x, 1.f, uv.y));
			sum += texture(EnvironmentMap, view) * texture(SHLookup, vec4(view, nIndex));

			view = normalize(vec3(uv.x, -1.f, -uv.y));
			sum += texture(EnvironmentMap, view) * texture(SHLookup, vec4(view, nIndex));

			view = normalize(vec3(uv.x, -uv.y, 1.f));
			sum += texture(EnvironmentMap, view) * texture(SHLookup, vec4(view, nIndex));

			view = normalize(vec3(-uv.x, -uv.y, -1.f));
			sum += texture(EnvironmentMap, view) * texture(SHLookup, vec4(view, nIndex));
		}
	}

	AlmYlm = sum.r * fNormalizationFactor;
}
