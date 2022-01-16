#version 450
#extension GL_EXT_samplerless_texture_functions : require

layout(location = 0) out vec4 color;

layout(binding = 0) uniform texture2D Chroma;
layout(binding = 1) uniform texture2D Luma;
layout(binding = 2) uniform texture2D Normals;


float EvaluateSHIrradiance(vec3 dir, vec4 SH)
{
    const float c0 = 0.886227;
    const float c1 = 0.511664;

    return max(0.0, c0 * SH.x + 2.0 * c1 * (SH.w * dir.x + SH.y * dir.y + SH.z * dir.z));
}


vec3 DecodeNormal(in vec3 e) 
{
	e = e * 2.f - 1.f;
	
	vec2 temp = vec2(e.x + e.y, e.x - e.y) * 0.5;
	vec3 v = vec3(temp, 1.0 - abs(temp.x) - abs(temp.y));

	return normalize(v) * sign(e.z);
}


void main( void )
{
	vec3 chroma		= texelFetch(Chroma, ivec2(gl_FragCoord.xy), 0).rgb;
	vec4 luma		= texelFetch(Luma, ivec2(gl_FragCoord.xy), 0);
	vec3 normal		= DecodeNormal(texelFetch(Normals, ivec2(gl_FragCoord.xy), 0).rga);

	vec3 irradiance = max(0.f.xxx, chroma * EvaluateSHIrradiance(normal, luma));

	color = vec4(irradiance, 0.f);
}
