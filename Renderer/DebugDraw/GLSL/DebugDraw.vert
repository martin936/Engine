#version 450

layout(location = 0) in vec3 Position;
layout(location = 1) in vec4 Color;

layout(location = 0) out vec4 color;

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
 
void main(void)
{
	color = Color;
	gl_Position = m_ViewProj * vec4(Position, 1.f);
}
