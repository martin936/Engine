// shadertype=glsl

#version 450

layout(location = 0) in vec3 Position;
layout(location = 1) in vec2 Texcoord;

layout(location = 0) out vec4 outPosition;
layout(location = 1) out int layerID;
layout(location = 2) out vec2 texc0;


layout(push_constant) uniform pc0
{
	mat3x4	ModelMatrix;
	uint	mask[6];
};


int getIndex()
{
	uint n = 0;
	int i = 0, j = 0;

	for (j = 0; j < 6; j++)
	{
		for (i = 0; i < 32; i++)
		{
			if ((mask[j] & (1 << i)) != 0)
				n++;

			if (n == gl_InstanceIndex + 1)
				return 32 * j + i;
		}
	}

	return i;
}


void main() 
{
	layerID = getIndex();

	outPosition = vec4(vec4(Position, 1.f) * ModelMatrix, 1.f);

	texc0	= Texcoord;
	texc0.y	= 1.f - Texcoord.y;
}
