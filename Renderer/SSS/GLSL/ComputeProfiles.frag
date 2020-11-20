// shadertype=glsl

#version 450

in struct PS_INPUT
{
	vec2 uv;
} interp;


layout (std140) uniform cb0
{
	vec3	Near[5];
	vec3	Far[5];
	vec3	Ratio[5];
};

layout(location = 0) out vec4 Samples;
layout(location = 1) out vec4 Profile;


void main(void)
{
	int		index	= int(gl_FragCoord.y);
	float	x		= interp.uv.x;

	Profile.rgb = mix( exp(-pow(x * Near[index], 2.f.xxx)), exp(-pow(x * Far[index], 2.f.xxx)), Ratio[index]);
	Profile.a = 0.f;

	int i = int(5.f - gl_FragCoord.x) + 1;

	float z = cos(3.141592654f * (i - 0.25) / (9.f + 0.5));
	float p1, p2, p3, pp, z1;

	do 
	{
		p1=1.f;
		p2=0.f;

		for (int j = 1; j <= 9; j++) 
		{
			p3 = p2;
			p2 = p1;
			p1 = ((2.f * j - 1.f) * z * p2 - (j - 1.f) * p3) / j;
		}

		pp = 9 * (z * p1 - p2) / (z * z - 1.f);
		z1 = z;
		z = z1 - p1 / pp;

	} while (abs(z - z1) > 1e-5f);

	Samples.rgb = 2.f * mix( exp(-pow(z * Near[index], 2.f.xxx)), exp(-pow(z * Far[index], 2.f.xxx)), Ratio[index]) / ((1.f - z * z) * pp * pp);
	Samples.a = z;
}
