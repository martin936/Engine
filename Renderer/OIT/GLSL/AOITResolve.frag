#version 450
#extension GL_EXT_samplerless_texture_functions : require

#define AOIT_NODE_COUNT 4
#define AOIT_RT_COUNT ((AOIT_NODE_COUNT + 3) / 4)

#define AOIT_EMPTY_NODE_DEPTH   (0.)  //(3.40282E38)
#define AOIT_TRANS_BIT_COUNT    (8)
#define AOIT_MAX_UNNORM_TRANS   ((1 << AOIT_TRANS_BIT_COUNT) - 1)
#define AOIT_TRANS_MASK         (0xFFFFFFFF - uint(AOIT_MAX_UNNORM_TRANS))

#define AOIT_TILED_ADDRESSING



struct AOITCtrlSurface
{
	bool clear;
	bool opaque;
	float depth;
};


struct AOITData
{
	vec4 depth;
	uvec4 color;
};


struct AOITDepthData
{
	vec4 depth[AOIT_RT_COUNT];
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


uniform layout(binding=0)	utexture2D	AOITCtrlBuffer;
uniform layout(binding=1)	utexture2D	AOITColorDataBuffer;
uniform layout(binding=2)	texture2D	AOITDepthDataBuffer;


layout(location = 0) out vec4 ColorOutput;


float UnpackUnnormAlpha(uint packedInput)
{
	return float(packedInput >> 24U);
}


// ToRGBE - takes a float RGB value and converts it to a float RGB value with a shared exponent
vec4 AOIT_ToRGBE(vec4 inColor)
{
    float base = max(inColor.r, max(inColor.g, inColor.b));
    int e;
    float m = frexp(base, e);
    return vec4(clamp(inColor.rgb / exp2(e), 0.f.xxx, 1.f.xxx), e + 127);
}


// FromRGBE takes a float RGB value with a shared exponent and converts it to a 
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


void AOITLoadData(in uvec2 pixelAddr, out AOITNode nodeArray[AOIT_NODE_COUNT])
{
    AOITData data;
    
	data.color = texelFetch(AOITColorDataBuffer, ivec2(pixelAddr), 0);
	data.depth = texelFetch(AOITDepthDataBuffer, ivec2(pixelAddr), 0);

    for (uint j = 0; j < 4; j++)
    {
        AOITNode node =
        {
            0, 
			float(floatBitsToUint(data.depth[j]) & uint(AOIT_MAX_UNNORM_TRANS)),
			data.color[j]
        };

        nodeArray[j] = node;
    }
}


void AOITLoadControlSurface(in uint data, out AOITCtrlSurface surface)
{
	surface.clear	= bool(data & 0x1);
	surface.opaque  = bool(data & 0x2);
	surface.depth   = uintBitsToFloat((data & 0xFFFFFFFCU) | 0x3U);
}

void AOITLoadControlSurface(in uvec2 pixelAddr, out AOITCtrlSurface surface)
{
	uint data = texelFetch(AOITCtrlBuffer, ivec2(pixelAddr), 0).r;
	AOITLoadControlSurface(data, surface);
}


void main( void )
{
	vec4 outColor = vec4(0.f, 0.f, 0.f, 0.f);
	uvec2 pixelAddr = uvec2(gl_FragCoord.xy);

	// Load control surface
	AOITCtrlSurface ctrlSurface;
	AOITLoadControlSurface(pixelAddr, ctrlSurface);

	// Any transparent fragment contributing to this pixel?
	if (!ctrlSurface.clear) 
	{
		// Load all nodes for this pixel    
		AOITNode nodeArray[AOIT_NODE_COUNT];
		AOITLoadData(pixelAddr, nodeArray);

		// Accumulate final transparent colors
		float  trans = 1.f;
		vec3 color = 0.f.xxx;       
		for(uint i = 0; i < AOIT_NODE_COUNT; i++) 
		{
			color += trans * AOIT_FromRGBE(UnpackRGBA(nodeArray[i].color)).rgb;
			trans  = nodeArray[i].trans / 255.f;
		}
		outColor = vec4(color, 1.f - nodeArray[AOIT_NODE_COUNT - 1].trans / 255.f);
	}

	ColorOutput = outColor;
}
