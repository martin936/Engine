// shadertype=glsl

#version 450

#extension GL_ARB_fragment_shader_interlock : enable

in struct VS_OUTPUT
{
	vec3	Pos;

} interp;


struct Node
{
	float	packedSH[15];
	uint	addr;
};


layout(location = 0) out float Color;


layout (std430, binding = 0) buffer VolumetricLightMap
{
	Node LightMap[];
};


layout (std430, binding = 1) buffer GlobalBuffer
{
	uint NextAvailableAddr;
};



uint SubdivNode(uint addr, uvec3 child, vec3 pos, float cellSize)
{
	uint fullAddr = 64U * addr + 16U * child.x + 4U * child.y + child.z;
	uint returnAddr = LightMap[fullAddr].addr;

	bool done = false;

	while(!done)
	{
		if ((returnAddr = atomicCompSwap(LightMap[fullAddr].addr, 0U, 0xFFFFFFFF)) != 0xFFFFFFFF)
		{
			if (returnAddr == 0U)
			{
				returnAddr = atomicAdd(NextAvailableAddr, 1U);
				LightMap[fullAddr].addr = returnAddr;

				for (uint i = 0U; i < 4U; i++)
					for(uint j = 0U; j < 4U; j++)
						for(uint k = 0U; k < 4U; k++)
						{
							vec3 center = pos + (uvec3(i, j, k) + 0.5f.xxx) * cellSize;

							fullAddr = 64U * returnAddr + 16U * i + 4U * j + k;

							LightMap[fullAddr].packedSH[0] = center.x;
							LightMap[fullAddr].packedSH[1] = center.y;
							LightMap[fullAddr].packedSH[2] = center.z;
						}

				
			}

			done = true;
		}
	}

	return returnAddr;
}


void main( void )
{
	vec3 LowerCorner = 0.f.xxx;
	vec3 UpperCorner = 1.f.xxx;

	float cellSize = 0.25f;

	vec3 pos = interp.Pos;

	uint addr = 0U;

	beginInvocationInterlockARB();

	for(uint level = 0U; level < 3U; level++)
	{
		uvec3 coords;

		coords = uvec3(floor(4.f * (pos - LowerCorner) / (UpperCorner - LowerCorner)));

		LowerCorner += coords * cellSize;
		UpperCorner = LowerCorner + cellSize;

		cellSize *= 0.25f;

		addr = SubdivNode(addr, coords, LowerCorner, cellSize);
	}

	endInvocationInterlockARB();

	Color = 1.f;
}
