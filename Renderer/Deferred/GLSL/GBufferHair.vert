// shadertype=glsl

#version 450

layout(location = 0) in vec3 Position;
layout(location = 1) in vec3 Normal;
layout(location = 2) in vec3 Tangent;
layout(location = 3) in vec3 Bitangent;
layout(location = 4) in vec2 Texcoord;
//layout(location = 5) in vec4 Color;

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


layout(push_constant) uniform pc0
{
	mat3x4	m_ModelMatrix;
	mat3x4	m_LastModelMatrix;
};


layout(location= 0) out struct
{
	vec3			Normal;
	vec3			Tangent;
	vec3			Bitangent;
	vec3			WorldPos;
	vec2			Texcoords;
	precise vec3	CurrPos;
	precise vec3	LastPos;
	vec4			VisibilitySH;
} interp;


void RotateSH(inout vec4 SH, in mat3 World)
{
    vec3 sh_in = vec3(SH.w, SH.z, -SH.y);

    SH.y = dot(vec3(World[1][0], World[1][1], World[1][2]), sh_in);
    SH.z = dot(vec3(World[2][0], World[2][1], World[2][2]), sh_in);
    SH.z = dot(vec3(World[0][0], World[0][1], World[0][2]), sh_in);
}


void main() 
{
	vec4 pos			= m_ViewProj * vec4(vec4(Position, 1.f) * m_ModelMatrix, 1.f);
	vec4 lastPos		= m_LastViewProj * vec4(vec4(Position, 1.f) * m_LastModelMatrix, 1.f);

	interp.CurrPos		= pos.xyw * vec3(0.5f, -0.5f, 1.f);
	interp.LastPos		= lastPos.xyw * vec3(0.5f, -0.5f, 1.f);

	pos.xy				+= m_CameraOffset.xy * pos.w;

	gl_Position			= pos;

	interp.Normal		= Normal * mat3(m_ModelMatrix);
	interp.Tangent		= Tangent * mat3(m_ModelMatrix);
	interp.Bitangent	= -cross(interp.Tangent, interp.Normal) * mat3(m_ModelMatrix);
	interp.WorldPos		= Position;

	vec4 VisibilitySH	= vec4(1, 0, 0, 0);//Color * (1.f / 64.f) - 1.f;
	RotateSH(VisibilitySH, mat3(m_ModelMatrix));

	interp.VisibilitySH = VisibilitySH;

	interp.Texcoords	= Texcoord;
	interp.Texcoords.y	= 1.f - Texcoord.y;
}
