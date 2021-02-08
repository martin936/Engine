// shadertype=glsl

#version 450

layout(location = 0) in vec2 texcoord;

layout (binding = 2, std140) uniform cb2
{
	mat4	m_View;
	mat4	m_Proj;
	mat4	m_ViewProj;
	mat4	m_InvView;
	mat4	m_InvViewProj;

	mat4	m_LastView;
	mat4	m_LastProj;
	mat4	m_LastViewProj;
	mat4	m_LastInvView;
	mat4	m_LastInvViewProj;

	vec4	m_Eye;
	vec4	m_CameraOffset;
};

layout(location = 0) out vec4 Albedo;
layout(location = 1) out vec4 Normal;
layout(location = 2) out vec4 Info;
layout(location = 3) out vec2 Velocity;

layout(binding = 0) uniform textureCube Skybox;
layout(binding = 1) uniform sampler		samp;


void main( void )
{
	vec4 pos		= m_InvViewProj * vec4(2.f * texcoord - 1.f, 0.5f, 1.f);
	vec4 lastPos	= m_LastViewProj * pos;
	pos /= pos.w;

	vec3 view = normalize(pos.xyz - m_Eye.xyz);
	view.xy *= -1.f;

	Albedo.rgb = texture(samplerCube(Skybox, samp), view.xzy).rgb;
	Albedo.a = 0.f;

	Normal	= 0.f.xxxx;
	Info	= 0.f.xxxx;

	vec2 lastuv = (lastPos.xy / lastPos.w) * 0.5f + 0.5f;

	Velocity	= texcoord - lastuv;
	Velocity	= sign(Velocity) * sqrt(abs(Velocity));
}
