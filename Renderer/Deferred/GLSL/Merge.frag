#version 450
#extension GL_EXT_samplerless_texture_functions : require

layout(location = 0) out vec4 color;

layout(binding = 0) uniform texture2D Albedo;
layout(binding = 1) uniform texture2D Diffuse;
layout(binding = 2) uniform texture2D Specular;

void main( void )
{
	vec4 albedo		= texelFetch(Albedo, ivec2(gl_FragCoord.xy), 0);
	vec3 diffuse	= texelFetch(Diffuse, ivec2(gl_FragCoord.xy), 0).rgb;
	vec3 specular	= texelFetch(Specular, ivec2(gl_FragCoord.xy), 0).rgb;
	float Emissive = pow(albedo.a, 4.f) * 2500.f;

	color.rgb = albedo.rgb * (diffuse + Emissive.xxx) + specular;
	color.a = 0.f;
}
