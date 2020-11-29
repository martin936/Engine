#version 450

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

layout (location = 0) in vec4 position[3];
layout(location = 1)  in vec3 WorldPos[3];
layout(location = 2)  in vec3 WorldNormal[3];
layout(location = 3)  in int layerID[3];

layout (location = 0) out vec3 Pos;
layout (location = 1) out vec3 Normal;


void main() 
{
	for (uint i = 0; i < 3; i++)
	{
		Pos			= WorldPos[i].xyz;
		Normal		= WorldNormal[i].xyz;

		gl_Position = position[i];
		gl_Layer	= layerID[i];

		EmitVertex();
	}
}
