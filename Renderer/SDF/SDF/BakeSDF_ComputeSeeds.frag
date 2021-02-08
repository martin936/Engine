#version 450
#extension GL_EXT_nonuniform_qualifier : require


layout(location = 0) in vec3 WorldPos;
layout(location = 1) in flat vec3 VertexPosition[3];
layout(location = 4) in flat vec3 VertexNormal[3];
layout(location = 7) in flat vec2 VertexTexcoord[3];

layout(binding = 0, r32ui)	uniform readonly uimage3D	Points;
layout(binding = 1, r32ui)	uniform uimage3D	        BandSign;
layout(binding = 2, r32ui)	uniform writeonly uimage3D	VoronoiTex;
layout(binding = 3, rgba8)	uniform writeonly image3D	AlbedoTex;

layout(binding = 4) uniform texture2D	MaterialTex[];
layout(binding = 5) uniform sampler		sampLinear;

layout(binding = 6, std140) uniform cb6
{
	vec4	Color;

	float	Roughness;
	float	Emissive;
	float	BumpHeight;
	float	Reflectivity;

	float	Metalness;
	float	SSSProfileID;
	float	SSSRadius;
	float	SSSThickness;

	uint 	DiffuseTextureID;
	uint 	NormalTextureID;
	uint 	InfoTextureID;
	uint	SSSTextureID;
};


layout(push_constant) uniform pc0
{
	vec4	Center;
	vec4	Size;
    vec4    TextureSize;
};


#define UNSIGNED (TextureSize.w)


float dot2(vec3 x)
{
    return dot(x, x);
}


float udTriangle( vec3 p, vec3 a, vec3 b, vec3 c )
{
  vec3 ba = b - a; vec3 pa = p - a;
  vec3 cb = c - b; vec3 pb = p - b;
  vec3 ac = a - c; vec3 pc = p - c;
  vec3 nor = cross( ba, ac );

  return sqrt(
    (sign(dot(cross(ba,nor),pa)) +
     sign(dot(cross(cb,nor),pb)) +
     sign(dot(cross(ac,nor),pc))<2.0)
     ?
     min( min(
     dot2(ba*clamp(dot(ba,pa)/dot2(ba),0.0,1.0)-pa),
     dot2(cb*clamp(dot(cb,pb)/dot2(cb),0.0,1.0)-pb) ),
     dot2(ac*clamp(dot(ac,pc)/dot2(ac),0.0,1.0)-pc) )
     :
     dot(nor,pa)*dot(nor,pa)/dot2(nor) );
}


vec3 GetBarycentricCoordinates(vec3 p, vec3 a, vec3 b, vec3 c)
{
	vec3 coords;

	vec3 pa = p - a;
	vec3 ba = b - a;
	vec3 ca = c - a;
	vec3 cb = c - b;

	float d00 = dot2(ba);
	float d01 = dot(ba, ca);
	float d11 = dot2(ca);
	float d20 = dot(pa, ba);
	float d21 = dot(pa, ca);

	float invdet = 1.f / (d00 * d11 - d01 * d01);

	coords.y = (d11 * d20 - d01 * d21) * invdet;
	coords.z = (d00 * d21 - d01 * d20) * invdet;
	coords.x = 1.f - coords.y - coords.z;

	return coords;
}


vec3 closesPointOnTriangle(vec3 p, vec3 A, vec3 B, vec3 C)
{
    vec3 edge0 = B - A;
    vec3 edge1 = C - A;
    vec3 v0 = A - p;

    float a = dot2(edge0);
    float b = dot(edge0, edge1);
    float c = dot2(edge1);
    float d = dot(edge0, v0);
    float e = dot(edge1, v0);

    float det = a*c - b*b;
    float s = b*e - c*d;
    float t = b*d - a*e;

    if ( s + t < det )
    {
        if ( s < 0.f )
        {
            if ( t < 0.f )
            {
                if ( d < 0.f )
                {
                    s = clamp( -d/a, 0.f, 1.f );
                    t = 0.f;
                }
                else
                {
                    s = 0.f;
                    t = clamp( -e/c, 0.f, 1.f );
                }
            }
            else
            {
                s = 0.f;
                t = clamp( -e/c, 0.f, 1.f );
            }
        }
        else if ( t < 0.f )
        {
            s = clamp( -d/a, 0.f, 1.f );
            t = 0.f;
        }
        else
        {
            float invDet = 1.f / det;
            s *= invDet;
            t *= invDet;
        }
    }
    else
    {
        if ( s < 0.f )
        {
            float tmp0 = b+d;
            float tmp1 = c+e;
            if ( tmp1 > tmp0 )
            {
                float numer = tmp1 - tmp0;
                float denom = a-2*b+c;
                s = clamp( numer/denom, 0.f, 1.f );
                t = 1-s;
            }
            else
            {
                t = clamp( -e/c, 0.f, 1.f );
                s = 0.f;
            }
        }
        else if ( t < 0.f )
        {
            if ( a+d > b+e )
            {
                float numer = c+e-b-d;
                float denom = a-2*b+c;
                s = clamp( numer/denom, 0.f, 1.f );
                t = 1-s;
            }
            else
            {
                s = clamp( -e/c, 0.f, 1.f );
                t = 0.f;
            }
        }
        else
        {
            float numer = c+e-b-d;
            float denom = a-2*b+c;
            s = clamp( numer/denom, 0.f, 1.f );
            t = 1.f - s;
        }
    }

    return A + s * edge0 + t * edge1;
}


vec3 GetClosestBarycentricCoordinates(vec3 p, vec3 a, vec3 b, vec3 c)
{
    vec3 pt = closesPointOnTriangle(p, a, b, c);

    return GetBarycentricCoordinates(pt, a, b, c);
}


void WriteAlbedo(ivec3 coords, vec2 texc)
{
	vec4 albedo;

	if (DiffuseTextureID == 0xffffffff)
		albedo		= Color;
	else
		albedo		= texture(sampler2D(MaterialTex[DiffuseTextureID], sampLinear), texc);

	albedo.a = pow(Emissive * (1.f / 2500.f), 0.25f);

	imageStore(AlbedoTex, coords, albedo);
}


uint packPosition(vec3 pos)
{
	uvec3 p = uvec3((pos / Size.xyz) * 1024.f) & 0x3ffu;

	return p.x | (p.y << 10u) | (p.z << 20u) | (1 << 31u);
}


void WritePosition(ivec3 coords, vec3 position, bool s)
{
	imageStore(VoronoiTex, coords, packPosition(position).xxxx);
    imageAtomicMax(BandSign, coords, s ? 1 : 0);
}



void main( void )
{
	ivec3 size = imageSize(Points).xyz;

	ivec3 coords = ivec3((WorldPos / Size.xyz) * size);

	precise vec3 cellCenter = ((coords + 0.5f) / size) * Size.xyz;

    precise float d = udTriangle(cellCenter, VertexPosition[0], VertexPosition[1], VertexPosition[2]);

    vec3 cellSize = Size.xyz / TextureSize.x;

	if (d > 2.f * min(cellSize.x, min(cellSize.y, cellSize.z)))
		discard;

    uint ud = uint(4294967295.f * (d / (max(Size.x, max(Size.y, Size.z)))));

	if (ud > imageLoad(Points, coords).x)
		discard;

    precise vec3 bary = GetClosestBarycentricCoordinates(cellCenter, VertexPosition[0], VertexPosition[1], VertexPosition[2]);
	precise vec3 pos = bary.x * VertexPosition[0] + bary.y * VertexPosition[1] + bary.z * VertexPosition[2];
	precise vec2 texc = bary.x * VertexTexcoord[0] + bary.y * VertexTexcoord[1] + bary.z * VertexTexcoord[2];
    precise vec3 normal = normalize(VertexNormal[0] * bary.x + VertexNormal[1] * bary.y + VertexNormal[2] * bary.z);

	WriteAlbedo(coords, texc);

	bool s = sign(dot(cellCenter - pos, normal)) > 0.f;

    if (UNSIGNED > 0.5f)
        s = true;

	WritePosition(coords, pos, s);
}
