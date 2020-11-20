#version 450

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

layout (location = 0) in vec4 position[3];
layout (location = 1) in int layerID[3];
layout (location = 2) in vec2 texc0[3];

layout (location = 0) out vec3 WorldPos;
layout (location = 1) out vec2 Texcoord;
layout (location = 2) out vec2 Zcmp;


layout(binding = 0, std140) uniform cb0
{
	mat4	ShadowMatrix[32 * 6];
};


void main() 
{
	for (uint i = 0; i < 3; i++)
	{
		Texcoord = texc0[i];
		WorldPos = position[i].xyz;
		gl_Position = ShadowMatrix[layerID[i]] * position[i];
		gl_Layer = layerID[i];
		Zcmp = gl_Position.zw;

		EmitVertex();
	}
}
