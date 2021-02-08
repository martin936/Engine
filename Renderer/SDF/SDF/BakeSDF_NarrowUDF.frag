#version 450


layout(location = 0) in vec3 WorldPos;
layout(location = 1) in flat vec3 VertexPos[3];

layout(binding = 0, r32ui)	uniform coherent volatile uimage3D	Points;


layout(push_constant) uniform pc0
{
	vec4	Center;
	vec4	Size;
    vec4    TextureSize;
};


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



layout(early_fragment_tests) in;
void main( void )
{
	ivec3 size = imageSize(Points).xyz;

	ivec3 coords = ivec3((WorldPos / Size.xyz) * size);

	precise vec3 cellCenter = ((coords + 0.5f) / size) * Size.xyz;

    precise float d = udTriangle(cellCenter, VertexPos[0], VertexPos[1], VertexPos[2]);

    vec3 cellSize = Size.xyz / TextureSize.x;

	if (d > 2.f * min(cellSize.x, min(cellSize.y, cellSize.z)))
		discard;

    uint ud = uint(4294967295.f * (d / (max(Size.x, max(Size.y, Size.z)))));

    imageAtomicMin(Points, coords, ud);
}
