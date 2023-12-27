#version 460

#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_buffer_reference2 : require

hitAttributeEXT vec2 attribs;

struct Hit 
{
    vec3 position;
    vec3 normal;
};

struct Ray_Payload 
{
    Hit  hit;
    uint color;
};

vec4 ToRGBE(vec3 inColor)
{
    float base = max(inColor.r, max(inColor.g, inColor.b));
    int e;
    float m = frexp(base, e);
    return vec4(clamp(inColor.rgb / exp2(e), 0.f.xxx, 1.f.xxx), e + 127);
}

vec3 FromRGBE(vec4 inColor)
{
    return inColor.rgb * exp2(inColor.a - 127);
}


vec3 UnpackRGBE(uint packedInput)
{
    vec4 unpackedOutput;
    uvec4 p = uvec4((packedInput & 0xFFU),
		(packedInput >> 8U) & 0xFFU,
		(packedInput >> 16U) & 0xFFU,
		(packedInput >> 24U));

    unpackedOutput = vec4(p) / vec4(255, 255, 255, 1.0f);
    return FromRGBE(unpackedOutput);
}


uint PackRGBE(vec3 unpackedInput)
{
    uvec4 u = uvec4(ToRGBE(unpackedInput) * vec4(255, 255, 255, 1.0f));
    uint packedOutput = (u.w << 24U) | (u.z << 16U) | (u.y << 8U) | u.x;
    return packedOutput;
}

struct Material
{
    vec4	Color;

	float	Roughness;
	float	Emissive;
	float	BumpHeight;
	float	Reflectivity;

	float	Metalness;
	float	SSSProfileID;
	float	SSSRadius;
	float	SSSThickness;

	uint 	DiffuseTextureID;
	uint 	NormalTextureID;
	uint 	InfoTextureID;
	uint	SSSTextureID;

    vec4    padding[12];
};



layout (binding = 3, std140) readonly buffer buf0
{
	Material materials[];
};


layout(buffer_reference, std430, buffer_reference_align = 4) readonly buffer Vertices
{
    float VertexData;
};


layout(buffer_reference, std430, buffer_reference_align = 4) readonly buffer Indices
{
    uint Index;
};


struct ObjectDesc
{
    uint        matID;
    uint        normalOffset;
    uint        texcoordOffset;
    uint        stride;

    Vertices    vertexBuffer;
    Indices     indexBuffer;
};


layout(binding = 4, std430) readonly buffer     ObectBuf
{
    ObjectDesc  object[];    
};


layout(binding = 5) uniform texture2D	MaterialTex[];
layout(binding = 6) uniform sampler		samp;


layout (location=0) rayPayloadInEXT Ray_Payload payload;


struct Triangle
{
    vec3 vertices[3];
    vec2 texcoords[3];
    vec3 normal[3];
    uint textureID;
};


vec2 barycentric_interpolate(float b1, float b2, vec2 v0, vec2 v1, vec2 v2) 
{
    return (1.0 - b1 - b2)*v0 + b1*v1 + b2*v2;
}

vec3 barycentric_interpolate(float b1, float b2, vec3 v0, vec3 v1, vec3 v2) 
{
    return (1.0 - b1 - b2)*v0 + b1*v1 + b2*v2;
}


void GetTriangle(in ObjectDesc obj, out Triangle tri)
{
    Indices    indices      = Indices(obj.indexBuffer);
    Vertices   vertices     = Vertices(obj.vertexBuffer);

    uvec3 triIndices        = uvec3(indices[gl_PrimitiveID * 3].Index, indices[gl_PrimitiveID * 3 + 1].Index, indices[gl_PrimitiveID * 3 + 2].Index);
    tri.vertices[0]         = vec3(vertices[obj.stride * triIndices.x].VertexData, vertices[obj.stride * triIndices.x + 1].VertexData, vertices[obj.stride * triIndices.x + 2].VertexData);
    tri.vertices[1]         = vec3(vertices[obj.stride * triIndices.y].VertexData, vertices[obj.stride * triIndices.y + 1].VertexData, vertices[obj.stride * triIndices.y + 2].VertexData);
    tri.vertices[2]         = vec3(vertices[obj.stride * triIndices.z].VertexData, vertices[obj.stride * triIndices.z + 1].VertexData, vertices[obj.stride * triIndices.z + 2].VertexData);
    
    tri.texcoords[0]        = vec2(vertices[obj.stride * triIndices.x + obj.texcoordOffset].VertexData, vertices[obj.stride * triIndices.x + obj.texcoordOffset + 1].VertexData);
    tri.texcoords[1]        = vec2(vertices[obj.stride * triIndices.y + obj.texcoordOffset].VertexData, vertices[obj.stride * triIndices.y + obj.texcoordOffset + 1].VertexData);
    tri.texcoords[2]        = vec2(vertices[obj.stride * triIndices.z + obj.texcoordOffset].VertexData, vertices[obj.stride * triIndices.z + obj.texcoordOffset + 1].VertexData);
    
    tri.normal[0]           = vec3(vertices[obj.stride * triIndices.x + obj.normalOffset].VertexData, vertices[obj.stride * triIndices.x + obj.normalOffset + 1].VertexData, vertices[obj.stride * triIndices.x + obj.normalOffset + 2].VertexData);
    tri.normal[1]           = vec3(vertices[obj.stride * triIndices.y + obj.normalOffset].VertexData, vertices[obj.stride * triIndices.y + obj.normalOffset + 1].VertexData, vertices[obj.stride * triIndices.y + obj.normalOffset + 2].VertexData);
    tri.normal[2]           = vec3(vertices[obj.stride * triIndices.z + obj.normalOffset].VertexData, vertices[obj.stride * triIndices.z + obj.normalOffset + 1].VertexData, vertices[obj.stride * triIndices.z + obj.normalOffset + 2].VertexData);

    tri.textureID           = materials[obj.matID].DiffuseTextureID;
}


void main() 
{
    ObjectDesc obj          = object[gl_GeometryIndexEXT];
    
    Triangle tri;
    GetTriangle(obj, tri);

    vec3 position = barycentric_interpolate(attribs.x, attribs.y, tri.vertices[0], tri.vertices[1], tri.vertices[2]);
    position = gl_ObjectToWorldEXT * vec4(position, 1.f);

    vec3 normal = barycentric_interpolate(attribs.x, attribs.y, tri.normal[0], tri.normal[1], tri.normal[2]);
    normal = normalize(vec3(gl_WorldToObjectEXT * vec4(normal, 0.f)));

    vec3 color = 0.f.xxx;

    vec2 uv = barycentric_interpolate(attribs.x, attribs.y, tri.texcoords[0], tri.texcoords[1], tri.texcoords[2]);
    uv.y = 1.f - uv.y;

    if (tri.textureID != 0xffffffff)
        color   = textureLod(sampler2D(MaterialTex[tri.textureID], samp), uv, 0.f).rgb;
    else
        color   = materials[obj.matID].Color.rgb;

    payload.color = PackRGBE(color * (1.f / 3.1415926f));

    Hit hit;
    hit.position = position;
    hit.normal = normal * sign(dot(normal, gl_WorldRayDirectionEXT));

    payload.hit = hit;
}
