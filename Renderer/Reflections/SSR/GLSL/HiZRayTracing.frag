// shadertype=glsl

#version 450

in struct PS_INPUT
{
	vec2 uv;
} interp;


layout (std140) uniform cb0
{
	mat4	ViewProj;
	mat4	InvViewProj;
	vec4	Eye;
	vec4	NearFar;
};


#define Near	NearFar.x
#define Far		NearFar.y

#define PixelSize NearFar.zw

#define MAX_LEVEL 3U

layout(location = 0) uniform sampler2D ZMap;
layout(location = 1) uniform sampler2D NormalMap;

layout(location = 0) out vec4 RayData;


vec3 Unpack_Pos(out float fZDist)
{
	fZDist	= textureLod(ZMap, interp.uv, 0.f).r * 2.f - 1.f;
	vec4 pos		= InvViewProj * vec4( 2.f*interp.uv - 1.f , fZDist, 1.f );
	
	return pos.xyz / pos.w;
}


vec3 Unpack_Normal()
{
	vec4 normalTex = textureLod(NormalMap, interp.uv, 0.f);

	return normalize(normalTex.xyz * 2.f - 1.f);
}


float radicalInverse_VdC(uint inBits) 
{
	uint bits = inBits;
    bits = (bits << 16U) | (bits >> 16U);
    bits = ((bits & 0x55555555U) << 1U) | ((bits & 0xAAAAAAAAU) >> 1U);
    bits = ((bits & 0x33333333U) << 2U) | ((bits & 0xCCCCCCCCU) >> 2U);
    bits = ((bits & 0x0F0F0F0FU) << 4U) | ((bits & 0xF0F0F0F0U) >> 4U);
    bits = ((bits & 0x00FF00FFU) << 8U) | ((bits & 0xFF00FF00U) >> 8U);
    return float(bits) * 2.3283064365386963e-10;
}
                    
vec2 Hammersley(uint i, uint N)
{
    return vec2(float(i)/float(N), radicalInverse_VdC(i));
}


uint wang_hash(uint seed)
{
    seed = (seed ^ 61) ^ (seed >> 16);
    seed *= 9;
    seed = seed ^ (seed >> 4);
    seed *= 0x27d4eb2d;
    seed = seed ^ (seed >> 15);
    return seed;
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


float GetDepth(vec3 pos, out float fOutOfBounds)
{
	vec4 pos_proj = ViewProj * vec4(pos, 1.f);
	pos_proj.xyz = vec3(0.5f, 0.5f, 1.f) * pos_proj.xyz / pos_proj.w + vec3(0.5f, 0.5f, 0.f);

	float fZDist = textureLod(ZMap, pos_proj.xy, 0.f).r * 2.f - 1.f;

	fOutOfBounds = 1.f - step(0.f, pos_proj.x * (1.f - pos_proj.x)) * step(0.f, pos_proj.y * (1.f - pos_proj.y));

	return fZDist - pos_proj.z;
}


void main(void)
{
	float fZDist	= 0.f;

	vec3 pos			= Unpack_Pos(fZDist);
	vec3 normal			= Unpack_Normal();
	vec3 view			= normalize(pos - Eye.xyz);

	vec3 refl			= reflect(view, normal);

	uint seed			= uint(gl_FragCoord.y) * 1920U + uint(gl_FragCoord.x) + uint(Eye.w);

	refl				= ImportanceSampleGGX(Hammersley(wang_hash(seed) % 1000003U, 1000003U), 0.25f, refl);

	vec4 p				= ViewProj * vec4(pos, 1.f);
	vec4 r				= ViewProj * vec4(refl, 0.f);

	vec2	dir			= p.w * r.xy - r.w * p.xy;
	float	scale		= length(dir);
	dir /= scale;

	uint nCurrentLevel	= 0U;

	float	pixel		= 2.f * length(PixelSize);
	float	depth		= 0.f;

	vec2 texc = 0.5f * p.xy / p.w + 0.5f.xx;

	bool	bDone = false;
	bool	bShouldGoBack = false;
	uint	i = 0U;

	float t;
	float delta = pixel;

	if (fZDist < 1.f)
	{
		while(!bDone && i++ < 1000)
		{
			t = delta * p.w * p.w / (scale - delta * r.w * p.w);
			texc = p.xy / p.w + delta * dir;
			texc = 0.5f * texc + 0.5f;

			depth = textureLod(ZMap, texc, 0.f).r * 2.f - 1.f;
			depth -= (p.z + t * r.z) / (p.w + t * r.w);

			if(texc.x * (1.f - texc.x) < 0.f || texc.y * (1.f - texc.y) < 0.f)
			{
				RayData = -1.f.xxxx;
				bDone = true;
				break;
			}

			if (depth < 0.f)
			{
				if (nCurrentLevel == 0U)
				{
					bDone = true;	

					if (depth > -5e-4f)
						RayData = vec4(pos + t * refl, t);
					else
						RayData = -1.f.xxxx;

					break;
				}

				else
				{
					if (bShouldGoBack)
					{
						vec2 dt = min(2.f * texc / (pixel * dir) - ceil(2.f * texc / (pixel * dir)), floor(2.f * texc / (pixel * dir)) - 2.f * texc / (pixel * dir)) * 0.5f * pixel * dir;
						delta += min(0.f, max(dt.x, dt.y)) + 1e-6f;

						bShouldGoBack = false;
					}

					nCurrentLevel--;
					pixel /= 2.f;
				}
			}

			else
			{
				delta += pixel;
				bShouldGoBack = true;

				if (nCurrentLevel < MAX_LEVEL)
				{
					pixel *= 2.f;
					nCurrentLevel++;
				}
			}
		}
	}

	if (!bDone)
		RayData = -1.f.xxxx;
}
