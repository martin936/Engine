// shadertype=glsl

#version 450

layout(location = 0) in vec3 Position;
layout(location = 1) in vec3 Normal;
layout(location = 2) in vec2 Texcoord;

layout(location = 0) out vec4 outPosition;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec2 outTexcoord;
layout(location = 3) out int layerID;
layout(location = 4) out vec3 WorldPos;


layout(binding = 0, std140) uniform cb0
{
	mat4	ViewProj[64 * 6];
};


layout(push_constant) uniform pc0
{
	uint mask[12];
	uint matID;
	float Near;
	float Far;
};


int getIndex()
{
	uint n = 0;
	int i = 0, j = 0;

	for (j = 0; j < 12; j++)
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
	layerID = gl_InstanceIndex;//getIndex();

	WorldPos = Position;
	outPosition = ViewProj[layerID] * vec4(Position, 1.f);
	outNormal = Normal;
	outTexcoord = Texcoord;
	outTexcoord.y = 1.f - Texcoord.y;
}
