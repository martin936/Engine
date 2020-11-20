#version 450

layout (location = 0) in vec3 WorldPos;
layout (location = 1) in vec2 Texcoord;
layout (location = 2) in vec2 Zcmp;


struct LightInfo
{
	vec4 Pos;
	vec4 NearFar;
};


layout (binding = 1, std140) uniform cb1
{
	LightInfo	lightInfo[32];
};


void main( void )
{
	float depth = distance(WorldPos, lightInfo[gl_Layer / 6].Pos.xyz);

	float Near	= lightInfo[gl_Layer / 6].NearFar.x;
	float Far	= lightInfo[gl_Layer / 6].NearFar.y;

	gl_FragDepth = ((Far + Near - 2.f * (Far * Near) / depth) / (Near - Far)) * 0.5f + 0.5f - abs(gl_FragCoord.z - Zcmp.x / Zcmp.y);
}
