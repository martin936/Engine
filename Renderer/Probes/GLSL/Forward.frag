// shadertype=glsl

#version 450

in struct VS_OUTPUT
{
	vec3 position;
	vec3 normal;

} interp;


layout (std140) uniform cb0
{
	mat4	ViewProj;
	vec4	Eye;

	vec4	DiffuseColor;

	vec3	LightDir;
	float	LightPower;

	mat4	ShadowMatrix;
	vec4	ShadowPos;
	vec4	ShadowDir;

	float	NearPlane;
	float	FarPlane;
};

struct SSHProbe
{
	vec4 coeffs[9];
};

layout (std140) uniform cb1
{
	SSHProbe SHProbe[1];
};

layout(location = 0) out vec4 Color;


uniform sampler2DArray ShadowMap;
uniform int ShadowMapIndex;


void BuildMatrix(out mat4 MR, out mat4 MG, out mat4 MB)
{

	MR[0][0] = 0.429043f * SHProbe[0].coeffs[8].r;
	MR[1][1] = -0.429043f * SHProbe[0].coeffs[8].r;
	MR[2][2] = 0.743125f * SHProbe[0].coeffs[6].r;
	MR[3][3] = 0.886227f * SHProbe[0].coeffs[0].r - 0.247708f * SHProbe[0].coeffs[6].r;

	MR[0][1] = MR[1][0] = 0.429043f * SHProbe[0].coeffs[4].r;
	MR[0][2] = MR[2][0] = 0.429043f * SHProbe[0].coeffs[7].r;
	MR[0][3] = MR[3][0] = 0.511664f * SHProbe[0].coeffs[3].r;
	MR[1][2] = MR[2][1] = 0.429043f * SHProbe[0].coeffs[5].r;
	MR[2][3] = MR[3][2] = 0.511664f * SHProbe[0].coeffs[2].r;
	MR[1][3] = MR[3][1] = 0.511664f * SHProbe[0].coeffs[1].r;

	MG[0][0] = 0.429043f * SHProbe[0].coeffs[8].g;
	MG[1][1] = -0.429043f * SHProbe[0].coeffs[8].g;
	MG[2][2] = 0.743125f * SHProbe[0].coeffs[6].g;
	MG[3][3] = 0.886227f * SHProbe[0].coeffs[0].g - 0.247708f * SHProbe[0].coeffs[6].g;

	MG[0][1] = MG[1][0] = 0.429043f * SHProbe[0].coeffs[4].g;
	MG[0][2] = MG[2][0] = 0.429043f * SHProbe[0].coeffs[7].g;
	MG[0][3] = MG[3][0] = 0.511664f * SHProbe[0].coeffs[3].g;
	MG[1][2] = MG[2][1] = 0.429043f * SHProbe[0].coeffs[5].g;
	MG[2][3] = MG[3][2] = 0.511664f * SHProbe[0].coeffs[2].g;
	MG[1][3] = MG[3][1] = 0.511664f * SHProbe[0].coeffs[1].g;

	MB[0][0] = 0.429043f * SHProbe[0].coeffs[8].b;
	MB[1][1] = -0.429043f * SHProbe[0].coeffs[8].b;
	MB[2][2] = 0.743125f * SHProbe[0].coeffs[6].b;
	MB[3][3] = 0.886227f * SHProbe[0].coeffs[0].b - 0.247708f * SHProbe[0].coeffs[6].b;

	MB[0][1] = MB[1][0] = 0.429043f * SHProbe[0].coeffs[4].b;
	MB[0][2] = MB[2][0] = 0.429043f * SHProbe[0].coeffs[7].b;
	MB[0][3] = MB[3][0] = 0.511664f * SHProbe[0].coeffs[3].b;
	MB[1][2] = MB[2][1] = 0.429043f * SHProbe[0].coeffs[5].b;
	MB[2][3] = MB[3][2] = 0.511664f * SHProbe[0].coeffs[2].b;
	MB[1][3] = MB[3][1] = 0.511664f * SHProbe[0].coeffs[1].b;
}



float GetLitFactor()
{
	vec4 shadow_uv = ShadowMatrix * vec4(interp.position, 1.f);
	shadow_uv /= shadow_uv.w;
	shadow_uv.xy = 0.5f*shadow_uv.xy + 0.5f;

	float lit_factor = 0.f;

	float dist = abs(dot( interp.position.xyz - ShadowPos.xyz, LightDir.xyz ));
	float bias = 0.05f;

	float oldz, shadowDepth;

	shadowDepth = textureLod( ShadowMap, vec3(shadow_uv.xy, ShadowMapIndex), 0.f ).r * 2.f - 1.f;

	oldz = shadowDepth * (FarPlane - NearPlane) + NearPlane;

	lit_factor = step(0.f, oldz - dist + bias);

	return lit_factor;
}


void main( void )
{
	vec4 shadow_uv = ShadowMatrix * vec4(interp.position, 1.f);
	shadow_uv /= shadow_uv.w;
	shadow_uv.xyz = 0.5f * shadow_uv.xyz + 0.5f;

	float bias = 1e-4f;

	float lit_factor = GetLitFactor();

	vec3 l = -normalize(LightDir);                                    
	vec3 n = normalize(interp.normal);

	mat4 MR, MG, MB;

	BuildMatrix(MR, MG, MB);

	vec3 light;
	light.r = dot(vec4(n, 1.f), MR * vec4(n, 1.f));
	light.g = dot(vec4(n, 1.f), MG * vec4(n, 1.f));
	light.b = dot(vec4(n, 1.f), MB * vec4(n, 1.f));

	vec3 Ambient = max(vec3(0.f), light);

	Color = DiffuseColor * (LightPower * lit_factor * clamp(dot(l, n), 0.f, 1.f) + vec4(Ambient.xxx, 1.f));
}
