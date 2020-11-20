// shadertype=glsl

#version 450

layout (location = 0) out float Llm;

layout (std140) uniform cb0
{
	int nIndex;
};

void main( void )
{
	vec3 view;
	vec2 texcoords = (gl_FragCoord.xy / 64.f) * 2.f - 1.f;

	vec3 views[6] = vec3[] ( 
								vec3(1.f, -texcoords.y, -texcoords.x),
								vec3(-1.f, -texcoords.y, texcoords.x),
								vec3(texcoords.x, 1.f, texcoords.y),
								vec3(texcoords.x, -1.f, -texcoords.y),
								vec3(texcoords.x, -texcoords.y, 1.f),
								vec3(-texcoords.x, -texcoords.y, -1.f)
							);

	view = normalize(views[nIndex % 6]);

	float SH[9] =	{	
						0.282097f,
						0.48860f * view.y,
						0.48860f * view.z,
						0.48860f * view.x,
						1.09255f * view.x * view.y,
						1.09255f * view.y * view.z,
						0.31539f * (3.f * view.z * view.z - 1.f),
						1.09255f * view.x * view.z,
						0.54627f * (view.x * view.x - view.y * view.y)
					};

	Llm = SH[nIndex / 6];
}
