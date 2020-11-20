#version 450

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

layout (location = 0) in vec4 position[3];
layout (location = 1) in int layerID[3];

layout (location = 0) out vec2 texcoords;


void main() 
{
	for (uint i = 0; i < 3; i++)
	{
		gl_Position = position[i];
		gl_Layer = layerID[i];

		texcoords = position[i].xy * 0.5f + 0.5f.xx;

		EmitVertex();
	}
}
