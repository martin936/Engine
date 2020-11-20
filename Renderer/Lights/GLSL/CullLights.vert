#version 450


layout (location = 0)	in vec3 Position;
layout (location = 1)	in vec4 InstanceMatrix0;
layout (location = 2)	in vec4 InstanceMatrix1;
layout (location = 3)	in vec4 InstanceMatrix2;


layout (location = 0) out vec4	outPosition;
layout (location = 1) out uint	LightID;

 
void main(void)
{
	LightID = uint(InstanceMatrix2.x);

	bool bSphere = InstanceMatrix2.y > 0.5f;

	vec3 center = InstanceMatrix0.xyz;
	float radius = InstanceMatrix0.w;

	if (bSphere)
	{
		outPosition.xyz = radius * Position.xyz + center;
	}

	else
	{
		outPosition.xyz = Position.xyz;

		vec3 dir = normalize(InstanceMatrix1.xyz);
		float angle = InstanceMatrix1.w;

		vec3 e1 = cross(dir, vec3(1.f, 0.f, 0.f));

		if (dot(e1, e1) < 1e-3f)
			e1 = cross(dir, vec3(0.f, 1.f, 0.f));

		vec3 e2 = normalize(cross(e1, dir));
		e1 = normalize(cross(dir, e2));

		outPosition.xz *= angle;

		outPosition.xyz = outPosition.y * dir + outPosition.x * e1 + outPosition.z * e2;

		if (dot(outPosition.xyz, outPosition.xyz) > 1e-3f)
			outPosition.xyz = normalize(outPosition.xyz);

		outPosition.xyz -= 0.15f * dir;

		outPosition.xyz = 1.35f * radius * outPosition.xyz + center;
	}

	outPosition.w = 1.f;
}
