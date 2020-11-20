#version 450

layout(location= 0) in struct
{
	vec3	Normal;
	vec3	Tangent;
	vec3	Bitangent;
	vec3	WorldPos;
	vec2	Texcoords;
	vec3	CurrPos;
	vec3	LastPos;
} interp;


layout(push_constant) uniform pc0
{
	uint mat;
};


layout(location = 0) out uint MatID;



layout(early_fragment_tests) in;
void main( void )
{
	MatID = mat;
}
