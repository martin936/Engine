#version 450

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

layout (location = 0) in vec4 Position[3];
layout (location = 1) in uint LightID[3];


layout(location = 0) flat out struct
{
	uint LightID;
	vec4 ZBounds;

} interp;


layout(push_constant) uniform pc0
{
	vec4 m_Center;
	vec4 m_Size;
};

 
void main(void)
{
	vec3 P[3];
    float ZMax = 0.f, ZMin = 0.f;
    uint i = 0U;

	for (i = 0U; i < 3U; i++)
    {
        P[i] = 2.f * (Position[i].xyz - m_Center.xyz) / m_Size.xyz;
		P[i].z = P[i].z * 0.5f + 0.5f;
    }

    vec3 n = normalize(cross(P[1].xyz - P[0].xyz, P[2].xyz - P[0].xyz));    

    ZMax = clamp(max(max(P[0].z, P[1].z), P[2].z), 0.f, 1.f);
    ZMin = clamp(min(min(P[0].z, P[1].z), P[2].z), 0.f, 1.f);

	float nz = 1.f / max(n.z, 1e-3f);

	for (uint i = 0; i < 3; i++)
	{
		interp.LightID = LightID[i];
		interp.ZBounds = vec4(ZMin, ZMax, n.x * nz, n.y * nz);
		gl_Position = vec4(P[i], 1.f);

		EmitVertex();
	}
}
