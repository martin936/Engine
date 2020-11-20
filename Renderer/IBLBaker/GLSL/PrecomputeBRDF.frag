// shadertype=glsl

#version 450

layout (location = 0) out vec4 BRDF;

in struct PS_INPUT
{
	vec2 uv;
} interp;


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


float G1V(float Roughness, float NoV)
{
    float k = Roughness * Roughness;
    return NoV / (NoV * (1.0f - k) + k);
}

float G_Smith(float Roughness, float NoV, float NoL)
{
    return G1V(Roughness, NoV) * G1V(Roughness, NoL);
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


vec2 IntegrateBRDF( float Roughness, float NoV )
{
    vec3 V;
    V.x = sqrt( 1.0f - NoV * NoV );
	V.y = 0;
    V.z = NoV;

	vec3 N = vec3(0.f, 0.f, 1.f);

	float A = 0.f;
    float B = 0.f;
    const uint NumSamples = 1024;

    for( uint i = 0; i < NumSamples; i++ )
	{
        vec2 Xi = Hammersley( i, NumSamples );
        vec3 H = ImportanceSampleGGX( Xi, Roughness, N );

        vec3 L = 2 * dot( V, H ) * H - V;
        float NoL = clamp( L.z, 0.f, 1.f );
        float NoH = clamp( H.z, 0.f, 1.f );
        float VoH = clamp( dot( V, H ), 0.f, 1.f );

        if( NoL > 0 )
		{
            float G = G_Smith(Roughness, NoV, NoL);
            float G_Vis = G * VoH / (NoH * NoV);
            float Fc = pow( 1 - VoH, 5 );
            A += (1 - Fc) * G_Vis;
            B += Fc * G_Vis;
        }

	}

	return vec2( A, B ) / NumSamples;
}




void main( void )
{
	BRDF.xy = IntegrateBRDF(interp.uv.y, interp.uv.x);
	BRDF.zw = 0.f.xx;
}
