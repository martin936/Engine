#version 450
#extension GL_EXT_samplerless_texture_functions : require

layout(location = 0) in vec3 Position;

layout(location = 0) out vec2	Texcoord;
layout(location = 1) out ivec3	probeID;

layout(binding = 1) uniform itexture2DArray	probeMetadata;
layout(binding = 2) uniform sampler			samp;


layout (binding = 3, std140) uniform cb3
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
};


layout(push_constant) uniform pc0
{
	vec4	m_Up;
	vec4	m_Right;
	vec4	Center;
	vec4	Size;
	ivec4	numProbes;
};


ivec3 GetProbeCoords()
{
	int index = int(gl_InstanceIndex);
	ivec3 coords;
	coords.z = index / (numProbes.x * numProbes.y);
	coords.y = (index - coords.z * numProbes.x * numProbes.y) / numProbes.x;
	coords.x = index - coords.z * numProbes.x * numProbes.y - coords.y * numProbes.x;

	return coords;
}


vec3 GetProbePos(in ivec3 coords, out bool enabled)
{
	ivec3 size = textureSize(probeMetadata, 0).xyz;

	vec3 cellCenter = Center.xyz + vec3((coords.x + 0.5f) / size.x - 0.5f, (coords.y + 0.5f) / size.y - 0.5f, (coords.z + 0.5f) / size.z - 0.5f) * Size.xyz;
	vec3 cellSize	= 0.5f * Size.xyz / size;

	ivec4 probeData = texelFetch(probeMetadata, coords, 0);
	vec3 relativePos = probeData.xyz * (1.f / 127.f);

	enabled = (probeData.w & 1) == 1;

	return cellCenter + cellSize * relativePos;
}


void main() 
{
	probeID = GetProbeCoords();

	bool enabled;
	vec3 pos = GetProbePos(probeID, enabled) + 0.1f * Position.x * m_Right.xyz + 0.1f * Position.y * m_Up.xyz;

	Texcoord = Position.xy * 0.5f + 0.5f;
	   
	gl_Position = m_ViewProj * vec4(pos, 1.f);
}
