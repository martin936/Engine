// shadertype=glsl

#version 450

layout (location = 0) out vec4 FilteredMap;

layout (std140) uniform cb0
{
	float	Size; 
	float	Level;
	float	Face;
	float	Padding;
};

uniform samplerCube CubeMap[1];
#define EnvMap CubeMap[0]


//in int gl_Layer;


float radicalInverse_VdC(uint inBits) 
{
	uint bits = inBits;
    bits = (bits << 16U) | (bits >> 16U);
    bits = ((bits & 0x55555555U) << 1U) | ((bits & 0xAAAAAAAAU) >> 1U);
    bits = ((bits & 0x33333333U) << 2U) | ((bits & 0xCCCCCCCCU) >> 2U);
    bits = ((bits & 0x0F0F0F0FU) << 4U) | ((bits & 0xF0F0F0F0U) >> 4U);
    bits = ((bits & 0x00FF00FFU) << 8U) | ((bits & 0xFF00FF00U) >> 8U);
    return float(bits) * 2.3283064365386963e-10;
    // / 0x100000000
}
                    
vec2 Hammersley(uint i, uint N)
{
    return vec2(float(i)/float(N), radicalInverse_VdC(i));
}


vec3 ImportanceSampleGGX( vec2 Xi, float roughness, vec3 N )
{
	float a = roughness * roughness;
    float Phi = 2 * 3.141592f * Xi.x;
    float CosTheta = sqrt( (1 - Xi.y) / ( 1 + (a*a - 1) * Xi.y ) );
    float SinTheta = sqrt( 1 - CosTheta * CosTheta );
    vec3 H;

    H.x = SinTheta * cos( Phi );
    H.y = SinTheta * sin( Phi );
    H.z = CosTheta;

    vec3 UpVector = abs(N.z) < 0.999 ? vec3(0,0,1) : vec3(1,0,0);
    vec3 TangentX = normalize( cross( UpVector, N ) );
    vec3 TangentY = cross( N, TangentX );

    // Tangent to world space
	return TangentX * H.x + TangentY * H.y + N * H.z;
}




vec3 PrefilterEnvMap( float roughness, vec3 R )
{
	vec3 N = R;
    vec3 V = R;
    vec3 PrefilteredColor = vec3(0.f);
    const uint NumSamples = 1024;
	float TotalWeight = 0.f;

    for( uint i = 0; i < NumSamples; i++ )
	{
        vec2 Xi = Hammersley( i, NumSamples );
        vec3 H = ImportanceSampleGGX( Xi, roughness, N );

        vec3 L = 2 * dot( V, H ) * H - V;
        float NoL = clamp(dot( N, L ), 0.f, 1.f);

        if( NoL > 0 )
		{
            PrefilteredColor += textureLod( EnvMap , L, 0 ).rgb * NoL;
            TotalWeight += NoL;
        }

	}

	return PrefilteredColor / TotalWeight;
}




void main( void )
{
	vec3 view;
	vec2 texcoords = (gl_FragCoord.xy / Size) * 2.f - 1.f;

	vec3 views[6] = vec3[] ( 
								vec3(1.f, -texcoords.y, -texcoords.x),
								vec3(-1.f, -texcoords.y, texcoords.x),
								vec3(texcoords.x, 1.f, texcoords.y),
								vec3(texcoords.x, -1.f, -texcoords.y),
								vec3(texcoords.x, -texcoords.y, 1.f),
								vec3(-texcoords.x, -texcoords.y, -1.f)
							);

	view = normalize(views[uint(Face)]);
	view.xy *= -1.f;

	float roughness = Level * Level;

	if (roughness == 0.f)
		FilteredMap.rgb = textureLod(EnvMap, view.xzy, 0).rgb;

	else
		FilteredMap.rgb = PrefilterEnvMap(roughness, view.xzy);

	FilteredMap.a = 1.f;
}
