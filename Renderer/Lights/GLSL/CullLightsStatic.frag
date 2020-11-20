#version 450
#extension GL_ARB_fragment_shader_interlock : enable
#extension GL_GOOGLE_include_directive	: require

#include "Clustered.glsl"


layout(location = 0) flat in struct
{
	uint LightID;
	vec4 ZBounds;

} interp;


layout(binding = 0, r32ui) uniform coherent uimage2D LinkedListHead;
layout(binding = 1, std430) coherent buffer buf0
{
	uint	nextAddr;
	uvec2	Nodes[];
};

layout(location = 0) out float dummy;


uint AllocateNode()
{
	return atomicAdd(nextAddr, 1);
}



uvec2 newNode(uint lightID, int Zmin, int Zmax)
{
	uvec2 node;
	node.x = lightID << 16;

	if (Zmin >= 0)
		node.x |= (Zmin & 0xff) << 8;

	if (Zmax >= 0)
		node.x |= Zmax & 0xff;

	node.y = 0xffffffff;

	return node;
}



void InsertZ(uvec2 pixel, uint lightID, int Zmin, int Zmax)
{
	uint head = imageLoad(LinkedListHead, ivec2(pixel)).r;

	if (head == 0xffffffff)
	{
		head = AllocateNode();

		Nodes[head] = newNode(lightID, Zmin, Zmax);
		imageStore(LinkedListHead, ivec2(pixel), uvec4(head, 0, 0, 0));
	}

	else
	{
		uvec2 node;
		bool done = false;
		bool found = false;

		while(!done)
		{
			node = Nodes[head];

			if ((node.x >> 16) == lightID)
			{
				if (Zmin >= 0)
				node.x |= (Zmin & 0xff) << 8;

				if (Zmax >= 0)
					node.x |= Zmax & 0xff;

				Nodes[head].x = node.x;
			
				found = true;
				done = true;
			}

			if (node.y == 0xffffffff)
				done = true;

			else
				head = node.y;
		}

		if (!found)
		{
			node.y = AllocateNode();
			Nodes[node.y] = newNode(lightID, Zmin, Zmax);

			Nodes[head].y = node.y;
		}
	}
}


void main(void)
{
	vec2 pixel = 1.f / vec2(64.f, 60.f);

    vec2 grad = interp.ZBounds.zw;

	vec2 pixelPos = gl_FragCoord.xy;

    float dz = dot(0.5f.xx, abs(interp.ZBounds.zw));

	int Zmin = -1, Zmax = -1;
	float Z;

	if (gl_FrontFacing)
		Z = interp.ZBounds.x;//max(interp.ZBounds.x, interp.ZBounds.x - dz);
	
	else
		Z = interp.ZBounds.y;//min(interp.ZBounds.y, interp.ZBounds.y + dz);

    int FinalZCell = int(clamp(ceil(Z * 16.f), 0.f, 15.f));

	if (gl_FrontFacing)
		Zmin = FinalZCell;

	else
		Zmax = FinalZCell;

	beginInvocationInterlockARB();

	InsertZ(uvec2(gl_FragCoord.xy), interp.LightID, Zmin, Zmax);

	endInvocationInterlockARB();

	dummy = 1.;
}
