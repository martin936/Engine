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


layout(push_constant) uniform pc0
{
	float dv_norm;
	float NearCam;
	float FarCam;
};


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


vec2 Clip(vec2 cIn, vec2 cMin, vec2 cMax, vec2 cM)
{
    vec2 t = 1.f.xx;

    vec2 diff = sign(cIn - cM) * max(1e-6f.xx, abs(cIn - cM));

    t = clamp(min(t, max((cMax - cM) / diff, (cMin - cM) / diff)), 0.f, 1.f);

    return min(t.r, t.g) * (cIn - cM) + cM;
}


void main(void)
{
	vec2 pixel = 1.f / vec2(64.f, 60.f);

    vec2 grad = interp.ZBounds.zw;

	vec2 pixelPos = gl_FragCoord.xy;

    vec2 dx = Clip(pixelPos.xy * pixel + normalize(grad), floor(pixelPos.xy) * pixel, ceil(pixelPos.xy) * pixel, pixelPos.xy * pixel) - pixelPos.xy * pixel;

    dx *= 2.f;

	int Zmin = -1, Zmax = -1;

	float Zd;
	float delta_Zd = abs(dot(grad, dx));

	float Zd_center = 2.f * FarCam * NearCam / (FarCam + NearCam + (2.f * gl_FragCoord.z - 1.f) * (FarCam - NearCam));

	if (gl_FrontFacing)
		Zd = max(interp.ZBounds.x, interp.ZBounds.x - delta_Zd);
	
	else
		Zd = min(interp.ZBounds.y, interp.ZBounds.y + delta_Zd);

    float Zc = log2((Zd + DISTRIB_OFFSET) / (NearCam + DISTRIB_OFFSET)) / log2((FarCam + DISTRIB_OFFSET) / (NearCam + DISTRIB_OFFSET));
    int FinalZCell = int(clamp(ceil(Zc * 64.f), 0.f, 63.f));

	if (gl_FrontFacing)
		Zmin = FinalZCell;

	else
		Zmax = FinalZCell;

	beginInvocationInterlockARB();

	InsertZ(uvec2(gl_FragCoord.xy), interp.LightID, Zmin, Zmax);

	endInvocationInterlockARB();

	dummy = 1.;
}
