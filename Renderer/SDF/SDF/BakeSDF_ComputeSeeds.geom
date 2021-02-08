#version 450

layout(triangles) in;
layout(triangle_strip, max_vertices = 4) out;

layout(location = 0) in vec3	WorldPos[3];
layout(location = 1) in vec3	WorldNormal[3];
layout(location = 2) in vec2	Texc0[3];
layout(location = 3) in int	    InstanceId[3];

layout (location = 0) out vec3 Pos;
layout (location = 1) out flat vec3 VertexPos[3];
layout (location = 4) out flat vec3 VertexNormal[3];
layout (location = 7) out flat vec2 VertexTexcoord[3];


layout(push_constant) uniform pc0
{
	vec4	Center;
	vec4	Size;
    vec4    TextureSize;
};


void main() 
{
    vec3 e[3];
    e[0] = WorldPos[2] - WorldPos[1];
    e[1] = WorldPos[0] - WorldPos[2];
    e[2] = WorldPos[1] - WorldPos[0];

    precise vec3 diag = e[0];
    float maxD = dot(e[0], e[0]);
    int index = 0;

    for (int i = 1; i < 3; i++)
    {
        float d = dot(e[i], e[i]);
        if (d > maxD)
        {
            maxD = d;
            diag = e[i];
            index = i;
        }
    }

    precise vec3 n = normalize(cross(e[0], e[1]));
    precise vec3 eh = normalize(cross(n, diag));

    vec3 O = WorldPos[(index + 1) % 3];

    precise float h = dot(WorldPos[index] - O, eh);
    precise float l = length(diag);

    diag /= l;

    if (h < 0.f)
    {
        eh = -eh;
        h = -h;
    }
           
    vec3 cellSize = Size.xyz / TextureSize.xyz;
    vec2 offset = 3.f * vec2(dot(cellSize, abs(diag)), dot(cellSize, abs(eh)));

    l += 2.f * offset.x;
    h += 2.f * offset.y;

    O -= offset.x * diag + offset.y * eh;

    O += 0.75f * (InstanceId[0] - 4) * dot(cellSize, abs(n)) * n;

    vec2 p = vec2(l / Size.x, h / Size.y);

    for (int i = 0; i < 3; i++)
    {
        VertexPos[i]        = WorldPos[i];
        VertexNormal[i]     = WorldNormal[i];
        VertexTexcoord[i]   = Texc0[i];
    }

    Pos = O;
    gl_Position = vec4(-p.x, -p.y, 0.5, 1.0);
    EmitVertex();

    for (int i = 0; i < 3; i++)
    {
        VertexPos[i]        = WorldPos[i];
        VertexNormal[i]     = WorldNormal[i];
        VertexTexcoord[i]   = Texc0[i];
    }

    Pos = O + l * diag;
    gl_Position = vec4(p.x, -p.y, 0.5, 1.0);
    EmitVertex();

    for (int i = 0; i < 3; i++)
    {
        VertexPos[i]        = WorldPos[i];
        VertexNormal[i]     = WorldNormal[i];
        VertexTexcoord[i]   = Texc0[i];
    }

    Pos = O + h * eh;
    gl_Position = vec4(-p.x, p.y, 0.5, 1.0);
    EmitVertex();

    for (int i = 0; i < 3; i++)
    {
        VertexPos[i]        = WorldPos[i];
        VertexNormal[i]     = WorldNormal[i];
        VertexTexcoord[i]   = Texc0[i];
    }

    Pos = O + h * eh + l * diag;
    gl_Position = vec4(p.x, p.y, 0.5, 1.0);
    EmitVertex();
}
