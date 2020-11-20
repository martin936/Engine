// shadertype=glsl

#version 450

in struct PS_INPUT
{
	vec2 uv;
} interp;


layout (std140) uniform cb0
{
	vec4 pixel;
};


layout( location = 0 ) out vec4 color;

layout(location = 0) uniform sampler2D ZMap;
layout(location = 1) uniform sampler2D Norm;


void main( void )
{
	int i, j;
	vec2 Offset;
	vec3 Hr_Norm = vec3(0.f), Vr_Norm = vec3(0.f);
	float Hr_Depth = 0.f, Vr_Depth = 0.f;
	vec3 Normal = vec3(0.f);
	float Depth = 0.f;

	Offset = vec2(-pixel.x, -pixel.y);

	Normal	= 2.f * texture(Norm, interp.uv + Offset).rgb - 1.f;
	Depth	= texture(ZMap, interp.uv + Offset).r * 2.f - 1.f;

	Hr_Norm += Normal;
	Vr_Norm += Normal;

	Hr_Depth += Depth;
	Vr_Depth += Depth;

	Offset = vec2(0.f, -pixel.y);

	Normal	= 2.f * texture(Norm, interp.uv + Offset).rgb - 1.f;
	Depth	= texture(ZMap, interp.uv + Offset).r * 2.f - 1.f;

	Hr_Norm += Normal * 2.f;
	Hr_Depth += Depth * 2.f;

	Offset = vec2(pixel.x, -pixel.y);

	Normal	= 2.f * texture(Norm, interp.uv + Offset).rgb - 1.f;
	Depth	= texture(ZMap, interp.uv + Offset).r * 2.f - 1.f;

	Hr_Norm += Normal;
	Vr_Norm += -1.f * Normal;

	Hr_Depth += Depth;
	Vr_Depth += -1.f * Depth;

	Offset = vec2(-pixel.x, 0.f);

	Normal	= 2.f * texture(Norm, interp.uv + Offset).rgb - 1.f;
	Depth	= texture(ZMap, interp.uv + Offset).r * 2.f - 1.f;

	Vr_Norm += 2.f * Normal;
	Vr_Depth += 2.f * Depth;

	Offset = vec2(pixel.x, 0.f);

	Normal	= 2.f * texture(Norm, interp.uv + Offset).rgb - 1.f;
	Depth	= texture(ZMap, interp.uv + Offset).r * 2.f - 1.f;

	Vr_Norm += -2.f * Normal;
	Vr_Depth += -2.f * Depth;

	Offset = vec2(-pixel.x, pixel.y);

	Normal	= 2.f * texture(Norm, interp.uv + Offset).rgb - 1.f;
	Depth	= texture(ZMap, interp.uv + Offset).r * 2.f - 1.f;

	Hr_Norm += -1.f * Normal;
	Vr_Norm += Normal;

	Hr_Depth += -1.f * Depth;
	Vr_Depth += Depth;

	Offset = vec2(0.f, pixel.y);

	Normal	= 2.f * texture(Norm, interp.uv + Offset).rgb - 1.f;
	Depth	= texture(ZMap, interp.uv + Offset).r * 2.f - 1.f;

	Hr_Norm += -2.f * Normal;
	Hr_Depth += -2.f * Depth;

	Offset = vec2(pixel.x, pixel.y);

	Normal	= 2.f * texture(Norm, interp.uv + Offset).rgb - 1.f;
	Depth	= texture(ZMap, interp.uv + Offset).r * 2.f - 1.f;

	Hr_Norm += -1.f * Normal;
	Vr_Norm += -1.f * Normal;

	Hr_Depth += -1.f * Depth;
	Vr_Depth += -1.f * Depth;

	float result_Norm = 1.f - pixel.z * sqrt(dot(Hr_Norm, Hr_Norm) + dot(Vr_Norm, Vr_Norm));
	float result_Depth = 1.f - sqrt(dot(Hr_Depth, Hr_Depth) + dot(Vr_Depth, Vr_Depth));

	float result = 1e8f * result_Norm;

	color = vec4(result);
}
