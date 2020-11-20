// shadertype=glsl

#version 450

layout(triangles, invocations=6) in;
layout(triangle_strip, max_vertices=3) out;

layout (std140) uniform cb0
{
	int nIndex;
};

out int gl_Layer;

void main() 
{
	gl_Layer = gl_InvocationID + 6 * nIndex;

	for (int i = 0; i < 3; i++)
	{
		gl_Position = gl_in[i].gl_Position; 
		EmitVertex();
	}

	EndPrimitive();
}
