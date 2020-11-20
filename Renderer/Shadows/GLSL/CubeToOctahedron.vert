// shadertype=glsl

#version 450

layout(location = 0) in vec3 Position;

layout(location = 0) out vec4 outPosition;
layout(location = 1) out int layerID;

layout(push_constant) uniform pc0
{
	uint mask;
};


int getIndex()
{
	uint n = 0;
	int i = 0;
	for (i = 0; i < 32; i++)
	{
		if ((mask & (1 << i)) != 0)
			n++;

		if (n == gl_InstanceIndex + 1)
			return i;
	}

	return i;
}


void main() 
{
	layerID = getIndex();

	outPosition = vec4(Position, 1.f);
}