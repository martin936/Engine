#version 450

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

layout(location = 0) in vec4 position[3];
layout(location = 1) in vec3 normal[3];
layout(location = 2) in vec2 texcoord[3];
layout(location = 3) in int layerID[3];
layout(location = 4) in vec3 WorldPos[3];

layout(location = 0) out vec3 outNormal;
layout(location = 1) out vec2 outTexcoord;
layout(location = 2) out vec3 outWorldPos;
layout(location = 3) out vec2 Zcmp;


void main() 
{
	for (uint i = 0; i < 3; i++)
	{
		gl_Position = position[i];
		gl_Layer = layerID[i];
		outNormal = normal[i];
		outTexcoord = texcoord[i];
		outWorldPos = WorldPos[i];
		Zcmp = gl_Position.zw;

		EmitVertex();
	}
}
