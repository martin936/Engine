// shadertype=glsl

#version 450

in struct PS_INPUT
{
	vec2 uv;
} interp;


layout (std140) uniform cb0
{
	vec4 Params;
	vec4 Rotation;
	
	vec2 Padding;
	vec2 tan_fov;

	mat4 View;
};


#define SamplePattern	Params.xy
#define PixelSize		Params.zw
#define Near			Rotation.z
#define Far				Rotation.w


layout(location = 0) out vec4 InterleavedIrradiance;


layout(location = 0) uniform sampler2D ZMap;
layout(location = 1) uniform sampler2D IrradianceMap;
layout(location = 2) uniform sampler2D NormalMap;


const vec2 SliceDir[2] = 
{
	vec2(1.f, 0.f),
	vec2(0.f, 1.f)
};


void main( void )
{
	vec2	Texcoords = interp.uv + SamplePattern * PixelSize;

	float	ZDist	= textureLod(ZMap, Texcoords, 0.f).r * 2.f - 1.f;
	float	depth	= 2.f * (Far * Near) / (Far + Near - (2.f * ZDist - 1.f) * (Far - Near));	

	vec3	normal	= textureLod(NormalMap, Texcoords, 0.f).xyz * 2.f - 1.f;
	normal = (View * vec4(normal, 0.f)).xyz;
	normal.z = -normal.z;

	vec3 pos_sample = vec3((Texcoords.xy * 2.f - 1.f) * tan_fov, -1.f) * depth;

	mat2 Rot = mat2(Rotation.x, Rotation.y, -Rotation.y, Rotation.x);

	vec3 IndirectIrradiance = 0.f.xxx;

	for (uint i = 0U; i < 2U; i++)
	{
		vec2 Dir = Rot * SliceDir[i];
		vec2 Offset = Texcoords;
		vec3 pos;
		float sin_theta0 = 0.f;
		float dw = 0.f;
		float ZOffset = 0.f;
		float d = 0.f;
		float theta0 = 0.f;
		float theta1 = 0.f;
		vec3 normal_sample = 0.f.xxx;

		vec4 Irradiance = 0.f.xxxx;

		for (uint j = 0U; j < 8U; j++)
		{
			Offset += 3.125f * PixelSize * Dir * (j + 1.f) * 1.f;

			ZOffset = textureLod(ZMap, Offset, 0.f).r * 2.f - 1.f;
			ZOffset = 2.f * (Far * Near) / (Far + Near - (2.f * ZOffset - 1.f) * (Far - Near));

			Irradiance = textureLod(IrradianceMap, Offset, 0.f);

			pos = vec3((Offset.xy * 2.f - 1.f) * tan_fov, -1.f) * ZOffset;

			if (step(0.f, Offset.x * (1.f - Offset.x)) * step(0.f, Offset.y * (1.f - Offset.y)) > 0.f)
			{
				d = length(pos - pos_sample);
				
				sin_theta0 = clamp(dot(pos - pos_sample, normal) / d, 0.f, 1.f);

				theta1 = asin(sin_theta0);

				uint normalBits = floatBitsToUint(Irradiance.a);
				normal_sample.x = (((normalBits >> 8U) & 0xff) / 255.f) * 2.f - 1.f;
				normal_sample.y = ((normalBits & 0xff) / 255.f) * 2.f - 1.f;
				normal_sample.z = sqrt(1.f - dot(normal_sample.xy, normal_sample.xy));

				dw = 3.141592f * abs(ZOffset - depth) * max(0.f, theta1 - theta0) / (4.f * d);

				IndirectIrradiance += Irradiance.rgb * dw * sin_theta0 * step(0.f, dot(pos_sample - pos, normal_sample));
				theta0 = max(theta0, theta1);
			}	
		}

		Offset = Texcoords;
		float sin_theta1 = 0.f;
		theta0 = 0.f;
		theta1 = 0.f;

		for (uint j = 0U; j < 8U; j++)
		{
			Offset -= 3.125f * PixelSize * Dir * (j + 1.f) * 1.f;

			ZOffset = textureLod(ZMap, Offset, 0.f).r * 2.f - 1.f;
			ZOffset = 2.f * (Far * Near) / (Far + Near - (2.f * ZOffset - 1.f) * (Far - Near));

			Irradiance = textureLod(IrradianceMap, Offset, 0.f);

			pos = vec3((Offset.xy * 2.f - 1.f) * tan_fov, -1.f) * ZOffset;

			if (step(0.f, Offset.x * (1.f - Offset.x)) * step(0.f, Offset.y * (1.f - Offset.y)) > 0.f)
			{
				d = length(pos - pos_sample);
				
				sin_theta0 = clamp(dot(pos - pos_sample, normal) / d, 0.f, 1.f);

				theta1 = asin(sin_theta0);

				uint normalBits = floatBitsToUint(Irradiance.a);
				normal_sample.x = (((normalBits >> 8U) & 0xff) / 255.f) * 2.f - 1.f;
				normal_sample.y = ((normalBits & 0xff) / 255.f) * 2.f - 1.f;
				normal_sample.z = sqrt(1.f - dot(normal_sample.xy, normal_sample.xy));

				dw = 3.141592f * abs(ZOffset - depth) * max(0.f, theta1 - theta0) / (4.f * d);

				IndirectIrradiance += Irradiance.rgb * dw * sin_theta0 * step(0.f, dot(pos_sample - pos, normal_sample));
				theta0 = max(theta0, theta1);
			}
		}
	}

	InterleavedIrradiance.rgb = IndirectIrradiance;
	InterleavedIrradiance.a = 0.f;
}
