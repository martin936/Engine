// shadertype=glsl

#version 450

layout(location = 0) in vec3 Position;
layout(location = 1) in vec3 Normal;
layout(location = 2) in vec3 Tangent;
layout(location = 3) in vec3 Bitangent;
layout(location = 4) in vec2 Texcoord;

layout (binding = 0, std140) uniform cb0
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

layout(location= 0) out struct
{
	vec3	Normal;
	vec3	Tangent;
	vec3	Bitangent;
	vec3	WorldPos;
	vec2	Texcoords;
	precise vec3	CurrPos;
	precise vec3	LastPos;
} interp;

void main() 
{
	vec4 pos			= m_ViewProj * vec4( Position, 1.f );
	vec4 lastPos		= m_LastViewProj * vec4( Position, 1.f );

	interp.CurrPos		= pos.xyw * vec3(0.5f, -0.5f, 1.f);
	interp.LastPos		= lastPos.xyw * vec3(0.5f, -0.5f, 1.f);

	pos.xy				+= m_CameraOffset.xy * pos.w;

	gl_Position			= pos;

	interp.Normal		= Normal;
	interp.Tangent		= Tangent;
	interp.Bitangent	= Bitangent;
	interp.WorldPos		= Position;

	interp.Texcoords	= Texcoord;
	interp.Texcoords.y	= 1.f - Texcoord.y;
}
