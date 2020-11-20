#version 450

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

layout (location = 0) in vec4 Position[3];
layout (location = 1) in uint LightID[3];


layout (binding = 2, std140) uniform cb2
{
	mat4	m_View;
	mat4	m_Proj;
	mat4	m_ViewProj;
	mat4	m_InvView;
	mat4	m_InvViewProj;

	mat4	m_LastView;
	mat4	m_LastProj;
	mat4	m_LastViewProj;
	mat4	m_LastInvView;
	mat4	m_LastInvViewProj;

	vec4	m_Eye;
};


layout(location = 0) flat out struct
{
	uint LightID;
	vec4 ZBounds;

} interp;


layout(push_constant) uniform pc0
{
	float dv_norm;
	float NearCam;
	float FarCam;
};

 
void main(void)
{
	vec4 P[3];
    float ZMax = 0.f, ZMin = 0.f;
    uint i = 0U;

    vec3 r = vec3(m_View[0][0], m_View[1][0], m_View[2][0]);
    vec3 u = vec3(m_View[0][1], m_View[1][1], m_View[2][1]);
    vec3 v = vec3(m_View[0][2], m_View[1][2], m_View[2][2]);

    vec3 n = normalize(cross(Position[1].xyz - Position[0].xyz, Position[2].xyz - Position[0].xyz));

    float d = dot(v, n);
    d = sign(d) / max(1e-6f, abs(d));
    
    vec3 w = v - n * d;

    vec3 dv_x = vec3(r.x, u.x, v.x) * dv_norm;
    vec3 dv_y = vec3(r.y, u.y, v.y) * dv_norm;

    vec2 grad = vec2(dot(w, dv_x - dot(v, dv_x) * v), dot(w, dv_y - dot(v, dv_y) * v)) * dot(Position[0].xyz - m_Eye.xyz, n) * d;
	
    for (i = 0U; i < 3U; i++)
    {
        P[i] = m_ViewProj * Position[i];
    }

    float Z[3];

    for (i = 0U; i < 3U; i++)
    {
        Z[i] = min(1.f, P[i].z) / max(1.f, P[i].w);
    }

    ZMax = clamp(min(min(Z[0], Z[1]), Z[2]), 0.f, 1.f);
    ZMin = clamp(max(max(Z[0], Z[1]), Z[2]), 0.f, 1.f);

	ZMax = 2.f * FarCam * NearCam / (FarCam + NearCam + (2.f * ZMax - 1.f) * (FarCam - NearCam));
	ZMin = 2.f * FarCam * NearCam / (FarCam + NearCam + (2.f * ZMin - 1.f) * (FarCam - NearCam));

	for (uint i = 0; i < 3; i++)
	{
		interp.LightID = LightID[i];
		interp.ZBounds = vec4(ZMin, ZMax, grad);
		gl_Position = P[i];

		EmitVertex();
	}
}
