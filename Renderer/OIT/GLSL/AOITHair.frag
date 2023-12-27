#version 450
#extension GL_ARB_fragment_shader_interlock : enable
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require


#include "../../Lights/GLSL/Lighting.glsl"
#include "../../Lights/GLSL/Clustered.glsl"
#include "AOIT.glsl"


layout(location= 0) in struct
{
	vec3	Normal;
	vec3	Tangent;
	vec3	Bitangent;
	vec3	WorldPos;
	vec2	Texcoords;
	vec3	CurrPos;
	vec3	LastPos;
	vec4	VisibilitySH;
} interp;


layout (binding = 12, std140) uniform cb12
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
	uint 	FlowTextureID;
	uint 	InfoTextureID;
	uint	SSSTextureID;
};


layout(binding = 13, std140) uniform cb13
{
	vec4    Eye;
    vec4    SunColor;
	vec4    SunDir;

	float   Near;
	float   Far;
    vec2    screenSize;
};



layout(binding = 4) uniform sampler				sampLinearWrap;
layout(binding = 5) uniform sampler				sampLinearClamp;


layout(binding = 6) uniform utexture3D	LightListPtr;

layout(binding = 7, std430) readonly buffer buf1
{
	uint	dummy;
	uint	LightIndices[];
};

layout(binding = 8) uniform texture2D			MaterialTex[];
layout(binding = 9) uniform texture2D           MarschnerLUT;


layout (binding = 10, std140) uniform cb10
{
	SLight lightData[128];
};


layout (binding = 11, std140) uniform cb11
{
	SLightShadow shadowLightData[128];
};


vec4 SHProduct(const in vec4 a, const in vec4 b)
{
    float ta, tb, t;
    vec4 r = 0.f.xxxx;

    const float C0 = 0.282094792935999980;

	// [0,0]: 0,
    r.x = C0 * a.x * b.x;

	// [1,1]: 0,6,8,
    ta = C0 * a.x;
    tb = C0 * b.x;
    r.y = ta * b.y + tb * a.y;
    t = a.y * b.y;
    r.x += C0 * t;

	// [2,2]: 0,6,
    ta = C0 * a.x;
    tb = C0 * b.x;
    r.z += ta * b.z + tb * a.z;
    t = a.z * b.z;
    r.x += C0 * t;

	// [3,3]: 0,6,8,
    ta = C0 * a.x;
    tb = C0 * b.x;
    r.w += ta * b.w + tb * a.w;
    t = a.w * b.w;
    r.x += C0 * t;
    
    return r;
}


vec4 Ylm(vec3 dir)
{
    const float c0 = 0.28209479177387814347403972578039; // 1/2 sqrt(1/pi)
    const float c1 = 0.48860251190291992158638462283835; // 1/2 sqrt(3/pi)

    float x = dir.x;
    float y = dir.y;
    float z = dir.z;
    
    vec4 r;

    r.x = c0;
    r.y = c1 * y;
    r.z = c1 * z;
    r.w = c1 * x;
    
    return r;
}


vec4 RotateZH(vec2 ZH, vec3 dir)
{
    ZH *= vec2(3.5449077018110320545963349666823f, 2.0466534158929769769591032497785f);
    
    return Ylm(dir) * ZH.xyyy;
}


float dot2(vec3 x)
{
    return dot(x, x);
}


vec3 Marschner(vec3 u, vec3 wi, vec3 wr, vec3 color)
{
    vec3 lightPerp  = wi - dot(wi, u) * u;
    vec3 viewPerp   = wr - dot(wr, u) * u;
    
    float cos_phi   = dot(viewPerp, lightPerp) * inversesqrt(dot2(viewPerp) * dot2(lightPerp));
    
    float sin_th_i  = dot(wi, u);
    float sin_th_r  = dot(wr, u);
    float cos_th_i  = sqrt(max(0.f, 1.f - sin_th_i * sin_th_i));
    float cos_th_r  = sqrt(max(0.f, 1.f - sin_th_r * sin_th_r));

    float th_i      = asin(sin_th_i);
    float th_r      = asin(sin_th_r);

    float cos_th_h  = cos((th_r + th_i) * 0.5f);
    float cos_th_d  = cos((th_r - th_i) * 0.5f);
    float sin_th_h  = sin((th_r + th_i) * 0.5f);
    float sin_th_d  = sin((th_r - th_i) * 0.5f);

    float inv_cos2  = 1.f / (cos_th_d * cos_th_d);
    
    /*float cos_th_i2 = sqrt(0.5f * (1.f + cos_th_i)); 
    float cos_th_r2 = sqrt(0.5f * (1.f + cos_th_r)); 
    float sin_th_i2 = sign(sin_th_i) * sqrt(0.5f * (1.f - cos_th_i));
    float sin_th_r2 = sign(sin_th_r) * sqrt(0.5f * (1.f - cos_th_r));
    
    float cos_th_h  = cos_th_i2 * cos_th_r2 - sin_th_i2 * sin_th_r2;  
    float cos_th_d  = cos_th_i2 * cos_th_r2 + sin_th_i2 * sin_th_r2;
    
    float inv_cos2  = 1.f / (cos_th_d * cos_th_d);
    
    float sin_th_h  = dot(0.5f * (wi + wr), u);*/
    
    float Mr        = 0.035105f * Henyey_Greenstein(0.9f, 0.99619469822f * cos_th_h - 0.08715574126f * sin_th_h) * inv_cos2;
    float Mtt       = 0.009772f * Henyey_Greenstein(0.865f, 0.99904822161f * cos_th_h + 0.04361938662f * sin_th_h) * inv_cos2;
    float Mtrt      = 0.112854f * Henyey_Greenstein(0.7f, 0.99144486166f * cos_th_h + 0.13052619f * sin_th_h) * inv_cos2;
    
    vec3  N         = texture(sampler2D(MarschnerLUT, sampLinearClamp), vec2(0.5f * (1.f + cos_phi), 1.f - 0.5f * (1.f + cos_th_d))).rgb;
    
    float eta       = sqrt((1.55f * 1.55f - cos_th_d * cos_th_d + 1.f) / cos_th_d);
    float l         = 2.f - 1.5f / (eta * eta);

    return cos_th_r.xxx;//Mr * N.x + Mtt * N.y * exp2(-1.44269f / color) + Mtrt * N.z * exp2(-1.44269f * l / color);
}


vec3 Hair_GI(vec3 v, vec3 u, vec3 color, vec4 lightSH)
{
    float sin_th        = dot(v, u);
    float cos_th        = sqrt(max(0.f, 1.f - sin_th * sin_th));
    float cos_th_2      = sqrt(0.5f * (1.f + cos_th));
    float sin_th_2      = sign(sin_th) * sqrt(0.5f * (1.f - cos_th));
    float cos_th_4      = sqrt(0.5f * (1.f + cos_th_2));
    float sin_th_4      = sign(sin_th_2) * sqrt(0.5f * (1.f - cos_th_2));
    
    float sin_th_3_2    = sin_th * cos_th_2 + sin_th_2 * cos_th;
    float sin_th_5_4    = sin_th * cos_th_4 + sin_th_4 * cos_th;
    float sin_th_7_4    = 2.f * sin_th * cos_th * cos_th_4 - sin_th_4 * (1.f - 2.f * sin_th * sin_th);
    
    vec2  M_r           = vec2(0.0224f * sin_th_3_2 * sin_th_3_2 + 0.0276f, -0.066f * sin_th_5_4);
    vec2  M_tt          = vec2(0.56f * sin_th_3_2 * sin_th_3_2 + 0.23f, -1.25f * sin_th_5_4 * abs(sin_th_5_4));
    vec2  M_trt         = vec2(0.18f * sin_th_7_4 * sin_th_7_4 + 1.61f, -1.9f * sin_th_5_4);
    
    vec4 visibility     = interp.VisibilitySH;
    
    return dot(lightSH, SHProduct(RotateZH(M_r, -u), visibility)) + dot(lightSH, SHProduct(RotateZH(M_tt, u) + RotateZH(M_trt, -u), visibility)) * exp2(-1.44269f / color);
}


vec3 Shade(in vec3 albedo, in vec3 u, in vec3 view)
{
    vec3 L = 0*Hair_GI(view, u, vec3(115, 92, 66) / 255.f, vec4(5.f, 0, 0, 0));

    if (true)//SunColor.w > 0.f)
	{
		vec3 Illuminance	= SunColor.w * SunColor.rgb;

		L += Marschner(u, -normalize(SunDir.xyz), view, vec3(115, 92, 66) / 255.f);
	}

    /*vec2 texCoords			= gl_FragCoord.xy / screenSize;

	uint index				= GetLightListIndex(LightListPtr, texCoords, gl_FragCoord.z, Near, Far);
	uint numLights			= index == 0xffffffff ? 0 : LightIndices[index];
	index++;

	while (numLights > 0)
	{
		uint lightID		= LightIndices[index];

		if (lightID == subgroupMin(lightID))
		{
			vec3 l, Illuminance;

			SLight light;

			if ((lightID & (1 << 15)) == 0)
				light = lightData[lightID];
			else
				light = shadowLightData[lightID & 0x7fff].m_light;

			ComputeLight(light, interp.WorldPos.xyz, 100.f.xxx, Illuminance, l);

			L += Illuminance * Marschner(u, l, view, albedo);

			numLights--;
			index++;
		}
	}*/

    return L;
}


layout(early_fragment_tests) in;
void main( void )
{
    vec4 albedo     = texture(sampler2D(MaterialTex[DiffuseTextureID], sampLinearWrap), interp.Texcoords);

    albedo.a = 1.f;

    if (albedo.a == 0.f)
		discard;

    vec3 pos	    = interp.WorldPos;
	vec3 view	    = normalize(Eye.xyz - pos.xyz);

    vec3 VN		    = normalize(interp.Normal);
    vec3 VT         = normalize(interp.Tangent);
	vec3 VB         = normalize(interp.Bitangent);

    vec4 flowTex    = texture(sampler2D(MaterialTex[FlowTextureID], sampLinearWrap), interp.Texcoords);

    vec3 NTex;
	NTex.xy		    = 2.f * (flowTex.xz - 0.5f.xx);
	float fdotz     = 1.f - dot(NTex.xy, NTex.xy);
	NTex.z		    = sqrt(max(fdotz, 0.f));

	vec3 tangent    = -normalize(NTex.z * VN - NTex.x * VT - NTex.y * VB);

    vec3 Lr         = VN * 0.5f + 0.5f;//dot(VN, view).xxx;//Shade(albedo.rgb, tangent, view);

	beginInvocationInterlockARB();

	WriteNewPixelToAOIT(gl_FragCoord.xy, gl_FragCoord.z, vec4(Lr, albedo.a));

	endInvocationInterlockARB();
}
