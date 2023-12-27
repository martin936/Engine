
#define AOIT_NODE_COUNT 4
#define AOIT_RT_COUNT ((AOIT_NODE_COUNT + 3)/ 4)

// Various constants used by the algorithm
#define AOIT_EMPTY_NODE_DEPTH   (0.)   //(3.40282E38)
#define AOIT_TRANS_BIT_COUNT    (8U)
#define AOIT_MAX_UNNORM_TRANS   ((1U << AOIT_TRANS_BIT_COUNT) - 1U)
#define AOIT_TRANS_MASK         (0xFFFFFFFFU - uint(AOIT_MAX_UNNORM_TRANS))

#define AOIT_TILED_ADDRESSING

struct AOITCtrlSurface
{
	bool clear;
	bool opaque;
	float depth;
};

struct AOITData
{
	precise vec4 depth;
	uvec4 color;
};

struct AOITDepthData
{
	precise vec4 depth[AOIT_RT_COUNT];
};

struct AOITColorData
{
	uvec4  color[AOIT_RT_COUNT];
};


struct AOITNode
{
	float depth;
	float trans;
	uint color;
};


uniform layout(binding = 1, r8ui)		uimage2D	AOITCtrlBuffer;
uniform layout(binding = 2, rgba32ui)	uimage2D	AOITColorDataBuffer;
uniform layout(binding = 3, rgba32f)	image2D		AOITDepthDataBuffer;



// ToRGBE - takes a float RGB value and converts it to a float RGB value with a shared exponent
vec4 AOIT_ToRGBE(vec4 inColor)
{
    float base = max(inColor.r, max(inColor.g, inColor.b));
    int e;
    float m = frexp(base, e);
    return vec4(clamp(inColor.rgb / exp2(e), 0.f, 1.f), e + 127);
}


// FromRGBE takes a float RGB value with a sahred exponent and converts it to a 
//	float RGB value
vec4 AOIT_FromRGBE(vec4 inColor)
{
    return vec4(inColor.rgb * exp2(inColor.a - 127), inColor.a);
}


// UnpackRGBA takes a uint value and converts it to a float4
vec4 UnpackRGBA(uint packedInput)
{
    vec4 unpackedOutput;
    uvec4 p = uvec4((packedInput & 0xFFU),
		(packedInput >> 8U) & 0xFFU,
		(packedInput >> 16U) & 0xFFU,
		(packedInput >> 24U));

    unpackedOutput = vec4(p) / vec4(255, 255, 255, 1.0f);
    return unpackedOutput;
}


// PackRGBA takes a float4 value and packs it into a UINT (8 bits / float)
uint PackRGBA(vec4 unpackedInput)
{
    uvec4 u = uvec4(unpackedInput * vec4(255, 255, 255, 1.0f));
    uint packedOutput = (u.w << 24U) | (u.z << 16U) | (u.y << 8U) | u.x;
    return packedOutput;
}



void AOITInsertFragment(	in float  fragmentDepth,
							in float  fragmentTrans,
							in vec3 fragmentColor,
							inout AOITNode nodeArray[AOIT_NODE_COUNT])
{	
    int i, j;

    float  depth[AOIT_NODE_COUNT + 1];	
    float  trans[AOIT_NODE_COUNT + 1];	 
    uint   color[AOIT_NODE_COUNT + 1];	 

    ///////////////////////////////////////////////////
    // Unpack AOIT data
    ///////////////////////////////////////////////////                   
    for (i = 0; i < AOIT_NODE_COUNT; ++i) 
	{
        depth[i] = nodeArray[i].depth;
        trans[i] = nodeArray[i].trans;
        color[i] = nodeArray[i].color;
    }	
	
    // Find insertion index 
    int index = 0;
    float prevTrans = 255;
    for (i = 0; i < AOIT_NODE_COUNT; ++i) 
	{
        if (fragmentDepth < depth[i]) 
		{
            index++;
            prevTrans = trans[i];
        }
    }

    // Make room for the new fragment. Also composite new fragment with the current curve 
    // (except for the node that represents the new fragment)
    for (i = AOIT_NODE_COUNT - 1; i >= 0; --i) 
	{
        if (index <= i) 
		{
            depth[i + 1] = depth[i];
            trans[i + 1] = trans[i] * fragmentTrans;
            color[i + 1] = color[i];
        }
    }
    
	// Insert new fragment
	const float newFragTrans = fragmentTrans * prevTrans;
	const uint newFragColor = PackRGBA(AOIT_ToRGBE(vec4(fragmentColor * (1 - fragmentTrans), 1)));

	for (i = 0; i <= AOIT_NODE_COUNT; ++i) 
	{
		if (index == i) 
		{
			depth[i] = fragmentDepth;
			trans[i] = newFragTrans;
			color[i] = newFragColor;
		}
	} 

	float EMPTY_NODE = uintBitsToFloat(floatBitsToUint(float(AOIT_EMPTY_NODE_DEPTH)) & uint(AOIT_TRANS_MASK));

	// pack representation if we have too many nodes
    if (uintBitsToFloat(floatBitsToUint(float(depth[AOIT_NODE_COUNT])) & uint(AOIT_TRANS_MASK)) != EMPTY_NODE)
    {
        vec3 toBeRemovedCol = AOIT_FromRGBE(UnpackRGBA(color[AOIT_NODE_COUNT])).rgb;
        vec3 toBeAccumulCol = AOIT_FromRGBE(UnpackRGBA(color[AOIT_NODE_COUNT - 1])).rgb;
        color[AOIT_NODE_COUNT - 1] = PackRGBA(AOIT_ToRGBE(vec4(toBeAccumulCol + toBeRemovedCol * trans[AOIT_NODE_COUNT - 1] / (trans[AOIT_NODE_COUNT - 2]), 1)));
        trans[AOIT_NODE_COUNT - 1] = trans[AOIT_NODE_COUNT];
    }
   
    // Pack AOIT data
    for (i = 0; i < AOIT_NODE_COUNT; ++i) 
	{
        nodeArray[i].depth = depth[i];
        nodeArray[i].trans = trans[i];
        nodeArray[i].color = color[i];
    }
}


void AOITClearData(inout AOITData data, float depth, vec4 color)
{
	uint packedColor = PackRGBA(AOIT_ToRGBE(vec4(0, 0, 0, 1)));

	data.depth = uintBitsToFloat(((floatBitsToUint(AOIT_EMPTY_NODE_DEPTH) & uint(AOIT_TRANS_MASK))) | uint(clamp(1.0f - color.w, 0.f, 1.f) * 255 + 0.5)).xxxx;
	data.color = packedColor.xxxx;

	data.depth[0] = depth;
	data.color[0] = PackRGBA(AOIT_ToRGBE(vec4(color.www * color.xyz, 1)));
}


void AOITLoadData(in ivec2 pixelAddr, out AOITNode nodeArray[AOIT_NODE_COUNT])
{
    AOITData data;

	data.color = imageLoad(AOITColorDataBuffer, pixelAddr);
	data.depth = imageLoad(AOITDepthDataBuffer, pixelAddr);

    for (uint j = 0; j < 4; j++)
    {
        AOITNode node =
        {
            uintBitsToFloat(floatBitsToUint(data.depth[j]) & uint(AOIT_TRANS_MASK)),
			float(floatBitsToUint(data.depth[j]) & uint(AOIT_MAX_UNNORM_TRANS)),
			data.color[j]
        };

        nodeArray[j] = node;
    }
}


void AOITStoreData(in ivec2 pixelAddr, AOITNode nodeArray[AOIT_NODE_COUNT])
{
    AOITData data;

	for (uint j = 0; j < 4; j++)
    {
        data.depth[j] = uintBitsToFloat((floatBitsToUint(nodeArray[j].depth) & uint(AOIT_TRANS_MASK)) | (uint(nodeArray[j].trans) & uint(AOIT_MAX_UNNORM_TRANS)));
        data.color[j] = (nodeArray[j].color);
    }
	
	imageStore(AOITDepthDataBuffer, pixelAddr, data.depth);
	imageStore(AOITColorDataBuffer, pixelAddr, data.color);
}


void AOITLoadControlSurface(in uint data, out AOITCtrlSurface surface)
{
	surface.clear	= bool(data & 0x1);
	surface.opaque  = bool(data & 0x2);
	surface.depth   = uintBitsToFloat((data & 0xFFFFFFFCU) | 0x3U);
}

void AOITLoadControlSurface(in ivec2 pixelAddr, out AOITCtrlSurface surface)
{
	uint data = imageLoad(AOITCtrlBuffer, pixelAddr).r;
	AOITLoadControlSurface(data, surface);
}

void AOITStoreControlSurface(in ivec2 pixelAddr, in AOITCtrlSurface surface)
{
	uint data;
	data  = floatBitsToUint(surface.depth) & 0xFFFFFFFCU;
	data |= surface.opaque ? 0x2 : 0x0;
	data |= surface.clear  ? 0x1 : 0x0;	 

	imageStore(AOITCtrlBuffer, pixelAddr, uvec4(data, 0, 0, 0));
}


void WriteNewPixelToAOIT(vec2 Position, float  surfaceDepth, vec4 surfaceColor)
{	
	AOITNode nodeArray[AOIT_NODE_COUNT];    
	ivec2 pixelAddr = ivec2(Position.xy);

	// Load AOIT control surface
	AOITCtrlSurface ctrlSurface;
	AOITLoadControlSurface(pixelAddr, ctrlSurface);

	// If we are modifying this pixel for the first time we need to clear the AOIT data
	if (ctrlSurface.clear) 
	{			
		// Clear AOIT data and initialize it with first transparent layer
		AOITData data;
		AOITClearData(data, surfaceDepth, surfaceColor);			

#if AOIT_NODE_COUNT != 2
		imageStore(AOITDepthDataBuffer, pixelAddr, data.depth);
#endif
		imageStore(AOITColorDataBuffer, pixelAddr, data.color);
		imageStore(AOITCtrlBuffer, pixelAddr, uvec4(0, 0, 0, 0));
	} 
	else 
	{ 
		// Load AOIT data
		AOITLoadData(pixelAddr, nodeArray);

		// Update AOIT data
		AOITInsertFragment(	surfaceDepth,		
							1.0f - surfaceColor.w,
							surfaceColor.xyz,
							nodeArray);
		// Store AOIT data
		AOITStoreData(pixelAddr, nodeArray);
	}
}
