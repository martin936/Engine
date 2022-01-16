#version 450
#extension GL_EXT_samplerless_texture_functions : require


layout(push_constant) uniform pc0
{
	uint TemporalOffset;
};


layout(binding = 0) uniform texture2D	ColorMap;
layout(binding = 1) uniform utexture2D	Sobol16;
layout(binding = 2) uniform utexture3D	OwenScrambling16;
layout(binding = 3) uniform utexture3D	OwenRanking16;
layout(binding = 4) uniform sampler sampLinear;

layout (location = 0) out vec4		BlurredColor;


float OwenScrambledSobol(uint pixel_i, uint pixel_j, uint sampleIndex, uint sampleDimension)
{
	// wrap arguments
	pixel_i = pixel_i & 127;
	pixel_j = pixel_j & 127;
	sampleIndex = sampleIndex & 255;
	sampleDimension = sampleDimension & 255;

	// xor index based on optimized ranking
	uint rankedSampleIndex = sampleIndex ^ texelFetch(OwenRanking16, ivec3(pixel_i, pixel_j, sampleDimension & 7), 0).r;

	// fetch value in sequence
	uint value = texelFetch(Sobol16, ivec2(sampleDimension, rankedSampleIndex), 0).r;

	// If the dimension is optimized, xor sequence value based on optimized scrambling
	value = value ^ texelFetch(OwenScrambling16, ivec3(pixel_i, pixel_j, sampleDimension & 7), 0).r;

	// convert to float and return
	float v = (0.5f+value)/256.0f;
	return v;
}


void main( void )
{
	vec2 size = textureSize(ColorMap, 0).xy;

	vec2 texCoords = gl_FragCoord.xy / size;

	vec4 base		= texelFetch(ColorMap, ivec2(gl_FragCoord.xy), 0);

	float w			= 0.382483f;
	vec3 color		= w * base.rgb;

	float radius	= base.a;

	float angle = 3.141592f * OwenScrambledSobol(uint(gl_FragCoord.x), uint(gl_FragCoord.y), TemporalOffset, 0);
    float cos_a = cos(angle);
	float sin_a = sin(angle);

    vec2 dir	= vec2(cos_a, sin_a);
        
    vec2 offset = dir / size;
    float rng = OwenScrambledSobol(uint(gl_FragCoord.x), uint(gl_FragCoord.y), TemporalOffset, 1);
    texCoords += 0.33f * (rng - 0.5f) * radius * offset;

    if (radius > 0.0f)
    {
		color = w * textureLod(sampler2D(ColorMap, sampLinear), texCoords, 0).rgb;

        float currRadius = 0.33f * radius;
		base = textureLod(sampler2D(ColorMap, sampLinear), texCoords + currRadius * offset, 0);
        float sampleRadius = base.w;
        float w1 = step(currRadius, sampleRadius) * 0.308758f;

        color += w1 * base.rgb;
        w += w1;

        base = textureLod(sampler2D(ColorMap, sampLinear), texCoords - currRadius * offset, 0);
        sampleRadius = base.w;
        w1 = step(currRadius, sampleRadius) * 0.308758f;

        color += w1 * base.rgb;
        w += w1;
    }

    color /= max(1e-3f, w);

	BlurredColor = vec4(color, radius);
}