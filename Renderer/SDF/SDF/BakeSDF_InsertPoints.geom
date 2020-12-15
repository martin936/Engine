#version 450

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

layout (location = 0) in vec2 position[3];
layout(location = 1)  in vec3 WorldPos[3];
layout(location = 2)  in vec3 WorldNormal[3];

layout (location = 0) out vec3 Pos;
layout (location = 1) out vec3 Normal;
layout (location = 2) out vec4 AABB;


void main() 
{
    vec2 e[3];
    e[0] = normalize(position[2] - position[1]);
    e[1] = normalize(position[2] - position[0]);
    e[2] = normalize(position[1] - position[0]);

    AABB = vec4(1e8f, -1e8f, 1e8f, -1e8f);

    vec2 n[3];
    for (int i = 0; i < 3; i++)
    {
        n[i] = vec2(e[i].y, -e[i].x);
        if (dot(position[(i + 1) % 3] - position[i], n[i]) > 0.f)
            n[i] *= -1.f;

        AABB.x = min(AABB.x, position[i].x);
        AABB.y = max(AABB.y, position[i].y);
        AABB.z = min(AABB.z, position[i].x);
        AABB.w = max(AABB.w, position[i].y);
    }

    AABB += vec4(-1.f, 1.f, -1.f, 1.f) * (1.f / 196.f);

    float h = 1.f / 196.f;
            
    for (int i = 0; i < 3; i++)
    {
        float cos_a = abs(dot(e[(i + 1) % 3], n[(i + 2) % 3]));
        float cos_b = abs(dot(e[(i + 2) % 3], n[(i + 1) % 3]));
        float sin_a = sqrt(max(0.f, 1.f - cos_a * cos_a));
        float sin_b = sqrt(max(0.f, 1.f - cos_b * cos_b));

        float l = length(n[(i + 1) % 3] - n[(i + 2) % 3]) * h;

        float r = l / (cos_a + sin_a * cos_a / sin_b);

        gl_Position = vec4(position[i] + h * n[(i + 1) % 3] + r * e[(i + 1) % 3], 0.5f, 1.f);

        Pos			= WorldPos[i].xyz;
		Normal		= WorldNormal[i].xyz;

        EmitVertex();
    }
}
